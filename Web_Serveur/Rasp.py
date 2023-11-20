import socket
import time
import logging

logging.basicConfig(filename='sender.log', level=logging.INFO, format='%(asctime)s - %(message)s')

# Configuration de l'adresse IP et du port du PC
pc_ip = '192.168.56.1'
pc_port = 12345
previous_line = ""

# Emplacement du fichier .log
log_file = 'test.log'

# Créer une socket pour la communication
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

while True:
    with open(log_file, 'r') as f:
        lines = f.readlines()
    
    # Obtenir la dernière ligne du fichier
    last_line = lines[-1].strip()
    if previous_line != last_line:
    
        message = last_line  # Stocker la dernière ligne dans la variable message
        s.sendto(message.encode('utf-8'), (pc_ip, pc_port))
        logging.info(f"Sent: {message}")
        previous_line = last_line  # Mettre à jour la ligne précédente
    
    time.sleep(6)  # Attendre 1 seconde avant de vérifier à nouveau le fichier
