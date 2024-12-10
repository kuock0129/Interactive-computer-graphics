#include <float.h>      /* DBL_MAX */
#include <math.h>       /* ceil */
#include <assert.h>     // assert
#include <fstream>
#include <iostream>
#include <functional>   
#include <algorithm>    
#include <vector>
#include "uselibpng.h"

using namespace std;
typedef unsigned char uc;

// global variables
bool enable_hyp = false;

struct Position2 { int x; int y; };
struct Color4 { uc r; uc g; uc b; uc a; };

class Vertex {
public:
    Vertex(): _data(VERTEX_DIM, 0) {
        _data[3] = 1.0; // w
        _data[7] = 1.0; // alpha
    }

    double &x() { return _data[0]; }
    double &y() { return _data[1]; }
    double &z() { return _data[2]; }
    double &w() { return _data[3]; }
    double &r() { return _data[4]; }
    double &g() { return _data[5]; }
    double &b() { return _data[6]; }
    double &a() { return _data[7]; }
    double &s() { return _data[8]; }
    double &t() { return _data[9]; }

    double x() const { return _data[0]; }
    double y() const { return _data[1]; }
    double z() const { return _data[2]; }
    double w() const { return _data[3]; }
    double r() const { return _data[4]; }
    double g() const { return _data[5]; }
    double b() const { return _data[6]; }
    double a() const { return _data[7]; }
    double s() const { return _data[8]; }
    double t() const { return _data[9]; }

    double &operator[](size_t idx) { return _data[idx]; }
    const double &operator[](size_t idx) const { return _data[idx]; }

    Vertex operator-(const Vertex &v) {
        Vertex ret;
        for (int i = 0; i < VERTEX_DIM; i++) {
            ret[i] = _data[i] - v[i];
        }
        return ret;
    }

    Vertex operator+(const Vertex &v) {
        Vertex ret;
        for (int i = 0; i < VERTEX_DIM; i++) {
            ret[i] = _data[i] + v[i];
        }
        return ret;
    }

    Vertex operator/(const double div) {
        Vertex ret;
        if (div == 0) return ret;
        for (int i = 0; i < VERTEX_DIM; i++) {
            ret[i] = _data[i] / div;
        }
        return ret;
    }

    Vertex operator*(const double mul) {
        Vertex ret;
        for (int i = 0; i < VERTEX_DIM; i++) {
            ret[i] = _data[i] * mul;
        }
        return ret;
    }

    Vertex & operator+=(const Vertex &v) {
        for (int i = 0; i < VERTEX_DIM; i++) {
            _data[i] += v[i];
        }
        return *this;
    }

    Vertex w_norm(const int width, const int height, bool enable_hyp) const {
        Vertex ret;
        double w = this->w();
        for (int i = 0; i < VERTEX_DIM; i++) {
            if (i == 3) ret[i] = 1.0 / w;
            else if (i < 3 || enable_hyp) ret[i] = _data[i] / w;
            else ret[i] = _data[i];
        }
        ret[0] = (ret[0] + 1) * width / 2;
        ret[1] = (ret[1] + 1) * height / 2;

        return ret;
    }

    Vertex w_undo(bool enable_hyp) const {
        Vertex ret;
        double w = this->w();
        for (int i = 0; i < VERTEX_DIM; i++) {
            if (i == 3) ret[i] = 1.0 / w;
            else if (i < 3) ret[i] = _data[i];
            else if (!enable_hyp) ret[i] = _data[i];
            else ret[i] = _data[i] / w;
        }
        return ret;
    }

    void SetPos(const vector<double> &position) {
        SetData(position, 0);
    }

    void SetColor(const vector<double> &color) {
        SetData(color, 4);
    }

    void SetData(const vector<double> &data, int offset) {
        for (int i = 0; i < data.size(); i++)
            _data[offset + i] = data[i];
    }

    friend auto operator<<(std::ostream& os, Vertex const& v) -> std::ostream& { 
        os << "Vertex:\n  pos: " << v.x() << ", " << v.y() << ", " << v.z() << ", " << v.w() << '\n';
        os << "  st: " << v.s() << ", " << v.t() << '\n';
        return os;
    }

private:
    static int VERTEX_DIM;
    vector<double> _data;
};
int Vertex::VERTEX_DIM = 10;

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

    static inline uc linear_2char(double color) {
        return round(color * 255);
    }
    
    static inline uc sRGB_2char(double color) {
        if (color <= 0.0031308)
            color *= 12.92;
        else
            color = 1.055 * pow(color, 1.0 / 2.4) - 0.055;
        return round(color * 255);
    }
    
    static inline double linear_2double(uc color) {
        return (double)color / 255.0;
    }
    
    static inline double sRGB_2double(uc color) {
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
        p = p.w_norm(picture.GetWidth(), picture.GetHeight(), enable_hyp),
        q = q.w_norm(picture.GetWidth(), picture.GetHeight(), enable_hyp),
        r = r.w_norm(picture.GetWidth(), picture.GetHeight(), enable_hyp);
        Texture *tex = texture;
        DDATraverse(p, q, r, [&picture, enable_hyp, tex](const Vertex &v_n) {
            Vertex to_render = v_n.w_undo(enable_hyp);
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
        else if (command[0] == "sRGB")
            picture.SetsRBG();
        else if (command[0] == "hyp")
            enable_hyp = true;
        else if (command[0] == "uniformMatrix")
            parseMatrix(command);
        else if (command[0] == "texture")
            resetTexture(command, &texture);
        else if (command[0] == "texcoord")
            parseTexcood(command, vertices);
    }
    picture.ExportPNG();
    if (texture) delete texture;
    if (matrix) delete matrix;
    return 0;
}