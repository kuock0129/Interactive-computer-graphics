const IlliniBlue = new Float32Array([0.075, 0.16, 0.292, 1])
const IlliniOrange = new Float32Array([1, 0.373, 0.02, 1])
const IdentityMatrix = new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1])

// CelestialBodyType is an enum for the type of celestial body
const CelestialBodyType = {
    TETRAHEDRON: 0,  // tetrahedron
    OCTAHEDRON: 1    // octahedron  
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
    gl.clearColor(...IlliniBlue) // f(...[1,2,3]) means f(1,2,3)
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
    gl.useProgram(program)


    ///// set up view matrix
    let view = m4view([5,7,2], [0,0,0], [0,0,1])
    view = m4mul(p, view)
    solarSystem.draw(seconds, view)
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
        window.p = m4perspNegZ(0.1, 32, 1.5, canvas.width, canvas.height)
    }
}





















// Constants for configuration
const DEGREES_TO_RADIANS = Math.PI / 180;
const DEFAULT_MATRIX = new Float32Array([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);

// Configuration object for celestial bodies
const CelestialBodyConfig = {
    SUN: {
        type: 'OCTAHEDRON',
        scale: 1.0,
        rotationPeriod: 2.0, // seconds
    },
    EARTH: {
        type: 'OCTAHEDRON',
        scale: 0.5,
        rotationPeriod: 3.0, // seconds
        orbitConfig: {
            distance: 3.5,
            inclinationDegrees: 23.5,
            revolutionPeriod: 10.0, // seconds
        }
    },
    MOON: {
        type: 'TETRAHEDRON',
        scale: 0.1,
        rotationPeriod: 3.0,
        orbitConfig: {
            distance: 0.7,
            inclinationDegrees: 0,
            revolutionPeriod: 3.0, // Same as rotation for tidal locking
        }
    },
    MARS: {
        type: 'OCTAHEDRON',
        scale: 0.4,
        rotationPeriod: 2.2,
        orbitConfig: {
            distance: 5.6, // 3.5 * 1.6
            inclinationDegrees: 25,
            revolutionPeriod: 19.0, // 10 * 1.9
            rotationZDegrees: 60, // Additional rotation to prevent parallel axis
        }
    },
    PHOBOS: {
        type: 'TETRAHEDRON',
        scale: 0.06,
        rotationPeriod: 1.0,
        orbitConfig: {
            distance: 0.5,
            inclinationDegrees: 0,
            revolutionPeriod: 1.0, // Tidal locked
        }
    },
    DEIMOS: {
        type: 'TETRAHEDRON',
        scale: 0.03,
        rotationPeriod: 2.3,
        orbitConfig: {
            distance: 1.0,
            inclinationDegrees: 0,
            revolutionPeriod: 2.3, // Tidal locked
        }
    }
};

class CelestialBody {
    /**
     * @param {string} name - Name of the celestial body (matches config key)
     * @param {Object} config - Configuration object for the body
     */
    constructor(name, config) {
        this.name = name;
        this.shape = config.type;
        this.scale = config.scale;
        this.rotationFreq = 2.0 * Math.PI / config.rotationPeriod;
        
        this.children = [];
        this.orbitData = new Map();
    }

    /**
     * Adds a child celestial body with its orbital parameters
     * @param {CelestialBody} child - The orbiting celestial body
     * @param {Object} orbitConfig - Orbital configuration parameters
     */
    addSatellite(child, orbitConfig) {
        this.children.push(child);
        
        const orbitData = {
            coordinate: this.calculateOrbitCoordinate(orbitConfig),
            distance: new Float32Array([orbitConfig.distance, 0, 0, 0]),
            revolveFreq: 2.0 * Math.PI / orbitConfig.revolutionPeriod
        };
        
        this.orbitData.set(child, orbitData);
    }

    /**
     * Calculates the orbital coordinate transformation matrix
     * @private
     */
    calculateOrbitCoordinate(orbitConfig) {
        if (!orbitConfig.inclinationDegrees && !orbitConfig.rotationZDegrees) {
            return DEFAULT_MATRIX;
        }

        let matrix = DEFAULT_MATRIX;
        
        if (orbitConfig.rotationZDegrees) {
            matrix = m4mul(
                m4rotZ(orbitConfig.rotationZDegrees * DEGREES_TO_RADIANS),
                matrix
            );
        }
        
        if (orbitConfig.inclinationDegrees) {
            matrix = m4mul(
                m4rotY(orbitConfig.inclinationDegrees * DEGREES_TO_RADIANS),
                matrix
            );
        }
        
        return matrix;
    }

    /**
     * Renders the celestial body and its satellites
     * @param {number} seconds - Current time in seconds
     * @param {Float32Array} transform - Parent transformation matrix
     */
    draw(seconds, transform) {
        const geometry = this.shape === 'TETRAHEDRON' ? tetrahedron : octahedron;
        gl.bindVertexArray(geometry.vao);

        // Apply rotation and scaling
        const localTransform = m4mul(
            m4rotZ(seconds * this.rotationFreq),
            m4scale(this.scale, this.scale, this.scale)
        );
        
        const finalTransform = m4mul(transform, localTransform);
        gl.uniformMatrix4fv(program.uniforms.mv, false, finalTransform);
        gl.drawElements(geometry.mode, geometry.count, geometry.type, 0);

        // Draw satellites
        this.children.forEach(child => {
            const orbit = this.orbitData.get(child);
            const displacement = m4mul(
                m4rotZ(orbit.revolveFreq * seconds),
                orbit.distance
            );
            
            const childTransform = m4mul(
                m4trans(...displacement.slice(0, 3)),
                orbit.coordinate
            );
            
            child.draw(seconds, m4mul(transform, childTransform));
        });
    }
}

// Create solar system
function createSolarSystem() {
    const sun = new CelestialBody('SUN', CelestialBodyConfig.SUN);
    const earth = new CelestialBody('EARTH', CelestialBodyConfig.EARTH);
    const moon = new CelestialBody('MOON', CelestialBodyConfig.MOON);
    const mars = new CelestialBody('MARS', CelestialBodyConfig.MARS);
    const phobos = new CelestialBody('PHOBOS', CelestialBodyConfig.PHOBOS);
    const deimos = new CelestialBody('DEIMOS', CelestialBodyConfig.DEIMOS);

    // Build hierarchy
    earth.addSatellite(moon, CelestialBodyConfig.MOON.orbitConfig);
    mars.addSatellite(phobos, CelestialBodyConfig.PHOBOS.orbitConfig);
    mars.addSatellite(deimos, CelestialBodyConfig.DEIMOS.orbitConfig);
    sun.addSatellite(earth, CelestialBodyConfig.EARTH.orbitConfig);
    sun.addSatellite(mars, CelestialBodyConfig.MARS.orbitConfig);

    return sun;
}

const solarSystem = createSolarSystem();




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

    let tetrahedron = await fetch('tetrahedron.json').then(r=>r.json())
    let octahedron = await fetch('octahedron.json').then(r=>r.json())
    window.tetrahedron = setupGeomery(tetrahedron)
    window.octahedron = setupGeomery(octahedron)

    fillScreen()
    window.addEventListener('resize', fillScreen)
    requestAnimationFrame(tick)
})