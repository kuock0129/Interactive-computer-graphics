#include <fstream>
#include <iostream>
#include <functional>   
#include <algorithm>    
#include <vector>
#include <float.h>      
#include <math.h>       
#include <assert.h>     
#include "uselibpng.h"
using namespace std;

bool enable_hyp = false;
struct Position2 { int x; int y; };
struct Color4 { unsigned char r; unsigned char g; unsigned char b; unsigned char a; };

class Vertex {
public:
    // Direct member variables with underscore prefix
    double _x{0}, _y{0}, _z{0}, _w{1.0}; // Position
    double _r{0}, _g{0}, _b{0}, _a{1.0}; // Color
    double _s{0}, _t{0};                 // Texture coordinates

    // Accessor methods
    double x() const { return _x; }
    double y() const { return _y; }
    double z() const { return _z; }
    double w() const { return _w; }
    double r() const { return _r; }
    double g() const { return _g; }
    double b() const { return _b; }
    double a() const { return _a; }
    double s() const { return _s; }
    double t() const { return _t; }

    // Accessor methods using references
    double& operator[](size_t idx) {
        switch(idx) {
            case 0: return _x;
            case 1: return _y;
            case 2: return _z;
            case 3: return _w;
            case 4: return _r;
            case 5: return _g;
            case 6: return _b;
            case 7: return _a;
            case 8: return _s;
            case 9: return _t;
            default: throw std::out_of_range("Invalid index");
        }
    }

    const double& operator[](size_t idx) const {
        switch(idx) {
            case 0: return _x;
            case 1: return _y;
            case 2: return _z;
            case 3: return _w;
            case 4: return _r;
            case 5: return _g;
            case 6: return _b;
            case 7: return _a;
            case 8: return _s;
            case 9: return _t;
            default: throw std::out_of_range("Invalid index");
        }
    }

    Vertex operator-(const Vertex &v) const {
        Vertex ret;
        ret._x = _x - v._x;
        ret._y = _y - v._y;
        ret._z = _z - v._z;
        ret._w = _w - v._w;
        ret._r = _r - v._r;
        ret._g = _g - v._g;
        ret._b = _b - v._b;
        ret._a = _a - v._a;
        ret._s = _s - v._s;
        ret._t = _t - v._t;
        return ret;
    }

    Vertex operator+(const Vertex &v) const {
        Vertex ret;
        ret._x = _x + v._x;
        ret._y = _y + v._y;
        ret._z = _z + v._z;
        ret._w = _w + v._w;
        ret._r = _r + v._r;
        ret._g = _g + v._g;
        ret._b = _b + v._b;
        ret._a = _a + v._a;
        ret._s = _s + v._s;
        ret._t = _t + v._t;
        return ret;
    }

    Vertex operator/(const double div) const {
        if (div == 0) return Vertex();
        Vertex ret;
        ret._x = _x / div;
        ret._y = _y / div;
        ret._z = _z / div;
        ret._w = _w / div;
        ret._r = _r / div;
        ret._g = _g / div;
        ret._b = _b / div;
        ret._a = _a / div;
        ret._s = _s / div;
        ret._t = _t / div;
        return ret;
    }

    Vertex operator*(const double mul) const {
        Vertex ret;
        ret._x = _x * mul;
        ret._y = _y * mul;
        ret._z = _z * mul;
        ret._w = _w * mul;
        ret._r = _r * mul;
        ret._g = _g * mul;
        ret._b = _b * mul;
        ret._a = _a * mul;
        ret._s = _s * mul;
        ret._t = _t * mul;
        return ret;
    }

    Vertex& operator+=(const Vertex &v) {
        _x += v._x;
        _y += v._y;
        _z += v._z;
        _w += v._w;
        _r += v._r;
        _g += v._g;
        _b += v._b;
        _a += v._a;
        _s += v._s;
        _t += v._t;
        return *this;
    }

    void SetPos(const vector<double> &position) {
        if (position.size() > 0) _x = position[0];
        if (position.size() > 1) _y = position[1];
        if (position.size() > 2) _z = position[2];
        if (position.size() > 3) _w = position[3];
    }

    void SetColor(const vector<double> &color) {
        if (color.size() > 0) _r = color[0];
        if (color.size() > 1) _g = color[1];
        if (color.size() > 2) _b = color[2];
        if (color.size() > 3) _a = color[3];
    }

