/** constants */
const DIFFUSION_COLOR = new Float32Array([1, 0.373, 0.02, 1])
const IlliniBlue = new Float32Array([0.075, 0.16, 0.292, 1])
const IdentityMatrix = new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1])
var animationStarted = false


class HalfEdge {
    constructor(v, next, twin, face) {
        this.v = v         // vertex index
        this.next = next   // next half-edge in the face
        this.twin = twin   // opposing half-edge
        this.face = face   // face this half-edge belongs to
        this.used = false  // helper for processing
    }
}

class Mesh {
    constructor() {
        this.vertices = []  // array of [x,y,z] positions
        this.faces = []     // array of arrays of vertex indices
        this.halfEdges = [] // array of HalfEdge objects
        this.faceEdges = new Map() // map face index to first half-edge
        this.vertexEdges = new Map() // map vertex index to one of its half-edges
    }

    buildHalfEdgeStructure() {
        const edgeMap = new Map() // maps "v1,v2" to half-edge

        // Create half-edges for each face
        this.faces.forEach((faceVerts, faceIdx) => {
            const faceEdges = []
            
            // Create edges for this face
            for(let i = 0; i < faceVerts.length; i++) {
                const v1 = faceVerts[i]
                const v2 = faceVerts[(i + 1) % faceVerts.length]
                const edge = new HalfEdge(v1, null, null, faceIdx)
                faceEdges.push(edge)
                this.halfEdges.push(edge)
                
                this.vertexEdges.set(v1, edge)
    
                // Check for existing twin
                const twinKey = `${v2},${v1}`
                if(edgeMap.has(twinKey)) {
                    const twin = edgeMap.get(twinKey)
                    edge.twin = twin
                    twin.twin = edge
                } else {
                    edgeMap.set(`${v1},${v2}`, edge)
                }
            }
    
            // Connect next pointers
            for(let i = 0; i < faceEdges.length; i++) {
                faceEdges[i].next = faceEdges[(i + 1) % faceEdges.length]
            }
    
            this.faceEdges.set(faceIdx, faceEdges[0])
        })
    
        // Second pass: Create virtual twins for boundary edges
        this.halfEdges.forEach(edge => {
            if (!edge.twin) {
                const virtualTwin = new HalfEdge(edge.next.v, null, edge, null)
                edge.twin = virtualTwin
                this.halfEdges.push(virtualTwin)
            }
        })
    }

    validateMesh() {
        // Check for missing twins
        let hasError = false
        this.halfEdges.forEach(edge => {
            if (!edge.twin && !this.isBoundaryEdge(edge)) {
                console.error(`Missing twin edge for vertex pair ${edge.v}->${edge.next.v}`)
                hasError = true
            }
        })

        // Check face connectivity
        this.faces.forEach((face, faceIdx) => {
            const startEdge = this.faceEdges.get(faceIdx)
            let edge = startEdge
            let vertCount = 0
            do {
                vertCount++
                edge = edge.next
                if (vertCount > face.length) {
                    console.error(`Invalid face connectivity in face ${faceIdx}`)
                    hasError = true
                    break
                }
            } while (edge !== startEdge)
        })

        if (hasError) {
            throw new Error("Mesh validation failed")
        }
    }

    isBoundaryEdge(edge) {
        // Implementation to detect if an edge is on mesh boundary
        let edgeCount = 0
        this.halfEdges.forEach(e => {
            if ((e.v === edge.next.v && e.next.v === edge.v) || 
                (e.v === edge.v && e.next.v === edge.next.v)) {
                edgeCount++
            }
        })
        return edgeCount === 1
    }

