/**
 * Compiles two shaders, links them together, looks up their uniform locations,
 * and returns the result. Reports any shader errors to the console.
 *
 * @param {string} vs_source - the source code of the vertex shader
 * @param {string} fs_source - the source code of the fragment shader
 * @return {WebGLProgram} the compiled and linked program
 */



// /**
//  * Fetches, reads, and compiles GLSL; sets two global variables; and begins
//  * the animation
//  */
// async function setup() {
//     window.gl = document.querySelector('canvas').getContext('webgl2')
//     const vs = await fetch('vertexShader.glsl').then(res => res.text())
//     const fs = await fetch('fragmentShader.glsl').then(res => res.text())
//     window.program = compile(vs,fs)
//     tick(0) // <- ensure this function is called only once, at the end of setup
// }

function compile(vs_source, fs_source) {
    
    // compile the vertex shader
    const vs = gl.createShader(gl.VERTEX_SHADER)
    gl.shaderSource(vs, vs_source)
    gl.compileShader(vs)
    if (!gl.getShaderParameter(vs, gl.COMPILE_STATUS)) {
        console.error(gl.getShaderInfoLog(vs))
        throw Error("Vertex shader compilation failed")
    }

    // compile the fragment shader
    const fs = gl.createShader(gl.FRAGMENT_SHADER)
    gl.shaderSource(fs, fs_source)
    gl.compileShader(fs)
    if (!gl.getShaderParameter(fs, gl.COMPILE_STATUS)) {
        console.error(gl.getShaderInfoLog(fs))
        throw Error("Fragment shader compilation failed")
    }
    
    // link the two shaders into a program
    const program = gl.createProgram()
    gl.attachShader(program, vs)
    gl.attachShader(program, fs)
    gl.linkProgram(program)
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
        console.error(gl.getProgramInfoLog(program))
        throw Error("Linking failed")
    }
    
    // loop through all uniforms in the shader source code
    // get their locations and store them in the GLSL program object for later use
    const uniforms = {}
    for(let i=0; i<gl.getProgramParameter(program, gl.ACTIVE_UNIFORMS); i+=1) {
        let info = gl.getActiveUniform(program, i)
        uniforms[info.name] = gl.getUniformLocation(program, info.name)
    }
    program.uniforms = uniforms

    return program
}

/**
 * Draw every ticks
 * @param {*} milliseconds 
 */
function tick(milliseconds) {
    draw(milliseconds)
    requestAnimationFrame(tick) // asks browser to call tick before next frame
}

/**
 * Draw shapes given the information from geom
 * @param {*} milliseconds 
 */
function draw(milliseconds) {
    gl.clear(gl.COLOR_BUFFER_BIT) 
    gl.useProgram(program)
    
    // values that do not vary between vertexes or fragments are called "uniforms"
    gl.uniform1f(program.uniforms.seconds, milliseconds/1000)
    
    gl.bindVertexArray(geom.vao)
    gl.drawElements(geom.mode, geom.count, geom.type, 0)
}




/**
 * Read from the json file and parse all the shapes
 * @param {*} geom 
 * @returns {Object} 
 */
function setupGeomery(geom) {
    // Create a new Vertex Array Object (VAO) to store all the state for this geometry
    var triangleArray = gl.createVertexArray()
    gl.bindVertexArray(triangleArray) // Bind the VAO for configuration


    var number = geom.attributes.length;

    // Iterate over each attribute set to create and bind buffers
    for(let i=0; i<number; i+=1) {
        let buff = gl.createBuffer() // Create a buffer for the current attribute
        gl.bindBuffer(gl.ARRAY_BUFFER, buff) // Bind the buffer to the ARRAY_BUFFER target

        // Convert the attribute data into a Float32Array and load it into the buffer
        let float32 = new Float32Array(geom.attributes[i].flat())
        gl.bufferData(gl.ARRAY_BUFFER, float32, gl.STATIC_DRAW)
        
        // Specify how the data will be fed to the shader
        // `geom.attributes[i][0].length` defines the number of components per vertex (e.g., 3 for positions)
        gl.vertexAttribPointer(i, geom.attributes[i][0].length, gl.FLOAT, false, 0, 0)
        gl.enableVertexAttribArray(i) // Enable this attribute for rendering
    }

    // Create an element array buffer for triangle indices
    var indices = new Uint16Array(geom.triangles.flat()); // Flatten the triangle indices array
    var indexBuffer = gl.createBuffer(); // Create a new buffer for the indices
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer); // Bind it as an ELEMENT_ARRAY_BUFFER
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indices, gl.STATIC_DRAW); // Load index data into the buffer

    return {
        mode: gl.TRIANGLES,          // Rendering mode (TRIANGLES means each set of 3 vertices forms a triangle)
        count: indices.length,       // Total number of indices
        type: gl.UNSIGNED_SHORT,     // Data type of the indices (16-bit integers)
        vao: triangleArray           // The Vertex Array Object containing the configuration
    }
}





window.addEventListener('load', async (event) => {
    window.gl = document.querySelector('canvas').getContext('webgl2')
    let fs = await fetch('fragmentShader.glsl').then(res => res.text())
    let vs = await fetch('vertexShader.glsl').then(res => res.text())
    window.program = compile(vs,fs)
    let data = await fetch('UIUC_icon.json').then(r=>r.json())
    window.geom = setupGeomery(data)
    requestAnimationFrame(tick) // asks browser to call tick before first frame
})