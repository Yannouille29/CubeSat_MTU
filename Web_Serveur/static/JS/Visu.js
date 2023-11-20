var targetWidth = 1500; // Target width
var targetHeight = 743.617; // Target height

// Creating a GSAP Timeline to control animations
// var timeline = gsap.timeline();

function animateCubeRotation(pitch, yaw, roll) {
    // If all values (pitch, yaw, roll) are equal to zero, orient the cube with one face towards the viewpoint
    if (pitch === 0 && yaw === 0 && roll === 0) {
        // Change the default orientation of the cube based on the camera view
        var defaultOrientation = calculateDefaultOrientation();
        gsap.to(cube.rotation, {
            x: defaultOrientation.x,
            y: defaultOrientation.y,
            z: defaultOrientation.z,
            duration: 0.3,
            ease: "power2.inOut"
        });
    } else {
        // Rotate the cube to the provided pitch, yaw, and roll angles
        gsap.to(cube.rotation, {
            x: pitch,
            y: yaw,
            z: roll,
            duration: 0.3,
            ease: "power2.inOut"
        });
    }
}
// Function to calculate the default orientation when pitch, yaw, and roll are all zero
function calculateDefaultOrientation() {
    // Determine the default orientation based on the camera view
    // For example, facing forward (positive z)
    // You can adjust these values according to the desired orientation
    var defaultX = 0;  // Adjust these values to orient the cube along the X axis
    var defaultY = 0;  // Adjust these values to orient the cube along the Y axis
    var defaultZ = 0;  // Adjust these values to orient the cube along the Z axis

    return { x: defaultX, y: defaultY, z: defaultZ };
}

var scene = new THREE.Scene();
var camera = new THREE.PerspectiveCamera(75, targetWidth / targetHeight, 0.1, 1000);
var renderer = new THREE.WebGLRenderer();
var canMakeRequest = true;

// Adding four spotlights distributed in the corners of the scene
const spotLight1 = new THREE.SpotLight(0xffffff); // White light
spotLight1.position.set(-20, 20, -20);
scene.add(spotLight1);

const spotLight2 = new THREE.SpotLight(0xffffff); // White light
spotLight2.position.set(20, 20, -20);
scene.add(spotLight2);

const spotLight3 = new THREE.SpotLight(0xffffff); // White light
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

// Camera position
camera.position.z = 5;

// Adding the cube to the scene
scene.add(cube);

// Create an AxesHelper and attach it to the cube
const axesHelper = new THREE.AxesHelper(5);
cube.add(axesHelper);

// Position the AxesHelper to be aligned with the cube (at the center, for example)
axesHelper.position.set(0, 0, 0);

// Function to resize the canvas based on the window dimensions
function resizeCanvas() {
    var newWidth = window.innerWidth;

    // Ensure the canvas width doesn't exceed the target width
    if (newWidth > targetWidth) {
        newWidth = targetWidth;
    }

    var newHeight = (newWidth / targetWidth) * targetHeight;

    // Set renderer size and update camera aspect ratio
    renderer.setSize(newWidth, newHeight);
    camera.aspect = newWidth / newHeight;
    camera.updateProjectionMatrix();
}

// Add event listeners to handle window load and resize events
window.addEventListener('load', resizeCanvas);
window.addEventListener('resize', resizeCanvas);

// Resize the canvas on page load
resizeCanvas();

// Append the renderer's DOM element to the visualization container in the HTML
document.getElementById("visualization-container").appendChild(renderer.domElement);

// Function to perform animation and retrieve data
function animate() {
    if (canMakeRequest) {
        canMakeRequest = false;

        // Make an HTTP GET request using axios to retrieve data from the server
        axios.get("http://localhost:8080/get_data")
            .then(function (response) {
                if (response.data.length > 0) {
                    var latestData = response.data[0]; // Retrieve the first array of data

                    // Check if the data array has enough values
                    if (latestData.length >= 34) {
                        // Extract roll, pitch, and yaw values from the received data
                        var roll = parseFloat(latestData[31]) - 2048; // 32nd value (index 31)
                        var pitch = parseFloat(latestData[32]) - 2048; // 33rd value (index 32)
                        var yaw = parseFloat(latestData[33]) - 2048; // 34th value (index 33)

                        // Convert roll, pitch, and yaw to radians
                        roll = (Math.PI / 180) * roll;
                        pitch = (Math.PI / 180) * pitch;
                        yaw = (Math.PI / 180) * yaw;

                        // Call the animateCubeRotation function to animate the rotation based on the received data
                        animateCubeRotation(pitch, yaw, roll);
                    }
                }

                // Render the scene with updated changes
                renderer.render(scene, camera);
            })
            .catch(function (error) {
                console.error("Error fetching data: " + error);
            })
            .finally(function () {
                canMakeRequest = true;
            });
    }
}

// Call the animate function every 10 milliseconds using setInterval
setInterval(animate, 10);

// Initial call to the animate function
animate();
