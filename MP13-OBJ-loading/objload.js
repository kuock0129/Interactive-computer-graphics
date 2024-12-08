
var diffusionColor = new Float32Array([1,1,1,0.3])
var animationStarted = false
var useTexture = false
var textureProgram
var nontextureProgram
var slot = 0

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

    return buf
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

/** resize the given array to a fixed size */
function resize(arr, newSize, defaultValue) {
    for (let i = arr.length; i < newSize; i+= 1) {
        arr.push(defaultValue)
    }
}

/** Parse obj */
async function readObj(filename) {
    const vPs = []
    const vTs = []
    const vNs = []
    const colors = []

    const COLOR_ID = 3
    const data = [
        vPs,
        vTs,
        vNs,
        colors
    ]

    let webGLVertices = [
        [], // pos (vec3 + color)
        [], // texture (vec2)
        [], // normal (vec3)
        []  // color (vec3)
    ]
    let triangles = []
    let vertexMp = new Map()
    // parse each face
    function addVertex(face) {
        if (vertexMp.has(face)) {
            return vertexMp.get(face)
        }
        let vid = webGLVertices[0].length
        let idxs = face.split('/')
        for (let i = 0; i < 3; i+=1) {
            if (i < idxs.length && idxs[i]) {
                let realIdx = parseInt(idxs[i]) - 1 // 0-indexed
                webGLVertices[i].push(data[i][realIdx])
                // add color
                if (i === 0) {
                    webGLVertices[COLOR_ID].push(data[COLOR_ID][realIdx])
                }
            }
        }
        vertexMp.set(face, vid)
        return vid
    }

    function addTexCood() {
        if (webGLVertices[1].length === webGLVertices[0].length) {
            return
        }
        resize(webGLVertices[1], webGLVertices[0].length, [0,0])
    }

    function addNormals() {
        if (webGLVertices[2].length === webGLVertices[0].length) {
            return
        }
        resize(webGLVertices[2], webGLVertices[0].length, [0,0,0])
        for (let t = 0; t < triangles.length; t += 1) {
            let [x,y,z] = triangles[t]
            const e1 = sub(webGLVertices[0][y], webGLVertices[0][x])
            const e2 = sub(webGLVertices[0][z], webGLVertices[0][x])
            const n = cross(e1, e2)
            // add normal
            webGLVertices[2][x] = add(webGLVertices[2][x], n)
            webGLVertices[2][y] = add(webGLVertices[2][y], n)
            webGLVertices[2][z] = add(webGLVertices[2][z], n)
        }
        for (let i = 0; i < webGLVertices[2].length; i += 1) {
            webGLVertices[2][i] = normalize(webGLVertices[2][i])
        }
    }

    function transformObjToCenter() {
        let minCoods = [...webGLVertices[0][0]]
        let maxCoods = [...webGLVertices[0][0]]
        for (let i = 1; i < webGLVertices[0].length; i += 1) {
            for (let c = 0; c < 3; c += 1) {
                minCoods[c] = Math.min(minCoods[c], webGLVertices[0][i][c])
                maxCoods[c] = Math.max(maxCoods[c], webGLVertices[0][i][c])
            }
        }

        // align center to (min + max) / 2
        const center = div(add(minCoods, maxCoods), 2)
        const scale = 2 / Math.max(...sub(maxCoods,minCoods))

        for (let i = 0; i < webGLVertices[0].length; i += 1) {
            webGLVertices[0][i] = mul(sub(webGLVertices[0][i], center), scale)
        }
    }

    const keywords = {
        v(args) {
            let nums = args.map(parseFloat)
            vPs.push(nums.slice(0,3))
            if (nums.length === 6) {
                colors.push(nums.slice(3))
            } else {
                colors.push([0.7,0.7,0.7]) // light-gray
            }
        },
        vt(args) {
            vTs.push(args.map(parseFloat))
        },
        vn(args) {
            vNs.push(args.map(parseFloat))
        },
        f(args) {
            let vids = args.map(face => addVertex(face))
            for (let i = 1; i < vids.length - 1; i+=1) {
                triangles.push([vids[0], vids[i], vids[i+1]])
            }
        }
    }

    let text = await fetch(filename).then(res => res.text())
    text.split('\n').forEach((line) => {
        if (line === '' || line.startsWith('#')) {
            return
        }
        line = line.trim()
        tokens = line.split(/\s+/)
        const keyword = tokens[0]
        const args = tokens.slice(1)
        let handler = keywords[keyword]
        if (!handler) {
            console.warn("unknow keyword", keyword)
            return
        }
        handler(args)
    })

    // vt, vn may be empty
    addTexCood()
    addNormals()

    // scale and align
    transformObjToCenter()

    let ret = {
        "triangles": triangles,
        "attributes": webGLVertices
    }
    return ret
}

