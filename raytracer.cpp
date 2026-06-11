#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <limits>

const int WIDTH = 1960;
const int HEIGHT = 1080;
const double INF = std::numeric_limits<double>::max();

// 3D vector
struct Vec3 {
    double x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& v) const { return Vec3(x+v.x, y+v.y, z+v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x-v.x, y-v.y, z-v.z); }
    Vec3 operator*(double s) const { return Vec3(x*s, y*s, z*s); }
    Vec3 operator/(double s) const { return Vec3(x/s, y/s, z/s); }
    double dot(const Vec3& v) const { return x*v.x + y*v.y + z*v.z; }
    Vec3 cross(const Vec3& v) const {
        return Vec3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
    }
    double length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalize() const { return *this / length(); }
};

struct Ray {
    Vec3 origin, direction;
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d.normalize()) {}
};

struct Sphere {
    Vec3 center;
    double radius;
    Vec3 color;
    double reflectivity; // 0 = matte, 1 = mirror

    Sphere(const Vec3& c, double r, const Vec3& col, double refl = 0.0)
        : center(c), radius(r), color(col), reflectivity(refl) {}

    bool intersect(const Ray& ray, double& t) const {
        Vec3 oc = ray.origin - center;
        double b = oc.dot(ray.direction);
        double c = oc.dot(oc) - radius * radius;
        double disc = b*b - c;
        if (disc < 0) return false;
        double sqrt_disc = std::sqrt(disc);
        double t1 = -b - sqrt_disc;
        double t2 = -b + sqrt_disc;
        if (t1 > 0.001) { t = t1; return true; }
        if (t2 > 0.001) { t = t2; return true; }
        return false;
    }
};

// Checkerboard plane at y = -2
bool intersectPlane(const Ray& ray, double& t, Vec3& color, const Vec3& hitPoint) {
    double planeY = -2.0;
    if (std::abs(ray.direction.y) < 1e-6) return false;
    t = (planeY - ray.origin.y) / ray.direction.y;
    if (t < 0.001) return false;

    Vec3 p = ray.origin + ray.direction * t;
    // Checkerboard pattern
    int cx = static_cast<int>(std::floor(p.x / 2.0));
    int cz = static_cast<int>(std::floor(p.z / 2.0));
    if ((cx + cz) % 2 == 0)
        color = Vec3(0.9, 0.9, 0.9);
    else
        color = Vec3(0.3, 0.3, 0.3);
    return true;
}

// Clamp color to [0, 255]
int toByte(double x) {
    int v = static_cast<int>(x * 255);
    if (v < 0) return 0;
    if (v > 255) return 255;
    return v;
}

Vec3 trace(const Ray& ray, const std::vector<Sphere>& spheres, int depth) {
    if (depth > 3) return Vec3(0.05, 0.05, 0.08); // sky

    double closest_t = INF;
    int hitIdx = -1;
    bool hitPlane = false;
    Vec3 planeColor;
    double planeT = INF;

    // Find closest sphere intersection
    for (size_t i = 0; i < spheres.size(); i++) {
        double t;
        if (spheres[i].intersect(ray, t) && t < closest_t) {
            closest_t = t;
            hitIdx = static_cast<int>(i);
        }
    }

    // Check plane intersection
    Vec3 pc;
    double pt;
    if (intersectPlane(ray, pt, pc, Vec3()) && pt < closest_t) {
        closest_t = pt;
        hitPlane = true;
        planeColor = pc;
        planeT = pt;
        hitIdx = -1;
    }

    if (hitIdx < 0 && !hitPlane) {
        // Sky gradient
        double t = 0.5 * (ray.direction.y + 1.0);
        return Vec3(0.05, 0.05, 0.08) * (1.0 - t) + Vec3(0.5, 0.7, 1.0) * t;
    }

    Vec3 hitPoint = ray.origin + ray.direction * closest_t;
    Vec3 normal;
    Vec3 objColor;

    if (hitPlane) {
        normal = Vec3(0, 1, 0);
        objColor = planeColor;
    } else {
        const Sphere& s = spheres[hitIdx];
        normal = (hitPoint - s.center).normalize();
        objColor = s.color;
    }

    // Lighting
    Vec3 lightDir = Vec3(5, 10, 3).normalize();
    Vec3 ambient = objColor * 0.15;

    // Diffuse
    double diff = std::max(0.0, normal.dot(lightDir));
    Vec3 diffuse = objColor * diff * 0.7;

    // Specular
    Vec3 viewDir = (ray.origin - hitPoint).normalize();
    Vec3 halfVec = (lightDir + viewDir).normalize();
    double spec = std::pow(std::max(0.0, normal.dot(halfVec)), 64);
    Vec3 specular = Vec3(1, 1, 1) * spec * 0.3;

    // Shadow check
    bool inShadow = false;
    Ray shadowRay(hitPoint + normal * 0.001, lightDir);
    for (const auto& s : spheres) {
        double t;
        if (s.intersect(shadowRay, t)) { inShadow = true; break; }
    }
    if (inShadow) {
        diffuse = Vec3(0, 0, 0);
        specular = Vec3(0, 0, 0);
    }

    // Reflection
    Vec3 reflection(0, 0, 0);
    if (!hitPlane && spheres[hitIdx].reflectivity > 0) {
        Vec3 reflDir = ray.direction - normal * 2.0 * ray.direction.dot(normal);
        Ray reflRay(hitPoint + normal * 0.001, reflDir);
        reflection = trace(reflRay, spheres, depth + 1) * spheres[hitIdx].reflectivity;
    }

    return ambient + diffuse + specular + reflection;
}

int main() {
    std::vector<Sphere> spheres;
    // Ground sphere (large)
    spheres.push_back(Sphere(Vec3(0, -10002, 0), 10000, Vec3(0.2, 0.3, 0.2)));
    // Main sphere - red
    spheres.push_back(Sphere(Vec3(-1.0, 0.5, 5), 1.5, Vec3(0.9, 0.2, 0.2), 0.4));
    // Blue sphere
    spheres.push_back(Sphere(Vec3(2.0, -0.5, 4), 1.0, Vec3(0.2, 0.3, 0.9), 0.3));
    // Green sphere
    spheres.push_back(Sphere(Vec3(-2.5, 0.0, 3.5), 0.8, Vec3(0.2, 0.8, 0.3), 0.1));
    // Small yellow sphere on the checkerboard
    spheres.push_back(Sphere(Vec3(0.5, -1.0, 3), 0.5, Vec3(0.9, 0.8, 0.1), 0.2));

    Vec3 camera(0, 1.5, -2);
    double fov = 60.0 * M_PI / 180.0;
    double aspect = static_cast<double>(WIDTH) / HEIGHT;

    // Output PPM
    std::ofstream out("test.ppm");
    out << "P3\n" << WIDTH << " " << HEIGHT << "\n255\n";

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            double px = (2.0 * (x + 0.5) / WIDTH - 1.0) * std::tan(fov / 2.0) * aspect;
            double py = (1.0 - 2.0 * (y + 0.5) / HEIGHT) * std::tan(fov / 2.0);

            Ray ray(camera, Vec3(px, py, 1));
            Vec3 color = trace(ray, spheres, 0);

            out << toByte(color.x) << " " << toByte(color.y) << " " << toByte(color.z) << " ";
        }
        out << "\n";
        if (y % 100 == 0) std::cerr << "Rendering: " << (100 * y / HEIGHT) << "%\r";
    }

    out.close();
    std::cerr << "Rendering: 100%\nDone. PPM written to test.ppm\n";
    return 0;
}
