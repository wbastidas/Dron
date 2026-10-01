#pragma once

#include <cmath>

namespace evasion {

// Vector 3D. Por convención todo el núcleo trabaja en marco NED centrado en el
// dron (x = norte, y = este, z = abajo), ya compensado por la actitud.
struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double k) const { return {x * k, y * k, z * k}; }
    Vec3 operator/(double k) const { return {x / k, y / k, z / k}; }
    Vec3& operator+=(const Vec3& o) {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }

    double punto(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    double norma() const { return std::sqrt(punto(*this)); }

    // Devuelve el vector unitario, o el vector nulo si la norma es ~0.
    Vec3 unitario() const {
        const double n = norma();
        return n > 1e-12 ? *this / n : Vec3{};
    }

    // Componente perpendicular a la dirección unitaria `u`.
    Vec3 perpendicular_a(const Vec3& u) const { return *this - u * punto(u); }
};

inline Vec3 operator*(double k, const Vec3& v) { return v * k; }

constexpr double kGravedad = 9.80665;  // m/s², hacia abajo (+z en NED)

}  // namespace evasion
