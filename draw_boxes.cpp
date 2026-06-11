#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstring>

const int WIDTH  = 1960;
const int HEIGHT = 1080;

// ---------- 3D math ----------
struct Vec3 { double x, y, z; };
struct Pixel { int r, g, b; };

// Camera (same as raytracer)
const Vec3 CAMERA = {0.0, 1.5, -2.0};
const double FOV  = 60.0 * M_PI / 180.0;
const double ASPECT = (double)WIDTH / HEIGHT;
const double TAN_HALF = std::tan(FOV / 2.0);
const double SX = 1.0 / (TAN_HALF * ASPECT);
const double SY = 1.0 / TAN_HALF;

// Sphere definitions
struct Sphere {
    std::string name;
    Vec3 center;
    double radius;
    Pixel color; // box color
};
const Sphere SPHERES[] = {
    {"Red sphere",    {-1.0,  0.5, 5.0}, 1.5, {255,   0,   0}},
    {"Blue sphere",   { 2.0, -0.5, 4.0}, 1.0, { 50, 100, 255}},
    {"Green sphere",  {-2.5,  0.0, 3.5}, 0.8, {  0, 255,   0}},
    {"Yellow sphere", { 0.5, -1.0, 3.0}, 0.5, {255, 200,   0}},
};
const int N_SPHERES = sizeof(SPHERES) / sizeof(SPHERES[0]);

// ---------- Perspective projection ----------
struct Point2D { double x, y; bool valid; };

Point2D project(const Vec3& world) {
    double cx = world.x - CAMERA.x;
    double cy = world.y - CAMERA.y;
    double cz = world.z - CAMERA.z;
    if (cz <= 0.001) return {0, 0, false};

    double px = cx / cz;
    double py = cy / cz;
    return {
        (px * SX + 1.0) * 0.5 * WIDTH - 0.5,
        (1.0 - py * SY) * 0.5 * HEIGHT - 0.5,
        true
    };
}

// ---------- Bounding box from sphere ----------
struct BBox { int x1, y1, x2, y2; bool valid; };

BBox computeBBox(const Sphere& s, int nSamples = 300) {
    std::vector<Point2D> pts;
    // AABB corners
    for (int dx = -1; dx <= 1; dx += 2)
    for (int dy = -1; dy <= 1; dy += 2)
    for (int dz = -1; dz <= 1; dz += 2) {
        Vec3 corner = {s.center.x + dx*s.radius,
                       s.center.y + dy*s.radius,
                       s.center.z + dz*s.radius};
        Point2D p = project(corner);
        if (p.valid) pts.push_back(p);
    }

    // Fibonacci sphere surface samples (for tighter box near silhouette edges)
    for (int i = 0; i < nSamples; i++) {
        double phi   = std::acos(1.0 - 2.0 * (i + 0.5) / nSamples);
        double theta = M_PI * (1.0 + std::sqrt(5.0)) * i;
        Vec3 pt = {
            s.center.x + s.radius * std::sin(phi) * std::cos(theta),
            s.center.y + s.radius * std::sin(phi) * std::sin(theta),
            s.center.z + s.radius * std::cos(phi)
        };
        Point2D p = project(pt);
        if (p.valid) pts.push_back(p);
    }

    if (pts.empty()) return {0, 0, 0, 0, false};

    double minX = pts[0].x, maxX = pts[0].x;
    double minY = pts[0].y, maxY = pts[0].y;
    for (const auto& p : pts) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    return {
        std::max(0, (int)minX), std::max(0, (int)minY),
        std::min(WIDTH-1, (int)maxX), std::min(HEIGHT-1, (int)maxY),
        true
    };
}

