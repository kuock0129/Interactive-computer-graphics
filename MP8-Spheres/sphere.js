const IlliniBlue = new Float32Array([0.075, 0.16, 0.292, 1])
const IlliniOrange = new Float32Array([1, 0.373, 0.02, 1])
const IdentityMatrix = new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1])

const SimulationConfig = {
    numberSpheres: 50,
    cubeWidth: 2,
    cubeWall: 2 / 2,
    sphereToCubeRatio: 0.15,
    sphereRadius: (2 * 0.15) / 2,
    RESET_INTERVAL_SECONDS: 15,
    ELASTICITY: 0.9,
    G: 9.8,
    MAX_V: 3
};

var SimulationState = {
    spherePositions: [],    // reset every RESET_INTERVAL_SECONDS
    sphereVelocity: [],     // reset every RESET_INTERVAL_SECONDS
    sphereColors: [],
    lastResetSecond: 0,
    previousSecond: 0
};

/** simulate each sphere's movement */
function simulatePhysic(deltaSeconds) {
    const { numberSpheres, cubeWall, sphereRadius, ELASTICITY, G } = SimulationConfig;
    const { spherePositions, sphereVelocity } = SimulationState;

    for (let i = 0; i < numberSpheres; i += 1) {
        spherePositions[i] = add(spherePositions[i], mul(sphereVelocity[i], deltaSeconds));

        // handle velocity
        for (let axis = 0; axis < 3; axis += 1) {
            if ((spherePositions[i][axis] - sphereRadius < -cubeWall) &&
                (sphereVelocity[i][axis] < 0) // toward wall
            ) {
                sphereVelocity[i][axis] = -sphereVelocity[i][axis] * ELASTICITY;
                spherePositions[i][axis] = -cubeWall + sphereRadius;
            }
            if ((spherePositions[i][axis] + sphereRadius > cubeWall) &&
                (sphereVelocity[i][axis] > 0) // toward wall
            ) {
                sphereVelocity[i][axis] = -sphereVelocity[i][axis] * ELASTICITY;
                spherePositions[i][axis] = cubeWall - sphereRadius;
            }
        }
    }

    let newVelocity = [...sphereVelocity];
    for (let i = 0; i < numberSpheres; i += 1) {
        // handle collision
        for (let j = 0; j < i; j += 1) {
            const displacement = sub(spherePositions[j], spherePositions[i]);
            const d2 = magDot(displacement);
            const ddoti = dot(displacement, sphereVelocity[i]);
            const ddotj = dot(displacement, sphereVelocity[j]);
            if ((d2 < 4 * sphereRadius * sphereRadius) &&
                (dot(displacement, sub(sphereVelocity[i], sphereVelocity[j])) > 0)
            ) {
                const si = mul(displacement, ddoti / d2);
                const sj = mul(displacement, ddotj / d2);
                const s = sub(si, sj);
                const bounce = mul(s, 0.5 * (1 + ELASTICITY)); // each sphere has the same mass
                newVelocity[i] = sub(newVelocity[i], bounce);
                newVelocity[j] = add(newVelocity[j], bounce);
            }
        }
        // create gravity
        const Z_AXIS = 2;
        newVelocity[i][Z_AXIS] -= G * deltaSeconds;
    }
    SimulationState.sphereVelocity = newVelocity;
}

/** init each sphere's color */
function initSimulation() {
    const { numberSpheres } = SimulationConfig;
    const { sphereColors, spherePositions, sphereVelocity } = SimulationState;

    for (let i = 0; i < numberSpheres; i += 1) {
        sphereColors.push([Math.random(), Math.random(), Math.random(), 1]);
        spherePositions.push([0, 0, 0]);
        sphereVelocity.push([0, 0, 0]);
    }
    resetSimulation();
}

/** reset each sphere's pos, velocity */
function resetSimulation() {
    const { numberSpheres, cubeWidth, MAX_V } = SimulationConfig;
    const { spherePositions, sphereVelocity } = SimulationState;

    for (let i = 0; i < numberSpheres; i += 1) {
        // TODO: find collision-free initial states
        spherePositions[i] = [cubeWidth * Math.random() - 1, cubeWidth * Math.random() - 1, cubeWidth * Math.random() - 1];
        sphereVelocity[i] = mul([Math.random() - 1, Math.random() - 1, Math.random() - 1], MAX_V);
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
function draw(seconds) {
    const { numberSpheres, sphereRadius } = SimulationConfig;
    const { spherePositions, sphereColors } = SimulationState;
    
    // gl.clearColor(...IlliniBlue) // f(...[1,2,3]) means f(1,2,3)
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
    gl.useProgram(program)

    let view = m4view([5,7,2], [0,0,0], [0,0,1])
    view = m4mul(p, view)
    // TODO : render spheres
    gl.bindVertexArray(sphere.vao)
    const eyePosition = [2.5, 0, 0.5]
    let v = m4view(eyePosition, [0,0,0], [0,0,1])
    let scale = m4scale(sphereRadius, sphereRadius, sphereRadius) // the given model rad = 1
    // light is fixed relative to eye
    let ld = [1,0.3,0.4]
    let h = normalize(add(ld, [0,0,1])) // after view matrix, eye direction is [0,0,1]
    gl.uniform3fv(program.uniforms.lightdir, ld)
    gl.uniform3fv(program.uniforms.lightcolor, [1,1,1])
    gl.uniform3fv(program.uniforms.halfway, h)
    gl.uniformMatrix4fv(program.uniforms.p, false, p)
    for (let i = 0; i < numberSpheres; i+=1) {
        let mv = m4mul(v, m4trans(...spherePositions[i]), scale)
        gl.uniformMatrix4fv(program.uniforms.mv, false, mv)
        gl.uniform4fv(program.uniforms.color, sphereColors[i])
        gl.drawElements(sphere.mode, sphere.count, sphere.type, 0)
    }


}




/** Compute any time-varying or animated aspects of the scene */
function tick(milliseconds) {
    let seconds = milliseconds / 1000;

    if (seconds - SimulationState.lastResetSecond >= SimulationConfig.RESET_INTERVAL_SECONDS) {
        resetSimulation()
        SimulationState.lastResetSecond = seconds
    }


    const deltaSeconds = seconds - SimulationState.previousSecond
    SimulationState.previousSecond = seconds
    simulatePhysic(deltaSeconds)



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
    let sphere = await fetch('sphere.json').then(r=>r.json())
    window.sphere = setupGeomery(sphere)
    fillScreen()
    window.addEventListener('resize', fillScreen)
    // TODO: set up initial condition
    initSimulation()
    requestAnimationFrame(tick)
})