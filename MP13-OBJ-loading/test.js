/** constants */
var diffusionColor = new Float32Array([0.5,0.5,0.5,1])
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

/** Set non-texture color on terrain */
function setShaderColor(r,g,b,a) {
    useTexture = false
    diffusionColor = new Float32Array([r,g,b,a])
}

/** Parse obj */
async function readObj(filename) {
    const vPs = [[0,0,0]]
    const vTs = [[0,0]]
    const vNs = [[0,0,0]]
    
    const vertices = [
        vPs,
        vTs,
        vNs,
    ]

    let webGLVertices = [
        [], // pos (vec3 + color)
        [], // texture (vec2)
        [], // normal (vec3)
    ]
    let triangles = []
    // parse each face
    function addVertex(face) {
        face.split('/').forEach((idxStr, i) => {
            let realIdx = 0
            if (idxStr) {
                realIdx = parseInt(idxStr)
            }
            webGLVertices[i].push(vertices[i][realIdx])
        }
        )
    }

    const keywords = {
        v(args) {
            // TODO color
            vPs.push(args.map(parseFloat).slice(0,3))
        },
        vt(args) {
            vTs.push(args.map(parseFloat))
        },
        vn(args) {
            vNs.push(args.map(parseFloat))
        },
        f(args) {
            let start = webGLVertices[0].length
            args.forEach(face => addVertex(face))
            for (let i = start + 1; i < start + args.length - 1; i+=1) {
                triangles.push([start, i, i+1])
            }
        }
    }

    let text = await fetch(filename).then(res => res.text())
    text.split('\n').forEach((line) => {
        if (line === '' || line.startsWith('#')) {
            return
        }
        tokens = line.split(/\s+/)
        const keyword = tokens[0]
        const args = tokens.slice(1)
        keywords[keyword](args)
    })

    let ret = {
        "triangles": triangles,
        "attributes": webGLVertices
    }
    return ret
}

