const IlliniBlue = new Float32Array([0.075, 0.16, 0.292, 1])
const IlliniOrange = new Float32Array([1, 0.373, 0.02, 1])
const IdentityMatrix = new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1])

const cubeWidth = 2
const cubeWall = cubeWidth / 2
const sphereToCubeRatio = 0.15
const RESET_INTERVAL_SECONDS = 15
const ELASTICITY = 0.9
const G = 3 // self-defined value

var numberSpheres = 2
const MAX_V = 3
var spherePositions = []    // reset every RESET_INTERVAL_SECONDS
var sphereVelocity = []     // reset every RESET_INTERVAL_SECONDS
var sphereColors = []
var sphereRadius = []
var sphereMass = []
var posBuff
var mp
var gridSize
var gridLen
var lastResetSecond = 0
var previousSecond = 0

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

/** set up points */
function setupPoints() {
    posBuff = supplyDataBuffer(spherePositions, 0, gl.DYNAMIC_DRAW)
    supplyDataBuffer(sphereRadius, 1, gl.STATIC_DRAW)
    supplyDataBuffer(sphereColors, 2, gl.STATIC_DRAW)
}

/** update positions */
function updatePoints() {
    gl.bindBuffer(gl.ARRAY_BUFFER, posBuff)
    const f32 = new Float32Array(spherePositions.flat())
    gl.bufferData(gl.ARRAY_BUFFER, f32, gl.DYNAMIC_DRAW)
}



/** handle collision by updating
 * @param i vertex index
 * @param j vertex index
 * @param newVelocity velocity array
 */
function handleCollision(i, j, newVelocity) {
    const displacement = sub(spherePositions[j], spherePositions[i])
    const d2 = magDot(displacement)
    const ddoti = dot(displacement, sphereVelocity[i])
    const ddotj = dot(displacement, sphereVelocity[j])
    const rr = (sphereRadius[i] + sphereRadius[j])
    if ((d2 < rr**2) &&
        (dot(displacement, sub(sphereVelocity[i], sphereVelocity[j])) > 0)
    ) {
        const si = mul(displacement, ddoti / d2)
        const sj = mul(displacement, ddotj / d2)
        const s = sub(si, sj)
        const sumMass = sphereMass[i] + sphereMass[j]
        const wi = sphereMass[j] / sumMass
        const wj = sphereMass[i] / sumMass
        const bi = mul(s, -wi * (1 + ELASTICITY))
        const bj = mul(s, wj * (1 + ELASTICITY))
        newVelocity[i] = add(newVelocity[i], bi)
        newVelocity[j] = add(newVelocity[j], bj)
    }
}
/** update collision map */
function updateMap() {
    let nmp = []
    for (let i = 0; i < gridSize; i += 1) {
        let row = []
        for (let j = 0; j < gridSize; j += 1) {
            let col = []
            for (let k = 0; k < gridSize; k += 1) {
                col.push([])
            }
            row.push(col)
        }
        nmp.push(row)
    }
    for (let i = 0; i < numberSpheres; i+=1) {
        const pos = spherePositions[i]
        const x = Math.floor((pos[0] + cubeWall) / gridLen)
        const y = Math.floor((pos[1] + cubeWall) / gridLen)
        const z = Math.floor((pos[2] + cubeWall) / gridLen)
        nmp[x][y][z].push(i)
    }
    mp = nmp
}



/** simulate each sphere's movement */
function simulatePhysic(deltaSeconds) {
    for (let i = 0; i < numberSpheres; i += 1) {
        spherePositions[i] = add(spherePositions[i], mul(sphereVelocity[i],deltaSeconds))
        const rad = sphereRadius[i]
        // handle velocity
        for (let axis = 0; axis < 3; axis += 1) {
            if ((spherePositions[i][axis] - rad < -cubeWall) &&
            (sphereVelocity[i][axis] < 0) // torward wall
            ) {
                sphereVelocity[i][axis] = -sphereVelocity[i][axis]*ELASTICITY
                spherePositions[i][axis] = -cubeWall + rad
            }
            if ((spherePositions[i][axis] + rad > cubeWall) &&
            (sphereVelocity[i][axis] > 0) // torward wall
            ) {
                sphereVelocity[i][axis] = -sphereVelocity[i][axis]*ELASTICITY
                spherePositions[i][axis] = cubeWall - rad
            }
        }
    }
    newVelocity = [...sphereVelocity]
    // collision
    for (let i = 0; i < gridSize; i += 1) {
        for (let j = 0; j < gridSize; j += 1) {
            for (let k = 0; k < gridSize; k += 1) {
                let cur = mp[i][j][k]
                for (let s = 0; s < cur.length; s += 1) {
                    // check self-collision
                    for (let t = 0; t < s; t++) {
                        handleCollision(cur[s], cur[t], newVelocity)
                    }
                    const dir = [[0,0,1],[0,1,0],[1,0,0],
                        [1,1,0],[1,0,1],[0,1,1],[1,1,1]]
                    for (let d of dir) {
                        if ((i + d[0] >= gridSize) ||
                            (j + d[1] >= gridSize) ||
                            (k + d[2] >= gridSize)) {
                            continue
                        }
                        let neighbors = mp[i+d[0]][j+d[1]][k+d[2]]
                        for (let t = 0; t < neighbors.length; t++) {
                            handleCollision(cur[s], neighbors[t], newVelocity)
                        }
                    }
                }
            }
        }

        // create gravity
        for (let i = 0; i < numberSpheres; i += 1) {
            const Z_AXIS = 2
            newVelocity[i][Z_AXIS] -= G * deltaSeconds
        }
    }
    sphereVelocity = newVelocity
    updateMap()
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
    let ld = [1,0.3,0.4]
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

// init each sphere's color
function initSimulation() {
    for (let i = 0; i < numberSpheres; i+=1) {
        const rad = (Math.random()+.25) * (0.75/numberSpheres**(1/3))
        sphereRadius.push(rad)
        sphereMass.push(rad**3)
        sphereColors.push([Math.random(), Math.random(), Math.random(),1])
        spherePositions.push([0,0,0])
        sphereVelocity.push([0,0,0])
    }
    resetSimulation()
    setupPoints()
    gridLen = Math.ceil(2.0/numberSpheres**(1/3)) // 2.0 > 2 * 1.25 * 0.75
    gridSize = Math.ceil(cubeWidth / gridLen)
    updateMap()
}

// reset each sphere's pos, velocity
function resetSimulation() {
    for (let i = 0; i < numberSpheres; i+=1) {
        spherePositions[i] = [cubeWidth*Math.random()-1, cubeWidth*Math.random()-1, cubeWidth*Math.random()-1]
        sphereVelocity[i] = mul([Math.random()-1, Math.random()-1, Math.random()-1], MAX_V)
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
    initSimulation()
    requestAnimationFrame(tick)

    document.querySelector('#submit').addEventListener('click', event => {
        numberSpheres = Number(document.querySelector('#spheres').value) || 1
        initSimulation()
    })
})