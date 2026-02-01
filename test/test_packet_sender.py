import serial
import struct
import time
import random

# CHANGE THIS to your COM port
PORT = 'COM7' 
BAUD = 115200

try:
    print(f"Connecting to {PORT}...")
    ser = serial.Serial(PORT, BAUD, timeout=1)
    time.sleep(2) # Wait for ESP32 reset

    while True:

        # Pack: 0xAA (Header) + 1 Float
        # < = Little Endian
        # B = Unsigned Char (1 byte)
        # f = Float (4 bytes)
        x=input("Enter a float value to send (or 'q' to quit): ")
        if x.lower() == 'q':
            break
        packet = struct.pack('<Bf', 0xAA, float(x))
        
        ser.write(packet)
        print(f"Sent: {x}")

except Exception as e:
    print(f"Error: {e}")