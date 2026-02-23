import serial
import struct
import time
import random

PORT = 'COM7' 
BAUD = 115200

# Our Sensor IDs
SENSORS = {
    "V1": 0x01, "V2": 0x02, "V3": 0x03, "V4": 0x04,
    "CURRENT": 0x0A,
    "TEMP_CHG": 0x0B, "TEMP_DIS": 0x0C,
    "CHARGER_CONN": 0x0D
}

try:
    print(f"Connecting to {PORT}...")
    ser = serial.Serial(PORT, BAUD, timeout=0.1)
    time.sleep(2) # Wait for ESP32 reset

    while True:
        # 1. Generate Fake Healthy Data
        v_cells = [round(random.uniform(3.8, 4.0), 2) for _ in range(4)]
        current = round(random.uniform(1.0, 5.0), 1)
        t_chg = round(random.uniform(30.0, 35.0), 1)
        t_dis = t_chg + random.uniform(-2.0, 2.0)
        charger_connected = 1.0 # True

        # 2. Pack and Send Partial Frames (0xAA + ID + Float)
        for i, v in enumerate(v_cells):
            ser.write(struct.pack('<BBf', 0xAA, SENSORS[f"V{i+1}"], v))
        
        ser.write(struct.pack('<BBf', 0xAA, SENSORS["CURRENT"], current))
        ser.write(struct.pack('<BBf', 0xAA, SENSORS["TEMP_CHG"], t_chg))
        ser.write(struct.pack('<BBf', 0xAA, SENSORS["TEMP_DIS"], t_dis))
        ser.write(struct.pack('<BBf', 0xAA, SENSORS["CHARGER_CONN"], charger_connected))

        print(f"Sent Healthy Data -> V1:{v_cells[0]}V | Temp:{t_chg}C")

        # 3. Read Actuator Feedback from ESP32
        while ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"[ESP32] {line}")

        time.sleep(0.5) 

except Exception as e:
    print(f"Error: {e}")