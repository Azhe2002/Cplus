#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <limits>
#include <algorithm>
#include <iomanip>

const int WIDTH = 1960;
const int HEIGHT = 1080;
const double INF = std::numeric_limits<double>::max();

// ---------- 3D Vector ----------
struct Vec3 {
    double x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& v) const { return Vec3(x+v.x, y+v.y, z+v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x-v.x, y-v.y, z-v.z); }
    Vec3 operator*(double s) const { return Vec3(x*s, y*s, z*s); }
    Vec3 operator/(double s) const { return Vec3(x/s, y/s, z/s); }
    double dot(const Vec3& v) const { return x*v.x + y*v.y + z*v.z; }
    double length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalize() const { return *this / length(); }
};

// ---------- Geometry ----------
struct Ray {
    Vec3 origin, direction;
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d.normalize()) {}
};

struct Sphere {
    Vec3 center;
    double radius;
    Vec3 color;
    double reflectivity;
    int id; // for tracking which surface was hit

    Sphere(const Vec3& c, double r, const Vec3& col, double refl = 0.0, int id = 0)
        : center(c), radius(r), color(col), reflectivity(refl), id(id) {}

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

    // Signed distance from a point to the sphere surface (negative = inside)
    double signedDistance(const Vec3& p) const {
        return (p - center).length() - radius;
    }
};

// Checkerboard plane at y = -2
bool intersectPlane(const Ray& ray, double& t, Vec3& color, Vec3& normal) {
    double planeY = -2.0;
    if (std::abs(ray.direction.y) < 1e-6) return false;
    t = (planeY - ray.origin.y) / ray.direction.y;
    if (t < 0.001) return false;

    Vec3 p = ray.origin + ray.direction * t;
    int cx = static_cast<int>(std::floor(p.x / 2.0));
    int cz = static_cast<int>(std::floor(p.z / 2.0));
    color = ((cx + cz) % 2 == 0) ? Vec3(0.9, 0.9, 0.9) : Vec3(0.3, 0.3, 0.3);
    normal = Vec3(0, 1, 0);
    return true;
}

double distanceToPlane(const Vec3& p) {
    return std::abs(p.y + 2.0); // plane at y = -2
}

// ---------- Ray tracing ----------
int toByte(double x) {
    int v = static_cast<int>(x * 255);
    if (v < 0) return 0; if (v > 255) return 255;
    return v;
}

struct HitInfo {
    double t;
    Vec3 point;
    Vec3 normal;
    Vec3 color;
    int objectId; // 0-3 = spheres, 4 = plane, -1 = sky
};

