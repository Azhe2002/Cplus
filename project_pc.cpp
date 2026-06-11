#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>
#include <limits>
#include <cstring>

const int WIDTH = 1960;
const int HEIGHT = 1080;
const double INF = std::numeric_limits<double>::max();

struct Point3D {
    double x, y, z;
    int r, g, b;
};

// Read a PLY ASCII file
std::vector<Point3D> readPLY(const std::string& filename) {
    std::vector<Point3D> points;
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Cannot open " << filename << "\n";
        return points;
    }

    std::string line;
    int vertexCount = 0;
    bool inHeader = true;

    // Parse header
    while (inHeader && std::getline(in, line)) {
        if (line.rfind("element vertex ", 0) == 0) {
            vertexCount = std::stoi(line.substr(15));
        }
        if (line == "end_header") {
            inHeader = false;
        }
    }

    std::cout << "Reading " << vertexCount << " points from " << filename << "...\n";
    points.reserve(vertexCount);

    // Read vertices
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        Point3D p;
        iss >> p.x >> p.y >> p.z >> p.r >> p.g >> p.b;
        points.push_back(p);
    }

    std::cout << "Loaded " << points.size() << " points.\n";
    return points;
}

int main() {
    // ---- Same camera as the original raytracer ----
    double camX = 0.0, camY = 1.5, camZ = -2.0;
    double fov = 60.0 * M_PI / 180.0;
    double aspect = static_cast<double>(WIDTH) / HEIGHT;
    double tanHalfFov = std::tan(fov / 2.0);

    // Projection scale factors (from the original pixel → ray mapping)
    double sx = 1.0 / (tanHalfFov * aspect); // maps px → normalized x
    double sy = 1.0 / tanHalfFov;             // maps py → normalized y

    // ---- Read point cloud ----
    std::vector<Point3D> points = readPLY("test.ply");
    if (points.empty()) return 1;

    // ---- Z-buffer and image buffer ----
    std::vector<double> zbuffer(WIDTH * HEIGHT, INF);
    struct Pixel { int r, g, b; };
    std::vector<Pixel> image(WIDTH * HEIGHT, {10, 10, 15}); // dark sky background

    int projected = 0, occluded = 0, outOfFrame = 0, behindCamera = 0;

    for (const auto& p : points) {
        // Transform to camera space
        double cx = p.x - camX;
        double cy = p.y - camY;
        double cz = p.z - camZ;  // cz = P.z + 2

        if (cz <= 0.001) { behindCamera++; continue; } // behind camera

        // Perspective division (same as the ray tracer's forward model)
        double px = cx / cz; // = ray_dir.x before normalization in original
        double py = cy / cz; // = ray_dir.y

        // Reverse the pixel ↔ ray mapping from the original tracer:
        //   ray_x = (2*(i+0.5)/W - 1) * tan(fov/2) * aspect   →  px = ray_x
        // So: (2*(i+0.5)/W - 1) = px / (tan(fov/2) * aspect) = px * sx
        //     i + 0.5 = (px*sx + 1) * 0.5 * W
        //     i = (px*sx + 1) * 0.5 * W - 0.5

        double fx = (px * sx + 1.0) * 0.5 * WIDTH - 0.5;
        double fy = (1.0 - py * sy) * 0.5 * HEIGHT - 0.5;

        int ix = static_cast<int>(std::round(fx));
        int iy = static_cast<int>(std::round(fy));

        // Check bounds
        if (ix < 0 || ix >= WIDTH || iy < 0 || iy >= HEIGHT) {
            outOfFrame++;
            continue;
        }

        int idx = iy * WIDTH + ix;

        // Z-buffer test: closer points win (smaller cz)
        if (cz < zbuffer[idx]) {
            zbuffer[idx] = cz;
            image[idx] = {p.r, p.g, p.b};
            projected++;
        } else {
            occluded++;
        }
    }

    // ---- Write PPM ----
    std::ofstream ppm("projected.ppm");
    ppm << "P3\n" << WIDTH << " " << HEIGHT << "\n255\n";
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            const auto& pix = image[y * WIDTH + x];
            ppm << pix.r << " " << pix.g << " " << pix.b << " ";
        }
        ppm << "\n";
    }
    ppm.close();

    // ---- Stats ----
    std::cout << "\n=================================\n";
    std::cout << "  Point Cloud Projection Stats\n";
    std::cout << "=================================\n";
    std::cout << "Total points:       " << points.size() << "\n";
    std::cout << "Projected (visible): " << projected << "\n";
    std::cout << "Occluded:           " << occluded << "\n";
    std::cout << "Out of frame:       " << outOfFrame << "\n";
    std::cout << "Behind camera:      " << behindCamera << "\n";
    std::cout << "Pixels filled:      " << projected << " / " << (WIDTH*HEIGHT)
              << " (" << (100.0 * projected / (WIDTH*HEIGHT)) << "%)\n";
    std::cout << "\nWritten: projected.ppm\n";

    return 0;
}
