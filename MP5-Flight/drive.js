const DIFFUSION_COLOR = new Float32Array([0.8, 0.6, 0.4, 1])
const UPWARD = new Float32Array([0,0,1])
const gridSize = 80, faults = 80
const DRIVE_HEIGHT = 0.05

// global variables
var prevSecond = 0
var eyePosition = [0, 0, 1]
var forward = normalize([1, 0, 0])

const m4viewF = (eye, forward, up) => m4mul(m4fixAxes(forward, up), m4trans(-eye[0],-eye[1],-eye[2]))


/** util function */
const clamp = (val, lb, ub) => Math.min(ub, Math.max(lb, val))

function moveCameraPosition() {
    let [x, y, z] = eyePosition
    x = clamp(x, -1.0, 1.0)
    y = clamp(y, -1.0, 1.0)
    let posx = gridSize * (x + 1.0) / 2.0
    let posy = gridSize * (y + 1.0) / 2.0
    let row = Math.min(Math.floor(posx), gridSize - 1)
    let col = Math.min(Math.floor(posy), gridSize - 1)
    let n = window.terrain.attributes[0].length
    let idx = row * gridSize + col

    let p00 = window.terrain.attributes[0][idx][2]
    let p01 = (idx+1 < n) ? window.terrain.attributes[0][idx+1][2] : p00
    let p10 = (idx+gridSize < n) ? window.terrain.attributes[0][idx+gridSize][2] : p00
    let p11 = (idx+gridSize+1 < n)? window.terrain.attributes[0][idx+gridSize+1][2] : p00
    let s = posx - row, t = posy - col
    z = (1-s)*(1-t)*p00 + (1-s)*t*p01 + s*(1-t)*p10 + s*t*p11 + DRIVE_HEIGHT
    eyePosition = [x,y,z]
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
    
    gl.vertexAttribPointer(loc, data[0].length, gl.FLOAT, false, 0, 0)
    gl.enableVertexAttribArray(loc)
    
    return buf;
}
/**
 * Creates a Vertex Array Object and puts into it all of the data in the given
 * JSON structure, which should have the following form:
 * 
 * ````
 * {"triangles": a list of of indices of vertices
 * ,"attributes":
 *  [ a list of 1-, 2-, 3-, or 4-vectors, one per vertex to go in location 0
 *  , a list of 1-, 2-, 3-, or 4-vectors, one per vertex to go in location 1
 *  , ...
 *  ]
 * }
 * ````
 * 
 * @returns an object with four keys:
 *  - mode = the 1st argument for gl.drawElements
 *  - count = the 2nd argument for gl.drawElements
 *  - type = the 3rd argument for gl.drawElements
 *  - vao = the vertex array object for use with gl.bindVertexArray
 */
function setupGeomery(geom) {
    var triangleArray = gl.createVertexArray()
    gl.bindVertexArray(triangleArray)
    for(let i=0; i<geom.attributes.length; i+=1) {
        let data = geom.attributes[i]
        supplyDataBuffer(data, i)
    }
    var indices = new Uint16Array(geom.triangles.flat())
    var indexBuffer = gl.createBuffer()
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer)
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indices, gl.STATIC_DRAW)
    return {
        mode: gl.TRIANGLES,
        count: indices.length,
        type: gl.UNSIGNED_SHORT,
        vao: triangleArray
    }
}
/** Draw one frame */
function draw() {
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
    gl.useProgram(program)
    gl.bindVertexArray(geom.vao)
    // gl.uniform4fv(program.uniforms.color, IlliniOrange)
    // let m = m4rotX(seconds)
    // let v = m4view([Math.cos(seconds/2),2,3], [0,0,0], [0,1,0])

    gl.uniform4fv(program.uniforms.color, DIFFUSION_COLOR)
    // const eyePosition = [1.3, 0.8, 0.8]
    // light
    let ld = normalize([1,1,2])
    gl.uniform3fv(program.uniforms.lightdir, ld)
    gl.uniform3fv(program.uniforms.lightcolor, [1,1,1])
    gl.uniform3fv(program.uniforms.eye, eyePosition)
    // let m = m4rotZ(0.2 * seconds) // rotating camera
    // let v = m4view(eyePosition, [0,0,0], [0,0,1])
    let v = m4viewF(eyePosition, forward, UPWARD)


    gl.uniformMatrix4fv(program.uniforms.mv, false, v)
    gl.uniformMatrix4fv(program.uniforms.p, false, p)
    
    gl.drawElements(geom.mode, geom.count, geom.type, 0)
}




