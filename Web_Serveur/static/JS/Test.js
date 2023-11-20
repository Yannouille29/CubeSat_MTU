function extractAltitude(response) {
    // Vérifiez si la réponse a des données
    if (response.data.length > 0 && response.data[0].length > 35) {
        // Assurez-vous qu'il y a au moins une ligne et 11 colonnes dans la première ligne (0-10)
        var altitude = response.data[2][28]; // La température est dans la 11e colonne de la première ligne
        // console.log("Température extraite : " + temperature);
        // Appelez la fonction pour mettre à jour la température dans l'animation
        updateAltitude(altitude);
    } else {
        console.error("La réponse ne contient pas suffisamment de lignes ou de colonnes pour extraire la température.");
    }
}

function updateAltitude(alt) {
    alt = Math.max(0, Math.min(2500, alt));
    var montHeight = (alt / 2500) * 100 + '%';
    $('#mont').css('height', montHeight);
}


function extractPressure(response) {
    // Vérifiez si la réponse a des données
    if (response.data.length > 0 && response.data[0].length > 35) {
        // Assurez-vous qu'il y a au moins une ligne et 11 colonnes dans la première ligne (0-10)
        var pressure = response.data[2][27]; // La température est dans la 11e colonne de la première ligne
        // console.log("Température extraite : " + temperature);
        // Appelez la fonction pour mettre à jour la température dans l'animation
        updateArrowPosition(pressure);
    } else {
        console.error("La réponse ne contient pas suffisamment de lignes ou de colonnes pour extraire la température.");
    }
}

function updateArrowPosition(press) {
    // Limitez le pourcentage entre 0 et 100
    press = Math.max(0, Math.min(2000, press));
    // Convertissez le pourcentage en un angle entre -90 et 90 degrés
    var angle = (press / 2000) * 180 - 90;
    // Mettez à jour la rotation de la flèche
    document.getElementById('arrow').style.transform = 'rotate(' + angle + 'deg)';
}

function extractHumidity(response) {
    // Vérifiez si la réponse a des données
    if (response.data.length > 0 && response.data[0].length > 35) {
        // Assurez-vous qu'il y a au moins une ligne et 11 colonnes dans la première ligne (0-10)
        var humidity = response.data[2][35]; // La température est dans la 11e colonne de la première ligne
        // console.log("Température extraite : " + temperature);
        // Appelez la fonction pour mettre à jour la température dans l'animation
        updateHumidity(humidity);
    } else {
        console.error("La réponse ne contient pas suffisamment de lignes ou de colonnes pour extraire la température.");
    }
}

function updateHumidity(hum) {
    hum = Math.max(0, Math.min(800, hum));
    var waterHeight = (hum / 800) * 100 + '%';
    $('#water').css('height', waterHeight);
}

function extractTemperature(response) {
    // Vérifiez si la réponse a des données
    if (response.data.length > 0 && response.data[0].length > 10) {
        // Assurez-vous qu'il y a au moins une ligne et 11 colonnes dans la première ligne (0-10)
        var temperature = response.data[2][12] / 10; // La température est dans la 11e colonne de la première ligne
        // console.log("Température extraite : " + temperature);
        // Appelez la fonction pour mettre à jour la température dans l'animation
        updateTemperature(temperature);
    } else {
        console.error("La réponse ne contient pas suffisamment de lignes ou de colonnes pour extraire la température.");
    }
}

function updateTemperature(temp) {
    temp = Math.max(0, Math.min(50, temp));
    var mercuryHeight = (temp / 50) * 100 + '%';
    $('#mercury').css('height', mercuryHeight);
}

function extractBatterie(response) {
    // Vérifiez si la réponse a des données
    if (response.data.length > 0 && response.data[0].length > 10) {
        // Assurez-vous qu'il y a au moins une ligne et 11 colonnes dans la première ligne (0-10)
        var batterie = response.data[0][7] / 100; // La température est dans la 7e colonne de la première ligne
        console.log("bat extraite : " + batterie);
        // Appelez la fonction pour mettre à jour la température dans l'animation
        updateBatterie(batterie);
    } else {
        console.error("La réponse ne contient pas suffisamment de lignes ou de colonnes pour extraire la température.");
    }
}

function updateBatterie(bat) {
    bat = Math.max(0, Math.min(5, bat));
    var batterieHeight = (bat / 5) * 100 + '%';
    $('#batterieplein').css('height', batterieHeight);
}

function getData() {
    // Faites une requête pour récupérer les données du back-end (le PC)
    axios.get("http://localhost:8080/get_data")
        .then(function (response) {
            // Créez le tableau en JavaScript
            var table = document.getElementById('data-table');
            var tbody = table.getElementsByTagName('tbody')[0];
            tbody.innerHTML = '';  // Effacez le contenu actuel du tableau
            console.log(response);
            response.data.forEach(function (rowArray) {
                var newRow = tbody.insertRow(tbody.rows.length);

                rowArray.forEach(function (cell) {
                    var newCell = newRow.insertCell(newRow.cells.length);
                    newCell.innerHTML = cell;
                });
            });
            document.getElementById('arrow').style.transform = 'rotate(0deg)';
            extractTemperature(response);
            extractBatterie(response);
            extractHumidity(response);
            extractPressure(response);
            extractAltitude(response);
        })
        .catch(function (error) {
            console.error("Erreur lors de la récupération des données : " + error);
        });
}

// Actualisez les données toutes les secondes
setInterval(getData, 1000);

// Appelez getData une première fois pour afficher les données initiales
getData();
