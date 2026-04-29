from flask import Flask, render_template, request, jsonify
import paho.mqtt.client as mqtt
from paho.mqtt.enums import CallbackAPIVersion
import serial
import threading
import time
import json
import sqlite3

app = Flask(__name__)

# ==========================================
# 1. CONFIGURATION
# ==========================================
SERIAL_PORT = 'COM7'  
BAUD_RATE = 115200

MQTT_BROKER = "broker.hivemq.com" 
MQTT_PORT = 1883
TOPIC_TELEMETRY = "bms/esp32/status"
TOPIC_COMMAND = "bms/esp32/command"

latest_telemetry = {}
ser = None  
sim_state = {
    "v1": 3.9, "v2": 3.9, "v3": 3.9, "v4": 3.9,
    "c": 0.0, "t1": 25.0, "t2": 25.0, "charger": 0
}

# ==========================================
# 2. DATABASE INITIALIZATION
# ==========================================
def init_db():
    """Creates the SQLite database and table if they don't exist"""
    conn = sqlite3.connect('bms_database.db')
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS telemetry (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                    state INTEGER,
                    faults INTEGER,
                    pack_voltage REAL,
                    current REAL,
                    soc REAL,
                    t1 REAL,
                    t2 REAL
                )''')
    conn.commit()
    conn.close()
    print("[DB] SQLite Database Initialized.")

# ==========================================
# 3. UART SIMULATOR ENGINE
# ==========================================
def calculate_checksum(payload_str):
    checksum = 0
    for char in payload_str:
        checksum ^= ord(char)
    return f"{checksum:02X}"

def uart_simulator_loop():
    global ser
    print(f"[UART] Attempting to open {SERIAL_PORT}...")
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1, write_timeout=1)
        ser.dtr = True 
        ser.rts = True
        print(f"[UART] Connected to {SERIAL_PORT}!")
    except Exception as e:
        print(f"\n[ERROR] Could not open {SERIAL_PORT}. Is ESP-IDF monitor open?")
        return

    while True:
        payload = f"v1={sim_state['v1']},v2={sim_state['v2']},v3={sim_state['v3']},v4={sim_state['v4']},c={sim_state['c']},t1={sim_state['t1']},t2={sim_state['t2']},CHG={sim_state['charger']}"
        checksum = calculate_checksum(payload)
        frame = f"{payload}*{checksum}\n"

        try:
            ser.write(frame.encode('utf-8'))
            ser.flush()
        except Exception as e:
            pass
        time.sleep(1.0) 

# ==========================================
# 4. MQTT ENGINE & DATA LOGGING
# ==========================================
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print(f"[MQTT] Connected to Broker successfully!")
        client.subscribe(TOPIC_TELEMETRY)

@app.route('/history')
def history():
    # Serve the new historical analysis dashboard
    return render_template('history.html')

@app.route('/api/history', methods=['GET'])
def fetch_history():
    # How many records to pull (default 100, but the web UI can ask for more)
    limit = request.args.get('limit', 100)
    
    try:
        conn = sqlite3.connect('bms_database.db')
        conn.row_factory = sqlite3.Row # This lets us access columns by name
        c = conn.cursor()
        
        # Get the latest X rows, ordered by newest first
        c.execute('''SELECT timestamp, pack_voltage, current, soc, t1, t2, faults 
                     FROM telemetry ORDER BY id DESC LIMIT ?''', (limit,))
        rows = c.fetchall()
        conn.close()
        
        # We reverse the list so it charts chronologically (left to right)
        data = [dict(row) for row in reversed(rows)]
        return jsonify({"status": "success", "data": data})
    except Exception as e:
        print(f"[DB ERROR] {e}")
        return jsonify({"status": "error", "data": []})
    
def on_message(client, userdata, msg):
    global latest_telemetry
    try:
        payload = msg.payload.decode('utf-8')
        data = json.loads(payload)
        latest_telemetry = data
        
        # Calculate pack voltage for the database
        pack_v = data.get('v1',0) + data.get('v2',0) + data.get('v3',0) + data.get('v4',0)
        
        # --- LOG TO SQLITE DATABASE ---
        conn = sqlite3.connect('bms_database.db')
        c = conn.cursor()
        c.execute('''INSERT INTO telemetry 
                     (state, faults, pack_voltage, current, soc, t1, t2) 
                     VALUES (?, ?, ?, ?, ?, ?, ?)''', 
                  (data.get('state', 0), data.get('faults', 0), pack_v,
                   data.get('current', 0.0), data.get('soc', 0.0), 
                   data.get('t1', 0.0), data.get('t2', 0.0)))
        conn.commit()
        conn.close()
        
    except Exception as e:
        print(f"[MQTT ERROR] {e}")

mqtt_client = mqtt.Client(CallbackAPIVersion.VERSION2)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

# ==========================================
# 5. FLASK ROUTES & WEB API
# ==========================================
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/telemetry', methods=['GET'])
def get_telemetry():
    return jsonify(latest_telemetry)

@app.route('/api/command', methods=['POST'])
def handle_command():
    global sim_state
    data = request.json
    cmd_type = data.get('type')
    action = data.get('action')
    
    print(f"[WEB API] Command Received -> Type: {cmd_type}")

    # 1. Hard Reset via MQTT
    if cmd_type == 'raw' and action == 'RESET':
        mqtt_client.publish(TOPIC_COMMAND, "RESET")
        return jsonify({"status": "success"})

    # 2. Custom Simulator Sliders
    if cmd_type == 'custom':
        sim_state.update(action) # action is a dictionary sent from our JS!
        return jsonify({"status": "success", "msg": "Custom state applied"})

    # 3. Preset Buttons
    if cmd_type == 'preset':
        if action == 'normal_idle':
            sim_state.update({"v1": 3.9, "v2": 3.9, "v3": 3.9, "v4": 3.9, "c": 0.0, "t1": 25.0, "t2": 25.0, "charger": 0})
        elif action == 'normal_charge':
            sim_state.update({"v1": 3.9, "v2": 3.9, "v3": 3.9, "v4": 3.9, "c": 5.0, "t1": 30.0, "t2": 30.0, "charger": 1})
        elif action == 'normal_discharge':
            sim_state.update({"v1": 3.8, "v2": 3.8, "v3": 3.8, "v4": 3.8, "c": -15.0, "t1": 32.0, "t2": 32.0, "charger": 0})
        elif action == 'storage_mode':
            sim_state.update({"v1": 3.8, "v2": 3.8, "v3": 3.8, "v4": 3.8, "c": 0.0, "t1": 25.0, "t2": 25.0, "charger": 0})
        elif action == 'fault_imbalance':
            sim_state.update({"v1": 3.8, "v2": 3.8, "v3": 4.3, "v4": 3.8}) # Cell 3 spiked
        elif action == 'deep_discharge':
            sim_state["c"] = 50.0 
        elif action == 'fault_ov':
            sim_state["v1"] = 4.35 
        elif action == 'fault_uv':
            sim_state["v2"] = 2.40 
        elif action == 'fault_ot':
            sim_state["t1"] = 65.0 
        elif action == 'extreme_cold':
            sim_state["t1"] = -10.0
            
        return jsonify({"status": "success"})

    return jsonify({"status": "error"}), 400


# ==========================================
# 6. SERVER BOOTSTRAP
# ==========================================
if __name__ == '__main__':
    init_db() # Create the DB on startup!
    
    uart_thread = threading.Thread(target=uart_simulator_loop, daemon=True)
    uart_thread.start()
    
    print("\n[SYSTEM] Starting Flask Master Dashboard...")
    app.run(debug=True, host='0.0.0.0', port=5000, use_reloader=False)