function controlCameraMovement(seconds, prevSecond, eyePosition, forward) {
    const right = cross(forward, UPWARD);
    const FORWARD_SPEED = 0.1;
    const SIDE_SPEED = 0.1;
    const ROTATE_SPEED = 0.5;
    const deltaTime = seconds - prevSecond;

    let newEyePosition = [...eyePosition];
    let newForward = [...forward];

    // move forward/backward
    if (keysBeingPressed['w']) {
        newEyePosition = add(newEyePosition, mul(forward, FORWARD_SPEED * deltaTime));
    } else if (keysBeingPressed['s']) {
        newEyePosition = sub(newEyePosition, mul(forward, FORWARD_SPEED * deltaTime));
    }

    // move right/left
    if (keysBeingPressed['a']) {
        newEyePosition = sub(newEyePosition, mul(right, SIDE_SPEED * deltaTime));
    } else if (keysBeingPressed['d']) {
        newEyePosition = add(newEyePosition, mul(right, SIDE_SPEED * deltaTime));
    }

    // rotate around right (pitch)
    if (keysBeingPressed['ArrowUp']) {
        newForward = m3mul(m3rotAxis(right, ROTATE_SPEED * deltaTime), newForward);
    } else if (keysBeingPressed['ArrowDown']) {
        newForward = m3mul(m3rotAxis(right, -ROTATE_SPEED * deltaTime), newForward);
    }

    // rotate around UPWARD (yaw)
    if (keysBeingPressed['ArrowLeft']) {
        newForward = m3mul(m3rotAxis(UPWARD, ROTATE_SPEED * deltaTime), newForward);
    } else if (keysBeingPressed['ArrowRight']) {
        newForward = m3mul(m3rotAxis(UPWARD, -ROTATE_SPEED * deltaTime), newForward);
    }

    return {
        eyePosition: newEyePosition,
        forward: newForward,
        prevSecond: seconds
    };
}




/** Compute any time-varying or animated aspects of the scene */
function tick(milliseconds) {
    let seconds = milliseconds / 1000;
    const cameraState = controlCameraMovement(seconds, prevSecond, eyePosition, forward);
    
    eyePosition = cameraState.eyePosition;
    forward = cameraState.forward;
    prevSecond = cameraState.prevSecond;
    

    moveCameraPosition()
    draw()
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
        // window.p = m4perspNegZ(0.1, 10, 1.5, canvas.width, canvas.height)
        // window.p = m4perspNegZ(0.1, 10, 1, canvas.width, canvas.height)
        window.p = m4perspNegZ(0.001, 10, 1.5, canvas.width, canvas.height)
    }
}







/**
 * Creates faults to adjust terrain heights.
 * @param {Array} positions - Array of vertex positions [x, y, z].
 */
function createFault(positions) {
    // Generate a random point p and a normalized direction vector norm
    const p = [(Math.random() - 0.5) * 2, (Math.random() - 0.5) * 2, 0]; // Random point in [-1, 1] range
    const theta = 2.0 * Math.PI * Math.random();
    const norm = [Math.cos(theta), Math.sin(theta), 0];
    const R = 1.5; // Fault influence radius

    positions.forEach(position => {
        const product = dot(sub(position, p), norm);
        if (Math.abs(product) >= R) return; // Skip if outside influence radius

        const coefficient = Math.pow(1 - Math.pow(product / R, 2), 2);
        position[2] += (product > 0 ? 1 : -1) * coefficient;
    });
}

