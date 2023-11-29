import socket
import threading
from flask import Flask, jsonify, render_template
from flask_cors import CORS
# Configuration de l'adresse IP et du port du PC
pc_ip = '192.168.163.134' # adresse ip de mon wifi partagé avec la raspberry
pc_port = 8080

# Emplacement du fichier texte où vous souhaitez enregistrer les données
output_file = 'donnees.txt'

app = Flask('__name')
CORS(app, resources={r"/get_data": {"origins": "*"}})

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





# Créer une route pour récupérer les données
#@app.route('/get_data', methods=['GET'])
#def get_data():
#    with open(output_file, 'r') as f:
#        data = f.read().splitlines()
#        str_data = []
#        for d in data:
#            str_data.append([d[1], d[2], d[3]])
#            
 #   return jsonify(str_data)

# Créer une route pour récupérer les données
@app.route('/get_data', methods=['GET'])
def get_data():
    with open(output_file, 'r') as f:
        data = f.read().splitlines()
        str_data = []

        # Si le fichier a plus de 3 lignes, prenez les 3 dernières lignes
        if len(data) > 3:
            data = data[-3:]

        # Divisez chaque ligne en utilisant la virgule comme délimiteur
        for d in data:
            values = d.split(',')
            # Ajoutez les valeurs à str_data
            str_data.append(values)

    return jsonify(str_data)


def receive_data():
    # Créer une socket pour la communication
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((pc_ip, pc_port))

    while True:
        data, addr = s.recvfrom(1024)
        received_data = data.decode('utf-8')
        
        # Enregistrez les données dans le fichier texte
        with open(output_file, 'a') as f:
            f.write(received_data + '\n')


if __name__ == '__main__':
    # Créer un thread pour exécuter la fonction receive_data
    receive_thread = threading.Thread(target=receive_data)

    # Démarrer le thread
    receive_thread.start()
    app.run(host='0.0.0.0', port=8080)  # Exécute le serveur Flask sur toutes les interfaces du PC