    void SetData(const vector<double> &data, int offset) {
        switch(offset) {
            case 0: // Position
                SetPos(data);
                break;
            case 4: // Color
                SetColor(data);
                break;
            case 8: // Texture coordinates
                if (data.size() > 0) _s = data[0];
                if (data.size() > 1) _t = data[1];
                break;
        }
    }

    Vertex normalize(const int width, const int height, bool enable_hyp) const {
        Vertex ret;
        double w_val = this->_w;
        
        // Position normalization
        ret._x = (_x / w_val + 1) * width / 2;
        ret._y = (_y / w_val + 1) * height / 2;
        ret._z = _z / w_val;
        ret._w = 1.0 / w_val;
        
        // Color and texture coordinates
        if (enable_hyp) {
            ret._r = _r / w_val;
            ret._g = _g / w_val;
            ret._b = _b / w_val;
            ret._a = _a / w_val;
            ret._s = _s / w_val;
            ret._t = _t / w_val;
        } else {
            ret._r = _r;
            ret._g = _g;
            ret._b = _b;
            ret._a = _a;
            ret._s = _s;
            ret._t = _t;
        }
        
        return ret;
    }

    Vertex undo(bool enable_hyp) const {
        Vertex ret;
        double w_val = this->_w;
        
        // Position always gets divided
        ret._x = _x;
        ret._y = _y;
        ret._z = _z;
        ret._w = 1.0 / w_val;
        
        // Color and texture coordinates only if enable_hyp is true
        if (enable_hyp) {
            ret._r = _r / w_val;
            ret._g = _g / w_val;
            ret._b = _b / w_val;
            ret._a = _a / w_val;
            ret._s = _s / w_val;
            ret._t = _t / w_val;
        } else {
            ret._r = _r;
            ret._g = _g;
            ret._b = _b;
            ret._a = _a;
            ret._s = _s;
            ret._t = _t;
        }
        
        return ret;
    }

    friend auto operator<<(std::ostream& os, Vertex const& v) -> std::ostream& { 
        os << "Vertex:\n  pos: " << v._x << ", " << v._y << ", " << v._z << ", " << v._w << '\n';
        os << "  st: " << v._s << ", " << v._t << '\n';
        return os;
    }

private:
    vector<double> _data;
};




class Matrix4 {
public:
    Matrix4(const vector<double> &vec) {
        _mat = vec;
    }
    vector<double> mul(const Vertex &vertex) const {
        vector<double> ret(4, 0.0);
        for (int i = 0; i < 4; i++) {
            double d = 0;
            for (int j = 0; j < 4; j++) {
                d += get(j, i) * vertex[j];
            }
            ret[i] = d;
        }
        return ret;
    }

private:
    double get(int i, int j) const {
        return _mat[i * 4 + j];
    }
    vector<double> _mat;
};





Matrix4 *matrix = nullptr;

class Texture {
public:
    Texture(const string &src_file) {
        _img = load_image(src_file.c_str());
        if (_img) {
            wid = _img->width;
            hei = _img->height;
        }
    }
    
    ~Texture() {
        if (_img)
            free_image(_img);
    }
    
    unsigned GetWidth() const { return wid; }
    unsigned GetHeight() const { return hei; }
    
    void sample(double x, double y,
            unsigned char &r, unsigned char &g,
            unsigned char &b, unsigned char &a) const {
        x = takeDecimal(x), y = takeDecimal(y);
        double u = x * wid, v = y * hei;
        int u0 = clamp(static_cast<int>(u + 0.5), wid - 1, 0);
        int v0 = clamp(static_cast<int>(v + 0.5), hei - 1, 0);
        // Change this line:
        pixel_t *pixel = &(_img->rgba[v0 * wid + u0]);
        r = pixel->r;
        g = pixel->g;
        b = pixel->b;
        a = pixel->a;
    }


    private:
        image_t *_img = nullptr;
        unsigned wid = 0, hei = 0;

        double takeDecimal(double num) const {
            double intpart;
            num = modf(num, &intpart);
            return (num < 0) ? num + 1.0 : num;
        }
        
        int clamp(int num, int ub, int lb = 0) const {
            return max(0, min(ub, num));
        }
    };