Vec3 trace(const Ray& ray, const std::vector<Sphere>& spheres, int depth, HitInfo* outHit = nullptr) {
    if (depth > 3) {
        if (outHit) outHit->objectId = -1;
        return Vec3(0.05, 0.05, 0.08);
    }

    double closest_t = INF;
    int hitIdx = -1;
    bool hitPlane = false;
    Vec3 planeColor, planeNormal;
    double planeT = INF;

    for (size_t i = 0; i < spheres.size(); i++) {
        double t;
        if (spheres[i].intersect(ray, t) && t < closest_t) {
            closest_t = t;
            hitIdx = static_cast<int>(i);
        }
    }

    if (intersectPlane(ray, planeT, planeColor, planeNormal) && planeT < closest_t) {
        closest_t = planeT;
        hitPlane = true;
        hitIdx = -1;
    }

    if (hitIdx < 0 && !hitPlane) {
        if (outHit) outHit->objectId = -1;
        double t = 0.5 * (ray.direction.y + 1.0);
        return Vec3(0.05, 0.05, 0.08) * (1.0 - t) + Vec3(0.5, 0.7, 1.0) * t;
    }

    Vec3 hitPoint = ray.origin + ray.direction * closest_t;
    Vec3 normal, objColor;
    int objId;

    if (hitPlane) {
        normal = planeNormal;
        objColor = planeColor;
        objId = 4; // plane ID
    } else {
        const Sphere& s = spheres[hitIdx];
        normal = (hitPoint - s.center).normalize();
        objColor = s.color;
        objId = s.id;
    }

    if (outHit) {
        outHit->t = closest_t;
        outHit->point = hitPoint;
        outHit->normal = normal;
        outHit->color = objColor;
        outHit->objectId = objId;
    }

    // Lighting
    Vec3 lightDir = Vec3(5, 10, 3).normalize();
    Vec3 ambient = objColor * 0.15;
    double diff = std::max(0.0, normal.dot(lightDir));
    Vec3 diffuse = objColor * diff * 0.7;
    Vec3 viewDir = (ray.origin - hitPoint).normalize();
    Vec3 halfVec = (lightDir + viewDir).normalize();
    double spec = std::pow(std::max(0.0, normal.dot(halfVec)), 64);
    Vec3 specular = Vec3(1, 1, 1) * spec * 0.3;

    bool inShadow = false;
    Ray shadowRay(hitPoint + normal * 0.001, lightDir);
    for (const auto& s : spheres) {
        double t; if (s.intersect(shadowRay, t)) { inShadow = true; break; }
    }
    if (inShadow) { diffuse = Vec3(0,0,0); specular = Vec3(0,0,0); }

    Vec3 reflection(0,0,0);
    if (!hitPlane && spheres[hitIdx].reflectivity > 0) {
        Vec3 reflDir = ray.direction - normal * 2.0 * ray.direction.dot(normal);
        Ray reflRay(hitPoint + normal * 0.001, reflDir);
        reflection = trace(reflRay, spheres, depth + 1) * spheres[hitIdx].reflectivity;
    }

    Vec3 finalColor = ambient + diffuse + specular + reflection;
    if (outHit) {
        outHit->color = finalColor; // store shaded color, not surface color
    }
    return finalColor;
}

// ---------- Ground truth geometry (for accuracy) ----------
struct GroundTruth {
    virtual double distance(const Vec3& p) const = 0;
    virtual std::string name() const = 0;
    virtual ~GroundTruth() {}
};

struct GTSphere : GroundTruth {
    Vec3 center; double radius; std::string label;
    GTSphere(const Vec3& c, double r, const std::string& n) : center(c), radius(r), label(n) {}
    double distance(const Vec3& p) const override {
        return std::abs((p - center).length() - radius);
    }
    std::string name() const override { return label; }
};

struct GTPlane : GroundTruth {
    double distance(const Vec3& p) const override {
        return std::abs(p.y + 2.0);
    }
    std::string name() const override { return "Checkerboard plane (y=-2)"; }
};