// ---------- PPM I/O ----------
std::vector<Pixel> readPPM(const std::string& filename) {
    std::vector<Pixel> img(WIDTH * HEIGHT, {0,0,0});
    std::ifstream in(filename);
    if (!in) { std::cerr << "Cannot open " << filename << "\n"; return img; }

    std::string magic; int w, h, maxVal;
    in >> magic >> w >> h >> maxVal;
    if (magic != "P3" || w != WIDTH || h != HEIGHT) {
        std::cerr << "Bad PPM format: " << magic << " " << w << "x" << h << "\n";
        return img;
    }
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        int r, g, b; in >> r >> g >> b;
        img[i] = {r, g, b};
    }
    return img;
}

void writePPM(const std::string& filename, const std::vector<Pixel>& img) {
    std::ofstream out(filename);
    out << "P3\n" << WIDTH << " " << HEIGHT << "\n255\n";
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            const auto& p = img[y * WIDTH + x];
            out << p.r << " " << p.g << " " << p.b << " ";
        }
        out << "\n";
    }
}

// ---------- Drawing ----------
void drawHLine(std::vector<Pixel>& img, int x1, int x2, int y, Pixel color, int thickness=2) {
    if (y < 0 || y >= HEIGHT) return;
    x1 = std::max(0, x1); x2 = std::min(WIDTH-1, x2);
    for (int t = 0; t < thickness; t++) {
        int yy = y + t - thickness/2;
        if (yy < 0 || yy >= HEIGHT) continue;
        for (int x = x1; x <= x2; x++)
            img[yy * WIDTH + x] = color;
    }
}

void drawVLine(std::vector<Pixel>& img, int x, int y1, int y2, Pixel color, int thickness=2) {
    if (x < 0 || x >= WIDTH) return;
    y1 = std::max(0, y1); y2 = std::min(HEIGHT-1, y2);
    for (int t = 0; t < thickness; t++) {
        int xx = x + t - thickness/2;
        if (xx < 0 || xx >= WIDTH) continue;
        for (int y = y1; y <= y2; y++)
            img[y * WIDTH + xx] = color;
    }
}

void drawRect(std::vector<Pixel>& img, const BBox& b, Pixel color, int thickness=3) {
    if (!b.valid) return;
    // Top
    for (int t = 0; t < thickness; t++) {
        drawHLine(img, b.x1, b.x2, b.y1 + t, color, 1);
        drawHLine(img, b.x1, b.x2, b.y2 - t, color, 1);
        drawVLine(img, b.x1 + t, b.y1, b.y2, color, 1);
        drawVLine(img, b.x2 - t, b.y1, b.y2, color, 1);
    }
}

void drawDot(std::vector<Pixel>& img, int cx, int cy, Pixel color, int r=4) {
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx*dx + dy*dy > r*r) continue;
            int x = cx + dx, y = cy + dy;
            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
                img[y * WIDTH + x] = color;
        }
    }
}

// Simple 5x7 bitmap font for labels (printable ASCII 32-90)
// Each char: 7 rows, each row is a 5-bit mask (bits 4..0, bit4=leftmost)
const unsigned char FONT[][7] = {
    // ' '(32)
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    // '!'
    {0x04,0x04,0x04,0x04,0x04,0x00,0x04},
    // '"'
    {0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00},
    // '#'
    {0x0A,0x0A,0x1F,0x0A,0x1F,0x0A,0x0A},
    // ... keep it simple, only uppercase letters
};

// Simple font: uppercase letters A-Z only
const unsigned char FONT_LETTERS[26][7] = {
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
    {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}, // G
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // I
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}, // J
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
    {0x11,0x11,0x19,0x15,0x13,0x11,0x11}, // N
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
    {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, // V
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}, // W
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // Z
};

void drawChar(std::vector<Pixel>& img, int x, int y, char c, Pixel color, int scale=2) {
    int idx = -1;
    if (c >= 'A' && c <= 'Z') idx = c - 'A';
    else if (c >= 'a' && c <= 'z') idx = c - 'a';
    else if (c == ' ') return;
    else return; // unsupported char

    for (int row = 0; row < 7; row++) {
        unsigned char bits = FONT_LETTERS[idx][row];
        for (int col = 0; col < 5; col++) {
            if (bits & (1 << (4 - col))) {
                for (int dy = 0; dy < scale; dy++)
                    for (int dx = 0; dx < scale; dx++) {
                        int px = x + col * scale + dx;
                        int py = y + row * scale + dy;
                        if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT)
                            img[py * WIDTH + px] = color;
                    }
            }
        }
    }
}