/** Parse material */
function changeMaterial(value) {
    if (value == '') {
        setShaderColor(...diffusionColor)
    } else if (/^#[0-9a-f]{8}$/i.test(value)) {
        const r = Number('0x' + value.substr(1,2))/255
        const g = Number('0x' + value.substr(3,2))/255
        const b = Number('0x' + value.substr(5,2))/255
        const a = Number('0x' + value.substr(7,2))/255
        setShaderColor(r,g,b,a)
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
            setShaderColor(1,0,1,0)
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
    } else {
        gl.uniform4fv(program.uniforms.color, diffusionColor)
    }

    const eyePosition = [1.4, 1.0, 1.4]
    let m = m4rotZ(0.2 * seconds) // rotating camera
    let v = m4view(eyePosition, [0,0,0], [0,0,1])
    let mv = m4mul(v, m)

    // light is fixed relative to terrain
    let ld = m4mul(mv,normalize([1,1,2,0])).slice(0, 3)
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

/** create fault on the given position array */
function createFault(positions) {
    // create random p
    let p = [(Math.random() - 0.5) * 2, (Math.random() - 0.5) * 2, 0] // [-1, 1]
    let theta = 2.0 * Math.PI * Math.random()
    let norm = [Math.cos(theta), Math.sin(theta), 0]
    const R = 1.5
    for (let i = 0; i < positions.length; i += 1) {
        let product = dot(sub(positions[i], p), norm)
        if (Math.abs(product) >= R) {
            continue
        }
        const coefficient = Math.pow(1 - Math.pow(product / R, 2), 2)
        if (product > 0) {
            positions[i][2] += coefficient
        } else {
            positions[i][2] -= coefficient
        }
    }
}

/** make terrain based on gridSize and faults */
function makeGeom(gridSize, faults) {
    g = {"triangles":
        []
    ,"attributes":
        [ // position
            []
        , // normal
            []
        , // texture
            []
        ]
    }
    // create grid
    // make x, y range from [-1, 1]
    const d = 2.0 / (gridSize - 1.0)

    for(let i = 0; i < gridSize; i+=1) {
        for (let j = 0; j < gridSize; j+=1) {
            g.attributes[0].push([-1.0 + d * i, -1.0 + d * j, 0])
        }
    }

    // create fault
    for (let i = 0; i < faults; i += 1) {
        createFault(g.attributes[0])
    }

    // normalize height
    let maxHeight = Math.max(...g.attributes[0].map(pos => pos[2]))
    let minHeight = Math.min(...g.attributes[0].map(pos => pos[2]))
    const PEAK_HEIGHT = 0.8
    g.attributes[0].forEach(pos => {
        if (maxHeight !== minHeight) {
            pos[2] = PEAK_HEIGHT * (pos[2] - (maxHeight + minHeight) / 2) / (maxHeight - minHeight)
        }
    })

    // create triangles
    for (let i = 0; i < gridSize - 1; i += 1) {
        for (let j = 0; j < gridSize - 1; j += 1) {
            let cur = i * gridSize + j
            g.triangles.push([
                cur, cur + 1, cur + gridSize
            ])
            g.triangles.push([
                cur + 1, cur + gridSize, cur + gridSize + 1
            ])
        }
    }

    // add normals
    for (let i = 0; i < g.attributes[0].length; i += 1) {
        let row = Math.floor(i / gridSize)
        let col = i % gridSize

        let n = (row > 0) ? g.attributes[0][i - gridSize] : g.attributes[0][i]
        let s = (row < gridSize - 1) ? g.attributes[0][i + gridSize] : g.attributes[0][i]
        let w = (col > 0) ? g.attributes[0][i - 1] : g.attributes[0][i]
        let e = (col < gridSize - 1) ? g.attributes[0][i + 1] : g.attributes[0][i]

        let normal = cross(sub(n, s), sub(w, e))
        g.attributes[1].push(normal)
    }

    // add texture coordinate
    for(let i = 0; i < gridSize; i+=1) {
        for (let j = 0; j < gridSize; j+=1) {
            g.attributes[2].push([i/(gridSize-1), j/(gridSize-1)])
        }
    }

    return g
}

/** generate geometry of the terrain */
function generateTerrain(gridSize, faults) {
    const terrain = makeGeom(gridSize, faults)
    window.geom = setupGeomery(terrain)

    // render if not yet started
    if (!animationStarted) {
        animationStarted = true
        requestAnimationFrame(tick)
    }
}

/** Compile, link, set up geometry */
window.addEventListener('load', async (event) => {
    window.gl = document.querySelector('canvas').getContext('webgl2',
    // optional configuration object: see https://developer.mozilla.org/en-US/docs/Web/API/HTMLCanvasElement/getContext
    {antialias: false, depth:true, preserveDrawingBuffer:true}
    )
    let vs = await fetch('vs.glsl').then(res => res.text())
    let nonTextureFs = await fetch('nontexturefs.glsl').then(res => res.text())
    let textureFs = await fetch('texturefs.glsl').then(res => res.text())
    window.nontextureProgram = compileShader(vs, nonTextureFs)
    window.textureProgram = compileShader(vs, textureFs)
    gl.enable(gl.DEPTH_TEST)
    gl.enable(gl.BLEND)
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)

    fillScreen()
    window.addEventListener('resize', fillScreen)

    // document.querySelector('#material').addEventListener('change', event=>{
    //     const material = document.querySelector('#material').value
    //     changeMaterial(material)
    // })

    document.querySelector('#submit').addEventListener('click', async event => {
        const filePath = document.querySelector('#filepath').value
        const imagePath = document.querySelector('#imagepath').value
        console.log(filePath, imagePath)
        // TO DO: generate a new gridsize-by-gridsize grid here, then apply faults to it
        const obj = await readObj(filePath)
        window.geom = setupGeomery(obj)
        // render
        if (!animationStarted) {
            animationStarted = true
            requestAnimationFrame(tick)
        }
        changeMaterial('')
    })
})