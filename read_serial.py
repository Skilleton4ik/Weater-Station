import serial
import time
import sys
try:
    ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=1)
    
    # Try to reset the board to catch boot logs
    ser.setDTR(False)
    ser.setRTS(True)
    time.sleep(0.1)
    ser.setRTS(False)
    
    # Read for 8 seconds
    end_time = time.time() + 8
    while time.time() < end_time:
        line = ser.readline()
        if line:
            print(line.decode('utf-8', errors='replace').strip())
except Exception as e:
    print(f"Error: {e}")