void drawText(std::vector<Pixel>& img, int x, int y, const std::string& text, Pixel color, int scale=2) {
    int cx = x;
    for (char c : text) {
        drawChar(img, cx, y, c, color, scale);
        cx += 6 * scale; // 5px char + 1px gap, scaled
    }
}

void drawLabel(std::vector<Pixel>& img, int x, int y, const std::string& text, Pixel bgColor, Pixel fgColor) {
    int charW = 6 * 2;  // scale=2
    int charH = 7 * 2;
    int padding = 4;
    int tw = text.length() * charW;
    int th = charH;

    // Background
    for (int dy = -padding; dy < th + padding; dy++) {
        for (int dx = -padding; dx < tw + padding; dx++) {
            int px = x + dx, py = y + dy;
            if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT)
                img[py * WIDTH + px] = bgColor;
        }
    }
    drawText(img, x, y, text, fgColor, 2);
}

// ---------- Main ----------
int main() {
    // Read input image (PPM format for easy C++ parsing)
    std::cout << "Reading projected.ppm...\n";
    std::vector<Pixel> img = readPPM("projected.ppm");

    // Compute all bounding boxes
    std::cout << "Computing bounding boxes...\n";
    for (int i = 0; i < N_SPHERES; i++) {
        const Sphere& s = SPHERES[i];

        // Project center
        Point2D cp = project(s.center);

        // Compute 2D bounding box
        BBox box = computeBBox(s);

        // Draw bounding box
        drawRect(img, box, s.color, 3);

        // Draw center dot
        if (cp.valid) {
            drawDot(img, (int)cp.x, (int)cp.y, {255, 255, 255}, 5);
            drawDot(img, (int)cp.x, (int)cp.y, s.color, 3);
        }

        // Label above the box
        int labelX = box.x1;
        int labelY = box.y1 - 22; // above the top edge
        if (labelY < 2) labelY = box.y2 + 4; // below if no room above

        // Dimmed version of sphere color for label bg
        Pixel bg = {(unsigned char)(s.color.r / 2),
                    (unsigned char)(s.color.g / 2),
                    (unsigned char)(s.color.b / 2)};
        drawLabel(img, labelX, labelY, s.name, bg, {255, 255, 255});

        std::cout << "  " << s.name << ": box=(" << box.x1 << "," << box.y1
                  << ")-(" << box.x2 << "," << box.y2 << ")"
                  << "  size=" << (box.x2-box.x1) << "x" << (box.y2-box.y1) << "px\n";
    }

    // Title bar
    for (int y = 0; y < 36; y++)
        for (int x = 0; x < WIDTH; x++)
            img[y * WIDTH + x] = {0, 0, 0};
    drawText(img, 12, 6, "POINT CLOUD  -  4 SPHERES  +  CHECKERBOARD", {255, 255, 255}, 2);

    // Color legend at bottom
    int legendY = HEIGHT - 32;
    for (int y = legendY - 4; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            img[y * WIDTH + x] = {10, 10, 15};

    int lx = 12;
    for (int i = 0; i < N_SPHERES; i++) {
        const Sphere& s = SPHERES[i];
        // Color swatch
        for (int dy = 4; dy < 22; dy++)
            for (int dx = 0; dx < 20; dx++)
                img[(legendY + dy) * WIDTH + (lx + dx)] = s.color;
        drawText(img, lx + 26, legendY + 6, s.name, {255, 255, 255}, 1);
        lx += 220;
    }

    // Write output
    std::string outFile = "boxed.ppm";
    std::cout << "Writing " << outFile << "...\n";
    writePPM(outFile, img);
    std::cout << "Done.\n";

    return 0;
}
