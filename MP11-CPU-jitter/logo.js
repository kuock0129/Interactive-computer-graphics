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






// /**
//  * Draw shapes given the information from geom
//  * @param {*} milliseconds 
//  */
// function draw(milliseconds) {
//     gl.clear(gl.COLOR_BUFFER_BIT);
//     gl.useProgram(program);
    
//     // Update vertex positions
//     const jitteredVertices = new Float32Array(originalVertices.length);
//     for(let i = 0; i < originalVertices.length; i += 2) {
//         // Add cumulative random jitter to each vertex
//         const xJitter = (Math.random() - 0.5) * 0.02;
//         const yJitter = (Math.random() - 0.5) * 0.02;
        
//         jitteredVertices[i] = originalVertices[i] + xJitter;
//         jitteredVertices[i+1] = originalVertices[i+1] + yJitter;
//     }
    
//     // Update the vertex buffer with new positions
//     gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
//     gl.bufferData(gl.ARRAY_BUFFER, jitteredVertices, gl.DYNAMIC_DRAW);
    
//     gl.bindVertexArray(geom.vao);
//     gl.drawElements(geom.mode, geom.count, geom.type, 0);
// }


// Global variables to store the current jittered positions
let currentVertices;
let vertexBuffer;

function setupGeomery(geom) {
    var triangleArray = gl.createVertexArray();
    gl.bindVertexArray(triangleArray);

    // 1. Store vertex buffer globally
    vertexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
    
    // Initialize current vertices from original positions
    currentVertices = new Float32Array(geom.attributes[0].flat());
    
    // 2. Use DYNAMIC_DRAW for the vertex buffer
    gl.bufferData(gl.ARRAY_BUFFER, currentVertices, gl.DYNAMIC_DRAW);
    gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(0);

    // Color buffer remains static
    var colorBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, colorBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(geom.attributes[1].flat()), gl.STATIC_DRAW);
    gl.vertexAttribPointer(1, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(1);

    var indices = new Uint16Array(geom.triangles.flat());
    var indexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indices, gl.STATIC_DRAW);

    return {
        mode: gl.TRIANGLES,
        count: indices.length,
        type: gl.UNSIGNED_SHORT,
        vao: triangleArray
    };
}


function draw(milliseconds) {
    gl.clear(gl.COLOR_BUFFER_BIT);
    gl.useProgram(program);
    
    // 3. Update vertex positions with cumulative random changes
    for(let i = 0; i < currentVertices.length; i += 2) {
        // Add small random changes to current positions
        currentVertices[i] += (Math.random() - 0.5) * 0.1;     // x coordinate
        currentVertices[i + 1] += (Math.random() - 0.5) * 0.1; // y coordinate
    }
    
    // 4. Update buffer with new positions
    gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, currentVertices, gl.DYNAMIC_DRAW);
    
    gl.uniform1f(program.uniforms.seconds, milliseconds/1000);
    gl.bindVertexArray(geom.vao);
    gl.drawElements(geom.mode, geom.count, geom.type, 0);
}


window.addEventListener('load', async (event) => {
    window.gl = document.querySelector('canvas').getContext('webgl2')
    let fs = await fetch('fs.glsl').then(res => res.text())
    let vs = await fetch('vs.glsl').then(res => res.text())
    window.program = compile(vs,fs)
    let data = await fetch('UIUC_icon.json').then(r=>r.json())
    window.geom = setupGeomery(data)
    requestAnimationFrame(tick) // asks browser to call tick before first frame
})