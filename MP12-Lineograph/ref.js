/**
 * class for celestial body simulated in this mp. There are some assumption for 
 * my celestial body to simplify the work.
 *  1. The axis for rotation should only be +Z on each body's coordinate system
 *  2. The axis for revolvement should only be X-Y plane on each body's coordinate system
 */
class CelestialBody {
    /**
     * 
     * @param {*} shape enum define in CelestialBodyType
     * @param {*} scale a number for the size of the body
     * @param {*} rotateFreqSeconds the coefficient for its rotation i.e. 2*pi/period
     */
    constructor(shape, scale, rotateFreqSeconds) {
        this.shape = shape // whether tetrahedron or octahedron
        this.scale = scale
        this.rotateFreqSeconds = rotateFreqSeconds

        // for next hierarchy
        this.children = []
        this.childrenCoordinates = []
        this.childrenDistances = []
        this.childrenRevolveFreqSeconds = []
    }

    /** draw current object on screen */
    draw(seconds, transform) {
        let geom = (this.shape === CelestialBodyType.TETRAHEDRON) ? tetrahedron : octahedron
        gl.bindVertexArray(geom.vao)

        // apply rotate, scaling
        let trans = m4mul(m4rotZ(seconds * this.rotateFreqSeconds), m4scale(this.scale, this.scale, this.scale))
        gl.uniformMatrix4fv(program.uniforms.mv, false,  m4mul(transform, trans))
        gl.drawElements(geom.mode, geom.count, geom.type, 0)

        // iterate all children
        for (let i = 0; i < this.children.length; i++) {
            let displacement = m4mul(m4rotZ(this.childrenRevolveFreqSeconds[i] * seconds), this.childrenDistances[i])
            let relativeMat = m4mul(m4trans(...displacement.slice(0, 3)), this.childrenCoordinates[i])
            this.children[i].draw(seconds, m4mul(transform, relativeMat))
        }
    }

    /**
     * add another celestial body under current object
     * @param child                 celestial object
     * @param coordinate            transformation from child coordinate system to parent
     * @param distance              the displacement between the child and the parent
     * @param revolveFreqSeconds    the coefficient for the revolvement i.e. 2*pi/period
    */
    appendChild(child, coordinate, distance, revolveFreqSeconds) {
        this.children.push(child)
        this.childrenCoordinates.push(coordinate)
        this.childrenDistances.push(distance)
        this.childrenRevolveFreqSeconds.push(revolveFreqSeconds)
    }
}

// construct celestial body
let sunRotationFreq = 2.0 * Math.PI / 2.0       // period = 2s
let earthRotationFreq = 2.0 * Math.PI / 1.0     // period = 1s
let moonRotationFreq = 2.0 * Math.PI / 3.0
let marsRotationFreq = earthRotationFreq / 2.2
let phobosRotationFreq = marsRotationFreq * 2.2
let deimosRotationFreq = marsRotationFreq + 0.1

var solarSystem = new CelestialBody(CelestialBodyType.OCTAHEDRON, 1.0, sunRotationFreq)
var earth = new CelestialBody(CelestialBodyType.OCTAHEDRON, 0.5, earthRotationFreq)
var moon = new CelestialBody(CelestialBodyType.TETRAHEDRON, 0.1, moonRotationFreq)
var mars = new CelestialBody(CelestialBodyType.OCTAHEDRON, 0.4, marsRotationFreq)
var phobos = new CelestialBody(CelestialBodyType.TETRAHEDRON, 0.06, phobosRotationFreq)
var deimos = new CelestialBody(CelestialBodyType.TETRAHEDRON, 0.03, deimosRotationFreq)

// relation between sun and earth
let earth2SunCoordinate = m4rotY(Math.PI * 23.5 / 180.0) // 23.5 degree
let earth2SunDistance = new Float32Array([3.5, 0, 0, 0])
let earthRevolveFreq = 2.0 * Math.PI / 10.0

// relation between earth and moon
let moon2EarthCoordinate = IdentityMatrix
let moon2EarthDistance = new Float32Array([0.7, 0, 0, 0])
let moonRevolveFreq = moonRotationFreq // making it always faces the side to the earth

// relation between mars and sun
// apply rotationZ to prevent having rotation axis parallel to the earth
let mars2SunCoordinate = m4mul(m4rotZ(Math.PI * 60.0 / 180.0), m4rotY(Math.PI * 25 / 180.0)) // 25 degree
let mars2SunDistance = mul(earth2SunDistance, 1.6)
let marsRevolveFreq = earthRevolveFreq / 1.9

// relation between mars and phobos
let phobos2MarsCoordinate = IdentityMatrix
let phobos2MarsDistance = new Float32Array([0.5, 0, 0, 0])
let phobosRevolveFreq = phobosRotationFreq // making it always faces the side to the mars

// relation between mars and deimos
let deimos2MarsCoordinate = IdentityMatrix
let deimos2MarsDistance = mul(phobos2MarsDistance, 2.0)
let deimosRevolveFreq = deimosRotationFreq // making it always faces the side to the mars

// building hierarchy
earth.appendChild(moon, moon2EarthCoordinate, moon2EarthDistance, moonRevolveFreq)
mars.appendChild(phobos, phobos2MarsCoordinate, phobos2MarsDistance, phobosRevolveFreq)
mars.appendChild(deimos, deimos2MarsCoordinate, deimos2MarsDistance, deimosRevolveFreq)

solarSystem.appendChild(earth, earth2SunCoordinate, earth2SunDistance, earthRevolveFreq)
solarSystem.appendChild(mars, mars2SunCoordinate, mars2SunDistance, marsRevolveFreq)





