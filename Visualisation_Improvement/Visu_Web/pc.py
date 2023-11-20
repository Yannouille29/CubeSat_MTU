import socket
import threading
from flask import Flask, jsonify, render_template
from flask_cors import CORS

# Configuration of PC's IP address and port
pc_ip = '192.168.163.134'  # IP address of the Wi-Fi shared with the Raspberry Pi
pc_port = 8080

# Location of the text file where you want to save the data
output_file = 'donnees.txt'

app = Flask('__name')
CORS(app, resources={r"/get_data": {"origins": "*"}})

# Routes for different pages
@app.route('/')
@app.route('/index')
def index():
    return render_template('index.html')

@app.route('/Reaction_Wheels')
def reaction_wheels():
    return render_template('Reaction_Wheels.html')

@app.route('/Wifi_Communication')
def wifi_communication():
    return render_template('Wifi_Communication.html')

@app.route('/3D_Visualisation')
def d_visualisation():
    return render_template('3D_Visualisation.html')

@app.route('/Meteor_Detection')
def meteor_detection():
    return render_template('Meteor_Detection.html')

@app.route('/Youtube_Link_RW')
def youtube_link_rw():
    return render_template('Youtube_Link_RW.html')

@app.route('/Youtube_Link_3D')
def youtube_link_3d():
    return render_template('Youtube_Link_3D.html')

@app.route('/Youtube_Link_Meteor')
def youtube_link_meteor():
    return render_template('Youtube_Link_Meteor.html')

@app.route('/Youtube_Link_WIFI')
def youtube_link_wifi():
    return render_template('Youtube_Link_WIFI.html')

@app.route('/Project_Link_Meteor')
def project_link_meteor():
    return render_template('Project_Link_Meteor.html')

@app.route('/Project_Link_3D')
def project_link_3d():
    return render_template('Project_Link_3D.html')

@app.route('/Project_Link_RW')
def project_link_rW():
    return render_template('Project_Link_RW.html')

@app.route('/Project_Link_WIFI')
def project_link_wifi():
    return render_template('Project_Link_WIFI.html')

# Create a route to retrieve data
@app.route('/get_data', methods=['GET'])
def get_data():
    with open(output_file, 'r') as f:
        data = f.read().splitlines()
        str_data = []

        # If the file has more than 3 lines, take the last 3 lines
        if len(data) > 3:
            data = data[-3:]

        # Split each line using comma as a delimiter
        for d in data:
            values = d.split(',')
            # Add values to str_data
            str_data.append(values)

    return jsonify(str_data)

# Function to receive data from a socket
def receive_data():
    # Create a socket for communication
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((pc_ip, pc_port))

    while True:
        data, addr = s.recvfrom(1024)
        received_data = data.decode('utf-8')
        
        # Save the data in the text file
        with open(output_file, 'a') as f:
            f.write(received_data + '\n')

if __name__ == '__main__':
    # Create a thread to execute the receive_data function
    receive_thread = threading.Thread(target=receive_data)

    # Start the thread
    receive_thread.start()
    app.run(host='0.0.0.0', port=8080)  # Run the Flask server on all PC interfaces
