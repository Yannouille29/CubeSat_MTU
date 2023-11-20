var targetWidth = 1500; // Largeur cible
var targetHeight = 743.617; // Hauteur cible

var scene = new THREE.Scene();
var camera = new THREE.PerspectiveCamera(75, targetWidth / targetHeight, 0.1, 1000);
var renderer = new THREE.WebGLRenderer();
var canMakeRequest = true;

const light = new THREE.SpotLight();
light.position.set(20, 20, 20);
scene.add(light);

// Création d'un cube
var geometry = new THREE.BoxGeometry(3, 3, 3);

// Création de six matériaux avec des couleurs différentes
var materials = [
    new THREE.MeshBasicMaterial({ color: 0xff0000, transparent: true, opacity: 0.75 }), // Face avant (rouge)
    new THREE.MeshBasicMaterial({ color: 0x00ff00, transparent: true, opacity: 0.75 }), // Face arrière (vert)
    new THREE.MeshBasicMaterial({ color: 0x0000ff, transparent: true, opacity: 0.75 }), // Face droite (bleu)
    new THREE.MeshBasicMaterial({ color: 0xffff00, transparent: true, opacity: 0.75 }), // Face gauche (jaune)
    new THREE.MeshBasicMaterial({ color: 0xff00ff, transparent: true, opacity: 0.75 }), // Face supérieure (magenta)
    new THREE.MeshBasicMaterial({ color: 0x00ffff, transparent: true, opacity: 0.75 })  // Face inférieure (cyan)
];

// Création d'un matériau de cube combiné à partir des matériaux individuels
var cubeMaterial = new THREE.MeshFaceMaterial(materials);

var cube = new THREE.Mesh(geometry, materials);

// Position de la caméra
camera.position.z = 5;

// Ajout du cube à la scène
scene.add(cube);

// Créez un AxesHelper et attachez-le au cube
const axesHelper = new THREE.AxesHelper(5);
cube.add(axesHelper);

// Positionnez l'AxesHelper pour qu'il soit aligné avec le cube (au centre par exemple)
axesHelper.position.set(0, 0, 0);

function resizeCanvas() {
    // Calculez la nouvelle largeur en fonction de la fenêtre
    var newWidth = window.innerWidth;

    // Vérifiez si la nouvelle largeur dépasse la largeur cible
    if (newWidth > targetWidth) {
        newWidth = targetWidth;
    }

    // Calculez la nouvelle hauteur en fonction de la nouvelle largeur
    var newHeight = (newWidth / targetWidth) * targetHeight;

    // Ajustez la taille du canvas
    renderer.setSize(newWidth, newHeight);
    camera.aspect = newWidth / newHeight;
    camera.updateProjectionMatrix();
}

// Appelez la fonction de redimensionnement au chargement de la page
window.addEventListener('load', resizeCanvas);

// Appelez la fonction de redimensionnement à chaque événement de redimensionnement de la fenêtre
window.addEventListener('resize', resizeCanvas);

// Configuration du rendu initial
resizeCanvas();

document.getElementById("visualization-container").appendChild(renderer.domElement);


function animate() {
    if (canMakeRequest) {
        canMakeRequest = false;  // Bloquez la possibilité de faire une nouvelle requête

        axios.get("http://localhost:8080/get_data")
            .then(function (response) {
                if (response.data.length >= 3) {
                    var latestData = response.data.slice(-1)[0].slice(-3); // Récupérez les trois dernières valeurs
                    var roll = parseFloat(latestData[0]);
                    var pitch = parseFloat(latestData[1]);
                    var yaw = parseFloat(latestData[2]);

                    roll = THREE.Math.degToRad(roll);
                    pitch = THREE.Math.degToRad(pitch);
                    yaw = THREE.Math.degToRad(yaw);

                    cube.rotation.x = pitch;
                    cube.rotation.y = yaw;
                    cube.rotation.z = roll;
                }

                renderer.render(scene, camera);
            })
            .catch(function (error) {
                console.error("Erreur lors de la récupération des données : " + error);
            })
            .finally(function () {
                canMakeRequest = true;  // Débloquez la possibilité de faire une nouvelle requête
            });
    }
}

// Actualisez les données toutes les 3 secondes
setInterval(animate, 3000);

animate(); // Démarrez la boucle d'animation
