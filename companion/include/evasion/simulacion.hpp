#pragma once

#include "evasion/amenaza.hpp"
#include "evasion/planificador.hpp"
#include "evasion/vector.hpp"

namespace evasion {

// Escenario: el dron está en modo hover y alguien le lanza una piedra
// apuntando a donde está. Sirve para probar el núcleo sin hardware.
struct EscenarioLanzamiento {
    Vec3 posicion_dron{0.0, 0.0, -15.0};      // NED: 15 m sobre el suelo
    Vec3 posicion_lanzador{12.0, 4.0, -1.5};  // altura del brazo del lanzador
    double tiempo_de_vuelo_s{1.2};            // el lanzador apunta para impactar en este tiempo
    double diametro_piedra_m{0.08};
    double frecuencia_camara_hz{30.0};
    double ruido_angular_rad{0.002};
    double ruido_tamano_relativo{0.05};
    double alcance_deteccion_m{30.0};          // más lejos la piedra ocupa muy pocos píxeles
    double latencia_actuacion_s{0.15};         // del comando al inicio de la respuesta
    double constante_tiempo_dron_s{0.3};       // rapidez con que el dron sigue la velocidad pedida
    double semienvergadura_m{0.6};             // a menos de esto hay impacto
    bool con_profundidad{false};               // si hay medida directa de distancia
    unsigned semilla{42};
};

struct ResultadoSimulacion {
    double distancia_minima_m{0.0};
    bool impacto{false};
    bool evasion_activada{false};
    double tiempo_primera_alerta_s{-1.0};
    double velocidad_lanzamiento_ms{0.0};
};

ResultadoSimulacion simular_lanzamiento(const EscenarioLanzamiento& escenario, bool evasion_habilitada,
                                        const ConfigAmenaza& config_amenaza = {},
                                        const ConfigEvasion& config_evasion = {});

}  // namespace evasion