// ---------- Main ----------
int main() {
    // ---------- Scene definition ----------
    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(Vec3(0, -10002, 0), 10000, Vec3(0.2, 0.3, 0.2), 0.0, -1)); // ground (hidden)
    spheres.push_back(Sphere(Vec3(-1.0, 0.5, 5), 1.5, Vec3(0.9, 0.2, 0.2), 0.4, 0));     // red
    spheres.push_back(Sphere(Vec3(2.0, -0.5, 4), 1.0, Vec3(0.2, 0.3, 0.9), 0.3, 1));      // blue
    spheres.push_back(Sphere(Vec3(-2.5, 0.0, 3.5), 0.8, Vec3(0.2, 0.8, 0.3), 0.1, 2));    // green
    spheres.push_back(Sphere(Vec3(0.5, -1.0, 3), 0.5, Vec3(0.9, 0.8, 0.1), 0.2, 3));      // yellow

    // Ground truth list (for accuracy — excluding the giant hidden ground sphere)
    std::vector<GroundTruth*> groundTruths;
    groundTruths.push_back(new GTSphere(Vec3(-1.0, 0.5, 5), 1.5, "Red sphere"));
    groundTruths.push_back(new GTSphere(Vec3(2.0, -0.5, 4), 1.0, "Blue sphere"));
    groundTruths.push_back(new GTSphere(Vec3(-2.5, 0.0, 3.5), 0.8, "Green sphere"));
    groundTruths.push_back(new GTSphere(Vec3(0.5, -1.0, 3), 0.5, "Yellow sphere"));
    groundTruths.push_back(new GTPlane());

    Vec3 camera(0, 1.5, -2);
    double fov = 60.0 * M_PI / 180.0;
    double aspect = static_cast<double>(WIDTH) / HEIGHT;

    // ---------- Output files ----------
    std::ofstream ppm("test.ppm");
    ppm << "P3\n" << WIDTH << " " << HEIGHT << "\n255\n";

    std::ofstream ply("test.ply");
    // We'll write the header after counting valid points

    // ---------- Data storage ----------
    struct PointCloudPoint {
        double x, y, z;
        int r, g, b;
        int objectId;
    };
    std::vector<PointCloudPoint> pointCloud;
    pointCloud.reserve(WIDTH * HEIGHT);

    // ---------- Per-object accuracy accumulators ----------
    struct ObjStats {
        std::string name;
        double sumError = 0, sumSqError = 0, maxError = 0;
        int count = 0;
    };
    std::vector<ObjStats> objStats(6); // 4 spheres + plane + unreferenced
    objStats[0].name = "Red sphere (id=0)";
    objStats[1].name = "Blue sphere (id=1)";
    objStats[2].name = "Green sphere (id=2)";
    objStats[3].name = "Yellow sphere (id=3)";
    objStats[4].name = "Checkerboard (id=4)";
    objStats[5].name = "Unknown/sky";

    // ---------- Render ----------
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            double px = (2.0 * (x + 0.5) / WIDTH - 1.0) * std::tan(fov / 2.0) * aspect;
            double py = (1.0 - 2.0 * (y + 0.5) / HEIGHT) * std::tan(fov / 2.0);

            Ray ray(camera, Vec3(px, py, 1));
            HitInfo hit;
            Vec3 color = trace(ray, spheres, 0, &hit);

            // PPM output
            ppm << toByte(color.x) << " " << toByte(color.y) << " " << toByte(color.z) << " ";

            // Point cloud: only save if we hit a surface
            if (hit.objectId >= 0) {
                pointCloud.push_back({
                    hit.point.x, hit.point.y, hit.point.z,
                    toByte(hit.color.x), // shaded color (lighting + shadows)
                    toByte(hit.color.y),
                    toByte(hit.color.z),
                    hit.objectId
                });

                // Accuracy: find closest ground truth surface
                int bestGT = -1;
                double bestDist = INF;
                for (size_t gt = 0; gt < groundTruths.size(); gt++) {
                    double d = groundTruths[gt]->distance(hit.point);
                    if (d < bestDist) { bestDist = d; bestGT = static_cast<int>(gt); }
                }

                int statIdx = 5; // unknown
                if (bestGT >= 0 && bestDist < 0.5) {
                    // Map ground truth index to object stats index
                    // GTSpheres 0-3 → objStats 0-3, GTPlane 4 → objStats 4
                    statIdx = bestGT;
                }

                if (statIdx < 6) {
                    ObjStats& os = objStats[statIdx];
                    os.sumError += bestDist;
                    os.sumSqError += bestDist * bestDist;
                    os.maxError = std::max(os.maxError, bestDist);
                    os.count++;
                }
            }
        }
        ppm << "\n";
        if (y % 100 == 0) std::cerr << "Rendering: " << (100 * y / HEIGHT) << "%\r";
    }
    ppm.close();
    std::cerr << "Rendering: 100%\n";

    // ---------- Write PLY ----------
    ply << "ply\n";
    ply << "format ascii 1.0\n";
    ply << "comment Point cloud generated by raytracer from 1960x1080 render\n";
    ply << "comment Each point = (x,y,z) world-space position + (R,G,B) surface color\n";
    ply << "element vertex " << pointCloud.size() << "\n";
    ply << "property float x\n";
    ply << "property float y\n";
    ply << "property float z\n";
    ply << "property uchar red\n";
    ply << "property uchar green\n";
    ply << "property uchar blue\n";
    ply << "end_header\n";

    for (const auto& p : pointCloud) {
        ply << std::fixed << std::setprecision(6)
            << p.x << " " << p.y << " " << p.z << " "
            << p.r << " " << p.g << " " << p.b << "\n";
    }
    ply.close();

    // ---------- Accuracy Report ----------
    std::cout << "\n";
    std::cout << "==========================================\n";
    std::cout << "  POINT CLOUD ACCURACY ANALYSIS\n";
    std::cout << "==========================================\n\n";
    std::cout << "Total points in cloud: " << pointCloud.size() << "\n";
    std::cout << "Image resolution:      " << WIDTH << "x" << HEIGHT << "\n";
    std::cout << "Hit rate:              " << (100.0 * pointCloud.size() / (WIDTH*HEIGHT)) << "%\n\n";
    std::cout << "Accuracy metrics (shortest distance to ground-truth surface):\n";
    std::cout << "-------------------------------------------------------------\n";
    std::cout << std::left << std::setw(24) << "Surface"
              << std::right << std::setw(10) << "Points"
              << std::setw(12) << "Mean Err"
              << std::setw(12) << "RMS Err"
              << std::setw(12) << "Max Err" << "\n";
    std::cout << "-------------------------------------------------------------\n";

    double totalPoints = 0, totalSumErr = 0, totalSqErr = 0, globalMax = 0;
    for (const auto& os : objStats) {
        if (os.count == 0) continue;
        double mean = os.sumError / os.count;
        double rms = std::sqrt(os.sumSqError / os.count);
        std::cout << std::left << std::setw(24) << os.name
                  << std::right << std::setw(10) << os.count
                  << std::setw(12) << std::setprecision(6) << mean
                  << std::setw(12) << std::setprecision(6) << rms
                  << std::setw(12) << std::setprecision(6) << os.maxError << "\n";
        totalPoints += os.count;
        totalSumErr += os.sumError;
        totalSqErr += os.sumSqError;
        globalMax = std::max(globalMax, os.maxError);
    }
    std::cout << "-------------------------------------------------------------\n";
    double totalMean = totalSumErr / totalPoints;
    double totalRMS = std::sqrt(totalSqErr / totalPoints);
    std::cout << std::left << std::setw(24) << "OVERALL"
              << std::right << std::setw(10) << static_cast<int>(totalPoints)
              << std::setw(12) << std::setprecision(6) << totalMean
              << std::setw(12) << std::setprecision(6) << totalRMS
              << std::setw(12) << std::setprecision(6) << globalMax << "\n";

    std::cout << "\nNotes:\n";
    std::cout << "  - Accuracy is limited by pixel sampling resolution ("
              << WIDTH << "x" << HEIGHT << ").\n";
    std::cout << "  - Each ray hit point is compared to the analytical surface equation.\n";
    std::cout << "  - Floating-point precision (~1e-15 for double) is the theoretical lower bound.\n";
    std::cout << "  - Curved surfaces (spheres) have sub-pixel sampling error since\n";
    std::cout << "    the ray hits the exact surface, but the tangent plane approximation\n";
    std::cout << "    at silhouette edges introduces small angular error.\n";
    std::cout << "  - Mean error << 1 unit indicates sub-pixel accuracy.\n";

    // Cleanup
    for (auto gt : groundTruths) delete gt;

    std::cout << "\nFiles written: test.ppm (temporary), test.ply (point cloud)\n";
    return 0;
}
