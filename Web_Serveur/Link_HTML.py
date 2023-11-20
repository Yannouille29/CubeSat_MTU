import socket

host = '0.0.0.0'  # Écoute sur toutes les interfaces
port = 12345  # Port sur lequel le serveur écoutera

log_file = 'test.log'  # Remplacez par le nom de votre fichier .log

def insert_table_into_html(html_content):
    try:
        with open(log_file, 'r') as file:

            log_data = []  # Créez une liste pour stocker les données

            for line in file:
                values = line.strip().split('\t')
                log_data.append(values)  # Ajoutez les valeurs de chaque ligne à la liste
            return log_data
        
    except FileNotFoundError:
        return 'Le fichier .log n\'a pas été trouvé.'

def send_html_response(client_socket, html_content):
    response = "HTTP/1.1 200 OK\n"
    response += "Content-Type: text/html; charset=utf-8\n"
    response += "Content-Length: " + str(len(html_content)) + "\n\n"
    response += html_content

    client_socket.send(response.encode('utf-8'))

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((host, port))
    server.listen(1)

    print(f"Le serveur est en écoute sur le port {port}")

    with open('C:/Users/Yann/Documents/CubeSat_MTU/Projet/Soft/Page_Web/Root2/HTML/index.html', 'r') as html_file:
        html_content = html_file.read()

    while True:
        client_socket, client_address = server.accept()
        print(f"Connexion acceptée de {client_address[0]}:{client_address[1]}")

        modified_html = insert_table_into_html(html_content)
        send_html_response(client_socket, modified_html)

        client_socket.close()

if __name__ == '__main__':
    main()
