const IlliniBlue = new Float32Array([0.075, 0.16, 0.292, 1])
const IlliniOrange = new Float32Array([1, 0.373, 0.02, 1])
const IdentityMatrix = new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1])
const RESET_INTERVAL_SECONDS = 15



// Global state for the simulation
let simulation;
let lastResetSecond = 0;
let previousSecond = 0;
let numberSpheres = 2;


// Initialize the simulation
function initSimulation() {
    // Create new simulation instance
    simulation = new SphereSimulation(numberSpheres);
    simulation.initialize(gl);
}

// Update sphere positions in WebGL
function updatePoints() {
    gl.bindBuffer(gl.ARRAY_BUFFER, simulation.positionBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(simulation.sphereState.positions.flat()), gl.DYNAMIC_DRAW);
}

// Simulate physics for the whole system
function simulatePhysic(deltaSeconds) {
    simulation.updatePhysics(deltaSeconds);
}

// Add this function after initSimulation() and before the tick() function
function resetSimulation() {
    simulation.resetSpherePositions();
    // Update GPU buffers with new positions
    gl.bindBuffer(gl.ARRAY_BUFFER, simulation.getPositionBuffer());
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(simulation.sphereState.positions.flat()), gl.DYNAMIC_DRAW);
}



class SphereSimulation {
    // Simulation constants
    static CUBE_WIDTH = 2;
    static CUBE_WALL = SphereSimulation.CUBE_WIDTH / 2;
    static SPHERE_TO_CUBE_RATIO = 0.15;
    static RESET_INTERVAL_SECONDS = 15;
    static ELASTICITY = 0.9;
    static GRAVITY = 3;
    static MAX_VELOCITY = 3;

    constructor(initialSphereCount = 2) {
        // Simulation state
        this.numberOfSpheres = initialSphereCount;
        this.lastResetSecond = 0;
        this.previousSecond = 0;

        // Sphere properties
        this.sphereState = {
            positions: [],    // Reset every RESET_INTERVAL_SECONDS
            velocities: [],   // Reset every RESET_INTERVAL_SECONDS
            colors: [],
            radii: [],
            masses: []
        };

        // Grid state for collision detection
        this.grid = {
            size: 0,
            cellLength: 0,
            map: null
        };

        // WebGL buffer for positions
        this.positionBuffer = null;
    }

    initialize(gl) {
        this.initializeSpheres();
        this.setupBuffers(gl);
        this.initializeGrid();
    }

    initializeSpheres() {
        // Clear existing arrays
        this.sphereState = {
            positions: [],
            velocities: [],
            colors: [],
            radii: [],
            masses: []
        };

        // Initialize sphere properties
        for (let i = 0; i < this.numberOfSpheres; i++) {
            const radius = this.calculateSphereRadius();
            this.sphereState.radii.push(radius);
            this.sphereState.masses.push(Math.pow(radius, 3));
            this.sphereState.colors.push([Math.random(), Math.random(), Math.random(), 1]);
            this.sphereState.positions.push([0, 0, 0]);
            this.sphereState.velocities.push([0, 0, 0]);
        }

        this.resetSpherePositions();
    }

    calculateSphereRadius() {
        return (Math.random() + 0.25) * (0.75 / Math.pow(this.numberOfSpheres, 1/3));
    }

    resetSpherePositions() {
        for (let i = 0; i < this.numberOfSpheres; i++) {
            this.sphereState.positions[i] = [
                SphereSimulation.CUBE_WIDTH * Math.random() - 1,
                SphereSimulation.CUBE_WIDTH * Math.random() - 1,
                SphereSimulation.CUBE_WIDTH * Math.random() - 1
            ];
            
            this.sphereState.velocities[i] = this.vector.multiply(
                [Math.random() - 1, Math.random() - 1, Math.random() - 1],
                SphereSimulation.MAX_VELOCITY
            );
        }
    }

    initializeGrid() {
        this.grid.cellLength = Math.ceil(2.0 / Math.pow(this.numberOfSpheres, 1/3));
        this.grid.size = Math.ceil(SphereSimulation.CUBE_WIDTH / this.grid.cellLength);
        this.updateCollisionMap();
    }

    setupBuffers(gl) {
        this.positionBuffer = this.createBuffer(gl, this.sphereState.positions, 0, gl.DYNAMIC_DRAW);
        this.createBuffer(gl, this.sphereState.radii, 1, gl.STATIC_DRAW);
        this.createBuffer(gl, this.sphereState.colors, 2, gl.STATIC_DRAW);
    }

