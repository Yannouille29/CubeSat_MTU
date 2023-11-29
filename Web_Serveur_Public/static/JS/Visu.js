var targetWidth = 1500; // Largeur cible
var targetHeight = 743.617; // Hauteur cible

// Création d'une Timeline GSAP pour contrôler les animations
// var timeline = gsap.timeline();

function animateCubeRotation(pitch, yaw, roll) {
    // Si toutes les valeurs (pitch, yaw, roll) sont égales à zéro, orienter le cube avec une face vers le point de vue
    if (pitch === 0 && yaw === 0 && roll === 0) {
        // Changer l'orientation par défaut du cube selon la vue de la caméra
        var defaultOrientation = calculateDefaultOrientation();
        gsap.to(cube.rotation, {
            x: defaultOrientation.x,
            y: defaultOrientation.y,
            z: defaultOrientation.z,
            duration: 0.3,
            ease: "power2.inOut"
        });
    } else {
        gsap.to(cube.rotation, {
            x: pitch,
            y: yaw,
            z: roll,
            duration: 0.3,
            ease: "power2.inOut"
        });
    }
}

// Fonction pour calculer l'orientation par défaut lorsque pitch, yaw et roll sont tous égaux à zéro
function calculateDefaultOrientation() {
    // Déterminer l'orientation par défaut selon la vue de la caméra
    // Par exemple, faire face à l'avant (z positif)
    // Vous pouvez ajuster ces valeurs en fonction de l'orientation souhaitée
    var defaultX = 0;  // Ajustez ces valeurs pour orienter le cube selon l'axe X
    var defaultY = 0;  // Ajustez ces valeurs pour orienter le cube selon l'axe Y
    var defaultZ = 0;  // Ajustez ces valeurs pour orienter le cube selon l'axe Z

    return { x: defaultX, y: defaultY, z: defaultZ };
}

var scene = new THREE.Scene();
var camera = new THREE.PerspectiveCamera(75, targetWidth / targetHeight, 0.1, 1000);
var renderer = new THREE.WebGLRenderer();
var canMakeRequest = true;

// Ajout de quatre lumières spot réparties dans les coins de la scène
const spotLight1 = new THREE.SpotLight(0xffffff); // Lumière blanche
spotLight1.position.set(-20, 20, -20);
scene.add(spotLight1);

const spotLight2 = new THREE.SpotLight(0xffffff); // Lumière blanche
spotLight2.position.set(20, 20, -20);
scene.add(spotLight2);

const spotLight3 = new THREE.SpotLight(0xffffff); // Lumière blanche
spotLight3.position.set(-20, 20, 20);
scene.add(spotLight3);

const spotLight4 = new THREE.SpotLight(0xffffff); // Lumière blanche
spotLight4.position.set(20, 20, 20);
scene.add(spotLight4);

const ambientLight = new THREE.AmbientLight(0xffc0c0); // Couleur rouge plus claire pour la lumière ambiante
scene.add(ambientLight);

// Création d'un cube
var geometry = new THREE.BoxGeometry(3, 3, 3);

var textureLoader = new THREE.TextureLoader();

var materials = Object.values(imageUrls).map(url => {
    var texture = textureLoader.load(url);
    return new THREE.MeshStandardMaterial({ map: texture });
});

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
    var newWidth = window.innerWidth;

    if (newWidth > targetWidth) {
        newWidth = targetWidth;
    }

    var newHeight = (newWidth / targetWidth) * targetHeight;

    renderer.setSize(newWidth, newHeight);
    camera.aspect = newWidth / newHeight;
    camera.updateProjectionMatrix();
}

window.addEventListener('load', resizeCanvas);
window.addEventListener('resize', resizeCanvas);

resizeCanvas();

document.getElementById("visualization-container").appendChild(renderer.domElement);

function animate() {
    if (canMakeRequest) {
        canMakeRequest = false;

        axios.get("http://localhost:8080/get_data")
            .then(function (response) {
                if (response.data.length > 0) {
                    var latestData = response.data[0]; // Prendre le premier tableau de données

                    if (latestData.length >= 34) {
                        var roll = parseFloat(latestData[31]) - 2048; // 32e valeur (index 31)
                        var pitch = parseFloat(latestData[32]) - 2048; // 33e valeur (index 32)
                        var yaw = parseFloat(latestData[33]) - 2048; // 34e valeur (index 33)

                        roll = (Math.PI / 180) * roll;
                        pitch = (Math.PI / 180) * pitch;
                        yaw = (Math.PI / 180) * yaw;

                        animateCubeRotation(pitch, yaw, roll); // Utiliser la nouvelle fonction pour animer la rotation
                    }
                }

                renderer.render(scene, camera);
            })
            .catch(function (error) {
                console.error("Erreur lors de la récupération des données : " + error);
            })
            .finally(function () {
                canMakeRequest = true;
            });
    }
}


setInterval(animate, 10);

animate();
