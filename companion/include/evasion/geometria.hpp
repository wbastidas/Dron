#pragma once

#include <cmath>
#include <optional>
#include <utility>

#include "evasion/vector.hpp"

namespace evasion {

struct Mat3 {
    double m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    Vec3 operator*(const Vec3& v) const {
        return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
                m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z};
    }

    Mat3 operator*(const Mat3& o) const {
        Mat3 r;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                r.m[i][j] = m[i][0] * o.m[0][j] + m[i][1] * o.m[1][j] + m[i][2] * o.m[2][j];
            }
        }
        return r;
    }

    Mat3 transpuesta() const {
        Mat3 r;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                r.m[i][j] = m[j][i];
            }
        }
        return r;
    }
};

// Actitud del avión como la publica ArduPilot (mensaje ATTITUDE): ángulos de
// Euler en radianes. Cuerpo FRD (x adelante, y derecha, z abajo); pitch positivo
// es nariz arriba.
struct Actitud {
    double roll_rad{0.0};
    double pitch_rad{0.0};
    double yaw_rad{0.0};
};

inline Mat3 rotacion_x(double a) {
    const double c = std::cos(a), s = std::sin(a);
    return Mat3{{{1, 0, 0}, {0, c, -s}, {0, s, c}}};
}

inline Mat3 rotacion_y(double a) {
    const double c = std::cos(a), s = std::sin(a);
    return Mat3{{{c, 0, s}, {0, 1, 0}, {-s, 0, c}}};
}

inline Mat3 rotacion_z(double a) {
    const double c = std::cos(a), s = std::sin(a);
    return Mat3{{{c, -s, 0}, {s, c, 0}, {0, 0, 1}}};
}

// Rotación del marco del cuerpo (FRD) al marco NED: R = Rz(yaw) · Ry(pitch) · Rx(roll).
inline Mat3 cuerpo_a_ned(const Actitud& a) {
    return rotacion_z(a.yaw_rad) * rotacion_y(a.pitch_rad) * rotacion_x(a.roll_rad);
}

// Cámara con el modelo de agujero (pinhole), sin distorsión. Marco óptico:
// x a la derecha de la imagen, y hacia abajo, z hacia adelante. Los valores por
// defecto aproximan una Raspberry Pi Global Shutter Camera (1456 × 1088) con un
// lente gran angular de ~2.8 mm; calibre la cámara real con OpenCV.
struct ModeloCamara {
    double fx{800.0};
    double fy{800.0};
    double cx{728.0};
    double cy{544.0};
    int ancho_px{1456};
    int alto_px{1088};
    // Cuánto apunta la cámara por debajo del eje longitudinal del avión.
    double inclinacion_abajo_rad{0.0};

    // Rotación del marco óptico al marco del cuerpo.
    Mat3 camara_a_cuerpo() const {
        const Mat3 optico_a_frd{{{0, 0, 1}, {1, 0, 0}, {0, 1, 0}}};
        return rotacion_y(-inclinacion_abajo_rad) * optico_a_frd;
    }

    // Dirección unitaria en NED hacia el píxel (u, v) dada la actitud.
    Vec3 direccion_ned(double u, double v, const Actitud& actitud) const {
        const Vec3 rayo{(u - cx) / fx, (v - cy) / fy, 1.0};
        return (cuerpo_a_ned(actitud) * (camara_a_cuerpo() * rayo)).unitario();
    }

    // Píxel donde se ve la dirección NED dada, o nada si queda fuera de la imagen.
    std::optional<std::pair<double, double>> proyectar(const Vec3& direccion_ned, const Actitud& actitud) const {
        const Vec3 p = camara_a_cuerpo().transpuesta() * (cuerpo_a_ned(actitud).transpuesta() * direccion_ned);
        if (p.z <= 1e-9) {
            return std::nullopt;
        }
        const double u = fx * p.x / p.z + cx;
        const double v = fy * p.y / p.z + cy;
        if (u < 0.0 || v < 0.0 || u >= ancho_px || v >= alto_px) {
            return std::nullopt;
        }
        return std::make_pair(u, v);
    }

    // Diámetro angular de un objeto que ocupa `px` píxeles cerca del centro.
    double tamano_angular_rad(double px) const { return 2.0 * std::atan(px / (2.0 * fx)); }
};

}  // namespace evasion