    createBuffer(gl, data, location, drawMode) {
        const buffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(data.flat()), drawMode);
        gl.vertexAttribPointer(location, data[0].length || 1, gl.FLOAT, false, 0, 0);
        gl.enableVertexAttribArray(location);
        return buffer;
    }

    // Vector operations helper
    vector = {
        add: (a, b) => a.map((v, i) => v + b[i]),
        subtract: (a, b) => a.map((v, i) => v - b[i]),
        multiply: (v, scalar) => v.map(x => x * scalar),
        dot: (a, b) => a.reduce((sum, v, i) => sum + v * b[i], 0),
        magnitude: v => Math.sqrt(v.reduce((sum, x) => sum + x * x, 0))
    };

    updatePhysics(deltaSeconds) {
        this.updatePositions(deltaSeconds);
        this.handleCollisions(deltaSeconds);
        this.applyGravity(deltaSeconds);
        this.updateCollisionMap();
    }

    updatePositions(deltaSeconds) {
        for (let i = 0; i < this.numberOfSpheres; i++) {
            this.updateSpherePosition(i, deltaSeconds);
            this.handleWallCollisions(i);
        }
    }

    updateSpherePosition(index, deltaSeconds) {
        const newPosition = this.vector.add(
            this.sphereState.positions[index],
            this.vector.multiply(this.sphereState.velocities[index], deltaSeconds)
        );
        this.sphereState.positions[index] = newPosition;
    }

    handleWallCollisions(index) {
        const radius = this.sphereState.radii[index];
        for (let axis = 0; axis < 3; axis++) {
            this.checkWallCollision(index, axis, radius);
        }
    }

    checkWallCollision(index, axis, radius) {
        const position = this.sphereState.positions[index][axis];
        const velocity = this.sphereState.velocities[index][axis];

        if (position - radius < -SphereSimulation.CUBE_WALL && velocity < 0) {
            this.handleWallBounce(index, axis, -SphereSimulation.CUBE_WALL + radius);
        } else if (position + radius > SphereSimulation.CUBE_WALL && velocity > 0) {
            this.handleWallBounce(index, axis, SphereSimulation.CUBE_WALL - radius);
        }
    }

    handleWallBounce(index, axis, newPosition) {
        this.sphereState.velocities[index][axis] *= -SphereSimulation.ELASTICITY;
        this.sphereState.positions[index][axis] = newPosition;
    }

    setNumberOfSpheres(count) {
        this.numberOfSpheres = count;
        this.initialize();
    }

    updateCollisionMap() {
        // Initialize 3D grid
        this.grid.map = Array(this.grid.size).fill().map(() => 
            Array(this.grid.size).fill().map(() => 
                Array(this.grid.size).fill().map(() => [])
            )
        );
    
        // Assign spheres to grid cells
        for (let i = 0; i < this.numberOfSpheres; i++) {
            const gridPosition = this.sphereState.positions[i].map(pos => 
                Math.floor((pos + SphereSimulation.CUBE_WALL) / this.grid.cellLength)
            );
    
            // Ensure position is within grid bounds
            for (let axis = 0; axis < 3; axis++) {
                gridPosition[axis] = Math.max(0, Math.min(this.grid.size - 1, gridPosition[axis]));
            }
    
            this.grid.map[gridPosition[0]][gridPosition[1]][gridPosition[2]].push(i);
        }
    }
    
    handleCollisions(deltaSeconds) {
        const newVelocities = this.sphereState.velocities.map(v => [...v]);
    
        // Check collisions within each grid cell and with neighboring cells
        for (let i = 0; i < this.grid.size; i++) {
            for (let j = 0; j < this.grid.size; j++) {
                for (let k = 0; k < this.grid.size; k++) {
                    const currentCell = this.grid.map[i][j][k];
                    
                    // Check collisions within current cell
                    for (let s = 0; s < currentCell.length; s++) {
                        for (let t = 0; t < s; t++) {
                            this.handleSpherePairCollision(currentCell[s], currentCell[t], newVelocities);
                        }
    
                        // Check collisions with neighboring cells
                        const directions = [
                            [0,0,1], [0,1,0], [1,0,0],
                            [1,1,0], [1,0,1], [0,1,1], [1,1,1]
                        ];
    
                        for (const [dx, dy, dz] of directions) {
                            const ni = i + dx;
                            const nj = j + dy;
                            const nk = k + dz;
    
                            if (ni >= this.grid.size || nj >= this.grid.size || nk >= this.grid.size) {
                                continue;
                            }
    
                            const neighborCell = this.grid.map[ni][nj][nk];
                            for (const neighborIndex of neighborCell) {
                                this.handleSpherePairCollision(currentCell[s], neighborIndex, newVelocities);
                            }
                        }
                    }
                }
            }
        }
    
        this.sphereState.velocities = newVelocities;
    }
    
    handleSpherePairCollision(index1, index2, newVelocities) {
        const pos1 = this.sphereState.positions[index1];
        const pos2 = this.sphereState.positions[index2];
        const rad1 = this.sphereState.radii[index1];
        const rad2 = this.sphereState.radii[index2];
    
        // Calculate distance between sphere centers
        const displacement = this.vector.subtract(pos2, pos1);
        const distance = this.vector.magnitude(displacement);
        const minDistance = rad1 + rad2;
    
        // Check if spheres are colliding
        if (distance < minDistance) {
            const vel1 = this.sphereState.velocities[index1];
            const vel2 = this.sphereState.velocities[index2];
            const mass1 = this.sphereState.masses[index1];
            const mass2 = this.sphereState.masses[index2];
    
            // Calculate normal vector of collision
            const normal = displacement.map(x => x / distance);
            
            // Calculate relative velocity along normal
            const relativeVelocity = this.vector.subtract(vel2, vel1);
            const velocityAlongNormal = this.vector.dot(relativeVelocity, normal);
    
            // Only resolve collision if objects are moving toward each other
            if (velocityAlongNormal > 0) return;
    
            // Calculate impulse scalar
            const restitution = SphereSimulation.ELASTICITY;
            const impulseScalar = -(1 + restitution) * velocityAlongNormal;
            const totalMass = mass1 + mass2;
            const impulse = impulseScalar / totalMass;
    
            // Apply impulse to velocities
            const impulseVector = this.vector.multiply(normal, impulse);
            newVelocities[index1] = this.vector.subtract(
                vel1,
                this.vector.multiply(impulseVector, mass2)
            );
            newVelocities[index2] = this.vector.add(
                vel2,
                this.vector.multiply(impulseVector, mass1)
            );
    
            // Separate spheres to prevent sticking
            const correction = (minDistance - distance) * 0.5;
            const correctionVector = this.vector.multiply(normal, correction);
            this.sphereState.positions[index1] = this.vector.subtract(pos1, correctionVector);
            this.sphereState.positions[index2] = this.vector.add(pos2, correctionVector);
        }
    }
    
    applyGravity(deltaSeconds) {
        for (let i = 0; i < this.numberOfSpheres; i++) {
            this.sphereState.velocities[i][2] -= SphereSimulation.GRAVITY * deltaSeconds;
            
            // Clamp velocity to maximum speed
            const velocity = this.sphereState.velocities[i];
            const speed = this.vector.magnitude(velocity);
            if (speed > SphereSimulation.MAX_VELOCITY) {
                this.sphereState.velocities[i] = this.vector.multiply(
                    velocity,
                    SphereSimulation.MAX_VELOCITY / speed
                );
            }
        }
    }

    // Add method to get sphere count
    getNumberOfSpheres() {
        return this.numberOfSpheres;
    }

    // Add method to set sphere count
    setNumberOfSpheres(count) {
        this.numberOfSpheres = count;
        this.initialize(gl);
    }

    // Add getter for buffer
    getPositionBuffer() {
        return this.positionBuffer;
    }
}