class Picture {
public:
    ~Picture() {
        if (_img)
            free_image(_img);
    }

    void setup(const string &name, const int width, const int height) {
        _name = name;
        _width = width;
        _height = height;
        if (_img)
            free_image(_img);
        _img = new_image(width, height);
        _zbuff.resize(width*height, DBL_MAX);
    }

    void SetDepth() { _useDepth = true; }
    void SetsRBG() { _usesRGB = true; }

    const string &GetName() const { return _name; }
    int GetWidth() const { return _width; }
    int GetHeight() const { return _height; }

    // In the Picture::render method, change:
    int render(const Vertex &v, Texture *texture) {
        int x = v.x(), y = v.y();

        if (x < 0 || x >= _width || y < 0 || y >= _height) {
            return -1;
        }
        int pos_idx = y * _width + x;
        if (_useDepth && _zbuff[pos_idx] < v.z())
            return 0;

        double r = v.r(), g = v.g(), b = v.b(), a = v.a();
        double r_d, g_d, b_d, a_d;
        
        // check texture
        if (texture) {
            r_d = r, g_d = g, b_d = b, a_d = a;
            unsigned char rc, gc, bc, ac;
            texture->sample(v.s(), v.t(), rc, gc, bc, ac);
            auto convert = (_usesRGB) ? sRGB_2double : linear_2double;
            r = convert(rc);
            g = convert(gc);
            b = convert(bc);
            a = linear_2double(ac);
        } else {
            // Change this line:
            pixel_t *pixel = &(_img->rgba[y * _width + x]);
            auto convert = (_usesRGB) ? sRGB_2double : linear_2double;
            r_d = convert(pixel->r);
            g_d = convert(pixel->g);
            b_d = convert(pixel->b);
            a_d = linear_2double(pixel->a);
        }

        // alpha blending
        if (a < 1) {
            double alpha = a;
            a = alpha + (1.0 - alpha) * a_d;
            r = lerp(r, r_d, alpha / a);
            g = lerp(g, g_d, alpha / a);
            b = lerp(b, b_d, alpha / a);
        }

        auto convert = (_usesRGB) ? sRGB_2char : linear_2char;
        // Change this line:
        pixel_t *pixel = &(_img->rgba[y * _width + x]);
        pixel->r = convert(r);
        pixel->g = convert(g);
        pixel->b = convert(b);
        pixel->a = linear_2char(a);
        _zbuff[pos_idx] = v.z();
        return 0;
    }


    void Print(std::ostream& os = cout) { 
        for (int c = 0; c < 4; c++) {
            os << "color: " << c << '\n';
            for (int i = 0; i < _width; i++) {
                for (int j = 0; j < _height; j++) {
                    // Change this line:
                    pixel_t *pixel = &(_img->rgba[j * _width + i]);
                    os << (int)(pixel->p[c]) << ' ';
                }
                os << '\n';
            }
        }
        os << '\n';
    }

    void ExportPNG() {
        save_image(_img, _name.c_str());
    }

private:
    int _width, _height;
    string _name;
    image_t *_img = nullptr;
    vector<double> _zbuff;
    bool _useDepth = false, _usesRGB = false;

    static inline unsigned char linear_2char(double color) {
        return round(color * 255);
    }
    
    static inline unsigned char sRGB_2char(double color) {
        if (color <= 0.0031308)
            color *= 12.92;
        else
            color = 1.055 * pow(color, 1.0 / 2.4) - 0.055;
        return round(color * 255);
    }
    
    static inline double linear_2double(unsigned char color) {
        return (double)color / 255.0;
    }
    
    static inline double sRGB_2double(unsigned char color) {
        double c = (double)color / 255.0;
        if (c <= 0.04045)
            c /= 12.92;
        else
            c = pow((c + 0.055) / 1.055, 2.4);
        return c;
    }
    
    static inline double lerp(double a, double b, double a_ratio) {
        return a * a_ratio + b * (1.0 - a_ratio);
    }
};

// Utilities
static size_t getToken(const string& str, string& tok, size_t pos = 0)
{
   const string del = " \t";
   size_t begin = str.find_first_not_of(del, pos);
   if (begin == string::npos) { tok = ""; return begin; }
   size_t end = str.find_first_of(del, begin);
   tok = str.substr(begin, end - begin);
   return end;
}

