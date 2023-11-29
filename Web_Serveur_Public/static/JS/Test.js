function extractAltitude(response) {
    // Check if the response has data
    if (response.data.length > 0 && response.data[0].length > 35) {
        // Ensure there is at least one row and 11 columns in the first row (0-10)
        var altitude = response.data[2][28]; // Altitude is in the 11th column of the first row
        // console.log("Extracted Altitude: " + altitude);
        // Call function to update the altitude in the animation
        updateAltitude(altitude);
    } else {
        console.error("The response does not contain enough rows or columns to extract the altitude.");
    }
}

function updateAltitude(alt) {
    alt = Math.max(0, Math.min(2500, alt));
    var montHeight = (alt / 2500) * 100 + '%';
    $('#mont').css('height', montHeight);
}

function extractPressure(response) {
    // Check if the response has data
    if (response.data.length > 0 && response.data[0].length > 35) {
        // Ensure there is at least one row and 11 columns in the first row (0-10)
        var pressure = response.data[2][27]; // Pressure is in the 10th column of the first row
        // console.log("Extracted Pressure: " + pressure);
        // Call function to update the arrow position in the animation
        updateArrowPosition(pressure);
    } else {
        console.error("The response does not contain enough rows or columns to extract the pressure.");
    }
}

function updateArrowPosition(press) {
    // Limit the pressure between 0 and 2000
    press = Math.max(0, Math.min(2000, press));
    // Convert the pressure percentage to an angle between -90 and 90 degrees
    var angle = (press / 2000) * 180 - 90;
    // Update the rotation of the arrow
    document.getElementById('arrow').style.transform = 'rotate(' + angle + 'deg)';
}

function extractHumidity(response) {
    // Check if the response has data
    if (response.data.length > 0 && response.data[0].length > 35) {
        // Ensure there is at least one row and 36 columns in the first row (0-35)
        var humidity = response.data[2][35]; // Humidity is in the 36th column of the first row
        // console.log("Extracted Humidity: " + humidity);
        // Call function to update the humidity in the animation
        updateHumidity(humidity);
    } else {
        console.error("The response does not contain enough rows or columns to extract the humidity.");
    }
}

function updateHumidity(hum) {
    hum = Math.max(0, Math.min(800, hum));
    var waterHeight = (hum / 800) * 100 + '%';
    $('#water').css('height', waterHeight);
}

function extractTemperature(response) {
    // Check if the response has data
    if (response.data.length > 0 && response.data[0].length > 10) {
        // Ensure there is at least one row and 13 columns in the first row (0-12)
        var temperature = response.data[2][12] / 10; // Temperature is in the 13th column of the first row
        // console.log("Extracted Temperature: " + temperature);
        // Call function to update the temperature in the animation
        updateTemperature(temperature);
    } else {
        console.error("The response does not contain enough rows or columns to extract the temperature.");
    }
}

function updateTemperature(temp) {
    temp = Math.max(0, Math.min(50, temp));
    var mercuryHeight = (temp / 50) * 100 + '%';
    $('#mercury').css('height', mercuryHeight);
}

function extractBattery(response) {
    // Check if the response has data
    if (response.data.length > 0 && response.data[0].length > 10) {
        // Ensure there is at least one row and 11 columns in the first row (0-10)
        var battery = response.data[0][7] / 100; // Battery level is in the 8th column of the first row
        console.log("Extracted battery level: " + battery);
        // Call function to update the battery level in the animation
        updateBattery(battery);
    } else {
        console.error("The response does not contain enough rows or columns to extract the temperature.");
    }
}

function updateBattery(bat) {
    bat = Math.max(0, Math.min(5, bat));
    var batteryHeight = (bat / 5) * 100 + '%';
    $('#batterieplein').css('height', batteryHeight);
}

function getData() {
    // Make a request to fetch data from the backend (PC)
    axios.get("http://localhost:8080/get_data")
        .then(function (response) {
            // Create the JavaScript table
            var table = document.getElementById('data-table');
            var tbody = table.getElementsByTagName('tbody')[0];
            tbody.innerHTML = '';  // Clear the current table content
            console.log(response);
            response.data.forEach(function (rowArray) {
                var newRow = tbody.insertRow(tbody.rows.length);

                rowArray.forEach(function (cell) {
                    var newCell = newRow.insertCell(newRow.cells.length);
                    newCell.innerHTML = cell;
                });
            });
            // Reset arrow rotation
            document.getElementById('arrow').style.transform = 'rotate(0deg)';
            
            // Extract and update temperature, battery, humidity, pressure, and altitude
            extractTemperature(response);
            extractBattery(response);
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