/**
 * Given the source code of a vertex and fragment shader, compiles them,
 * and returns the linked program.
 */
function compileShader(vs_source, fs_source) {
    const vs = gl.createShader(gl.VERTEX_SHADER)
    gl.shaderSource(vs, vs_source)
    gl.compileShader(vs)
    if (!gl.getShaderParameter(vs, gl.COMPILE_STATUS)) {
        console.error(gl.getShaderInfoLog(vs))
        throw Error("Vertex shader compilation failed")
    }

    const fs = gl.createShader(gl.FRAGMENT_SHADER)
    gl.shaderSource(fs, fs_source)
    gl.compileShader(fs)
    if (!gl.getShaderParameter(fs, gl.COMPILE_STATUS)) {
        console.error(gl.getShaderInfoLog(fs))
        throw Error("Fragment shader compilation failed")
    }

    const program = gl.createProgram()
    gl.attachShader(program, vs)
    gl.attachShader(program, fs)
    gl.linkProgram(program)
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
        console.error(gl.getProgramInfoLog(program))
        throw Error("Linking failed")
    }

    const uniforms = {}
    for(let i=0; i<gl.getProgramParameter(program, gl.ACTIVE_UNIFORMS); i+=1) {
        let info = gl.getActiveUniform(program, i)
        uniforms[info.name] = gl.getUniformLocation(program, info.name)
    }
    program.uniforms = uniforms

    return program
}