static bool toInt(const string& str, int& num)
{
   num = 0;
   size_t i = 0;
   int sign = 1;
   if (str[0] == '-') { sign = -1; i = 1; }
   bool valid = false;
   for (; i < str.size(); ++i) {
      if (isdigit(str[i])) {
         num *= 10;
         num += int(str[i] - '0');
         valid = true;
      }
      else return false;
   }
   num *= sign;
   return valid;
}

static void parseLineToCommand(const string &line, vector<string> &command) {
    string token;
    size_t pos = getToken(line, token);
    while (!token.empty()) {
        command.push_back(token);
        pos = getToken(line, token, pos);
    }
}

// DDA
int DDA(Vertex a, Vertex b, int n, std::function<void(const Vertex &v)> fn) {
    if (a[n] > b[n]) swap(a, b);
    else if (a[n] == b[n]) return 0;

    Vertex d = (b - a) / (b[n] - a[n]);
    Vertex p = a + (d * (ceil(a[n]) - a[n]));;

    while (p[n] < b[n]) {
        fn(p);
        p += d;
    }
    return 1;
}

int DDATraverse(const Vertex &p, const Vertex &q, const Vertex &r,
                std::function<void(const Vertex &v)> fn)
{
    Vertex top = p, mid = q, bottom = r;
    if (top.y() < mid.y()) swap(top, mid);
    if (top.y() < bottom.y()) {
        swap(top, bottom);
        swap(mid, bottom);
    } else if (mid.y() < bottom.y()) swap(mid, bottom);

    // DDA on edge tm, tb using y axis
    vector<Vertex> tm_edges, tb_edges, mb_edges;
    DDA(top, mid, 1, [&tm_edges](const Vertex &v) -> void { tm_edges.push_back(v); });
    DDA(mid, bottom, 1, [&mb_edges](const Vertex &v) -> void { mb_edges.push_back(v); });
    DDA(top, bottom, 1, [&tb_edges](const Vertex &v) -> void { tb_edges.push_back(v); });

    std::reverse(tm_edges.begin(), tm_edges.end());
    std::reverse(mb_edges.begin(), mb_edges.end());
    std::reverse(tb_edges.begin(), tb_edges.end());

    for (int i = 0; i < tm_edges.size(); i++) {
        DDA(tm_edges[i], tb_edges[i], 0, fn);
    }
    for (int i = 0, j = tm_edges.size(); i < mb_edges.size() ; i++, j++) {
        if (j >= tb_edges.size())
            break;
        DDA(mb_edges[i], tb_edges[j], 0, fn);
    }
    return 0;
}

class Rasterizer {
public:
    void SetTexture(Texture *tex) { texture = tex; }

    void DrawTriangle(const Vertex &a, const Vertex &b, const Vertex&c,
                        Picture &picture, bool enable_hyp = false) {

        Vertex p = a, q = b, r = c;
        if (matrix) {
            p.SetData(matrix->mul(a), 0);
            q.SetData(matrix->mul(b), 0);
            r.SetData(matrix->mul(c), 0);
        }
        // transform array
        p = p.normalize(picture.GetWidth(), picture.GetHeight(), enable_hyp),
        q = q.normalize(picture.GetWidth(), picture.GetHeight(), enable_hyp),
        r = r.normalize(picture.GetWidth(), picture.GetHeight(), enable_hyp);
        Texture *tex = texture;
        DDATraverse(p, q, r, [&picture, enable_hyp, tex](const Vertex &v_n) {
            Vertex to_render = v_n.undo(enable_hyp);
            picture.render(to_render, tex);
        });
    }

private:
    Texture *texture = nullptr;
};

Rasterizer rasterizer;

// helper
int parsePng(const vector<string> &command, Picture &picture) {
    const int PNG_CMD_SIZE = 4;
    if (command.size() != PNG_CMD_SIZE) {
        return -1;
    }
    int h, w;
    if (!toInt(command[1], w) || !toInt(command[2], h)) {
        return -1;
    }
    picture.setup(command[3], w, h);
    return 0;
}

