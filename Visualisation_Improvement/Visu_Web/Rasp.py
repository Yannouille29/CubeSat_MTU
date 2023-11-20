# This script is intended to be launched via a service on the Raspberry Pi, serving as a ground station.

import socket
import time
import logging

logging.basicConfig(filename='/home/pi/CubeSatSim/sender2.log', level=logging.INFO, format='%(asctime)s - %(message)s')

# Configuration of Flask server IP address and port
server_ip = '192.168.163.134'  # Replace this with your wifi server's IP address
server_port = 8080

# Location of the .log file
log_file = '/home/pi/FoxTelemetryData-CubeSatSim/FOXDB/FOX7rttelemetry_49_76.log' # Adjust this for the new payload file

# Create a socket for communication
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
previous_line = ""

while True:
    with open(log_file, 'r') as f:
        lines = f.readlines()
    
    # Get the last line of the file
    last_line = lines[-1].strip()
    
    # Check if the line has changed to avoid sending duplicates
    if previous_line != last_line:
        # Send the last line to the Flask server
        message = last_line
        s.sendto(message.encode('utf-8'), (server_ip, server_port))
        logging.info(f"Sent: {message}")
        previous_line = last_line  # Update the previous line
        
    time.sleep(1)  # Wait for 1 seconds before checking the file again
