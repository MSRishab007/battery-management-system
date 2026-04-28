import serial
import time
import threading

# ==========================================
# CONFIGURATION
# ==========================================
SERIAL_PORT = 'COM7'  
BAUD_RATE = 115200

# ==========================================
# CHECKSUM GENERATOR
# ==========================================
def calculate_checksum(payload_str):
    """Calculates the XOR checksum of the payload to match the ESP32 C++ logic"""
    checksum = 0
    for char in payload_str:
        checksum ^= ord(char)
    return f"{checksum:02X}"

# ==========================================
# SERIAL READER THREAD
# ==========================================
def listen_to_esp(ser):
    """Runs in the background and prints everything the ESP32 says"""
    print("[SYSTEM] Listening for ESP32 Logs...")
    while True:
        try:
            if ser.in_waiting > 0:
                # Read line, decode, and ignore random corrupted bytes
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    # Print with a green color code to separate it from Python text
                    print(f"\033[92m[ESP32]\033[0m {line}")
        except Exception:
            pass
        time.sleep(0.01)

# ==========================================
# MAIN SIMULATOR LOOP
# ==========================================
def main():
    print(f"[SYSTEM] Connecting to {SERIAL_PORT}...")
    try:
        # Added write_timeout to prevent infinite freezing
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1, write_timeout=None)
        time.sleep(3)
        
        # Force the ESP32 to recognize the PC is actively listening
        ser.dtr = True 
        ser.rts = True
        
        # time.sleep(2) # Give the port a second to stabilize
    except serial.SerialException as e:
        print(f"[ERROR] Could not open {SERIAL_PORT}. Is the ESP-IDF Monitor still open?")
        print("Press Ctrl+] in VS Code to close the monitor first!")
        return

    # Start the listening thread
    listener_thread = threading.Thread(target=listen_to_esp, args=(ser,), daemon=True)
    listener_thread.start()

    print("[SYSTEM] Simulator Active. Sending data at 1Hz.")
    print("Press Ctrl+C to stop.")
    
    try:
        while True:
            # --- EDIT THESE VALUES TO TEST YOUR FAULT LOGIC ---
            # Right now, v1 is set to 4.3 which WILL trigger a HARD FAULT
            v1, v2, v3, v4 = 3.9, 3.9, 3.9, 3.9
            current = 2.5
            t1, t2 = 30.0, 31.0
            charger = 1 # 1 = Connected, 0 = Disconnected

            # Package the payload
            payload = f"v1={v1},v2={v2},v3={v3},v4={v4},c={current},t1={t1},t2={t2},CHG={charger}"
            
            # Calculate checksum and append the newline exactly as C++ expects
            checksum = calculate_checksum(payload)
            frame = f"{payload}*{checksum}\n"

            # Print what we are sending in Blue
            print(f"\033[94m[PYTHON]\033[0m Sending: {frame.strip()}")
            
            # ==========================================================
            # THE FIX: Catch the USB lockup and prevent script freeze
            # ==========================================================
            try:
                # Fire it down the USB cable
                ser.write(frame.encode('utf-8'))
                ser.flush() # Force the OS to push the buffer immediately
            except serial.SerialTimeoutException:
                print("\033[91m[ERROR]\033[0m USB Bus busy! Dropped packet, retrying...")
            except Exception as e:
                print(f"\033[91m[ERROR]\033[0m {e}")
            
            time.sleep(1.0) # Send at 1Hz

    except KeyboardInterrupt:
        print("\n[SYSTEM] Simulator stopped by user.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()