/** Parse material */
function changeMaterial(value) {
    if (value == '') {
        useTexture = false
    } else if (/[.](jpg|png)$/.test(value)) {
        let img = new Image()
        img.crossOrigin = 'anonymous'
        img.src = value
        img.addEventListener('load', event => {
            useTexture = true
            // change fragment shader
            let texture = gl.createTexture()
            gl.activeTexture(gl.TEXTURE0 + slot) // slot = 0
            gl.bindTexture(gl.TEXTURE_2D, texture)

            // out of edge
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE)
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE)

            gl.texImage2D(
                gl.TEXTURE_2D, // destination slot
                0, // the mipmap level this data provides; almost always 0
                gl.RGBA, // how to store it in graphics memory
                gl.RGBA, // how it is stored in the image object
                gl.UNSIGNED_BYTE, // size of a single pixel-color in HTML
                img, // source data
            )
            gl.generateMipmap(gl.TEXTURE_2D) // lets you use a mipmapping min filter
        })
        img.addEventListener('error', event => {
            console.error("failed to load", value)
            useTexture = false
        })
    }
}

/** Draw one frame */
function draw(seconds) {
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
    let program = (useTexture)? textureProgram: nontextureProgram
    gl.useProgram(program)

    gl.bindVertexArray(geom.vao)

    if (useTexture) {
        let bindPoint = gl.getUniformLocation(textureProgram, 'image')
        gl.uniform1i(bindPoint, slot) // where `slot` is same it was in step 2 above
    }

    const eyePosition = [2.1, 1.0, 1.4]
    let m = m4rotZ(0.5 * seconds) // rotating camera
    let v = m4view(eyePosition, [0,0,0], [0,0,1])
    let mv = m4mul(v, m)

    // light is fixed relative to eye
    let ld = normalize([1,1,2])
    let h = normalize(add(ld, [0,0,1])) // after view matrix, eye direction is [0,0,1]
    gl.uniform3fv(program.uniforms.lightdir, ld)
    gl.uniform3fv(program.uniforms.lightcolor, [1,1,1])
    gl.uniform3fv(program.uniforms.halfway, h)

    gl.uniformMatrix4fv(program.uniforms.mv, false, mv)
    gl.uniformMatrix4fv(program.uniforms.p, false, p)

    gl.drawElements(geom.mode, geom.count, geom.type, 0)
}

/** Compute any time-varying or animated aspects of the scene */
function tick(milliseconds) {
    let seconds = milliseconds / 1000

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
        window.p = m4perspNegZ(0.1, 10, 1, canvas.width, canvas.height)
    }
}

/** Compile, link, set up geometry */
window.addEventListener('load', async (event) => {
    window.gl = document.querySelector('canvas').getContext('webgl2',
    // optional configuration object: see https://developer.mozilla.org/en-US/docs/Web/API/HTMLCanvasElement/getContext
    {antialias: false, depth:true, preserveDrawingBuffer:true}
    )
    let vs = await fetch('vertexShader.glsl').then(res => res.text())
    let nonTextureFs = await fetch('nontexture_fragmentShader.glsl').then(res => res.text())
    let textureFs = await fetch('texture_fragmentShader.glsl').then(res => res.text())
    window.nontextureProgram = compileShader(vs, nonTextureFs)
    window.textureProgram = compileShader(vs, textureFs)
    gl.enable(gl.DEPTH_TEST)
    gl.enable(gl.BLEND)
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)

    fillScreen()
    window.addEventListener('resize', fillScreen)

    document.querySelector('#imagepath').addEventListener('change', event=>{
        const imagePath = document.querySelector('#imagepath').value
        changeMaterial(imagePath)
    })

    document.querySelector('#filepath').addEventListener('change', async event=>{
        const filePath = document.querySelector('#filepath').value
        const obj = await readObj(filePath)
        window.geom = setupGeomery(obj)
    })

    document.querySelector('#submit').addEventListener('click', async event => {
        const filePath = document.querySelector('#filepath').value
        const imagePath = document.querySelector('#imagepath').value
        const obj = await readObj(filePath)
        window.geom = setupGeomery(obj)
        // render
        if (!animationStarted) {
            animationStarted = true
            requestAnimationFrame(tick)
        }
        changeMaterial(imagePath)
    })
    // // by default generate a 50x50 grid and 50 faults
    // generateTerrain(50, 50)

    // // initial
    // const gridSize = Number(document.querySelector('#gridsize').value) || 2
    // const faults = Number(document.querySelector('#faults').value) || 0
    // generateTerrain(gridSize, faults)
    
    // // render
    // requestAnimationFrame(tick)
})