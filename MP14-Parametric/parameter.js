/** constants */
const DIFFUSION_COLOR = new Float32Array([1, 0.37, 0.02, 1])



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
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
    gl.useProgram(program)
    gl.bindVertexArray(geom.vao)
    gl.uniform4fv(program.uniforms.color, DIFFUSION_COLOR)
    // const eyePosition = [2.0, 2.0, 0.7]
    const eyePosition = [2.0, 2.0, 1.5]
    // light
    let ld = normalize([1, 2, 1.5])
    gl.uniform3fv(program.uniforms.lightdir, ld)
    gl.uniform3fv(program.uniforms.lightcolor, [1,1,1])
    gl.uniform3fv(program.uniforms.eye, eyePosition)
    let m = m4rotZ(0.2 * seconds) // rotating camera
    let v = m4view(eyePosition, [0,0,0], [0,0,1])
    gl.uniformMatrix4fv(program.uniforms.mv, false, m4mul(v, m))
    gl.uniformMatrix4fv(program.uniforms.p, false, p)
    
    gl.drawElements(geom.mode, geom.count, geom.type, 0)
}
/** Compute any time-varying or animated aspects of the scene */
function tick(milliseconds) {
    let seconds = milliseconds / 1000;
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
        window.p = m4perspNegZ(0.1, 10, 1.5, canvas.width, canvas.height)
    }
}




/** make sphere center at [0,0,0] with radius 1 */
function makeSphere(rings, slices) {
    if (rings < 1 || slices < 3) {
        console.error("invalid input for sphere:", rings, slices)
    }
    geom = {"triangles": []
        ,"attributes":[[], []] // [positions, normals]
    }
    const RADIUS = 1
    // make top and down
    geom.attributes[0].push([0,0,1])
    geom.attributes[0].push([0,0,-1])
    const deltaAngle = (Math.PI) / (rings + 1)
    for(let i = 1; i <= rings; i+=1) {
        // generate a ring of radius
        const radius = RADIUS * Math.sin(i * deltaAngle)
        const z_coord = RADIUS * Math.cos(i * deltaAngle)
        const theta = 2 * Math.PI / slices
        for (let j = 0; j < slices; j+=1) {
            geom.attributes[0].push([
                radius * Math.cos(j * theta),
                radius * Math.sin(j * theta),
                z_coord])
        }
    }
    // for a sphere centered at [0,0,0] and radius 1
    // vertex position == vertex normal
    geom.attributes[1] = geom.attributes[0]
    // create triangles
    // top
    for (let i = 0; i < slices; i += 1) {
        geom.triangles.push([
            0, 2 + i, 2 + (i + 1) % slices
        ])
    }
    // mid
    for (let i = 0; i < rings - 1; i += 1) {
        for (let j = 0; j < slices; j += 1) {
            let nj = (1 + j) % slices
            geom.triangles.push([
                2 + slices * i + j,
                2 + slices * i + nj,
                2 + slices * (i + 1) + j
            ])
            geom.triangles.push([
                2 + slices * i + nj,
                2 + slices * (i + 1) + j,
                2 + slices * (i + 1) + nj
            ])
        }
    }
    // bottom
    let offset = 2 + (rings - 1) * slices
    for (let i = 0; i < slices; i += 1) {
        geom.triangles.push([
            offset + i, offset + (i + 1) % slices, 1
        ])
    }
    return geom
}


/** make torus which
 * centered at (0,0,0)
 * large radius = 1
 * small radius = 0.3
*/
function makeTorus(rings, slices) {
    if (rings < 3 || slices < 3) {
        console.error("invalid input for torus:", rings, slices)
    }
    geom = {"triangles": []
        ,"attributes": [[], []] // [positions, normals]
    }

    const OUTER_RADIUS = 1
    const INNER_RADIUS = 0.3
    const theta =  2.0 * Math.PI / slices
    let offset = new Float32Array([OUTER_RADIUS, 0, 0])
    for(let i = 0; i < slices; i += 1) {
        const rot = m3rotZ(theta * i)
        const phi = 2.0 * Math.PI / rings
        const rotatedCenter = m3mul(rot, offset)
        // create a circle around x-z and then rotate to the right position
        for (let j = 0; j < rings; j += 1) {
            let pos = [
                OUTER_RADIUS + INNER_RADIUS * Math.cos(j * phi),
                0,
                INNER_RADIUS * Math.sin(j * phi)
            ]
            const finalPos = m3mul(rot, pos)
            geom.attributes[0].push(finalPos)
            const normal = normalize(sub(finalPos, rotatedCenter))
            geom.attributes[1].push(normal)
        }
    }
    // create triangles
    for (let i = 0; i < slices; i += 1) {
        let ni = (i + 1) % slices
        for (let j = 0; j < rings; j += 1) {
            let nj = (j + 1) % rings;
            geom.triangles.push([
                i * rings + j,
                i * rings + nj,
                ni * rings + j
            ])
            geom.triangles.push([
                i * rings + nj,
                ni * rings + j,
                ni * rings + nj
            ])
        }
    }
    // console.log(g)
    return geom
}




/** generate geometry of the sphere */
function generateGeom() {
    const rings = Number(document.querySelector('#rings').value) || 1
    const slices = Number(document.querySelector('#slices').value) || 2
    const isTorus = document.querySelector('#torus').checked
    const shape = (isTorus)? makeTorus(rings, slices) : makeSphere(rings, slices)
    window.geom = setupGeomery(shape)
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
    window.addEventListener('resize', fillScreen)
    document.querySelector('#submit').addEventListener('click', event => {
        generateGeom()
    })
    // initial
    generateGeom()
    // render
    requestAnimationFrame(tick)
})