#pragma once

#include "evasion/amenaza.hpp"
#include "evasion/vector.hpp"

namespace evasion {

enum class ModoVuelo {
    Hover,    // modo multirrotor (despegue, aterrizaje, QLOITER): puede desplazarse en cualquier eje
    Crucero,  // ala fija: solo puede virar y trepar
};

struct EstadoVehiculo {
    ModoVuelo modo{ModoVuelo::Hover};
    double altura_sobre_suelo_m{0.0};
};

struct ConfigEvasion {
    double velocidad_maxima_hover_ms{8.0};
    // Cámara + inferencia + enlace MAVLink + respuesta del autopiloto.
    double latencia_s{0.25};
    // Nunca se ordena descender por debajo de esta altura.
    double altura_minima_m{6.0};
    // Distancia extra sobre el radio de seguridad que se busca ganar.
    double margen_m{0.5};
    double radio_seguridad_m{1.2};
};

struct ComandoEvasion {
    bool activo{false};
    Vec3 direccion;            // unitario, marco NED
    double velocidad_ms{0.0};  // en Hover: velocidad a mantener durante `duracion_s`
    double duracion_s{0.0};
    bool alcanzable{false};    // false: no hay tiempo físico para esquivar del todo
};

// Decide hacia dónde y a qué velocidad apartarse de una amenaza.
class PlanificadorEvasion {
public:
    explicit PlanificadorEvasion(ConfigEvasion config = {});

    ComandoEvasion planificar(const Amenaza& amenaza, const EstadoVehiculo& estado) const;

    const ConfigEvasion& config() const { return config_; }

private:
    Vec3 direccion_de_escape(const Amenaza& amenaza, const EstadoVehiculo& estado) const;

    ConfigEvasion config_;
};

}  // namespace evasion
