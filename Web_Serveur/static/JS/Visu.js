//----------------------------------------------------------------------------------------------------------------
//-----------------------------Script for the visualization on the web interface----------------------------------
//----------------------------------Creates by Yann HUGUET in 29/11/2023------------------------------------------
//----------------------------------------------------------------------------------------------------------------

// Target width and height for the animation
var targetWidth = 1500; // Target width
var targetHeight = 743.617; // Target height

// Function to animate the cube rotation based on pitch, yaw, and roll values
function animateCubeRotation(pitch, yaw, roll) {
    // If all values (pitch, yaw, roll) are equal to zero, orient the cube with a face towards the viewpoint
    if (pitch === 0 && yaw === 0 && roll === 0) {
        // Change the default orientation of the cube based on the camera view
        var defaultOrientation = calculateDefaultOrientation(); // Function call to calculate the default orientation
        gsap.to(cube.rotation, {
            x: defaultOrientation.x,
            y: defaultOrientation.y,
            z: defaultOrientation.z,
            duration: 0.3, // Duration of the animation
            ease: "power2.inOut" // Easing function for animation
        });
    } else {
        // Rotate the cube based on provided pitch, yaw, and roll values
        gsap.to(cube.rotation, {
            x: pitch,
            y: yaw,
            z: roll,
            duration: 0.3, // Duration of the animation
            ease: "power2.inOut" // Easing function for animation
        });
    }
}

// Function to calculate the default orientation when pitch, yaw, and roll are all zero
function calculateDefaultOrientation() {
    // Determine the default orientation based on the camera view
    // For example, facing forward (positive z)
    // You can adjust these values to orient the cube along the X, Y, Z axes as desired
    var defaultX = 0;  // Adjust these values to orient the cube along the X-axis
    var defaultY = 0;  // Adjust these values to orient the cube along the Y-axis
    var defaultZ = 0;  // Adjust these values to orient the cube along the Z-axis

    return { x: defaultX, y: defaultY, z: defaultZ };
}

// Creating a Three.js scene, camera, and renderer
var scene = new THREE.Scene();  // Creating a Three.js scene
var camera = new THREE.PerspectiveCamera(75, targetWidth / targetHeight, 0.1, 1000);  // Creating a camera
var renderer = new THREE.WebGLRenderer();  // Creating a WebGLRenderer

var canMakeRequest = true;  // Variable to control if a request can be made

// Adding four spot lights positioned at different corners of the scene
const spotLight1 = new THREE.SpotLight(0xffffff); // White light
spotLight1.position.set(-20, 20, -20);
scene.add(spotLight1);

const spotLight2 = new THREE.SpotLight(0xffffff); // White light
spotLight2.position.set(20, 20, -20);
scene.add(spotLight2);

const spotLight3 = new THREE.SpotLight(0xffffff); // White light
spotLight3.position.set(-20, 20, 20);
scene.add(spotLight3);

const spotLight4 = new THREE.SpotLight(0xffffff); // White light
spotLight4.position.set(20, 20, 20);
scene.add(spotLight4);

const ambientLight = new THREE.AmbientLight(0xffc0c0); // Light with a lighter shade of red for ambient lighting
scene.add(ambientLight);

// Creating a cube
var geometry = new THREE.BoxGeometry(3, 3, 3); // Box geometry with dimensions 3x3x3

var textureLoader = new THREE.TextureLoader();

// Loading textures and creating materials for each face of the cube
var materials = Object.values(imageUrls).map(url => {
    var texture = textureLoader.load(url);
    return new THREE.MeshStandardMaterial({ map: texture });
});

// Creating a mesh (cube) by combining geometry and materials
var cube = new THREE.Mesh(geometry, materials);

// Camera position
camera.position.z = 5; // Set the camera position along the z-axis

// Adding the cube to the scene
scene.add(cube);

// Create an AxesHelper and attach it to the cube
const axesHelper = new THREE.AxesHelper(5); // AxesHelper with a size of 5 units
cube.add(axesHelper); // Attach the AxesHelper to the cube

// Position the AxesHelper to align with the cube (for example, at the center)
axesHelper.position.set(0, 0, 0); // Set the position of the AxesHelper relative to the cube

// Function to resize the canvas
function resizeCanvas() {
    var newWidth = window.innerWidth;

    // Limit the newWidth to the targetWidth
    if (newWidth > targetWidth) {
        newWidth = targetWidth;
    }

    var newHeight = (newWidth / targetWidth) * targetHeight; // Calculate new height based on aspect ratio

    // Set renderer size, camera aspect ratio, and update projection matrix
    renderer.setSize(newWidth, newHeight);
    camera.aspect = newWidth / newHeight;
    camera.updateProjectionMatrix();
}

// Event listeners for window load and resize events
window.addEventListener('load', resizeCanvas); // Call resizeCanvas when the window loads
window.addEventListener('resize', resizeCanvas); // Call resizeCanvas when the window is resized

resizeCanvas(); // Initial call to resizeCanvas function

// Append the renderer's DOM element to the visualization container
document.getElementById("visualization-container").appendChild(renderer.domElement);
function animate() {
    if (canMakeRequest) {
        canMakeRequest = false; // Prevent making another request until the current one is finished

        axios.get("http://localhost:8080/get_data") // Make a GET request to fetch data from a local server
            .then(function (response) {
                if (response.data.length > 0) {
                    var latestData = response.data[0]; // Extracting the first array of data

                    if (latestData.length >= 34) {
                        // Extract roll, pitch, and yaw values from the data array and adjust them
                        var roll = parseFloat(latestData[31]) - 2048; // 32nd value (index 31)
                        var pitch = parseFloat(latestData[32]) - 2048; // 33rd value (index 32)
                        var yaw = parseFloat(latestData[33]) - 2048; // 34th value (index 33)

                        // Convert roll, pitch, and yaw values to radians
                        roll = (Math.PI / 180) * roll;
                        pitch = (Math.PI / 180) * pitch;
                        yaw = (Math.PI / 180) * yaw;

                        animateCubeRotation(pitch, yaw, roll); // Use the updated function to animate rotation
                    }
                }

                renderer.render(scene, camera); // Render the scene with the updated rotation
            })
            .catch(function (error) {
                console.error("Error fetching data: " + error);
            })
            .finally(function () {
                canMakeRequest = true; // Allow making a new request after finishing the current one
            });
    }
}

// Call the 'animate' function every 10 milliseconds using setInterval
setInterval(animate, 10);

// Call 'animate' initially to start the animation loop
animate();
