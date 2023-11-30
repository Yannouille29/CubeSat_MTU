# Web Server for Project CubeSat

**This server is meant to be installed locally. Indeed, if you are connected to the same Wi-Fi network as your ground station and follow the instructions provided in the "Visualisation_Improvement" README, you will be able to observe, in real-time on the local server, the changes in direction and data of the CubeSat.**

This folder contains all the necessary files to launch the website. It's important to note that this isn't just a collection of HTML pages; it's a functioning server. To run the server, ensure that you're connected to the Wi-Fi network and specify the IP address in the "pc_ip = 192.168.xxx.xxx" field within the 'pc.py' script.

Once this configuration is set, launch the server from the terminal or command prompt by navigating to your 'server' directory using the appropriate path:
local_project_path\serveur> python .\pc.py

After executing this command, the server will start and display a message like:

WARNING: This is a development server. Do not use it in a production deployment. Use a production WSGI server instead.
 * Running on all addresses (0.0.0.0)
 * Running on http://127.0.0.1:8080
 * Running on http://172.28.24.28:8080
Press CTRL+C to quit

To access the website, open your web browser and enter the following URL:
http://localhost:8080/index
This will direct you to the project's homepage.

The file 'donnees.txt' is important for the 3D Visualization page as it contains the lines received from the Raspberry Ground Station. This file facilitates real-time data modification on the website. Additionally, it helps verify if the communication is functioning correctly, indicated by the appearance of a new line approximately every 6 seconds.

The file 'Rasp.py' is also useful for the 3D Visualization page and should be placed on the ground station as explained in the 'Visualisation_Improvement' folder.