    computeFaceNormal(mesh, faceIdx) {
        const face = mesh.faces[faceIdx];
        if (face.length < 3) return [0, 1, 0];
        
        // Get three vertices of the face
        const v1 = mesh.vertices[face[0]];
        const v2 = mesh.vertices[face[1]];
        const v3 = mesh.vertices[face[2]];
        
        // Compute face normal using cross product
        const edge1 = [
            v2[0] - v1[0],
            v2[1] - v1[1],
            v2[2] - v1[2]
        ];
        const edge2 = [
            v3[0] - v1[0],
            v3[1] - v1[1],
            v3[2] - v1[2]
        ];
        
        // Cross product
        const normal = [
            edge1[1] * edge2[2] - edge1[2] * edge2[1],
            edge1[2] * edge2[0] - edge1[0] * edge2[2],
            edge1[0] * edge2[1] - edge1[1] * edge2[0]
        ];
        
        // Normalize
        const len = Math.sqrt(
            normal[0] * normal[0] + 
            normal[1] * normal[1] + 
            normal[2] * normal[2]
        );
        
        if (len > 0.000001) {
            normal[0] /= len;
            normal[1] /= len;
            normal[2] /= len;
        }
        
        return normal;
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
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
    gl.useProgram(program)

    gl.bindVertexArray(geom.vao)

    gl.uniform4fv(program.uniforms.color, DIFFUSION_COLOR)

    const eyePosition = [2.5, 2.5, 0.9]
    let m = m4rotZ(0.6 * seconds) // rotating camera
    let v = m4view(eyePosition, [0,0,0], [0,0,1])
    let mv = m4mul(v, m)

    // light
    let ld = m4mul(mv, normalize([1, -2, 8,0])).slice(0, 3)
    let h = normalize(add(ld, [0,0,1])) // after view matrix, eye direction becomes [0,0,1]

    let ld2 = m4mul(mv, normalize([0,0,-1,0])).slice(0, 3)
    let h2 = normalize(add(ld2, [0,0,1]))

    gl.uniform3fv(program.uniforms.lightdir, ld)
    gl.uniform3fv(program.uniforms.lightcolor, [1,1,1])
    gl.uniform3fv(program.uniforms.halfway, h)

    gl.uniform3fv(program.uniforms.lightdir2, ld2)
    gl.uniform3fv(program.uniforms.lightcolor2, [1,1,1])
    gl.uniform3fv(program.uniforms.halfway2, h2)

    gl.uniformMatrix4fv(program.uniforms.mv, false, mv)
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
        window.p = m4perspNegZ(0.1, 10, 1, canvas.width, canvas.height)
    }
}















function parseFaceVertex(str) {
    return parseInt(str.split('/')[0]) - 1
}

function parseOBJ(text) {
    const mesh = new Mesh()
    const lines = text.split('\n')
    
    for(const line of lines) {
        const tokens = line.trim().split(/\s+/)
        if(tokens[0] === 'v') {
            mesh.vertices.push([
                parseFloat(tokens[1]),
                parseFloat(tokens[2]),
                parseFloat(tokens[3])
            ])
        } else if(tokens[0] === 'f') {
            mesh.faces.push(tokens.slice(1).map(parseFaceVertex))
        }
    }
    
    mesh.buildHalfEdgeStructure()
    return mesh
}