/**
 * Generates a geometry grid with faults to simulate terrain.
 * @param {number} gridSize - Number of vertices per grid row/column.
 * @param {number} faults - Number of faults to apply to the terrain.
 * @returns {Object} The geometry data with vertices, normals, and triangles.
 */
function makeGeom(gridSize, faults) {
    const geom = {
        triangles: [],
        attributes: [[], []] // [positions, normals]
    };

    // Create grid with positions in [-1, 1] range
    const step = 2.0 / (gridSize - 1);
    for (let i = 0; i < gridSize; i++) {
        for (let j = 0; j < gridSize; j++) {
            geom.attributes[0].push([-1.0 + step * i, -1.0 + step * j, 0]);
        }
    }

    // Apply faults to create terrain height variation
    for (let i = 0; i < faults; i++) {
        createFault(geom.attributes[0]);
    }

    // Normalize terrain height
    const heights = geom.attributes[0].map(pos => pos[2]);
    const maxHeight = Math.max(...heights);
    const minHeight = Math.min(...heights);
    const PEAK_HEIGHT = 0.8;

    geom.attributes[0].forEach(pos => {
        if (maxHeight !== minHeight) {
            pos[2] = PEAK_HEIGHT * (pos[2] - (maxHeight + minHeight) / 2) / (maxHeight - minHeight);
        }
    });

    // Create triangles (indices)
    for (let i = 0; i < gridSize - 1; i++) {
        for (let j = 0; j < gridSize - 1; j++) {
            const cur = i * gridSize + j;
            geom.triangles.push([cur, cur + 1, cur + gridSize]);
            geom.triangles.push([cur + 1, cur + gridSize, cur + gridSize + 1]);
        }
    }

    // Calculate normals for smooth shading
    geom.attributes[0].forEach((_, i) => {
        const row = Math.floor(i / gridSize);
        const col = i % gridSize;

        const n = (row > 0) ? geom.attributes[0][i - gridSize] : geom.attributes[0][i];
        const s = (row < gridSize - 1) ? geom.attributes[0][i + gridSize] : geom.attributes[0][i];
        const w = (col > 0) ? geom.attributes[0][i - 1] : geom.attributes[0][i];
        const e = (col < gridSize - 1) ? geom.attributes[0][i + 1] : geom.attributes[0][i];

        const normal = cross(sub(n, s), sub(w, e));
        geom.attributes[1].push(normal);
    });

    return geom;
}

/**
 * Generates terrain geometry and initializes rendering.
 * @param {number} gridSize - Number of vertices per grid row/column.
 * @param {number} faults - Number of faults to apply to the terrain.
 */
function generateTerrain(gridSize, faults) {
    // Generate geometry
    window.terrain = makeGeom(gridSize, faults)
    window.geom = setupGeomery(window.terrain)

    requestAnimationFrame(tick);
}


/** Compile, link, set up geometry */
window.addEventListener('load', async (event) => {
    window.gl = document.querySelector('canvas').getContext('webgl2',
    // optional configuration object: see https://developer.mozilla.org/en-US/docs/Web/API/HTMLCanvasElement/getContext
    {antialias: false, depth:true, preserveDrawingBuffer:true}
    )
    // let vs = document.querySelector('#vert').textContent.trim()
    // let fs = document.querySelector('#frag').textContent.trim()
    let vs = await fetch('vertexShader.glsl').then(res => res.text())
    let fs = await fetch('fragmentShader.glsl').then(res => res.text())
    window.program = compileShader(vs,fs)
    gl.enable(gl.DEPTH_TEST)
    gl.enable(gl.BLEND)
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
    fillScreen()
    window.keysBeingPressed = {}
    window.addEventListener('keydown', event => keysBeingPressed[event.key] = true)
    window.addEventListener('keyup', event => keysBeingPressed[event.key] = false)
    generateTerrain(gridSize, faults)
})