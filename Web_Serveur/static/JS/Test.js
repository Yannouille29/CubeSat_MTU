function extractAltitude(response) {
    // Check if the response has data and contains enough columns and rows
    if (response.data.length > 0 && response.data[0].length > 35) {
        // Ensure there is at least one row and 36 columns in the first row (0-35)
        var altitude = response.data[2][28]; // Altitude is in the 29th column of the third row (index 28)
        // Call the function to update the altitude in the animation
        updateAltitude(altitude);
    } else {
        console.error("The response does not contain enough rows or columns to extract altitude.");
    }
}

function updateAltitude(alt) {
    alt = Math.max(0, Math.min(2500, alt)); // Limit altitude between 0 and 2500
    var montHeight = (alt / 2500) * 100 + '%'; // Calculate height percentage based on altitude
    $('#mont').css('height', montHeight); // Update the height of the element with ID 'mont' (assumed to represent altitude)
}

function extractPressure(response) {
    // Check if the response has data and contains enough columns and rows
    if (response.data.length > 0 && response.data[0].length > 35) {
        // Ensure there is at least one row and 36 columns in the first row (0-35)
        var pressure = response.data[2][27]; // Pressure is in the 28th column of the third row (index 27)
        // Call the function to update the arrow position in the animation
        updateArrowPosition(pressure);
    } else {
        console.error("The response does not contain enough rows or columns to extract pressure.");
    }
}

function updateArrowPosition(press) {
    // Limit the pressure value between 0 and 2000
    press = Math.max(0, Math.min(2000, press));
    // Convert the pressure value to an angle between -90 and 90 degrees
    var angle = (press / 2000) * 180 - 90;
    // Update the rotation of the arrow
    document.getElementById('arrow').style.transform = 'rotate(' + angle + 'deg)';
}

function extractHumidity(response) {
    // Check if the response has data and contains enough columns and rows
    if (response.data.length > 0 && response.data[0].length > 35) {
        // Ensure there is at least one row and 36 columns in the first row (0-35)
        var humidity = response.data[2][35]; // Humidity is in the 36th column of the third row (index 35)
        // Call the function to update the humidity in the animation
        updateHumidity(humidity);
    } else {
        console.error("The response does not contain enough rows or columns to extract humidity.");
    }
}

function updateHumidity(hum) {
    hum = Math.max(0, Math.min(800, hum)); // Limit humidity between 0 and 800
    var waterHeight = (hum / 800) * 100 + '%'; // Calculate height percentage based on humidity
    $('#water').css('height', waterHeight); // Update the height of the element with ID 'water' (assumed to represent humidity)
}

function extractTemperature(response) {
    // Check if the response has data and contains enough columns and rows
    if (response.data.length > 0 && response.data[0].length > 10) {
        // Ensure there is at least one row and 11 columns in the first row (0-10)
        var temperature = response.data[2][12] / 10; // Temperature is in the 13th column of the third row (index 12)
        // Call the function to update the temperature in the animation
        updateTemperature(temperature);
    } else {
        console.error("The response does not contain enough rows or columns to extract the temperature.");
    }
}

function updateTemperature(temp) {
    temp = Math.max(0, Math.min(50, temp)); // Limit temperature between 0 and 50
    var mercuryHeight = (temp / 50) * 100 + '%'; // Calculate height percentage based on temperature
    $('#mercury').css('height', mercuryHeight); // Update the height of the element with ID 'mercury' (presumably representing temperature)
}

function extractBatterie(response) {
    // Check if the response has data and contains enough columns and rows
    if (response.data.length > 0 && response.data[0].length > 10) {
        // Ensure there is at least one row and 11 columns in the first row (0-10)
        var batterie = response.data[0][7] / 100; // Battery level is in the 8th column of the first row (index 7)
        console.log("Battery extracted : " + batterie);
        // Call the function to update the battery level in the animation
        updateBatterie(batterie);
    } else {
        console.error("The response does not contain enough rows or columns to extract the battery level.");
    }
}
// Function to update the battery level visualization
function updateBatterie(bat) {
    bat = Math.max(0, Math.min(5, bat)); // Limit battery level between 0 and 5
    var batterieHeight = (bat / 5) * 100 + '%'; // Calculate height percentage based on battery level
    $('#batterieplein').css('height', batterieHeight); // Update the height of the element with ID 'batterieplein' (representing battery level)
}

// Function to fetch data from the backend (PC)
function getData() {
    // Make a request to fetch data from the backend (PC)
    axios.get("http://localhost:8080/get_data")
        .then(function (response) {
            // Create a table in JavaScript
            var table = document.getElementById('data-table');
            var tbody = table.getElementsByTagName('tbody')[0];
            tbody.innerHTML = ''; // Clear the current table content
            console.log(response);
            // Populate the table with received data
            response.data.forEach(function (rowArray) {
                var newRow = tbody.insertRow(tbody.rows.length);

                rowArray.forEach(function (cell) {
                    var newCell = newRow.insertCell(newRow.cells.length);
                    newCell.innerHTML = cell;
                });
            });
            document.getElementById('arrow').style.transform = 'rotate(0deg)';
            // Extract and update various data points from the response
            extractTemperature(response);
            extractBatterie(response);
            extractHumidity(response);
            extractPressure(response);
            extractAltitude(response);
        })
        .catch(function (error) {
            console.error("Error fetching data: " + error);
        });
}

// Refresh data every second
setInterval(getData, 1000);

// Call getData initially to display initial data
getData();

