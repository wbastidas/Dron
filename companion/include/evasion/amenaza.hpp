#pragma once

#include <cstddef>
#include <deque>
#include <limits>
#include <optional>

#include "evasion/vector.hpp"

namespace evasion {

// Una detección de un objeto rastreado (por ejemplo una piedra) en un instante.
// La produce la etapa de visión: detector neuronal + rastreador.
struct Observacion {
    double t_s{0.0};
    Vec3 direccion;                     // unitario dron -> objeto, marco NED
    double tamano_angular_rad{0.0};     // diámetro aparente del objeto
    std::optional<double> distancia_m;  // si hay cámara de profundidad / lidar
};

struct ConfigAmenaza {
    std::size_t muestras_minimas{4};
    std::size_t muestras_maximas{10};
    // Diámetro asumido cuando no hay medida de distancia. Un valor menor que el
    // real hace la estimación más conservadora: el objeto parece más cercano y
    // la distancia de paso estimada sale menor de lo que es.
    double tamano_supuesto_m{0.06};
    // Semienvergadura del dron más margen: dentro de este radio hay impacto.
    double radio_seguridad_m{1.2};
    // Solo se consideran amenazas que llegan antes de este horizonte.
    double horizonte_s{2.5};
    // Los objetos lanzados siguen una parábola: se modela la gravedad.
    bool considerar_gravedad{true};
};

struct Amenaza {
    bool peligro{false};
    double tiempo_al_cruce_s{std::numeric_limits<double>::infinity()};
    double distancia_actual_m{std::numeric_limits<double>::infinity()};
    double distancia_de_paso_m{std::numeric_limits<double>::infinity()};
    Vec3 direccion;    // dirección actual dron -> objeto
    Vec3 lado_de_paso; // unitario hacia donde pasará el objeto en el punto más cercano
};

// Estima trayectoria y punto de máximo acercamiento de un objeto a partir de
// sus observaciones recientes (mínimos cuadrados sobre una ventana deslizante).
class EvaluadorAmenaza {
public:
    explicit EvaluadorAmenaza(ConfigAmenaza config = {});

    Amenaza actualizar(const Observacion& obs);
    void reiniciar();

    const ConfigAmenaza& config() const { return config_; }

private:
    double distancia_de(const Observacion& obs) const;

    ConfigAmenaza config_;
    std::deque<Observacion> historial_;
};

}  // namespace evasion
