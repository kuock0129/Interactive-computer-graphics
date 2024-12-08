// Vector math utilities
const VectorMath = {
    add: (a, b) => a.map((v, i) => v + b[i]),
    sub: (a, b) => a.map((v, i) => v - b[i]),
    mul: (v, scalar) => v.map(x => x * scalar),
    div: (v, scalar) => v.map(x => x / scalar),
    cross: (a, b) => [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    ],
    normalize: (v) => {
        const length = Math.sqrt(v.reduce((sum, x) => sum + x * x, 0))
        return length > 0 ? v.map(x => x / length) : v
    }
}

export class OBJParser {
    constructor() {
        this.vertices = []
        this.textureCoords = []
        this.normals = []
        this.vertexColors = []
        this.triangles = []
        this.vertexMap = new Map()
        
        // WebGL buffer data
        this.bufferData = {
            positions: [],    // vec3 + color
            textures: [],    // vec2
            normals: [],     // vec3
            colors: []       // vec3
        }
    }

    async parseOBJFile(filename) {
        const text = await fetch(filename).then(res => res.text())
        this.parseFileContent(text)
        return {
            triangles: this.triangles,
            attributes: [
                this.bufferData.positions,
                this.bufferData.textures,
                this.bufferData.normals,
                this.bufferData.colors
            ]
        }
    }

    parseFileContent(content) {
        const lines = content.split('\n')
        lines.forEach(line => {
            line = line.trim()
            if (line === '' || line.startsWith('#')) return

            const [keyword, ...args] = line.split(/\s+/)
            const handler = this.keywordHandlers[keyword]
            
            if (handler) {
                handler.call(this, args)
            } else {
                console.warn(`Unknown OBJ keyword: ${keyword}`)
            }
        })

        // Post-processing steps
        this.ensureTextureCoordinates()
        this.calculateNormals()
        this.normalizeGeometry()
    }

    keywordHandlers = {
        v: function(args) {
            const values = args.map(parseFloat)
            this.vertices.push(values.slice(0, 3))
            this.vertexColors.push(values.length === 6 
                ? values.slice(3) 
                : [0.7, 0.7, 0.7]) // default light-gray
        },
        
        vt: function(args) {
            this.textureCoords.push(args.map(parseFloat))
        },
        
        vn: function(args) {
            this.normals.push(args.map(parseFloat))
        },
        
        f: function(args) {
            const vertexIndices = args.map(face => this.processVertex(face))
            this.createTriangles(vertexIndices)
        }
    }

    processVertex(faceVertex) {
        if (this.vertexMap.has(faceVertex)) {
            return this.vertexMap.get(faceVertex)
        }

        const vertexIndex = this.bufferData.positions.length
        const [positionIdx, textureIdx, normalIdx] = faceVertex.split('/').map(idx => 
            idx ? parseInt(idx) - 1 : null
        )

        // Add position and color
        this.bufferData.positions.push(this.vertices[positionIdx])
        this.bufferData.colors.push(this.vertexColors[positionIdx])

        // Add texture coordinates if present
        if (textureIdx !== null) {
            this.bufferData.textures.push(this.textureCoords[textureIdx])
        }

        // Add normal if present
        if (normalIdx !== null) {
            this.bufferData.normals.push(this.normals[normalIdx])
        }

        this.vertexMap.set(faceVertex, vertexIndex)
        return vertexIndex
    }

    createTriangles(vertexIndices) {
        for (let i = 1; i < vertexIndices.length - 1; i++) {
            this.triangles.push([
                vertexIndices[0],
                vertexIndices[i],
                vertexIndices[i + 1]
            ])
        }
    }

    ensureTextureCoordinates() {
        while (this.bufferData.textures.length < this.bufferData.positions.length) {
            this.bufferData.textures.push([0, 0])
        }
    }

    calculateNormals() {
        if (this.bufferData.normals.length === this.bufferData.positions.length) {
            return
        }

        // Initialize normals array
        this.bufferData.normals = Array(this.bufferData.positions.length)
            .fill()
            .map(() => [0, 0, 0])

        // Calculate normals for each triangle
        this.triangles.forEach(([x, y, z]) => {
            const e1 = VectorMath.sub(this.bufferData.positions[y], this.bufferData.positions[x])
            const e2 = VectorMath.sub(this.bufferData.positions[z], this.bufferData.positions[x])
            const normal = VectorMath.cross(e1, e2)

            // Accumulate normals
            ;[x, y, z].forEach(idx => {
                this.bufferData.normals[idx] = VectorMath.add(
                    this.bufferData.normals[idx],
                    normal
                )
            })
        })

        // Normalize all normal vectors
        this.bufferData.normals = this.bufferData.normals.map(VectorMath.normalize)
    }

    normalizeGeometry() {
        const positions = this.bufferData.positions
        const bounds = this.calculateBounds(positions)
        const center = VectorMath.div(
            VectorMath.add(bounds.min, bounds.max),
            2
        )
        const scale = 2 / Math.max(...VectorMath.sub(bounds.max, bounds.min))

        // Transform all vertices
        this.bufferData.positions = positions.map(pos => 
            VectorMath.mul(
                VectorMath.sub(pos, center),
                scale
            )
        )
    }

    calculateBounds(positions) {
        return positions.reduce((bounds, pos) => ({
            min: pos.map((val, i) => Math.min(bounds.min[i], val)),
            max: pos.map((val, i) => Math.max(bounds.max[i], val))
        }), {
            min: [...positions[0]],
            max: [...positions[0]]
        })
    }
}

// Helper function for external use
export async function loadOBJFile(filename) {
    const parser = new OBJParser()
    return await parser.parseOBJFile(filename)
}