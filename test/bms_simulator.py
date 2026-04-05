import serial
import time
import random

# --- CONFIGURATION ---
PORT = 'COM7'     # Change to your ESP32's COM port
BAUD = 115200

# --- SCENARIO TESTING ---
# 0 = Normal Charging
# 1 = Normal Discharging
# 2 = Over-Voltage Fault (Triggers OVP)
# 3 = Over-Temperature Fault (Triggers OTP)
# 4 = Stale Data Fault (Drops V2 from the frame)
ACTIVE_SCENARIO = 0  

def calculate_checksum(payload):
    """Calculates the XOR checksum of a string (NMEA style)"""
    checksum = 0
    for char in payload:
        checksum ^= ord(char)
    return checksum

def send_bms_frame(ser, data_dict):
    """Builds the payload, calculates checksum, and sends over UART"""
    # Create comma-separated string: "v1=3.50,v2=3.60,c=1.2"
    payload = ",".join([f"{k}={v}" for k, v in data_dict.items()])
    
    # Calculate Hex checksum
    chk = calculate_checksum(payload)
    
    # Format with asterisk and newline: "payload*XX\n"
    frame = f"{payload}*{chk:02X}\n"
    
    # Send to ESP32
    ser.write(frame.encode('utf-8'))
    return frame

try:
    print(f"Connecting to ESP32 on {PORT} at {BAUD} baud...")
    ser = serial.Serial(PORT, BAUD, timeout=0.1)
    time.sleep(2) # Wait for ESP32 to reboot after serial connection

    print("Connected! Starting simulation...\n")

    while True:
        # 1. Generate Base Data (Healthy)
        data = {
            "v1": round(random.uniform(3.90, 4.00), 2),
            "v2": round(random.uniform(3.90, 4.00), 2),
            "v3": round(random.uniform(3.90, 4.00), 2),
            "v4": round(random.uniform(3.90, 4.00), 2),
            "c":  round(random.uniform(1.0, 3.0), 2),  # Positive = Charging
            "t1": round(random.uniform(30.0, 32.0), 1),
            "t2": round(random.uniform(30.0, 32.0), 1),
            "CHG": 1  # 1 = Charger Connected
        }

        # 2. Modify Data Based on Active Scenario
        if ACTIVE_SCENARIO == 1: # Discharging
            data["c"] = round(random.uniform(-1.0, -5.0), 2) # Negative current
            data["CHG"] = 0

        elif ACTIVE_SCENARIO == 2: # Over-Voltage Fault
            data["v1"] = 4.35 # Breaches OVP_LIMIT (4.25V)
            
        elif ACTIVE_SCENARIO == 3: # Over-Temperature
            data["t1"] = 65.0 # Breaches OTP_LIMIT (60C)
            
        elif ACTIVE_SCENARIO == 4: # Stale Data
            # Deliberately remove 'v2' from the dictionary! 
            # The ESP32 timestamp for V2 will age out and trigger FAULT_STALE_DATA after 3 seconds.
            del data["v2"]

        # 3. Send the Frame
        sent_frame = send_bms_frame(ser, data)
        print(f"[TX] {sent_frame.strip()}")

        # 4. Read ESP32 Serial Outputs (Logs & Debugs)
        while ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"  --> [ESP32] {line}")

        time.sleep(1.0) # Send data once per second

except serial.SerialException as e:
    print(f"\n[!] Serial Port Error: {e}")
    print("Is the ESP32 plugged in, and is the COM port correct?")
except KeyboardInterrupt:
    print("\nSimulation stopped by user.")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()