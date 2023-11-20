#include "main.h"

int main(int argc, char * argv[]) {

  // Stop the rpitx service using a shell command
  FILE * rpitx_stop = popen("sudo systemctl stop rpitx", "r");
  pclose(rpitx_stop);
	
  // Delete two files "ready" and "cwready" in the specified directory, suppressing any output
  FILE * file_deletes = popen("sudo rm /home/pi/CubeSatSim/ready /home/pi/CubeSatSim/cwready > /dev/null", "r");
  pclose(file_deletes);	
	
  printf("Test bus 1\n");
  // Flush the standard output buffer to ensure immediate display of the message
  fflush(stdout);	
  // Test the I2C bus 1 and assign a value to i2c_bus1 based on the test result
  i2c_bus1 = (test_i2c_bus(1) != -1) ? 1 : OFF;
  
  //The same with bus3
  printf("Test bus 3\n");
  fflush(stdout);
  i2c_bus3 = (test_i2c_bus(3) != -1) ? 3 : OFF;
  
  // Display "Finished testing" message on the standard output
  printf("Finished testing\n");
  fflush(stdout);

  // Execute a shell command to restart the rpitx service
  FILE * rpitx_restart = popen("sudo systemctl restart rpitx", "r");
  pclose(rpitx_restart);

  // Set the mode to FSK and initialize the frame count
  mode = FSK;
  frameCnt = 1;

if (argc > 1) {
  // Check if a mode argument is provided and update the mode accordingly
  if (*argv[1] == 'b') {
    mode = BPSK;
    printf("Mode BPSK\n");
  } else if (*argv[1] == 'a') {
    mode = AFSK;
    printf("Mode AFSK\n");
  } else if (*argv[1] == 'm') {
    mode = CW;
    printf("Mode CW\n");
  } else {
    printf("Mode FSK\n");
  }

  if (argc > 2) {
    // Check if a loop count argument is provided and set the loop count
    loop = atoi(argv[2]);
    loop_count = loop;
  }
  printf("Looping %d times \n", loop);

  if (argc > 3) {
    // Check if a CW id argument is provided and disable CW id if necessary
    if (*argv[3] == 'n') {
      cw_id = OFF;
      printf("No CW id\n");
    }
  }
} else {
  // Read mode information from a file if no arguments are provided
  FILE * mode_file = fopen("/home/pi/CubeSatSim/.mode", "r");
  if (mode_file != NULL) {	
    char mode_string;	
    mode_string = fgetc(mode_file);
    fclose(mode_file);
    printf("Mode file /home/pi/CubeSatSim/.mode contains %c\n", mode_string);

    // Update the mode based on the content of the mode file
    if (mode_string == 'b') {
      mode = BPSK;
      printf("Mode is BPSK\n");
    } else if (mode_string == 'a') {
      mode = AFSK;
      printf("Mode is AFSK\n");
    } else if (mode_string == 's') {
      mode = SSTV;
      printf("Mode is SSTV\n");
    } else if (mode_string == 'm') {
      mode = CW;
      printf("Mode is CW\n");
    } else {
      printf("Mode is FSK\n");
    }	    
  }
}

// Open the configuration file for reading with callsign and reset count	
FILE *config_file = fopen("/home/pi/CubeSatSim/sim.cfg", "r");

if (config_file == NULL) {
  printf("Creating config file.");
  config_file = fopen("/home/pi/CubeSatSim/sim.cfg", "w");
  fprintf(config_file, "%s %d", " ", 100);
  fclose(config_file);
  config_file = fopen("/home/pi/CubeSatSim/sim.cfg", "r");
}

//  char * cfg_buf[100];

// Read data from the configuration file
fscanf(config_file, "%s %d %f %f %s", call, &reset_count, &lat_file, &long_file, sim_yes);
fclose(config_file);
printf("Config file /home/pi/CubeSatSim/sim.cfg contains %s %d %f %f %s\n", call, reset_count, lat_file, long_file, sim_yes);

// Increment the reset count and ensure it stays within a 16-bit range
reset_count = (reset_count + 1) % 0xffff;

// Check if latitude and longitude values in the config file are valid
if ((fabs(lat_file) > 0) && (fabs(lat_file) < 90.0) && (fabs(long_file) > 0) && (fabs(long_file) < 180.0)) {
  printf("Valid latitude and longitude in the config file\n");

  // Convert latitude and longitude to APRS DDMM.MM format
  latitude = toAprsFormat(lat_file);
  longitude = toAprsFormat(long_file);
  printf("Lat/Long in APRS DDMM.MM format: %f/%f\n", latitude, longitude);
} else { // Set default values for latitude and longitude
  latitude = toAprsFormat(latitude);
  longitude = toAprsFormat(longitude);
}

// Check if "sim_yes" in the config file is set to "yes"
if (strcmp(sim_yes, "yes") == 0)
  sim_mode = TRUE;

// Initialize the wiringPi library
wiringPiSetup();

// Check if the mode is AFSK
if (mode == AFSK) {
  // Check if SPI is enabled
  FILE *file = popen("sudo raspi-config nonint get_spi", "r");
  if (fgetc(file) == '0') {
    printf("SPI is enabled!\n");

    // Check for SPI devices
    FILE *file2 = popen("ls /dev/spidev0.* 2>&1", "r");
    printf("Result getc: %c \n", getc(file2));

    if (fgetc(file2) != 'l') {
      printf("SPI devices present!\n");

      // Configure SPI settings
      setSpiChannel(SPI_CHANNEL);
      setSpiSpeed(SPI_SPEED);
      initializeSpi();

      // Initialize AX.25 protocol
      ax25_init(&hax25, (uint8_t *)dest_addr, 11, (uint8_t *)call, 11, AX25_PREAMBLE_LEN, AX25_POSTAMBLE_LEN);

      // Initialize RF communication
      if (init_rf()) {
        printf("AX5043 successfully initialized!\n");
        ax5043 = TRUE;
        cw_id = OFF;
        printf("Mode AFSK with AX5043\n");
        transmit = TRUE;
      } else {
        printf("AX5043 not present!\n");
      }
      pclose(file2);
    }
  }
  pclose(file);
}

// Default settings for LEDs and board types
txLed = 0;
txLedOn = LOW;
txLedOff = HIGH;

// Check board type and set corresponding settings
if (!ax5043) {
  pinMode(2, INPUT);
  pullUpDnControl(2, PUD_UP);

  if (digitalRead(2) != HIGH) {
    printf("vB3 with TFB Present\n");
    vB3 = TRUE;
    txLed = 3;
    txLedOn = LOW;
    txLedOff = HIGH;
    onLed = 0;
    onLedOn = LOW;
    onLedOff = HIGH;
    transmit = TRUE;
  } else {

// Set GPIO pin 3 as an input and enable pull-up resistor
pinMode(3, INPUT);
pullUpDnControl(3, PUD_UP);

// Check the state of GPIO pin 3
if (digitalRead(3) != HIGH) {
  // If GPIO pin 3 is not in a high state, it indicates vB4 hardware with UHF BPF
  printf("vB4 Present with UHF BPF\n");
  txLed = 2;
  txLedOn = HIGH;
  txLedOff = LOW;
  vB4 = TRUE;
  onLed = 0;
  onLedOn = HIGH;
  onLedOff = LOW;
  transmit = TRUE;
} else {

  // If GPIO pin 3 is in a high state, check other pins
  pinMode(26, INPUT);
  pullUpDnControl(26, PUD_UP);

  // Check the state of GPIO pin 26
  if (digitalRead(26) != HIGH) {
    // If GPIO pin 26 is not in a high state, it indicates v1 hardware with UHF BPF
    printf("v1 Present with UHF BPF\n");
    txLed = 2;
    txLedOn = HIGH;
    txLedOff = LOW;
    vB5 = TRUE;
    onLed = 27;
    onLedOn = HIGH;
    onLedOff = LOW;
    transmit = TRUE;
  } else {

    // If GPIO pin 26 is in a high state, check another pin
    pinMode(23, INPUT);
    pullUpDnControl(23, PUD_UP);

    // Check the state of GPIO pin 23
    if (digitalRead(23) != HIGH) {
      // If GPIO pin 23 is not in a high state, it indicates v1 hardware with VHF BPF
      printf("v1 Present with VHF BPF\n");
      txLed = 2;
      txLedOn = HIGH;
      txLedOff = LOW;
      vB5 = TRUE;
      onLed = 27;
      onLedOn = HIGH;
      onLedOff = LOW;
      printf("VHF BPF not yet supported so no transmit\n");
      transmit = FALSE;
    }
  }
}

// Configure the GPIO pins for LEDs
pinMode(txLed, OUTPUT);              					// Set the GPIO pin specified by txLed as an output
digitalWrite(txLed, txLedOff);       					// Set the initial state of the transmit LED based on txLedOff
#ifdef DEBUG_LOGGING
printf("Tx LED Off\n");             					// If debugging is enabled, print a debug message
#endif

// Configure another GPIO pin for a different LED
pinMode(onLed, OUTPUT);              					// Set the GPIO pin specified by onLed as an output
digitalWrite(onLed, onLedOn);        					// Set the initial state of the power LED based on onLedOn
#ifdef DEBUG_LOGGING
printf("Power LED On\n");            					// If debugging is enabled, print a debug message
#endif

// Create or open a file called "sim.cfg" for writing
config_file = fopen("sim.cfg", "w");

// Write configuration data to the "sim.cfg" file
fprintf(config_file, "%s %d %8.4f %8.4f %s", call, reset_count, lat_file, long_file, sim_yes);
fclose(config_file);  							// Close the file

// Check hardware type (vB4, vB5, or other)
if (vB4) {
  // If it's vB4, set up mapping and bus configuration
  map[BAT] = BUS;
  map[BUS] = BAT;
  snprintf(busStr, 10, "%d %d", i2c_bus1, test_i2c_bus(0));
} else if (vB5) {
  // If it's vB5, set up different mapping and bus configuration
  map[MINUS_X] = MINUS_Y;
  map[PLUS_Z] = MINUS_X;	
  map[MINUS_Y] = PLUS_Z;

  // Check for the presence of I2C Bus 11
  if (access("/dev/i2c-11", W_OK | R_OK) >= 0) {
    printf("/dev/i2c-11 is present\n\n");
    snprintf(busStr, 10, "%d %d", test_i2c_bus(1), test_i2c_bus(11));
  } else {
    snprintf(busStr, 10, "%d %d", i2c_bus1, i2c_bus3);
  }
} else {
  // If it's neither vB4 nor vB5, set up another mapping and bus configuration
  map[BUS] = MINUS_Z;
  map[BAT] = BUS;
  map[PLUS_Z] = BAT;
  map[MINUS_Z] = PLUS_Z;
  snprintf(busStr, 10, "%d %d", i2c_bus1, test_i2c_bus(0));
  voltageThreshold = 8.0;
}

// Check for the presence of a camera using vcgencmd
FILE *file4 = popen("vcgencmd get_camera", "r");
fgets(cmdbuffer, 1000, file4);
char camera_present[] = "supported=1 detected=1";
camera = (strstr((const char *)&cmdbuffer, camera_present) != NULL) ? ON : OFF;  // Check if the camera is present
pclose(file4);  // Close the file

#ifdef DEBUG_LOGGING
  // This block of code is conditionally compiled only if DEBUG_LOGGING is defined.
  // It's used for debugging purposes to print information to the console.

  // Print information about the I2C bus status and camera presence.
  printf("INFO: I2C bus status 0: %d 1: %d 3: %d camera: %d\n", i2c_bus0, i2c_bus1, i2c_bus3, camera);
#endif

// Delete camera related files if they exist
FILE *file5 = popen("sudo rm /home/pi/CubeSatSim/camera_out.jpg > /dev/null 2>&1", "r");
file5 = popen("sudo rm /home/pi/CubeSatSim/camera_out.jpg.wav > /dev/null 2>&1", "r");
pclose(file5);

// Try connecting to STEM Payload board using UART
// /boot/config.txt and /boot/cmdline.txt must be set correctly for this to work

if (!ax5043 && !vB3 && !(mode == CW) && !(mode == SSTV)) {
  // Check various conditions to determine whether to test the STEM Payload board.
  // Conditions: AX5043 is not present, not in vB3 mode, not in CW mode, and not in SSTV mode.

  payload = OFF;  // Initialize the payload status

  // Attempt to open a UART connection
  if ((uart_fd = serialOpen("/dev/ttyAMA0", 115200)) >= 0) {
    // UART connection opened successfully

    char c;
    int charss = (char) serialDataAvail(uart_fd);		//le serialDataAvail(uart_fd) renvoie le nombre de caractère disponible dans le tampon buffer, charss = le nombre de caractère dans buffer

    if (charss != 0)
      printf("Clearing buffer of %d chars \n", charss);

    // Clear the UART buffer
    while ((charss-- > 0))
      c = (char) serialGetchar(uart_fd);

    unsigned int waitTime;
    int i;

    for (i = 0; i < 2; i++) {
      if (payload != ON) {
        // Query the payload board for its status by sending 'R'
        serialPutchar(uart_fd, 'R');
        printf("Querying payload with R to reset\n");
        waitTime = millis() + 500;

        while ((millis() < waitTime) && (payload != ON)) {
          // Check for payload response
          if (serialDataAvail(uart_fd)) {
	    //Ensuite, il attend une réponse du dispositif en surveillant les caractères reçus dans la boucle while. Si le dispositif répond par les caractères 'O' et 'K', il définit payload comme "ON" pour indiquer que le dispositif est prêt.
            printf("%c", c = (char) serialGetchar(uart_fd));
            fflush(stdout);
            if (c == 'O') {
              printf("%c", c = (char) serialGetchar(uart_fd));
              fflush(stdout);
              if (c == 'K')
                payload = ON;
            }
          }
          printf("\n");
        }
      }
    }

    if (payload == ON) {
      // STEM Payload is present
      printf("\nSTEM Payload is present!\n");
      sleep(2);  // Delay to give the payload time to get ready
    } else {
      // STEM Payload is not present
      printf("\nSTEM Payload not present!\n -> Is STEM Payload programmed and Serial1 set to 115200 baud?\n");
    }
  } else {
    // Failed to open UART connection
    fprintf(stderr, "Unable to open UART: %s\n -> Did you configure /boot/config.txt and /boot/cmdline.txt?\n", strerror(errno));
  }
}

if ((i2c_bus3 == OFF) || (sim_mode == TRUE)) {
  // This conditional block checks if either i2c_bus3 is OFF or sim_mode is TRUE.

  sim_mode = TRUE;  									// Set sim_mode to TRUE, indicating simulated telemetry mode.

  printf("Simulated telemetry mode!\n");  				// Print a message indicating simulation mode.

  srand((unsigned int)time(0));  						// Seed the random number generator with the current time.

  // Generate random values for the axis array within a specified range.
  axis[0] = rnd_float(-0.2, 0.2);
  if (axis[0] == 0)
    axis[0] = rnd_float(-0.2, 0.2);
  axis[1] = rnd_float(-0.2, 0.2);
  axis[2] = (rnd_float(-0.2, 0.2) > 0) ? 1.0 : -1.0;

  // Calculate angles based on the generated axis values.
  angle[0] = (float) atan(axis[1] / axis[2]);
  angle[1] = (float) atan(axis[2] / axis[0]);
  angle[2] = (float) atan(axis[1] / axis[0]);

  // Generate random maximum voltage values for different axes.
  volts_max[0] = rnd_float(4.5, 5.5) * (float) sin(angle[1]);
  volts_max[1] = rnd_float(4.5, 5.5) * (float) cos(angle[0]);
  volts_max[2] = rnd_float(4.5, 5.5) * (float) cos(angle[1] - angle[0]);

  // Generate a random average amperage value.
  float amps_avg = rnd_float(150, 300);

  // Calculate maximum amperage values for different axes.
  amps_max[0] = (amps_avg + rnd_float(-25.0, 25.0)) * (float) sin(angle[1]);
  amps_max[1] = (amps_avg + rnd_float(-25.0, 25.0)) * (float) cos(angle[0]);
  amps_max[2] = (amps_avg + rnd_float(-25.0, 25.0)) * (float) cos(angle[1] - angle[0]);

  // Generate random values for other telemetry parameters.
  batt = rnd_float(3.8, 4.3);
  speed = rnd_float(1.0, 2.5);
  eclipse = (rnd_float(-1, +4) > 0) ? 1.0 : 0.0;
  period = rnd_float(150, 300);
  tempS = rnd_float(20, 55);
  temp_max = rnd_float(50, 70);
  temp_min = rnd_float(10, 20);
}

#ifdef DEBUG_LOGGING
  for (int i = 0; i < 3; i++)
    printf("axis: %f angle: %f v: %f i: %f \n", axis[i], angle[i], volts_max[i], amps_max[i]);
  printf("batt: %f speed: %f eclipse_time: %f eclipse: %f period: %f temp: %f max: %f min: %f\n", batt, speed, eclipse_time, eclipse, period, tempS, temp_max, temp_min);
#endif

time_start = (long int) millis();  						// Record the current time in milliseconds.

eclipse_time = (long int)(millis() / 1000.0);  			// Calculate the eclipse time in seconds based on the current time.
if (eclipse == 0.0)
  eclipse_time -= period / 2;  							// If starting in eclipse, shorten the eclipse interval.

tx_freq_hz -= tx_channel * 50000;  						// Adjust the transmission frequency based on the selected channel.

if (transmit == FALSE) {
  // Print an error message if no CubeSatSim Band Pass Filter is detected, indicating no transmissions after the CW ID.
  fprintf(stderr, "\nNo CubeSatSim Band Pass Filter detected.  No transmissions after the CW ID.\n");
  fprintf(stderr, " See http://cubesatsim.org/wiki for info about building a CubeSatSim\n\n");
}

if (mode == FSK) {
    // Configuration for Frequency Shift Keying (FSK) mode
    bitRate = 200;                        				// Set the bit rate to 200 bits per second
    rsFrames = 1;                         				// Number of Reed-Solomon frames
    payloads = 1;                        				// Number of payloads
    rsFrameLen = 64;                   					// Reed-Solomon frame length
    headerLen = 6;                       				// Length of the header
    dataLen = 58;                        				// Length of data
    syncBits = 10;                      				// Number of synchronization bits
    syncWord = 0b0011111010;     						// Synchronization word
    parityLen = 32;                    					// Length of parity bits
    amplitude = 32767 / 3;     							// Amplitude of the signal
    samples = S_RATE / bitRate;   						// Number of samples per bit
    bufLen = (frameCnt * (syncBits + 10 * (headerLen + rsFrames * (rsFrameLen + parityLen))) * samples);

    samplePeriod = (int)(((float)((syncBits + 10 * (headerLen + rsFrames * (rsFrameLen + parityLen)))) / (float) bitRate) * 1000 - 500);
    sleepTime = 0.1f;                   		// Sleep time

    frameTime = ((float)((float)bufLen / (samples * frameCnt * bitRate))) * 1000; // Frame time in milliseconds

    printf("\n FSK Mode, %d bits per frame, %d bits per second, %d ms per frame, %d ms sample period\n",
        bufLen / (samples * frameCnt), bitRate, frameTime, samplePeriod);

} else if (mode == BPSK) {
    // Configuration for Binary Phase Shift Keying (BPSK) mode
    bitRate = 1200;                        				// Set the bit rate to 1200 bits per second
    rsFrames = 3;                           			// Number of Reed-Solomon frames
    payloads = 6;                          				// Number of payloads
    rsFrameLen = 159;                   				// Reed-Solomon frame length
    headerLen = 8;                         				// Length of the header
    dataLen = 78;                          				// Length of data
    syncBits = 31;                        				// Number of synchronization bits
    syncWord = 0b1000111110011010010000101011101; 		// Synchronization word
    parityLen = 32;                      				// Length of parity bits
    amplitude = 32767;                 					// Amplitude of the signal
    samples = S_RATE / bitRate;   						// Number of samples per bit
    bufLen = (frameCnt * (syncBits + 10 * (headerLen + rsFrames * (rsFrameLen + parityLen))) * samples);

    samplePeriod = ((float)((syncBits + 10 * (headerLen + rsFrames * (rsFrameLen + parityLen)))) / (float)bitRate) * 1000 - 1800;
    sleepTime = 2.2f;                     				// Sleep time

    frameTime = ((float)((float)bufLen / (samples * frameCnt * bitRate))) * 1000; // Frame time in milliseconds

    printf("\n BPSK Mode, bufLen: %d,  %d bits per frame, %d bits per second, %d ms per frame %d ms sample period\n",
        bufLen, bufLen / (samples * frameCnt), bitRate, frameTime, samplePeriod);

    sin_samples = S_RATE / freq_Hz;  			// Calculate the number of samples for a sine wave
    for (int j = 0; j < sin_samples; j++) {
        sin_map[j] = (short int)(amplitude * sin((float)(2 * M_PI * j / sin_samples))); // Generate a sine wave
    }

    printf("\n");
}

// Initialize arrays voltage, current, sensor, and other with zeros
memset(voltage, 0, sizeof(voltage));
memset(current, 0, sizeof(current));
memset(sensor, 0, sizeof(sensor));
memset(other, 0, sizeof(other));

// Check if mode is FSK or BPSK
if (((mode == FSK) || (mode == BPSK))) 					// && !sim_mode)
    get_tlm_fox();										// Fill transmit buffer with reset count 0 packets that will be ignored
firstTime = 1;

// Check if sim_mode is false
if (!sim_mode)
{
    // Concatenate pythonCmd and busStr into pythonStr
    strcpy(pythonStr, pythonCmd);
    strcat(pythonStr, busStr);

    // Concatenate pythonStr with " c" and store the result in pythonConfigStr
    strcat(pythonConfigStr, pythonStr);
    strcat(pythonConfigStr, " c");

    // Print the value of pythonConfigStr to stderr
    fprintf(stderr, "pythonConfigStr: %s\n", pythonConfigStr);

    // Call the sopen function with pythonConfigStr as an argument and store the result in file1
    file1 = sopen(pythonConfigStr);  					// Python sensor polling function

    // Read up to 1000 characters from file1 and store them in cmdbuffer
    fgets(cmdbuffer, 1000, file1);
    
    // Print the value of cmdbuffer to stderr
    fprintf(stderr, "pythonStr result: %s\n", cmdbuffer);
}

// Initialize arrays voltage_min, current_min, voltage_max, and current_max with specific values
for (int i = 0; i < 9; i++) {
    voltage_min[i] = 1000.0;
    current_min[i] = 1000.0;
    voltage_max[i] = -1000.0;
    current_max[i] = -1000.0;
}

// Initialize arrays sensor_min and sensor_max with specific values
for (int i = 0; i < 17; i++) {
    sensor_min[i] = 1000.0;
    sensor_max[i] = -1000.0;
}

// Initialize arrays other_min and other_max with specific values
for (int i = 0; i < 3; i++) {
    other_min[i] = 1000.0;
    other_max[i] = -1000.0;
}

// Declare a long integer variable loopTime and assign the result of the millis() function to it
long int loopTime;
loopTime = millis();

// Loop while 'loop' decrements until it reaches 0
while (loop-- != 0) {
    // Flush standard output and standard error streams
    fflush(stdout);
    fflush(stderr);

    // Set the first element of sensor_payload to 0
    sensor_payload[0] = 0;

    // Initialize arrays voltage, current, sensor, and other with zeros
    memset(voltage, 0, sizeof(voltage));
    memset(current, 0, sizeof(current));
    memset(sensor, 0, sizeof(sensor));
    memset(other, 0, sizeof(other));

    // Open the /proc/uptime file for reading
    FILE *uptime_file = fopen("/proc/uptime", "r");

    // Read a floating-point value from the file and store it in uptime_sec
    fscanf(uptime_file, "%f", &uptime_sec);

    // Convert uptime_sec to an integer and store it in uptime
    uptime = (int)(uptime_sec + 0.5);

    // Close the uptime_file
    fclose(uptime_file);

    // Print information about reset_count and uptime to stdout
    printf("INFO: Reset Count: %d Uptime since Reset: %ld \n", reset_count, uptime);

    // Flush standard output stream
    fflush(stdout);

    // Calculate and print the loop time in seconds
    printf("++++ Loop time: %5.3f sec +++++\n", (millis() - loopTime) / 1000.0);

    // Flush standard output stream
    fflush(stdout);

    // Update loopTime with the current time
    loopTime = millis();

    // Check if sim_mode is true (simulated telemetry)
    if (sim_mode) {
        // Calculate time and update eclipse mode
        double time = ((long int)millis() - time_start) / 1000.0;

        if ((time - eclipse_time) > period) {
            eclipse = (eclipse == 1) ? 0 : 1;
            eclipse_time = time;
            printf("\n\nSwitching eclipse mode! \n\n");
        }

        // Calculate values Xi, Yi, and Zi based on simulation parameters
double Xi = eclipse * amps_max[0] * (float)sin(2.0 * 3.14 * time / (46.0 * speed)) + rnd_float(-2, 2);
double Yi = eclipse * amps_max[1] * (float)sin((2.0 * 3.14 * time / (46.0 * speed)) + (3.14 / 2.0)) + rnd_float(-2, 2);
double Zi = eclipse * amps_max[2] * (float)sin((2.0 * 3.14 * time / (46.0 * speed)) + 3.14 + angle[2]) + rnd_float(-2, 2);

// Calculate values Xv, Yv, and Zv based on simulation parameters
double Xv = eclipse * volts_max[0] * (float)sin(2.0 * 3.14 * time / (46.0 * speed)) + rnd_float(-0.2, 0.2);
double Yv = eclipse * volts_max[1] * (float)sin((2.0 * 3.14 * time / (46.0 * speed)) + (3.14 / 2.0)) + rnd_float(-0.2, 0.2);
double Zv = 2.0 * eclipse * volts_max[2] * (float)sin((2.0 * 3.14 * time / (46.0 * speed)) + 3.14 + angle[2]) + rnd_float(-0.2, 0.2);

// Update current and voltage arrays based on calculated values
current[map[PLUS_X]] = (Xi >= 0) ? Xi : 0;
current[map[MINUS_X]] = (Xi >= 0) ? 0 : ((-1.0f) * Xi);
current[map[PLUS_Y]] = (Yi >= 0) ? Yi : 0;
current[map[MINUS_Y]] = (Yi >= 0) ? 0 : ((-1.0f) * Yi);
current[map[PLUS_Z]] = (Zi >= 0) ? Zi : 0;
current[map[MINUS_Z]] = (Zi >= 0) ? 0 : ((-1.0f) * Zi);

voltage[map[PLUS_X]] = (Xv >= 1) ? Xv : rnd_float(0.9, 1.1);
voltage[map[MINUS_X]] = (Xv <= -1) ? ((-1.0f) * Xv) : rnd_float(0.9, 1.1);
voltage[map[PLUS_Y]] = (Yv >= 1) ? Yv : rnd_float(0.9, 1.1);
voltage[map[MINUS_Y]] = (Yv <= -1) ? ((-1.0f) * Yv) : rnd_float(0.9, 1.1);
voltage[map[PLUS_Z]] = (Zv >= 1) ? Zv : rnd_float(0.9, 1.1);
voltage[map[MINUS_Z]] = (Zv <= -1) ? ((-1.0f) * Zv) : rnd_float(0.9, 1.1);

// Calculate temperature simulation
tempS += (eclipse > 0) ? ((temp_max - tempS) / 50.0f) : ((temp_min - tempS) / 50.0f);
tempS += rnd_float(-1.0, 1.0);

// Update the IHU_TEMP value in the 'other' array
other[IHU_TEMP] = tempS;

// Simulated values for BUS and BAT
voltage[map[BUS]] = rnd_float(5.0, 5.005);
current[map[BUS]] = rnd_float(158, 171);

// Calculate charging based on current values
float charging = eclipse * (fabs(amps_max[0] * 0.707) + fabs(amps_max[1] * 0.707) + rnd_float(-4.0, 4.0));

// Calculate battery current
current[map[BAT]] = ((current[map[BUS]] * voltage[map[BUS]]) / batt) - charging;

// Adjust battery state based on current conditions
batt -= (batt > 3.5) ? current[map[BAT]] / 30000 : current[map[BAT]] / 3000;
if (batt < 3.0) {
    batt = 3.0;
    SafeMode = 1;
    printf("Safe Mode!\n");
} else
    SafeMode = 0;

if (batt > 4.5)
    batt = 4.5;

// Update battery voltage with some randomness
voltage[map[BAT]] = batt + rnd_float(-0.01, 0.01);

// End of simulated telemetry

// In this block, we're handling the case when we are not in simulation mode.

else {
  int count1; 										// Declare an integer variable for looping.
  char *token; 										// Declare a pointer to a character for tokenization.
  
  // Add a newline character to the file stream 'file1'.
  fputc('\n', file1);
  
  // Read up to 1000 characters from 'file1' and store them in 'cmdbuffer'.
  fgets(cmdbuffer, 1000, file1);
  
  // Print the content of 'cmdbuffer' to the standard error stream.
  fprintf(stderr, "Python read Result: %s\n", cmdbuffer);

  // Define a space character as a delimiter for tokenization.
  const char space[2] = " ";
  
  // Tokenize 'cmdbuffer' based on spaces and initialize 'token'.
  token = strtok(cmdbuffer, space);

  // Loop to process up to 8 tokens.
  for (count1 = 0; count1 < 8; count1++) {
    if (token != NULL) {
      // Convert the token to a floating-point value and store it in 'voltage'.
      voltage[count1] = (float)atof(token);

      #ifdef DEBUG_LOGGING
        // Uncomment the next line to print voltage values (for debugging).
        // printf("voltage: %f ", voltage[count1]);
      #endif
      
      // Move to the next token.
      token = strtok(NULL, space);

      if (token != NULL) {
        // Convert the token to a floating-point value and store it in 'current'.
        current[count1] = (float)atof(token);

        // Ensure that current values are non-negative.
        if ((current[count1] < 0) && (current[count1] > -0.5))
          current[count1] *= (-1.0f);

        #ifdef DEBUG_LOGGING
          // Uncomment the next line to print current values (for debugging).
          // printf("current: %f\n", current[count1]);
        #endif

        // Move to the next token.
        token = strtok(NULL, space);
      }
    }
  }

  // Store battery voltage and current for further use.
  batteryVoltage = voltage[map[BAT]];
  batteryCurrent = current[map[BAT]];

  // Check if battery voltage is below 3.6V and set SafeMode accordingly.
  if (batteryVoltage < 3.6) {
    SafeMode = 1;
    printf("Safe Mode!\n");
  } else
    SafeMode = 0;

  // Open a file to read the CPU temperature sensor data.
  FILE *cpuTempSensor = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
  
  if (cpuTempSensor) {
    double cpuTemp;

    // Read the temperature value from the file and store it in 'cpuTemp'.
    fscanf(cpuTempSensor, "%lf", &cpuTemp);

    // Convert the temperature to degrees Celsius.
    cpuTemp /= 1000;


#ifdef DEBUG_LOGGING
// Print CPU temperature if DEBUG_LOGGING is enabled.
// printf("CPU Temp Read: %6.1f\n", cpuTemp);
#endif

// Assign the CPU temperature to the 'IHU_TEMP' position in the 'other' array.
other[IHU_TEMP] = (double)cpuTemp;

// Calculate IHUcpuTemp, but this line is currently commented out.
// IHUcpuTemp = (int)((cpuTemp * 10.0) + 0.5);

// Close the file associated with the CPU temperature sensor.
fclose(cpuTempSensor);


//--------------------------------STEM COM-------------------------------------------

// Check if 'payload' is ON.
if (payload == ON) {
  STEMBoardFailure = 0;

  char c;
  unsigned int waitTime;
  int i, end, trys = 0;
  sensor_payload[0] = 0;
  sensor_payload[1] = 0;
  
  // Perform a loop to query the payload status with a maximum of 10 tries.
  while (((sensor_payload[0] != 'O') || (sensor_payload[1] != 'K')) && (trys++ < 10)) {
    i = 0;

//-----------------------The '?' permit to have the data from sparkfun-------------

    // Send a '?' character to the UART (serial communication).
    serialPutchar(uart_fd, '?');
    sleep(0.05);  					// Add a small delay after sending '?'
    
    printf("%d Querying payload with ?\n", trys);
    waitTime = millis() + 500;
    end = FALSE;
    
    // Continue looping while waiting for a response or until a timeout.
    while ((millis() < waitTime) && !end) {
      int chars = (char) serialDataAvail(uart_fd);		//chars contiendra le nombre de caractères disponibles pour la lecture dans le tampon d'entrée de l'UART
      
      // Process available characters from the UART.
      while ((chars > 0) && !end) {
        chars--;
        c = (char) serialGetchar(uart_fd);				//tant que le nombre de caractères dans chars ne sera pas a 0 (tant que tampon non vide), recopie dans c les caractères.
        
        // Store received characters in 'sensor_payload' until a newline character is encountered.
        if (c != '\n') {
          sensor_payload[i++] = c;
        } else {
          end = TRUE;
        }
      }
    }
  }
//------------------------------------------------------------------------------

// Set a space character in 'sensor_payload' and increment the index 'i'.
sensor_payload[i++] = ' ';
// 'sensor_payload[i++] = '\n';' is commented out.
// Set a null terminator in 'sensor_payload' to terminate the string.
sensor_payload[i] = '\0';

// Print the response received from the STEM Payload board.
printf("Response from STEM Payload board: %s\n", sensor_payload);

// Add a short delay between loops.
sleep(0.1);

// Check if the response from the STEM Payload board is "OK".
if ((sensor_payload[0] == 'O') && (sensor_payload[1] == 'K')) {
  int count1;
  char *token;
 
  // Define a delimiter for tokenization.
  const char space[2] = " ";
  token = strtok(sensor_payload, space);

  // Parse and process sensor data.
  for (count1 = 0; count1 < 17; count1++) {
    if (token != NULL) {
      // Convert the token to a floating-point value and store it in 'sensor'.
      sensor[count1] = (float)atof(token);

      #ifdef DEBUG_LOGGING
      // Optionally print the sensor data for debugging.
      // printf("sensor: %f ", sensor[count1]);
      #endif
      // Get the next token.
      token = strtok(NULL, space);
    }
  }
  printf("\n");
}
else {
  // If the payload response is not "OK," turn off payload processing.
  payload = OFF;
}

//-----------------write and update on fox telem after com---------------------

// If the payload response is "OK," update sensor_min and sensor_max arrays.
if ((sensor_payload[0] == 'O') && (sensor_payload[1] == 'K')) {
  for (count1 = 0; count1 < 17; count1++) {

    // Update sensor_min and sensor_max based on the received sensor data.
    if (sensor[count1] < sensor_min[count1])
      sensor_min[count1] = sensor[count1];
    if (sensor[count1] > sensor_max[count1])
      sensor_max[count1] = sensor[count1];
  }
}

#ifdef DEBUG_LOGGING
// Print battery voltage and current information for debugging.
fprintf(stderr, "INFO: Battery voltage: %5.2f V  Threshold %5.2f V Current: %6.1f mA Threshold: %6.1f mA\n", batteryVoltage, voltageThreshold, batteryCurrent, currentThreshold);
#endif

// Check if battery conditions indicate a need for shutdown.
// Conditions include low battery voltage and current exceeding the threshold.
// The !sim_mode check ensures this won't happen when running on DC power.
if ((batteryCurrent > currentThreshold) && (batteryVoltage < voltageThreshold) && !sim_mode) {
  // Battery voltage is too low, initiate shutdown process.
  fprintf(stderr, "Battery voltage too low: %f V - shutting down!\n", batteryVoltage);
  
  // Turn off LEDs for visual indication.
  digitalWrite(txLed, txLedOff);
  digitalWrite(onLed, onLedOff);
  
  // Delay and toggle the onLed for visual indication.
  sleep(1);
  digitalWrite(onLed, onLedOn);
  sleep(1);
  digitalWrite(onLed, onLedOff);
  sleep(1);
  digitalWrite(onLed, onLedOn);
  sleep(1);
  digitalWrite(onLed, onLedOff);

  // Log the shutdown process to a file.
  FILE *file6 = popen("/home/pi/CubeSatSim/log > shutdown_log.txt", "r");
  pclose(file6);

  // Initiate a system shutdown command.
  sleep(40);	    
  file6 = popen("sudo shutdown -h now > /dev/null 2>&1", "r");
  pclose(file6);
  sleep(10);
}

// Reset a counter variable.
ctr = 0;

#ifdef DEBUG_LOGGING
// Debug message indicating the retrieval of telemetry data.
fprintf(stderr, "INFO: Getting TLM Data\n");
#endif

// Retrieve telemetry data based on the selected communication mode.
if ((mode == AFSK) || (mode == CW)) {
  // Get telemetry data for AFSK or CW mode.
  get_tlm();
} else if ((mode == FSK) || (mode == BPSK)) {
  // Get telemetry data for FSK or BPSK mode.
  get_tlm_fox();
} else {
  // In SSTV mode, sleep for a specified duration.
  fprintf(stderr, "Sleeping\n");
  sleep(50);	    
}


#ifdef DEBUG_LOGGING
// Debug message indicating preparation for sending data.
fprintf(stderr, "INFO: Getting ready to send\n");
#endif


// The code inside this block is executed when DEBUG_LOGGING is defined.

if (mode == BPSK) {
  // In BPSK mode, toggle the transmission LED for visual indication.
  // digitalWrite(txLed, txLedOn); // (This line is currently commented out.)

  #ifdef DEBUG_LOGGING
  // Debug message indicating the activation of the Tx LED.
  // printf("Tx LED On 1\n");
  #endif

  printf("Sleeping to allow BPSK transmission to finish.\n");
  // Sleep for a duration determined by 'loop_count' to wait for BPSK transmission to complete.
  sleep((unsigned int)(loop_count * 5));
  printf("Done sleeping\n");
  // Turn off the Tx LED.
  // digitalWrite(txLed, txLedOff); // (This line is currently commented out.)

  #ifdef DEBUG_LOGGING
  // Debug message indicating the deactivation of the Tx LED.
  // printf("Tx LED Off\n");
  #endif

} else if (mode == FSK) {
  printf("Sleeping to allow FSK transmission to finish.\n");
  // Sleep for a duration determined by 'loop_count' to wait for FSK transmission to complete.
  sleep((unsigned int)loop_count);
  printf("Done sleeping\n");
}

return 0;
// This line indicates that the function is returning an integer value of 0, typically indicating successful execution.

// Returns the lower digit of a number which must be less than 99
int lower_digit(int number) {
  int digit = 0;
  if (number < 100) {
    // Check if the input number is less than 100 (two digits).
    digit = number - ((int)(number / 10) * 10);
    // Calculate the lower digit by subtracting the tens place value.
  } else {
    fprintf(stderr, "ERROR: Not a digit in lower_digit!\n");
    // Print an error message if the input number is not within the expected range.
  }
  return digit;
}

// Returns the upper digit of a number which must be less than 99
int upper_digit(int number) {
  int digit = 0;
  if (number < 100) {
    // Check if the input number is less than 100 (two digits).
    digit = (int)(number / 10);
    // Calculate the upper digit by dividing the number by 10 (truncating the decimal part).
  } else {
    fprintf(stderr, "ERROR: Not a digit in upper_digit!\n");
    // Print an error message if the input number is not within the expected range.
  }
  return digit;
}

//-----------------------------Init de la radio-----------------------------------

static int init_rf() {
  int ret;
  fprintf(stderr, "Initializing AX5043\n");
  
  // Initialize the AX5043 radio.
  ret = ax5043_init(&hax5043, XTAL_FREQ_HZ, VCO_INTERNAL);
  if (ret != PQWS_SUCCESS) {
    fprintf(stderr,
      "ERROR: Failed to initialize AX5043 with error code %d\n", ret);
    // Print an error message if the initialization fails.
    // exit(EXIT_FAILURE); // (This line is currently commented out.)
    return (0);
    // Return 0 to indicate failure.
  }
  return (1);
  // Return 1 to indicate successful initialization of the AX5043 radio.
}

//-------------------------Definition fonction get_tlm----------------------------

void get_tlm(void) {
  // This function retrieves telemetry data.

  FILE *txResult;

  for (int j = 0; j < frameCnt; j++) {
    // Loop through the frames.

    fflush(stdout);
    fflush(stderr);

    int tlm[7][5];
    memset(tlm, 0, sizeof(tlm));
    // Initialize a 2D array for telemetry data and set all elements to 0.

    // Calculate telemetry values and store them in the telemetry array.
    tlm[1][A] = (int)(voltage[map[BUS]] / 15.0 + 0.5) % 100; 				// Current of 5V supply to Pi   changement ici
    tlm[1][B] = (int)(99.5 - current[map[PLUS_X]] / 10.0) % 100; 			// +X current [4]
    tlm[1][C] = (int)(99.5 - current[map[MINUS_X]] / 10.0) % 100; 			// X- current [10]
    tlm[1][D] = (int)(99.5 - current[map[PLUS_Y]] / 10.0) % 100; 			// +Y current [7]

    tlm[2][A] = (int)(99.5 - current[map[MINUS_Y]] / 10.0) % 100; 			// -Y current [10]
    tlm[2][B] = (int)(99.5 - current[map[PLUS_Z]] / 10.0) % 100; 			// +Z current [10]
    tlm[2][C] = (int)(99.5 - current[map[MINUS_Z]] / 10.0) % 100; 			// -Z current (was timestamp)
    tlm[2][D] = (int)(50.5 + current[map[BAT]] / 10.0) % 100; 				// NiMH Battery current

    // Calculate battery voltage telemetry value based on voltage range.
    if (voltage[map[BAT]] > 4.6)
      tlm[3][A] = (int)((voltage[map[BAT]] * 10.0) - 65.5) % 100; 			// 7.0 - 10.0 V for old 9V battery
    else
      tlm[3][A] = (int)((voltage[map[BAT]] * 10.0) + 44.5) % 100; 			// 0 - 4.5 V for new 3-cell battery
	  
    tlm[3][B] = (int)(voltage[map[BUS]] * 10.0) % 100; 					// 5V supply to Pi

    tlm[4][A] = (int)((95.8 - other[IHU_TEMP]) / 1.48 + 0.5) % 100;
    // Calculate and store IHU temperature telemetry value.

    tlm[6][B] = 0;
    tlm[6][D] = 49 + rand() % 3;
    // Set specific telemetry values, like [B] and [D], with random values.
  

// Initialize character arrays for constructing the transmission string and telemetry data
char str[1000];
char tlm_str[1000];

// Initialize header strings and related variables
char header_str[] = "\x03\xf0";  							// Control character-based header
char header_str3[] = "echo '";    							// Alternative telemetry header
char header_str2[] = "-11>APCSS:"; 							// APRS header
char header_str2b[30];           							// Storage for APRS coordinates
char header_lat[10];             							// Storage for latitude
char header_long[10];            							// Storage for longitude
char header_str4[] = "hi hi ";   							// Alternative header
char footer_str1[] = "\' > t.txt && echo \'";
char footer_str[] = "-11>APCSS:010101/hi hi ' >> t.txt && touch /home/pi/CubeSatSim/ready";

// Construct the transmission string based on conditions
if (ax5043) {
  strcpy(str, header_str); 									// Copy the control character-based header
} else {
  strcpy(str, header_str3); 								// Copy the alternative telemetry header
}

if (mode == AFSK) {
  strcat(str, call);      									// Append the 'call' variable
  strcat(str, header_str2); 								// Append the APRS header for AFSK mode
}
}

if (mode != CW) {
  // Constructing APRS latitude and longitude strings
  // Example format: "4003.79N\07534.33WShi hi"

  // Check if latitude is positive
  if (latitude > 0)
    sprintf(header_lat, "%7.2f%c", latitude, 'N'); 			// Format latitude as "4003.79N"
  else
    sprintf(header_lat, "%7.2f%c", latitude * (-1.0), 'S'); // Format latitude as "4003.79S" (negative)

  // Check if longitude is positive
  if (longitude > 0)
    sprintf(header_long, "%08.2f%c", longitude , 'E'); 								// Format longitude as "07534.33E"
  else
    sprintf(header_long, "%08.2f%c", longitude * (-1.0), 'W'); 						// Format longitude as "07534.33W" (negative)

  // Construct the APRS coordinate string based on latitude and longitude
  if (ax5043)
    sprintf(header_str2b, "=%s%c%sShi hi ", header_lat, 0x5c, header_long); 		// Example: "4003.79N\07534.33WShi hi" for AX5043
  else
    sprintf(header_str2b, "=%s%c%c%sShi hi ", header_lat, 0x5c, 0x5c, header_long); // Example: "4003.79N\\07534.33WShi hi" for non-AX5043

  // Append the constructed APRS coordinate string to the transmission string
  strcat(str, header_str2b);
} else {
  // Append an alternative header string for the CW mode
  strcat(str, header_str4);
}

// Print the constructed transmission string for debugging
printf("Str: %s \n", str);

// Iterate through telemetry channels and append telemetry data to the transmission string
int channel;
for (channel = 1; channel < 7; channel++) {
  // Format telemetry data and append to the transmission string
  sprintf(tlm_str, "%d%d%d %d%d%d %d%d%d %d%d%d ",
    channel, upper_digit(tlm[channel][1]), lower_digit(tlm[channel][1]),
    channel, upper_digit(tlm[channel][2]), lower_digit(tlm[channel][2]),
    channel, upper_digit(tlm[channel][3]), lower_digit(tlm[channel][3]),
    channel, upper_digit(tlm[channel][4]), lower_digit(tlm[channel][4]));

  // Append the telemetry data string to the transmission string
  strcat(str, tlm_str);
}

if (mode == CW) {

  char cw_str2[1000];
  char cw_header2[] = "echo '";
  char cw_footer2[] = "' > id.txt && gen_packets -M 20 id.txt -o morse.wav -r 48000 > /dev/null 2>&1 && cat morse.wav | csdr convert_i16_f | csdr gain_ff 7000 | csdr convert_f_samplerf 20833 | sudo /home/pi/rpitx/rpitx -i- -m RF -f 434.897e3";
  char cw_footer3[] = "' > cw.txt && touch /home/pi/CubeSatSim/cwready";  		// Transmission is done by rpitx.py

  // Append the CW footer to the telemetry string
  strcat(str, cw_footer3);
  printf("CW string to execute: %s\n", str);
  fflush(stdout);

  // Execute the CW transmission command
  FILE *cw_file = popen(str, "r");
  pclose(cw_file);

  // Wait for rpitx to finish transmitting
  while ((cw_file = fopen("/home/pi/CubeSatSim/cwready", "r")) != NULL) {
    fclose(cw_file);
    sleep(5);  										// Sleep while waiting for rpitx to finish
  }
}


//----------------------------refléchie----------------------------------------


else if (ax5043) {
  // Enable the transmission LED and print an INFO message
  digitalWrite(txLed, txLedOn);
  fprintf(stderr, "INFO: Transmitting X.25 packet using AX5043\n");

  // Copy telemetry data from 'str' to 'data' and print it
  memcpy(data, str, strnlen(str, 256));
  printf("data: %s \n", data);

  // Transmit the AX.25 frame using AX5043
  int ret = ax25_tx_frame(&hax25, &hax5043, data, strnlen(str, 256));
  if (ret) {
    fprintf(stderr,
      "ERROR: Failed to transmit AX.25 frame with error code %d\n",
      ret);
    exit(EXIT_FAILURE);
  }

  // Wait for the AX5043 to finish transmission and turn off the LED
  ax5043_wait_for_transmit();
  digitalWrite(txLed, txLedOff);

  if (ret) {
    fprintf(stderr,
      "ERROR: Failed to transmit entire AX.25 frame with error code %d\n",
      ret);
    exit(EXIT_FAILURE);
  }

  // Sleep for 4 seconds
  sleep(4);  												// was 2
} else {  													// APRS using rpitx
  // Construct the full transmission string by concatenating header and footer
  strcat(str, footer_str1);
  strcat(str, call);
  strcat(str, footer_str);

  // Print the telemetry string
  printf("\n\nTelemetry string is %s \n\n", str);

//-------------------------------transmission with the file----------------------------

  if (transmit) {
    // Execute the transmission command using rpitx
    FILE *file2 = popen(str, "r");
    pclose(file2);

    // Sleep for 2 seconds and turn off the LED
    sleep(2);
    digitalWrite(txLed, txLedOff);
  } else {
    // If no CubeSatSim Band Pass Filter is detected, print a message
    fprintf(stderr, "\nNo CubeSatSim Band Pass Filter detected.  No transmissions after the CW ID.\n");
    fprintf(stderr, " See http://cubesatsim.org/wiki for info about building a CubeSatSim\n\n");
  }

  // Sleep for 3 seconds
  sleep(3);
}

return;
}

//----------------------------------------------------------------------------------------
//---------------------------get_tlm_fox fonction-----------------------------------------
//----------------------------------------------------------------------------------------

void get_tlm_fox() {
  int i;

  // Initialize the sync variable with the syncWord
  long int sync = syncWord;

  // Calculate the 'smaller' value based on sample rate and frequency
  smaller = (int)(S_RATE / (2 * freq_Hz));

  // Initialize arrays for telemetry data (b, b_max, b_min)
  short int b[dataLen];
  short int b_max[dataLen];
  short int b_min[dataLen];
	
  // Set all elements in these arrays to zero
  memset(b, 0, sizeof(b));
  memset(b_max, 0, sizeof(b_max));
  memset(b_min, 0, sizeof(b_min));
	
  // Initialize an array for the header data (h) and set all elements to zero
  short int h[headerLen];
  memset(h, 0, sizeof(h));

  // Initialize the 'buffer' array with 0xa5 values
  memset(buffer, 0xa5, sizeof(buffer));

  // Initialize arrays and variables related to Reed-Solomon (RS) frames and parity checks
  short int rs_frame[rsFrames][223];
  unsigned char parities[rsFrames][parityLen], inputByte;

  // Initialize various telemetry-related variables
  int id, frm_type = 0x01, NormalModeFailure = 0, groundCommandCount = 0;
  int PayloadFailure1 = 0, PayloadFailure2 = 0;
  int PSUVoltage = 0, PSUCurrent = 0, Resets = 0, Rssi = 2048;
  int batt_a_v = 0, batt_b_v = 0, batt_c_v = 0, battCurr = 0;
  int posXv = 0, negXv = 0, posYv = 0, negYv = 0, posZv = 0, negZv = 0;
  int posXi = 0, negXi = 0, posYi = 0, negYi = 0, posZi = 0, negZi = 0;
  int head_offset = 0;

  // Initialize a buffer 'buffer_test' for testing purposes
  short int buffer_test[bufLen];
  int buffSize;

  // Set 'buffSize' to the size of the 'buffer_test' array
  buffSize = (int)sizeof(buffer_test);

if (mode == FSK)
    id = 7;
else
    id = 0; 																	// 99 in h[6]

// Loop through frames based on frameCnt
for (int frames = 0; frames < frameCnt; frames++) {
  
    if (firstTime != ON) {
        // Delay for sample period
      
        // Code section for delaying execution
        int startSleep = millis();
        if ((millis() - sampleTime) < ((unsigned int)frameTime - 250))  
            sleep(2.0); 														// Initial delay
        while ((millis() - sampleTime) < ((unsigned int)frameTime - 250))  
            sleep(0.1); 														// Subsequent sleep periods
        
        printf("Sleep period: %d\n", millis() - startSleep);
        fflush(stdout);
      
        sampleTime = (unsigned int) millis();
    } else
        printf("first time - no sleep\n");

    // Code to process telemetry data based on the mode
    {
        // Update min and max values for voltage and current
        for (int count1 = 0; count1 < 8; count1++) {
            if (voltage[count1] < voltage_min[count1])
                voltage_min[count1] = voltage[count1];
            if (current[count1] < current_min[count1])
                current_min[count1] = current[count1];
      
            if (voltage[count1] > voltage_max[count1])
                voltage_max[count1] = voltage[count1];
            if (current[count1] > current_max[count1])
                current_max[count1] = current[count1];
        }

        // Update min and max values for other telemetry data
        for (int count1 = 0; count1 < 3; count1++) {
            if (other[count1] < other_min[count1])
                other_min[count1] = other[count1];
            if (other[count1] > other_max[count1])
                other_max[count1] = other[count1];
        }
        
        // Determine frame type based on mode and loop count
        if (mode == FSK) {
            if (loop % 32 == 0) {
                // Sending MIN frame
                printf("Sending MIN frame \n");
                frm_type = 0x03;
                for (int count1 = 0; count1 < 17; count1++) {
                    if (count1 < 3)
                        other[count1] = other_min[count1];
                    if (count1 < 8) {
                        voltage[count1] = voltage_min[count1];
                        current[count1] = current_min[count1];
                    }
                    if (sensor_min[count1] != 1000.0) // Ensure values are valid
                        sensor[count1] = sensor_min[count1];
                }
            }
            if ((loop + 16) % 32 == 0) {
                // Sending MAX frame
                printf("Sending MAX frame \n");
                frm_type = 0x02;
                for (int count1 = 0; count1 < 17; count1++) {
                    if (count1 < 3)
                        other[count1] = other_max[count1];
                    if (count1 < 8) {
                        voltage[count1] = voltage_max[count1];
                        current[count1] = current_max[count1];
                    }
                    if (sensor_max[count1] != -1000.0) // Ensure values are valid
                        sensor[count1] = sensor_max[count1];
                }
            }
        } else
            frm_type = 0x02;  // For BPSK, always send MAX MIN frame
    }

    // Clear sensor_payload for the next payload
    sensor_payload[0] = 0;

// Initialize rs_frame and parities arrays with zeros
memset(rs_frame, 0, sizeof(rs_frame));
memset(parities, 0, sizeof(parities));

// Modify the first byte of the header 'h' with the telemetry ID
h[0] = (short int)((h[0] & 0xf8) | (id & 0x07)); // 3 bits

// Update reset count in the header if uptime is not zero
if (uptime != 0) {
    h[0] = (short int)((h[0] & 0x07) | ((reset_count & 0x1f) << 3));
    h[1] = (short int)((reset_count >> 5) & 0xff);
    h[2] = (short int)((h[2] & 0xf8) | ((reset_count >> 13) & 0x07));
}

// Update uptime values in the header
h[2] = (short int)((h[2] & 0x0e) | ((uptime & 0x1f) << 3));
h[3] = (short int)((uptime >> 5) & 0xff);
h[4] = (short int)((uptime >> 13) & 0xff);
h[5] = (short int)((h[5] & 0xf0) | ((uptime >> 21) & 0x0f));

// Set the frame type in the header
h[5] = (short int)((h[5] & 0x0f) | (frm_type << 4));

// If the mode is BPSK, set a specific value in the header
if (mode == BPSK)
    h[6] = 99;

// Calculate and update current and voltage values for telemetry
posXi = (int)(current[map[PLUS_X]] + 0.5) + 2048;
posYi = (int)(current[map[PLUS_Y]] + 0.5) + 2048;
posZi = (int)(current[map[PLUS_Z]] + 0.5) + 2048;
negXi = (int)(current[map[MINUS_X]] + 0.5) + 2048;
negYi = (int)(current[map[MINUS_Y]] + 0.5) + 2048;
negZi = (int)(current[map[MINUS_Z]] + 0.5) + 2048;

posXv = (int)(voltage[map[PLUS_X]] * 100);
posYv = (int)(voltage[map[PLUS_Y]] * 100);
posZv = (int)(voltage[map[PLUS_Z]] * 100);
negXv = (int)(voltage[map[MINUS_X]] * 100);
negYv = (int)(voltage[map[MINUS_Y]] * 100);
negZv = (int)(voltage[map[MINUS_Z]] * 100);

batt_c_v = (int)(voltage[map[BAT]] * 100);
battCurr = (int)(current[map[BAT]] + 0.5) + 2048;
PSUVoltage = (int)(voltage[map[BUS]] * 100);
PSUCurrent = (int)(current[map[BUS]] + 0.5) + 2048;

// Reset STEM board failure flag if payload is ON
if (payload == ON)
    STEMBoardFailure = 0;

// Encode various telemetry values into the 'b' array
encodeA(b, 0 + head_offset, batt_a_v);
encodeB(b, 1 + head_offset, batt_b_v);
encodeA(b, 3 + head_offset, batt_c_v);
encodeB(b, 4 + head_offset, (int)(sensor[ACCEL_X] * 100 + 0.5) + 2048); 		// Xaccel
encodeA(b, 6 + head_offset, (int)(sensor[ACCEL_Y] * 100 + 0.5) + 2048); 		// Yaccel
encodeB(b, 7 + head_offset, (int)(sensor[ACCEL_Z] * 100 + 0.5) + 2048); 		// Zaccel
encodeA(b, 9 + head_offset, battCurr);
encodeB(b, 10 + head_offset, (int)(sensor[TEMP] * 10 + 0.5)); 					// Temp

if (mode == FSK) {
  // Encoding sensor values for FSK mode
  // Position values
  encodeA(b, 12 + head_offset, posXv);
  encodeB(b, 13 + head_offset, negXv);
  encodeA(b, 15 + head_offset, posYv);
  encodeB(b, 16 + head_offset, negYv);
  encodeA(b, 18 + head_offset, posZv);
  encodeB(b, 19 + head_offset, negZv);

  // Current values
  encodeA(b, 21 + head_offset, posXi);
  encodeB(b, 22 + head_offset, negXi);
  encodeA(b, 24 + head_offset, posYi);
  encodeB(b, 25 + head_offset, negYi);
  encodeA(b, 27 + head_offset, posZi);
  encodeB(b, 28 + head_offset, negZi);
} else // BPSK
{
  // Encoding sensor values for BPSK mode
  // Position values
  encodeA(b, 12 + head_offset, posXv);
  encodeB(b, 13 + head_offset, posYv);
  encodeA(b, 15 + head_offset, posZv);
  encodeB(b, 16 + head_offset, negXv);
  encodeA(b, 18 + head_offset, negYv);
  encodeB(b, 19 + head_offset, negZv);

  // Current values
  encodeA(b, 21 + head_offset, posXi);
  encodeB(b, 22 + head_offset, posYi);
  encodeA(b, 24 + head_offset, posZi);
  encodeB(b, 25 + head_offset, negXi);
  encodeA(b, 27 + head_offset, negYi);
  encodeB(b, 28 + head_offset, negZi);
    
  // Encoding maximum sensor values for BPSK mode into a different array b_max
  // Voltage values
  encodeA(b_max, 12 + head_offset, (int)(voltage_max[map[PLUS_X]] * 100));
  encodeB(b_max, 13 + head_offset, (int)(voltage_max[map[PLUS_Y]] * 100));
  encodeA(b_max, 15 + head_offset, (int)(voltage_max[map[PLUS_Z]] * 100));
  encodeB(b_max, 16 + head_offset, (int)(voltage_max[map[MINUS_X]] * 100));
  encodeA(b_max, 18 + head_offset, (int)(voltage_max[map[MINUS_Y]] * 100));
  encodeB(b_max, 19 + head_offset, (int)(voltage_max[map[MINUS_Z]] * 100));

  // Current values
  encodeA(b_max, 21 + head_offset, (int)(current_max[map[PLUS_X]] + 0.5) + 2048);
  encodeB(b_max, 22 + head_offset, (int)(current_max[map[PLUS_Y]] + 0.5) + 2048);
  encodeA(b_max, 24 + head_offset, (int)(current_max[map[PLUS_Z]] + 0.5) + 2048);
  encodeB(b_max, 25 + head_offset, (int)(current_max[map[MINUS_X]] + 0.5) + 2048);
  encodeA(b_max, 27 + head_offset, (int)(current_max[map[MINUS_Y]] + 0.5) + 2048);
  encodeB(b_max, 28 + head_offset, (int)(current_max[map[MINUS_Z]] + 0.5) + 2048);

  // Other sensor values
  encodeA(b_max, 9 + head_offset, (int)(current_max[map[BAT]] + 0.5) + 2048);
  encodeA(b_max, 3 + head_offset, (int)(voltage_max[map[BAT]] * 100));
  encodeA(b_max, 30 + head_offset, (int)(voltage_max[map[BUS]] * 100));
  encodeB(b_max, 46 + head_offset, (int)(current_max[map[BUS]] + 0.5) + 2048);
    
  encodeB(b_max, 37 + head_offset, (int)(other_max[RSSI] + 0.5) + 2048);    
  encodeA(b_max, 39 + head_offset, (int)(other_max[IHU_TEMP] * 10 + 0.5));
  encodeB(b_max, 31 + head_offset, ((int)(other_max[SPIN] * 10)) + 2048);
}

if (sensor_min[0] != 1000.0) // Make sure values are valid
{
  // Encoding sensor maximum values when the minimum value is not equal to 1000.0 (indicating valid values)

  // Encode X accelerometer maximum value
  encodeB(b_max, 4 + head_offset, (int)(sensor_max[ACCEL_X] * 100 + 0.5) + 2048);

  // Encode Y accelerometer maximum value
  encodeA(b_max, 6 + head_offset, (int)(sensor_max[ACCEL_Y] * 100 + 0.5) + 2048);

  // Encode Z accelerometer maximum value
  encodeB(b_max, 7 + head_offset, (int)(sensor_max[ACCEL_Z] * 100 + 0.5) + 2048);

  // Encode pressure maximum value
  encodeA(b_max, 33 + head_offset, (int)(sensor_max[PRES] + 0.5));

  // Encode altitude maximum value
  encodeB(b_max, 34 + head_offset, (int)(sensor_max[ALT] * 10.0 + 0.5));

  // Encode X-axis gyro maximum value
  encodeB(b_max, 40 + head_offset, (int)(sensor_max[GYRO_X] + 0.5) + 2048);

  // Encode Y-axis gyro maximum value
  encodeA(b_max, 42 + head_offset, (int)(sensor_max[GYRO_Y] + 0.5) + 2048);

  // Encode Z-axis gyro maximum value
  encodeB(b_max, 43 + head_offset, (int)(sensor_max[GYRO_Z] + 0.5) + 2048);

  // Encode XS1 maximum value
  encodeA(b_max, 48 + head_offset, (int)(sensor_max[XS1] * 10 + 0.5) + 2048);

  // Encode XS2 maximum value
  encodeB(b_max, 49 + head_offset, (int)(sensor_max[XS2] * 10 + 0.5) + 2048);

  // Encode temperature maximum value
  encodeB(b_max, 10 + head_offset, (int)(sensor_max[TEMP] * 10 + 0.5));

  // Encode humidity maximum value
  encodeA(b_max, 45 + head_offset, (int)(sensor_max[HUMI] * 10 + 0.5));
}
else
{
  // Encoding default values (2048) for all sensor maximums when the minimum value is equal to 1000.0 (indicating invalid values)

  encodeB(b_max, 4 + head_offset, 2048); // X accelerometer
  encodeA(b_max, 6 + head_offset, 2048); // Y accelerometer
  encodeB(b_max, 7 + head_offset, 2048); // Z accelerometer

  encodeB(b_max, 40 + head_offset, 2048); // X-axis gyro
  encodeA(b_max, 42 + head_offset, 2048); // Y-axis gyro
  encodeB(b_max, 43 + head_offset, 2048); // Z-axis gyro

  encodeA(b_max, 48 + head_offset, 2048); // XS1
  encodeB(b_max, 49 + head_offset, 2048); // XS2

  encodeB(b_max, 10 + head_offset, 2048); // Temperature
  encodeA(b_max, 45 + head_offset, 2048); // Humidity
}


// Encode minimum values for various parameters into the 'b_min' array

// Encode minimum voltage values for different axes
encodeA(b_min, 12 + head_offset, (int)(voltage_min[map[PLUS_X]] * 100));
encodeB(b_min, 13 + head_offset, (int)(voltage_min[map[PLUS_Y]] * 100));
encodeA(b_min, 15 + head_offset, (int)(voltage_min[map[PLUS_Z]] * 100));
encodeB(b_min, 16 + head_offset, (int)(voltage_min[map[MINUS_X]] * 100));
encodeA(b_min, 18 + head_offset, (int)(voltage_min[map[MINUS_Y]] * 100));
encodeB(b_min, 19 + head_offset, (int)(voltage_min[map[MINUS_Z]] * 100));

// Encode minimum current values for different axes, with offset and scaling
encodeA(b_min, 21 + head_offset, (int)(current_min[map[PLUS_X]] + 0.5) + 2048);
encodeB(b_min, 22 + head_offset, (int)(current_min[map[PLUS_Y]] + 0.5) + 2048);
encodeA(b_min, 24 + head_offset, (int)(current_min[map[PLUS_Z]] + 0.5) + 2048);
encodeB(b_min, 25 + head_offset, (int)(current_min[map[MINUS_X]] + 0.5) + 2048);
encodeA(b_min, 27 + head_offset, (int)(current_min[map[MINUS_Y]] + 0.5) + 2048);
encodeB(b_min, 28 + head_offset, (int)(current_min[map[MINUS_Z]] + 0.5) + 2048);

// Encode minimum current and voltage values for battery and bus
encodeA(b_min, 9 + head_offset, (int)(current_min[map[BAT]] + 0.5) + 2048);
encodeA(b_min, 3 + head_offset, (int)(voltage_min[map[BAT]] * 100));
encodeA(b_min, 30 + head_offset, (int)(voltage_min[map[BUS]] * 100));
encodeB(b_min, 46 + head_offset, (int)(current_min[map[BUS]] + 0.5) + 2048);

// Encode minimum values for other parameters

// Encode minimum spin value with offset and scaling
encodeB(b_min, 31 + head_offset, ((int)(other_min[SPIN] * 10)) + 2048);

// Encode minimum RSSI value with offset
encodeB(b_min, 37 + head_offset, (int)(other_min[RSSI] + 0.5) + 2048);

// Encode minimum IHU temperature value with offset and scaling
encodeA(b_min, 39 + head_offset, (int)(other_min[IHU_TEMP] * 10 + 0.5));

// Check if sensor minimum values are valid
if (sensor_min[0] != 1000.0)
{
  // Encode minimum values for various sensors when values are valid

  // Encode minimum X accelerometer value with offset and scaling
  encodeB(b_min, 4 + head_offset, (int)(sensor_min[ACCEL_X] * 100 + 0.5) + 2048);

  // Encode minimum Y accelerometer value with offset and scaling
  encodeA(b_min, 6 + head_offset, (int)(sensor_min[ACCEL_Y] * 100 + 0.5) + 2048);

  // Encode minimum Z accelerometer value with offset and scaling
  encodeB(b_min, 7 + head_offset, (int)(sensor_min[ACCEL_Z] * 100 + 0.5) + 2048);

  // Encode minimum pressure value with offset
  encodeA(b_min, 33 + head_offset, (int)(sensor_min[PRES] + 0.5));

  // Encode minimum altitude value with offset and scaling
  encodeB(b_min, 34 + head_offset, (int)(sensor_min[ALT] * 10.0 + 0.5));

  // Encode minimum X-axis gyro value with offset and scaling
  encodeB(b_min, 40 + head_offset, (int)(sensor_min[GYRO_X] + 0.5) + 2048);

  // Encode minimum Y-axis gyro value with offset and scaling
  encodeA(b_min, 42 + head_offset, (int)(sensor_min[GYRO_Y] + 0.5) + 2048);

  // Encode minimum Z-axis gyro value with offset and scaling
  encodeB(b_min, 43 + head_offset, (int)(sensor_min[GYRO_Z] + 0.5) + 2048);

  // Encode minimum XS1 value with offset and scaling
  encodeA(b_min, 48 + head_offset, (int)(sensor_min[XS1] * 10 + 0.5) + 2048);

  // Encode minimum XS2 value with offset and scaling
  encodeB(b_min, 49 + head_offset, (int)(sensor_min[XS2] * 10 + 0.5) + 2048);

  // Encode minimum temperature value with offset and scaling
  encodeB(b_min, 10 + head_offset, (int)(sensor_min[TEMP] * 10 + 0.5));

  // Encode minimum humidity value with offset and scaling
  encodeA(b_min, 45 + head_offset, (int)(sensor_min[HUMI] * 10 + 0.5));
}
else
{
  // Encode default minimum values (2048) for all sensors when values are invalid

  encodeB(b_min, 4 + head_offset, 2048); // X accelerometer
  encodeA(b_min, 6 + head_offset, 2048); // Y accelerometer
  encodeB(b_min, 7 + head_offset, 2048); // Z accelerometer

  encodeB(b_min, 40 + head_offset, 2048); // X-axis gyro
  encodeA(b_min, 42 + head_offset, 2048); // Y-axis gyro
  encodeB(b_min, 43 + head_offset, 2048); // Z-axis gyro

  encodeA(b_min, 48 + head_offset, 2048); // XS1
  encodeB(b_min, 49 + head_offset, 2048); // XS2

  encodeB(b_min, 10 + head_offset, 2048); // Temperature
  encodeA(b_min, 45 + head_offset, 2048); // Humidity
}
}

// Encode various values into the 'b' array

// Encode PSU voltage (PSUVoltage) with offset
encodeA(b, 30 + head_offset, PSUVoltage);

// Encode spin value (SPIN) with offset and scaling
encodeB(b, 31 + head_offset, ((int)(other[SPIN] * 10)) + 2048);

// Encode pressure (PRES) with offset and rounding
encodeA(b, 33 + head_offset, (int)(sensor[PRES] + 0.5)); // Pressure

// Encode altitude (ALT) with offset, scaling, and rounding
encodeB(b, 34 + head_offset, (int)(sensor[ALT] * 10.0 + 0.5)); // Altitude

// Encode number of resets (Resets) with offset
encodeA(b, 36 + head_offset, Resets);

// Encode RSSI (Received Signal Strength Indicator) with offset and scaling
encodeB(b, 37 + head_offset, (int)(other[RSSI] + 0.5) + 2048);

// Encode IHU temperature (IHU_TEMP) with offset and scaling
encodeA(b, 39 + head_offset, (int)(other[IHU_TEMP] * 10 + 0.5));

// Encode X-axis gyro value with offset and scaling
encodeB(b, 40 + head_offset, (int)(sensor[GYRO_X] + 0.5) + 2048);

// Encode Y-axis gyro value with offset and scaling
encodeA(b, 42 + head_offset, (int)(sensor[GYRO_Y] + 0.5) + 2048);

// Encode Z-axis gyro value with offset and scaling
encodeB(b, 43 + head_offset, (int)(sensor[GYRO_Z] + 0.5) + 2048);

// Encode humidity (HUMI) with offset, scaling, and rounding
encodeA(b, 45 + head_offset, (int)(sensor[HUMI] * 10 + 0.5)); // in place of sensor1

// Encode PSU current (PSUCurrent) with offset
encodeB(b, 46 + head_offset, PSUCurrent);

// Encode XS1 value with offset and scaling
encodeA(b, 48 + head_offset, (int)(sensor[XS1] * 10 + 0.5) + 2048);

// Encode XS2 value with offset and scaling
encodeB(b, 49 + head_offset, (int)(sensor[XS2] * 10 + 0.5) + 2048);

// Calculate 'status' as a combination of multiple flags
int status = STEMBoardFailure + SafeMode * 2 + sim_mode * 4 + PayloadFailure1 * 8 +
  (i2c_bus0 == OFF) * 16 + (i2c_bus1 == OFF) * 32 + (i2c_bus3 == OFF) * 64 + (camera == OFF) * 128 + groundCommandCount * 256;

// Encode 'status' with offset
encodeA(b, 51 + head_offset, status);

// Encode antenna deployment status flags
encodeB(b, 52 + head_offset, rxAntennaDeployed + txAntennaDeployed * 2);

// Check if txAntennaDeployed is 0 and set it to 1 if true, also print a message
if (txAntennaDeployed == 0) {
  txAntennaDeployed = 1;
  printf("TX Antenna Deployed!\n");
}

// Check if the mode is BPSK (field experiments)
if (mode == BPSK) {
  unsigned long val = 0xffff;

  // Encode specific values for BPSK mode
  encodeA(b, 64 + head_offset, 0xff & val); 
  encodeA(b, 65 + head_offset, val >> 8); 	    
  encodeA(b, 63 + head_offset, 0x00); 
  encodeA(b, 62 + head_offset, 0x01);
  encodeB(b, 74 + head_offset, 0xfff); 
}

// Define two arrays of short integers
short int data10[headerLen + rsFrames * (rsFrameLen + parityLen)];
short int data8[headerLen + rsFrames * (rsFrameLen + parityLen)];

// Initialize counters
int ctr1 = 0; // Counter for header data
int ctr3 = 0; // Counter for payload data

// Loop through the Reed-Solomon frames and symbols within each frame
for (i = 0; i < rsFrameLen; i++) {
  for (int j = 0; j < rsFrames; j++) {
    // Skip the last symbol in the last frame for BPSK mode
    if (!((i == (rsFrameLen - 1)) && (j == 2))) {
      if (ctr1 < headerLen) {
        // Fill Reed-Solomon frame with header data
        rs_frame[j][i] = h[ctr1];
        update_rs(parities[j], h[ctr1]);
        data8[ctr1++] = rs_frame[j][i];
      } else {
        // Fill Reed-Solomon frame with payload data
        if (mode == FSK) {
          rs_frame[j][i] = b[ctr3 % dataLen];
          update_rs(parities[j], b[ctr3 % dataLen]);
        } else { // BPSK mode
          if ((int)(ctr3/dataLen) == 3) {
            rs_frame[j][i] = b_max[ctr3 % dataLen];
            update_rs(parities[j], b_max[ctr3 % dataLen]);
          } else if ((int)(ctr3/dataLen) == 4) {
            rs_frame[j][i] = b_min[ctr3 % dataLen];
            update_rs(parities[j], b_min[ctr3 % dataLen]);
          } else {
            rs_frame[j][i] = b[ctr3 % dataLen];
            update_rs(parities[j], b[ctr3 % dataLen]);
          }
        }
        data8[ctr1++] = rs_frame[j][i];
        ctr3++;
      }
    }
  }
}


#ifdef DEBUG_LOGGING
    // printf("\nAt the end of data8 write, %d ctr1 values written\n\n", ctr1);
    /*
    printf("Parities ");
    for (int m = 0; m < parityLen; m++) {
        printf("%d ", parities[0][m]);
    }
    printf("\n");
    */
#endif

int ctr2 = 0; // Counter for data10
memset(data10, 0, sizeof(data10)); // Initialize data10 array with zeros

for (i = 0; i < dataLen * payloads + headerLen; i++) // Loop through the data
{
    // Encode data8 to data10 using a predefined table (Encode_8b10b)
    data10[ctr2] = (Encode_8b10b[rd][((int) data8[ctr2])] & 0x3ff);

    // Extract nrd (non-return-to-zero data) from the encoded value
    nrd = (Encode_8b10b[rd][((int) data8[ctr2])] >> 10) & 1;

    // Update the value of rd (return-to-zero data)
    rd = nrd;

    // Increment the counter for data10
    ctr2++;
}


for (i = 0; i < parityLen; i++) {
    for (int j = 0; j < rsFrames; j++) {
        if ((uptime != 0) || (i != 0)) // Check if uptime is not zero, or it's not the first iteration
            data10[ctr2++] = (Encode_8b10b[rd][((int) parities[j][i])] & 0x3ff);

        nrd = (Encode_8b10b[rd][((int) parities[j][i])] >> 10) & 1;
        rd = nrd; // Update the return-to-zero data bit
    }
}

#ifdef DEBUG_LOGGING
// printf("\nAt end of data10 write, %d ctr2 values written\n\n", ctr2);
#endif

int data;
int val;

#ifdef DEBUG_LOGGING
// printf("\nAt the start of the buffer loop, syncBits %d samples %d ctr %d\n", syncBits, samples, ctr);
#endif

for (i = 1; i <= syncBits * samples; i++) {
    write_wave(ctr, buffer);

    if ((i % samples) == 0) {
        int bit = syncBits - i / samples + 1;
        val = sync;
        data = val & 1 << (bit - 1);

        if (mode == FSK) {
            // Phase modulation for FSK based on the sync bit
            phase = ((data != 0) * 2) - 1;
        } else {
            if (data == 0) {
                // Phase modulation for BPSK based on the sync bit
                phase *= -1;
                if ((ctr - smaller) > 0) {
                    // Apply a modification to the buffer for synchronization
                    for (int j = 1; j <= smaller; j++)
                        buffer[ctr - j] = buffer[ctr - j] * 0.4;
                }
                flip_ctr = ctr;
            }
        }
    }
}


#ifdef DEBUG_LOGGING
// Print debug information related to the value of ctr and buffer length
// printf("\n\nValue of ctr after header: %d Buffer Len: %d\n\n", ctr, buffSize);
#endif

for (i = 1; i <= (10 * (headerLen + dataLen * payloads + rsFrames * parityLen) * samples); i++) // 572
{
    // Call the write_wave function to write data to the buffer
    write_wave(ctr, buffer);

    if ((i % samples) == 0) {
        // Calculate the symbol and bit positions for data10
        int symbol = (int)((i - 1) / (samples * 10));
        int bit = 10 - (i - symbol * samples * 10) / samples + 1;

        // Retrieve the value of data10 for the current symbol
        val = data10[symbol];

        // Extract the bit from the value
        data = val & (1 << (bit - 1));

        // Print debug information about the data and its position
        // printf ("%d i: %d new frame %d data10[%d] = %x bit %d = %d \n",
        //     ctr/SAMPLES, i, frames, symbol, val, bit, (data > 0) );

        if (mode == FSK) {
            // If in FSK mode, calculate the phase to send based on the data bit
            phase = ((data != 0) * 2) - 1;
            // printf("Sending a %d\n", phase);
        } else {
            if (data == 0) {
                // If in BPSK mode and data is zero, invert the phase
                phase *= -1;

                // Check if there is enough buffer space for a modification
                if ((ctr - smaller) > 0) {
                    // Apply a modification to the buffer for synchronization
                    for (int j = 1; j <= smaller; j++)
                        buffer[ctr - j] = buffer[ctr - j] * 0.4;
                }
                flip_ctr = ctr; // Update the flip_ctr for synchronization
            }
        }
    }
}

#ifdef DEBUG_LOGGING
// Print debug information related to the value of ctr, buffer length, and calculations
// printf("\nValue of ctr after looping: %d Buffer Len: %d\n", ctr, buffSize);
// printf("\ctr/samples = %d ctr/(samples*10) = %d\n\n", ctr/samples, ctr/(samples*10));
#endif

int error = 0;
// int count;
//  for (count = 0; count < dataLen; count++) {
//      printf("%02X", b[count]);
//  }
//  printf("\n");

// socket write

if (!socket_open && transmit) {
    // Print a message indicating the socket is being opened
    printf("Opening socket!\n");

    // Initialize socket-related variables and structures
    struct sockaddr_in serv_addr;

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        error = 1;
    }

    // Initialize the serv_addr structure
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        error = 1;
    }

    // Attempt to establish a connection with the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        printf("Error: %s \n", strerror(errno));
        error = 1;
        
        // Sleep for 2 seconds if the socket connection is refused
        sleep(2.0);

        // Attempt to open the socket again
        error = 0;
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Socket creation error \n");
            error = 1;
        }

// Initialize the serv_addr structure with zeros
memset(&serv_addr, '0', sizeof(serv_addr));

// Set the address family to AF_INET (IPv4)
serv_addr.sin_family = AF_INET;

// Set the port number by converting it to network byte order (big-endian)
serv_addr.sin_port = htons(PORT);

// Convert IPv4 and IPv6 addresses from text to binary form 
if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    // Print an error message if the conversion fails
    printf("\nInvalid address/Address not supported\n");
    error = 1; // Set the error flag to indicate an error
}

// Attempt to establish a connection with the server using the created socket
if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    // Print an error message if the connection fails
    printf("\nConnection Failed\n");
    printf("Error: %s\n", strerror(errno)); // Print the specific error message
    error = 1; // Set the error flag to indicate an error
    // sleep(1.0); // Sleep for 1 second if the socket connection is refused (commented out)
}

// Check if there was an error during socket creation and connection
if (error == 1)
    ; // No action is taken, but this appears to be a placeholder for error handling
else
    socket_open = 1; // Set the socket_open flag to 1 to indicate that the socket is open
}

// Check if there are no errors and the 'transmit' flag is true
if (!error && transmit) {
    //	digitalWrite (0, LOW);

    // Get the current time in milliseconds
    start = millis();

    // Send data over the socket
    int sock_ret = send(sock, buffer, (unsigned int)(ctr * 2 + 2), 0);

    // Print information about the socket send operation
    printf("socket send 1 %d ms bytes: %d \n\n", (unsigned int)millis() - start, sock_ret);
    fflush(stdout);

    // Check if not all data was sent in the first send operation
    if (sock_ret < (ctr * 2 + 2)) {
        // Sleep for 0.5 seconds
        sleep(0.5);

        // Send the remaining data over the socket
        sock_ret = send(sock, &buffer[sock_ret], (unsigned int)(ctr * 2 + 2 - sock_ret), 0);

        // Print information about the second socket send operation
        printf("socket send 2 %d ms bytes: %d \n\n", millis() - start, sock_ret);
    }

loop_count++;

// The following conditions check when to send data frames based on the value of loop_count and mode.
if ((firstTime == 1) || (((loop_count % 180) == 0) && (mode == FSK)) || (((loop_count % 80) == 0) && (mode == BPSK))) {
    int max;

    // Determine the value of max based on the current mode and conditions.
    if (mode == FSK) {
        if (sim_mode)
            max = 6;
        else if (firstTime == 1)
            max = 4;
        else
            max = 3;
    } else {
        if (firstTime == 1)
            max = 5;
        else
            max = 4;
    }

    // Loop 'max' times to send frames until the buffer fills.
    for (int times = 0; times < max; times++) {
        start = millis();  // Record the start time for measuring the duration of the send operation.

        // Send data frames over the socket.
        sock_ret = send(sock, buffer, (unsigned int)(ctr * 2 + 2), 0);
        printf("socket send %d in %d ms bytes: %d \n\n", times + 2, (unsigned int)millis() - start, sock_ret);

        // Check if the buffer overfilled, which can be an indication of an issue.
        if ((millis() - start) > 500) {
            printf("Buffer overfilled!\n");
            break;
        }

        // If not all data was sent in the first send operation, resend the remaining data.
        if (sock_ret < (ctr * 2 + 2)) {
            sleep(0.5); // Sleep briefly before resending.
            sock_ret = send(sock, &buffer[sock_ret], (unsigned int)(ctr * 2 + 2 - sock_ret), 0);
            printf("socket resend %d in %d ms bytes: %d \n\n", times, millis() - start, sock_ret);
        }
    }

    sampleTime = (unsigned int)millis(); // Reset the sampleTime for sleeping.
    fflush(stdout);
}

// Handle the case where a socket error occurred.
if (sock_ret == -1) {
    printf("Error: %s \n", strerror(errno));
    socket_open = 0;
}

// Display a message if not in transmit mode.
if (!transmit) {
    fprintf(stderr, "\nNo CubeSatSim Band Pass Filter detected. No transmissions after the CW ID.\n");
    fprintf(stderr, "See http://cubesatsim.org/wiki for info about building a CubeSatSim\n\n");
}

// Update the firstTime flag based on the socket_open status.
if (socket_open == 1)
    firstTime = 0;

// Return from the function.

return;
}

FILE *sopen(const char *program)
{
    int fds[2];
    pid_t pid;

    // Create a socket pair for communication between parent and child processes.
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
        return NULL;

    // Create a child process using fork or vfork.
    switch(pid = vfork()) {
    case -1:    // Error
        close(fds[0]);
        close(fds[1]);
        return NULL;
    case 0:     // Child process
        close(fds[0]);

        // Redirect standard input (0) and standard output (1) of the child process
        // to the socket's file descriptors.
        dup2(fds[1], 0);   // Duplicate socket write descriptor to stdin (0).
        dup2(fds[1], 1);   // Duplicate socket write descriptor to stdout (1).
        close(fds[1]);

        // Replace the child process with a new shell process that runs the specified program.
        // The program argument is executed using /bin/sh, and it runs as a shell command.
        execl("/bin/sh", "sh", "-c", program, NULL);

        // If execl fails, exit the child process with an error status.
        _exit(127);
    }

    // Parent process: close the write end of the socket and return a FILE pointer
    // associated with the read end of the socket for bidirectional communication.
    close(fds[1]);
    return fdopen(fds[0], "r+");
}


 /*
 * TelemEncoding.h
 *
 *  Created on: Feb 3, 2014
 *      Author: fox
 */

/* 
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define false 0
#define true 1
*/

//----------------------------fonction for creat the wave in telem---------------------------------------

void write_wave(int i, short int *buffer)
{
    if (mode == FSK)
    {
        // Check if the difference between ctr and flip_ctr is less than smaller
        if ((ctr - flip_ctr) < smaller)
            // Calculate and store a value in the buffer based on phase and other parameters
            buffer[ctr++] = (short int)(0.1 * phase * (ctr - flip_ctr) / smaller);
        else
            // Calculate and store a different value in the buffer based on amplitude and phase
            buffer[ctr++] = (short int)(0.25 * amplitude * phase);
    }
    else
    {
        // Check if the difference between ctr and flip_ctr is less than smaller
        if ((ctr - flip_ctr) < smaller)
            // Calculate and store a value in the buffer based on amplitude, phase, and sine function
            buffer[ctr++] = (short int)(amplitude * 0.4 * phase * sin((float)(2*M_PI*i*freq_Hz/S_RATE)));
        else
            // Calculate and store a different value in the buffer based on amplitude, phase, and sine_map
            buffer[ctr++] = (short int)(amplitude * phase * sin((float)(2*M_PI*i*freq_Hz/S_RATE)));
    }
    // Continue with some additional calculations and buffer operations
}

int encodeA(short int *b, int index, int val) {
    // Encode an integer 'val' into two bytes in the 'b' buffer starting at 'index'
    b[index] = val & 0xff;
    b[index + 1] = (short int)((b[index + 1] & 0xf0) | ((val >> 8) & 0x0f));
    return 0;
}


int encodeB(short int *b, int index, int val) {
    // This function appears to encode an integer value 'val' into two bytes in the 'b' buffer starting at the specified 'index'.
    // The encoding process involves bit manipulation.
    
    // Set the first byte at 'index' in the buffer to the lower 4 bits of 'val' combined with the lower 4 bits of the existing value at 'index'.
    b[index] = (short int)((b[index] & 0x0f) | ((val << 4) & 0xf0));
    
    // Set the second byte at 'index + 1' in the buffer to the upper 4 bits of 'val' after shifting right by 4 bits.
    b[index + 1] = (val >> 4) & 0xff;
    
    // Return 0 to indicate successful encoding.
    return 0;
}

int twosToInt(int val, int len) {
    // This function converts a two's complement value 'val' to an integer.
    // The 'len' parameter specifies the number of bits in the two's complement representation.

    if (val & (1 << (len - 1)))
        val = val - (1 << len);

    return val;
}

float rnd_float(double min, double max) {
    // This function returns a random floating-point number with two decimal places between 'min' and 'max'.

    // Generate a random integer value between 'min' and 'max' with two decimal places.
    int val = (rand() % ((int)(max * 100) - (int)(min * 100) + 1)) + (int)(min * 100);

    // Convert the integer value to a floating-point number with two decimal places.
    float ret = ((float)(val) / 100);

    return(ret);
}

int test_i2c_bus(int bus) {
    // This function tests the availability and functionality of an I2C bus specified by 'bus' parameter.

    // Initialize the 'output' variable to the 'bus' parameter value. It will be modified based on the test result.
    int output = bus; 

    // Construct the I2C device path based on the 'bus' parameter.
    char busDev[20] = "/dev/i2c-";
    char busS[5];
    snprintf(busS, 5, "%d", bus);
    strcat(busDev, busS);

    // Print a message indicating the I2C bus being tested.
    printf("I2C Bus Tested: %s \n", busDev);

    // Check if the I2C bus is accessible (readable and writable).
    if (access(busDev, W_OK | R_OK) >= 0) {
        // If the bus is accessible, run the 'i2cdetect' command to scan for connected I2C devices.
        char result[128];
        const char command_start[] = "timeout 2 i2cdetect -y ";
        char command[50];
        strcpy(command, command_start);
        strcat(command, busS);

        FILE *i2cdetect = popen(command, "r");

        while (fgets(result, 128, i2cdetect) != NULL) {
            // Loop through the command output (not processing it here).
        }

        // Get the exit code of the 'i2cdetect' command.
        int error = pclose(i2cdetect) / 256;

        // Check if there was an error during the 'i2cdetect' command execution.
        if (error != 0) {
            printf("ERROR: %s bus has a problem \n  Check I2C wiring and pullup resistors \n", busDev);
            if (bus == 3)
                printf("-> If this is a CubeSatSim Lite, then this error is normal!\n");
            output = -1;
        }
    } else {
        // If the bus is not accessible, print an error message.
        printf("ERROR: %s bus has a problem \n  Check software to see if I2C enabled \n", busDev);
        output = -1;
    }

    // Return 'output,' which will be the original 'bus' value or -1 if there is a problem with the bus.
    return(output);
}

float toAprsFormat(float input) {
    // This function converts a decimal coordinate (latitude or longitude) to the APRS DDMM.MM format.
    
    // Extract the whole degrees (DD) from the input.
    int dd = (int) input;

    // Calculate the whole minutes (MM1) by subtracting the degrees and converting the fractional part to minutes.
    int mm1 = (int)((input - dd) * 60.0);

    // Calculate the remaining fractional seconds (MM2) by further subtracting degrees and whole minutes, 
    // and then converting the fractional part to seconds.
    int mm2 = (int)((input - dd - (float)mm1 / 60.0) * 60.0 * 60.0);

    // Combine the degrees, minutes, and fractional seconds into the APRS format.
    // DD * 100 represents degrees in two digits, mm1 represents whole minutes, and (float)mm2 * 0.01 represents fractional minutes in DDMM.MM format.
    float output = dd * 100 + mm1 + (float)mm2 * 0.01;

    // Return the result in APRS DDMM.MM format.
    return(output);
}