/**
 * Sends per-vertex data to the GPU and connects it to a VS input
 *
 * @param data    a 2D array of per-vertex data (e.g. [[x,y,z,w],[x,y,z,w],...])
 * @param loc     the layout location of the vertex shader's `in` attribute
 * @param mode    (optional) gl.STATIC_DRAW, gl.DYNAMIC_DRAW, etc
 *
 * @returns the ID of the buffer in GPU memory; useful for changing data later
 */
function supplyDataBuffer(data, loc, mode) {
    if (mode === undefined) mode = gl.STATIC_DRAW

    const buf = gl.createBuffer()
    gl.bindBuffer(gl.ARRAY_BUFFER, buf)
    const f32 = new Float32Array(data.flat())
    gl.bufferData(gl.ARRAY_BUFFER, f32, mode)

    gl.vertexAttribPointer(loc, (data[0].length||1), gl.FLOAT, false, 0, 0)
    gl.enableVertexAttribArray(loc)

    return buf;
}


/** Draw one frame */
function draw(seconds) {
    // gl.clearColor(...IlliniBlue) // f(...[1,2,3]) means f(1,2,3)
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
    gl.useProgram(program)
    updatePoints()

    const eyePosition = [2.5, 0, 0.5]
    let v = m4view(eyePosition, [0,0,0], [0,0,1])

    // light is fixed relative to eye
    let ld = [1, -0.3, 0.8]
    let h = normalize(add(ld, [0,0,1])) // after view matrix, eye direction is [0,0,1]
    gl.uniform3fv(program.uniforms.lightdir, ld)
    gl.uniform3fv(program.uniforms.lightcolor, [1,1,1])
    gl.uniform3fv(program.uniforms.halfway, h)

    gl.uniformMatrix4fv(program.uniforms.p, false, p)

    gl.uniform1f(program.uniforms.x, gl.canvas.width)
    gl.uniform1f(program.uniforms.projx, p[0]) // proj[0][0]
    gl.uniformMatrix4fv(program.uniforms.mv, false, v)
    gl.drawArrays(gl.POINTS, 0, numberSpheres)
}

/** Compute any time-varying or animated aspects of the scene */
function tick(milliseconds) {
    let seconds = milliseconds / 1000;
    const deltaSeconds = seconds - previousSecond
    let fps = 1 / deltaSeconds
    document.querySelector('#fps').innerHTML = fps.toFixed(1)

    if (seconds - lastResetSecond >= RESET_INTERVAL_SECONDS) {
        resetSimulation()
        lastResetSecond = seconds
    }
    previousSecond = seconds
    simulatePhysic(0.5*deltaSeconds)
    draw(seconds)
    requestAnimationFrame(tick)
}



/** Resizes the canvas to completely fill the screen */
function fillScreen() {
    let canvas = document.querySelector('canvas')
    document.body.style.margin = '0'
    canvas.style.width = '100vw'
    canvas.style.height = '100vh'
    canvas.width = canvas.clientWidth
    canvas.height = canvas.clientHeight
    canvas.style.width = ''
    canvas.style.height = ''
    if (window.gl) {
        gl.viewport(0,0, canvas.width, canvas.height)
        window.p = m4perspNegZ(0.1, 32, 1.5, canvas.width, canvas.height)
    }
}


/** Compile, link, set up geometry */
window.addEventListener('load', async (event) => {
    window.gl = document.querySelector('canvas').getContext('webgl2',
        // optional configuration object: see https://developer.mozilla.org/en-US/docs/Web/API/HTMLCanvasElement/getContext
        {antialias: false, depth:true, preserveDrawingBuffer:true}
    )
    let vs = await fetch('vertexShader.glsl').then(res => res.text())
    let fs = await fetch('fragmentShader.glsl').then(res => res.text())
    window.program = compileShader(vs,fs)
    gl.enable(gl.DEPTH_TEST)
    fillScreen()
    window.addEventListener('resize', fillScreen)
    numberSpheres = Number(document.querySelector('#spheres').value) || 1
    initSimulation();
    requestAnimationFrame(tick);

    document.querySelector('#submit').addEventListener('click', event => {
        numberSpheres = Number(document.querySelector('#spheres').value) || 1;
        initSimulation();
    })
})