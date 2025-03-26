#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>

struct Vec3 {
    float x, y, z;
    Vec3(float x_ = 0, float y_ = 0, float z_ = 0) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3 normalize() const {
        float mag = std::sqrt(x * x + y * y + z * z);
        return Vec3(x / mag, y / mag, z / mag);
    }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
};

struct Ray {
    Vec3 origin, direction;
    Ray(Vec3 o, Vec3 d) : origin(o), direction(d.normalize()) {}
};

struct Material {
    Vec3 color;
    float reflectivity;
    float refractivity;
    float ior;
};

struct Sphere {
    Vec3 center;
    float radius;
    Material material;
};

struct Plane {
    Vec3 point;
    Vec3 normal;
    Material material;
};

struct Light {
    Vec3 position;
    float intensity;
};

bool intersectSphere(const Ray& ray, const Sphere& sphere, float& t) {
    Vec3 oc = ray.origin - sphere.center;
    float a = ray.direction.dot(ray.direction);
    float b = 2.0f * oc.dot(ray.direction);
    float c = oc.dot(oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false;
    t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    return t > 0;
}

bool intersectPlane(const Ray& ray, const Plane& plane, float& t) {
    float denom = plane.normal.dot(ray.direction);
    if (std::abs(denom) > 1e-6) {
        t = (plane.point - ray.origin).dot(plane.normal) / denom;
        return t > 0;
    }
    return false;
}

Vec3 reflect(const Vec3& I, const Vec3& N) {
    return I - 2 * I.dot(N) * N;
}

bool inShadow(const Vec3& point, const Vec3& lightPos, const std::vector<Sphere>& spheres, const std::vector<Plane>& planes) {
    Vec3 lightDir = (lightPos - point).normalize();
    Ray shadowRay(point + lightDir * 0.001f, lightDir);
    float lightDist = (lightPos - point).length();
    for (const auto& sphere : spheres) {
        float t;
        if (intersectSphere(shadowRay, sphere, t) && t < lightDist) return true;
    }
    for (const auto& plane : planes) {
        float t;
        if (intersectPlane(shadowRay, plane, t) && t < lightDist) return true;
    }
    return false;
}

Vec3 trace(const Ray& ray, const std::vector<Sphere>& spheres, const std::vector<Plane>& planes, const std::vector<Light>& lights, int depth) {
    if (depth > 5) return Vec3(0, 0, 0);
    float tMin = 1e6;
    const Sphere* hitSphere = nullptr;
    const Plane* hitPlane = nullptr;
    for (const auto& sphere : spheres) {
        float t;
        if (intersectSphere(ray, sphere, t) && t < tMin) {
            tMin = t;
            hitSphere = &sphere;
            hitPlane = nullptr;
        }
    }
    for (const auto& plane : planes) {
        float t;
        if (intersectPlane(ray, plane, t) && t < tMin) {
            tMin = t;
            hitPlane = &plane;
            hitSphere = nullptr;
        }
    }
    if (hitSphere) {
        Vec3 hitPoint = ray.origin + ray.direction * tMin;
        Vec3 normal = (hitPoint - hitSphere->center).normalize();
        const Material& material = hitSphere->material;
        Vec3 localColor(0, 0, 0);
        for (const auto& light : lights) {
            if (!inShadow(hitPoint, light.position, spheres, planes)) {
                Vec3 lightDir = (light.position - hitPoint).normalize();
                float diffuse = std::max(0.0f, normal.dot(lightDir));
                localColor = localColor + material.color * diffuse * light.intensity;
            }
        }
        if (material.reflectivity > 0) {
            Vec3 reflectDir = reflect(ray.direction, normal).normalize();
            Ray reflectRay(hitPoint + normal * 0.001f, reflectDir);
            Vec3 reflectColor = trace(reflectRay, spheres, planes, lights, depth + 1);
            return localColor * (1 - material.reflectivity) + reflectColor * material.reflectivity;
        }
        return localColor;
    }
    if (hitPlane) {
        Vec3 hitPoint = ray.origin + ray.direction * tMin;
        Vec3 normal = hitPlane->normal;
        const Material& material = hitPlane->material;
        int ix = std::floor(hitPoint.x);
        int iz = std::floor(hitPoint.z);
        Vec3 patternColor = ((ix + iz) % 2 == 0) ? Vec3(1, 1, 1) : Vec3(0.5, 0.5, 0.5);
        Vec3 localColor(0, 0, 0);
        for (const auto& light : lights) {
            if (!inShadow(hitPoint, light.position, spheres, planes)) {
                Vec3 lightDir = (light.position - hitPoint).normalize();
                float diffuse = std::max(0.0f, normal.dot(lightDir));
                localColor = localColor + patternColor * diffuse * light.intensity;
            }
        }
        if (material.reflectivity > 0) {
            Vec3 reflectDir = reflect(ray.direction, normal).normalize();
            Ray reflectRay(hitPoint + normal * 0.001f, reflectDir);
            Vec3 reflectColor = trace(reflectRay, spheres, planes, lights, depth + 1);
            return localColor * (1 - material.reflectivity) + reflectColor * material.reflectivity;
        }
        return localColor;
    }
    return Vec3(0, 0, 0);
}

int main() {
    const int width = 800;
    const int height = 600;
    Vec3 cameraPos(0, 0, -5);
    float fov = 60.0f;
    float aspectRatio = static_cast<float>(width) / height;
    Material redMat = {Vec3(1, 0, 0), 0.5f, 0.0f, 1.0f};
    Material greenMat = {Vec3(0, 1, 0), 0.2f, 0.0f, 1.0f};
    Material blueMat = {Vec3(0, 0, 1), 0.2f, 0.0f, 1.0f};
    Material mirrorMat = {Vec3(0.8, 0.8, 0.8), 0.9f, 0.0f, 1.0f};
    Material floorMat = {Vec3(1, 1, 1), 0.3f, 0.0f, 1.0f};
    std::vector<Sphere> spheres = {
        Sphere(Vec3(-1, 0, 0), 1.0f, redMat),
        Sphere(Vec3(1, 0, 0), 1.0f, greenMat),
        Sphere(Vec3(0, 0, 2), 0.5f, blueMat),
        Sphere(Vec3(0, 1, 0), 0.3f, mirrorMat)
    };
    std::vector<Plane> planes = {Plane(Vec3(0, -1, 0), Vec3(0, 1, 0), floorMat)};
    std::vector<Light> lights = {
        Light{Vec3(5, 5, -5), 1.0f},
        Light{Vec3(-5, 5, -5), 1.0f}
    };
    std::ofstream out("output.ppm");
    out << "P3\n" << width << " " << height << "\n255\n";
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Vec3 color(0, 0, 0);
            float offsets[2] = {0.25f, 0.75f};
            for (int i = 0; i < 2; ++i) {
                for (int j = 0; j < 2; ++j) {
                    float u = (2.0f * (x + offsets[i]) / width - 1.0f) * aspectRatio * tan(fov / 2 * M_PI / 180);
                    float v = (1.0f - 2.0f * (y + offsets[j]) / height) * tan(fov / 2 * M_PI / 180);
                    Vec3 rayDir(u, v, 1);
                    Ray ray(cameraPos, rayDir);
                    color = color + trace(ray, spheres, planes, lights, 0);
                }
            }
            color = color * (1.0f / 4.0f);
            int r = static_cast<int>(color.x * 255);
            int g = static_cast<int>(color.y * 255);
            int b = static_cast<int>(color.z * 255);
            out << r << " " << g << " " << b << " ";
        }
        out << "\n";
    }
    out.close();
    std::cout << "Rendering complete! Check output.ppm\n";
    return 0;
}