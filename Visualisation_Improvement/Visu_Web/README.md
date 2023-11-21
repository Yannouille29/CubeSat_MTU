# Setting Up Ground Station Communication for Visualization

## Files Required:

1. **rasp.py**:
   - This file needs to be placed on the ground station.
   - Responsible for communication and data transmission.

2. **visualisation.service**:
   - Place this file in `/etc/systemd/system`.
   - Initializes the service to automatically launch the `rasp.py` script.

3. **pc.py**:
   - Executing this file grants access to the web server.
   - The web server receives information from the ground station to update the visualization on the website.

## Steps to Follow:

1. **File Placement**:
   - Move `rasp.py` to the ground station.
   - Place `visualisation.service` in the `/etc/systemd/system` directory.

2. **Service Initialization**:
   - Run the command `sudo systemctl start visualisation.service` to start the service.
   - Use `sudo systemctl enable visualisation.service` to enable automatic startup on boot.

3. **Executing pc.py**:
   - Run `pc.py` to access the web server.
   - Ensure both devices (ground station and PC) are connected to the same Wi-Fi network.

4. **Configuration Changes**:
   - Review and modify the script comments for any network-specific configurations (such as IP addresses, ports, etc.).
   - Check and update any paths or settings that might differ based on your specific setup.

5. **Monitoring the System**:
   - Monitor the system logs or specific log files (if generated) for any errors or debug messages.
   - Use `journalctl -u visualisation.service` to check service logs.

6. **Troubleshooting**:
   - If encountering issues, double-check network connectivity, script configurations, and ensure necessary permissions are set.

7. **Maintenance**:
   - Periodically review and update configurations if the network setup changes.
   - Maintain compatibility with any new software versions or updates.

Make sure to document any additional setup procedures or dependencies required for the successful functioning of the system. 
This README should serve as a guide for setup and maintenance of the communication system between the ground station and the PC for visualization purposes.

## Updating after Modifications in rasp.py:

After making any changes to the `rasp.py` script, follow these commands to update the service:

1. **Daemon Reload**:
   - Execute `sudo systemctl daemon-reload` to reload the daemon after updating the service configuration.

2. **Restart the Service**:
   - Use `sudo systemctl restart visualisation` to restart the visualisation service with the new changes.

3. **Check Status**:
   - Verify the status of the visualisation service for any errors or issues using `sudo systemctl status visualisation`.

These commands ensure that the modifications made in the `rasp.py` script are reflected and the service is running with the updated changes.
Everything else is handled automatically by the website (all the .js files).

But on the Raspberry, you can verify in the `sender2.log` file if the information is indeed being sent from the Raspberry to the website.