void updateData(const vector<string> &command, const int dimension, vector<Vertex> &vertices, int offset)
{
    int idx = 0;
    for (int i = 2; i < command.size(); i += dimension) {
        vector<double> arr(dimension);
        for (int j = 0; j < dimension; j++) {
            arr[j] = stod(command[i + j]);
        }
        if (idx == vertices.size()) vertices.push_back(Vertex());
        vertices[idx++].SetData(arr, offset);
    }
}

int parsePosition(const vector<string> &command, vector<Vertex> &vertices) {
    int dimension = 0;
    if (!toInt(command[1], dimension)) {
        return -1;
    }
    updateData(command, dimension, vertices, 0);
    return 0;
}

int parseColor(const vector<string> &command, vector<Vertex> &vertices) {
    int dimension = 0;
    if (!toInt(command[1], dimension)) {
        return -1;
    }
    updateData(command, dimension, vertices, 4);
    return 0;
}

int parseElements(const vector<string> &command, vector<int> &elements) {
    int idx;
    elements.clear();
    for (int i = 1; i < command.size(); i++) {
        if (!toInt(command[i], idx)) return -1;
        elements.push_back(idx);
    }
    return 0;
}

int parseMatrix(const vector<string> &command) {
    int idx;
    if (command.size() != 17) return -1;
    vector<double> data;
    data.reserve(16);
    for (int i = 1; i < command.size(); i++) {
        data.push_back(stod(command[i]));
    }
    if (matrix) delete matrix;
    matrix = new Matrix4(data);
    return 0;
}

int resetTexture(const vector<string> &command, Texture **texture) {
    if (command.size() != 2) return -1;
    if (*texture)
        delete *texture;
    *texture = new Texture(command[1]);
    return 0;
}

int parseTexcood(const vector<string> &command, vector<Vertex> &vertices) {
    int dimension = 0;
    if (!toInt(command[1], dimension)) {
        return -1;
    }
    assert(dimension == 2);
    updateData(command, dimension, vertices, 8); // for cood
    return 0;
}

int drawArraysTriangles(vector<string> command, Picture &picture,
                const vector<Vertex> &vertices, Texture *texture) {
    int start, count;
    if (command.size() != 3 || !toInt(command[1], start) || !toInt(command[2], count)) {
        return -1;
    }
    rasterizer.SetTexture(texture);
    for (int i = start; i < start + count; i += 3) {
        rasterizer.DrawTriangle(vertices[i], vertices[i+1], vertices[i+2],
                                picture, enable_hyp);
    }
    return 0;
}

int drawElementsTriangles(vector<string> command, Picture &picture,
                const vector<Vertex> &vertices, const vector<int> &elements, Texture *texture)
{
    int count, offset;
    if (command.size() != 3 || !toInt(command[1], count) || !toInt(command[2], offset)) {
        return -1;
    }
    rasterizer.SetTexture(texture);
    for (int i = offset; i < offset + count; i += 3) {
        rasterizer.DrawTriangle(vertices[elements[i]], vertices[elements[i+1]],
                                vertices[elements[i+2]], picture, enable_hyp);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        return -1;
    }

    Picture picture;
    vector<Vertex> vertices;
    vector<int> elements;
    Texture *texture = nullptr;
    ifstream fin(argv[1]);
    string line;
    while (getline(fin, line)) {
        vector<string> command;
        parseLineToCommand(line, command);
        if (command.empty()) continue;
        
        if (command[0] == "png")
            parsePng(command, picture);
        else if (command[0] == "position")
            parsePosition(command, vertices);
        else if (command[0] == "color")
            parseColor(command, vertices);
        else if (command[0] == "drawArraysTriangles")
            drawArraysTriangles(command, picture, vertices, texture);
        else if (command[0] == "depth")
            picture.SetDepth();
        else if (command[0] == "elements")
            parseElements(command, elements);
        else if (command[0] == "drawElementsTriangles")
            drawElementsTriangles(command, picture, vertices, elements, texture);
        else if (command[0] == "uniformMatrix")
            parseMatrix(command);
        else if (command[0] == "texture")
            resetTexture(command, &texture);
        else if (command[0] == "texcoord")
            parseTexcood(command, vertices);
        else if (command[0] == "sRGB")
            picture.SetsRBG();
        else if (command[0] == "hyp")
            enable_hyp = true;
    }
    picture.ExportPNG();
    if (texture) delete texture;
    if (matrix) delete matrix;
    return 0;
}