function catmullClarkSubdivide(mesh) {
    const newMesh = new Mesh();
    const vertexPoints = new Map();
    const edgePoints = new Map();
    const facePoints = new Map();
    
    try {
        // Reset used flags
        mesh.halfEdges.forEach(edge => edge.used = false);
        
        // Step 1: Compute face points - average of all vertices in face
        mesh.faces.forEach((faceVerts, faceIdx) => {
            if (!faceVerts || faceVerts.length < 3) return;
            
            const avgPos = [0, 0, 0];
            let validVerts = 0;
            
            for (const vIdx of faceVerts) {
                const vert = mesh.vertices[vIdx];
                if (!vert) continue;
                avgPos[0] += vert[0];
                avgPos[1] += vert[1];
                avgPos[2] += vert[2];
                validVerts++;
            }
            
            if (validVerts > 0) {
                avgPos[0] /= validVerts;
                avgPos[1] /= validVerts;
                avgPos[2] /= validVerts;
                const fpIdx = newMesh.vertices.length;
                newMesh.vertices.push(avgPos);
                facePoints.set(faceIdx, fpIdx);
            }
        });

        // Step 2: Compute edge points
        mesh.halfEdges.forEach(edge => {
            if (edge.used || !edge.next || edge.face === null) return;
            
            const v1 = mesh.vertices[edge.v];
            const v2 = mesh.vertices[edge.next.v];
            if (!v1 || !v2) return;
            
            const f1 = facePoints.get(edge.face);
            const f2 = edge.twin && edge.twin.face !== null ? facePoints.get(edge.twin.face) : f1;
            
            if (f1 === undefined) return;
            
            const f1Pos = newMesh.vertices[f1];
            const f2Pos = f2 !== undefined ? newMesh.vertices[f2] : f1Pos;
            
            // Edge point is average of: two original vertices and two face points
            const edgePoint = [
                (v1[0] + v2[0] + f1Pos[0] + f2Pos[0]) / 4,
                (v1[1] + v2[1] + f1Pos[1] + f2Pos[1]) / 4,
                (v1[2] + v2[2] + f1Pos[2] + f2Pos[2]) / 4
            ];
            
            const epIdx = newMesh.vertices.length;
            newMesh.vertices.push(edgePoint);
            
            const key1 = `${edge.v},${edge.next.v}`;
            const key2 = `${edge.next.v},${edge.v}`;
            edgePoints.set(key1, epIdx);
            edgePoints.set(key2, epIdx);
            
            edge.used = true;
            if (edge.twin) edge.twin.used = true;
        });

        // Step 3: Update original vertices
        mesh.vertices.forEach((pos, vIdx) => {
            if (!pos) return;
            
            let F = [0, 0, 0];  // Average of face points
            let R = [0, 0, 0];  // Average of edge midpoints
            let n = 0;          // Number of adjacent faces
            let m = 0;          // Number of adjacent edges
            
            // Find all adjacent faces and edges through half-edges
            mesh.halfEdges.forEach(edge => {
                if (edge.v === vIdx && edge.face !== null) {
                    // Add face point
                    const facePoint = newMesh.vertices[facePoints.get(edge.face)];
                    if (facePoint) {
                        F[0] += facePoint[0];
                        F[1] += facePoint[1];
                        F[2] += facePoint[2];
                        n++;
                    }
                    
                    // Add edge midpoint
                    const nextVert = mesh.vertices[edge.next.v];
                    if (nextVert) {
                        const midpoint = [
                            (pos[0] + nextVert[0]) / 2,
                            (pos[1] + nextVert[1]) / 2,
                            (pos[2] + nextVert[2]) / 2
                        ];
                        R[0] += midpoint[0];
                        R[1] += midpoint[1];
                        R[2] += midpoint[2];
                        m++;
                    }
                }
            });
            
            if (n > 0 && m > 0) {
                // Apply Catmull-Clark vertex point formula
                const newPos = [
                    (F[0]/n + 2*R[0]/m + (n-3)*pos[0]) / n,
                    (F[1]/n + 2*R[1]/m + (n-3)*pos[1]) / n,
                    (F[2]/n + 2*R[2]/m + (n-3)*pos[2]) / n
                ];
                const vpIdx = newMesh.vertices.length;
                newMesh.vertices.push(newPos);
                vertexPoints.set(vIdx, vpIdx);
            }
        });

        // Step 4: Create new faces with proper connectivity
        mesh.faces.forEach((face, faceIdx) => {
            if (!face || face.length < 3) return;  // Accept any face with 3 or more vertices
            
            const facePointIdx = facePoints.get(faceIdx);
            if (facePointIdx === undefined) return;
            
            // For each edge in the original face
            for (let i = 0; i < face.length; i++) {
                const v1 = face[i];
                const v2 = face[(i + 1) % face.length];
                
                const vp1 = vertexPoints.get(v1);
                const vp2 = vertexPoints.get(v2);
                const ep = edgePoints.get(`${v1},${v2}`);
                
                if (vp1 !== undefined && vp2 !== undefined && ep !== undefined) {
                    // Create new quad for each edge
                    const prevEdgePoint = edgePoints.get(`${v1},${face[(i-1+face.length)%face.length]}`);
                    if (prevEdgePoint !== undefined) {
                        newMesh.faces.push([vp1, ep, facePointIdx, prevEdgePoint]);
                    }
                }
            }
        });

        newMesh.buildHalfEdgeStructure();
        return newMesh;
    } catch (error) {
        console.error('Subdivision failed:', error);
        return mesh;
    }
}

