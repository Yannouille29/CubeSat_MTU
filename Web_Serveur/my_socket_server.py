import socket

host = '0.0.0.0'  # Écoute sur toutes les interfaces
port = 12345  # Port sur lequel le serveur écoutera

log_file = 'test.log'  # Remplacez par le nom de votre fichier .log

def send_data_table(client_socket):
    try:
        with open(log_file, 'r') as file:
            # Commencez la réponse HTML avec l'en-tête de tableau
            response = "<html><head><title>Tableau de données</title></head><body><table border='1'>"

            for line in file:
                values = line.strip().split('\t')  # Divisez chaque ligne en valeurs en utilisant la tabulation comme séparateur
                # Ajoutez une ligne de tableau HTML pour chaque ligne de valeurs
                response += "<tr>"
                for value in values:
                    response += f"<td>{value}</td>"
                response += "</tr>"

            # Terminez la réponse HTML
            response += "</table></body></html>"

            client_socket.send(response.encode('utf-8'))
    except FileNotFoundError:
        client_socket.send('Le fichier .log n\'a pas été trouvé.'.encode('utf-8'))

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((host, port))
    server.listen(1)

    print(f"Le serveur est en écoute sur le port {port}")

    while True:
        client_socket, client_address = server.accept()
        print(f"Connexion acceptée de {client_address[0]}:{client_address[1]}")
        send_data_table(client_socket)
        client_socket.close()

if __name__ == '__main__':
    main()