function meshToGeometry(mesh) {
    const positions = [];
    const normals = [];
    const indices = [];
    
    function computeFaceNormal(face) {
        if (face.length < 3) return [0, 1, 0];
        
        const v1 = mesh.vertices[face[0]];
        const v2 = mesh.vertices[face[1]];
        const v3 = mesh.vertices[face[2]];
        
        if (!v1 || !v2 || !v3) return [0, 1, 0];
        
        const edge1 = [
            v2[0] - v1[0],
            v2[1] - v1[1],
            v2[2] - v1[2]
        ];
        const edge2 = [
            v3[0] - v1[0],
            v3[1] - v1[1],
            v3[2] - v1[2]
        ];
        
        const normal = [
            edge1[1] * edge2[2] - edge1[2] * edge2[1],
            edge1[2] * edge2[0] - edge1[0] * edge2[2],
            edge1[0] * edge2[1] - edge1[1] * edge2[0]
        ];
        
        const len = Math.sqrt(
            normal[0] * normal[0] + 
            normal[1] * normal[1] + 
            normal[2] * normal[2]
        );
        
        if (len > 0.000001) {
            normal[0] /= len;
            normal[1] /= len;
            normal[2] /= len;
        }
        
        return normal;
    }
    
    // Handle both triangular and quad faces
    mesh.faces.forEach((face, faceIdx) => {
        if (face.length < 3) return;
            
        const faceNormal = computeFaceNormal(face);
        
        // Verify all vertices are valid
        const faceVertices = face.map(vIdx => mesh.vertices[vIdx])
                                .filter(v => v !== null && v !== undefined);
        
        if (faceVertices.length < 3) return;
        
        // Add vertices and normals
        faceVertices.forEach(vertex => {
            positions.push(vertex);
            normals.push(faceNormal);
        });
        
        // Triangulate the face using fan triangulation
        const baseIndex = positions.length - faceVertices.length;
        for (let i = 1; i < faceVertices.length - 1; i++) {
            indices.push([
                baseIndex,
                baseIndex + i,
                baseIndex + i + 1
            ]);
        }
    });
    
    // Check if we have valid geometry
    if (positions.length === 0 || normals.length === 0 || indices.length === 0) {
        // Return a minimal valid geometry if no valid faces were found
        return {
            "triangles": [[0, 1, 2]],
            "attributes": [
                [[0,0,0], [1,0,0], [0,1,0]],  // positions
                [[0,0,1], [0,0,1], [0,0,1]]   // normals
            ]
        };
    }
    
    return {
        "triangles": indices,
        "attributes": [positions, normals]
    };
}











// /** generate geometry of the sphere */
// function generateGeom() {
//     const levels = Number(document.querySelector('#levels').value) || 0
//     const shape = makeIcosahedron(levels)
//     window.geom = setupGeomery(shape)
    
//     // render
//     if (!animationStarted) {
//         animationStarted = true
//         requestAnimationFrame(tick)
//     }
// }

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
    gl.enable(gl.BLEND)
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)

    fillScreen()
    window.addEventListener('resize', fillScreen)

    document.getElementById('file').addEventListener('change', async event => {
        if (event.target.files.length != 1) {
            console.debug("No file selected")
            return
        }
        const txt = await event.target.files[0].text()
        if (!/^v .*^v .*^v .*^f /gms.test(txt)) {
            console.debug("File not a valid OBJ file")
            return
        }
        
        window.currentMesh = parseOBJ(txt)
        const level = Math.min(document.getElementById('levels').value, 5) | 0
        let subdividedMesh = currentMesh
        for(let i = 0; i < level; i++) {
            subdividedMesh = catmullClarkSubdivide(subdividedMesh)
        }
        window.geom = setupGeomery(meshToGeometry(subdividedMesh))
        
        if (!animationStarted) {
            animationStarted = true
            requestAnimationFrame(tick)
        }
    })

    document.getElementById('levels').addEventListener('change', event => {
        if (!window.currentMesh) return
        
        const level = Math.min(document.getElementById('levels').value, 5) | 0
        let subdividedMesh = currentMesh
        for(let i = 0; i < level; i++) {
            subdividedMesh = catmullClarkSubdivide(subdividedMesh)
        }
        window.geom = setupGeomery(meshToGeometry(subdividedMesh))
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