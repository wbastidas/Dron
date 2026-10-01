#include "evasion/simulacion.hpp"

#include <cmath>
#include <deque>
#include <random>
#include <utility>

namespace evasion {

namespace {

constexpr double kPaso_s = 0.001;
const Vec3 kGravedadNed{0.0, 0.0, kGravedad};

}  // namespace

ResultadoSimulacion simular_lanzamiento(const EscenarioLanzamiento& esc, bool evasion_habilitada,
                                        const ConfigAmenaza& config_amenaza,
                                        const ConfigEvasion& config_evasion) {
    std::mt19937 generador(esc.semilla);
    std::normal_distribution<double> normal(0.0, 1.0);

    // Velocidad inicial para que la parábola pase por la posición del dron.
    const double T = esc.tiempo_de_vuelo_s;
    Vec3 piedra = esc.posicion_lanzador;
    Vec3 v_piedra = (esc.posicion_dron - piedra - kGravedadNed * (0.5 * T * T)) / T;

    Vec3 dron = esc.posicion_dron;
    Vec3 v_dron;
    Vec3 v_ordenada;
    double fin_orden_s = 0.0;
    std::deque<std::pair<double, Vec3>> ordenes_en_camino;  // modela la latencia de actuación

    EvaluadorAmenaza evaluador(config_amenaza);
    PlanificadorEvasion planificador(config_evasion);

    ResultadoSimulacion res;
    res.velocidad_lanzamiento_ms = v_piedra.norma();
    res.distancia_minima_m = (piedra - dron).norma();

    const double periodo_camara = 1.0 / esc.frecuencia_camara_hz;
    double proximo_fotograma = 0.0;

    for (double t = 0.0; t <= T + 1.0; t += kPaso_s) {
        if (t >= proximo_fotograma) {
            proximo_fotograma += periodo_camara;
            const Vec3 relativa = piedra - dron;
            const double distancia = relativa.norma();
            if (distancia < esc.alcance_deteccion_m && distancia > 0.0) {
                Observacion obs;
                obs.t_s = t;
                const Vec3 ruido{normal(generador), normal(generador), normal(generador)};
                obs.direccion = (relativa.unitario() + ruido * esc.ruido_angular_rad).unitario();
                obs.tamano_angular_rad = 2.0 * std::atan(esc.diametro_piedra_m / (2.0 * distancia)) *
                                         (1.0 + esc.ruido_tamano_relativo * normal(generador));
                if (esc.con_profundidad) {
                    obs.distancia_m = distancia * (1.0 + 0.02 * normal(generador));
                }
                const Amenaza amenaza = evaluador.actualizar(obs);
                const EstadoVehiculo estado{ModoVuelo::Hover, -dron.z};
                const ComandoEvasion cmd = planificador.planificar(amenaza, estado);
                if (cmd.activo) {
                    if (!res.evasion_activada) {
                        res.tiempo_primera_alerta_s = t;
                    }
                    res.evasion_activada = true;
                    if (evasion_habilitada) {
                        ordenes_en_camino.emplace_back(t + esc.latencia_actuacion_s,
                                                       cmd.direccion * cmd.velocidad_ms);
                        fin_orden_s = t + esc.latencia_actuacion_s + cmd.duracion_s;
                    }
                }
            }
        }

        while (!ordenes_en_camino.empty() && ordenes_en_camino.front().first <= t) {
            v_ordenada = ordenes_en_camino.front().second;
            ordenes_en_camino.pop_front();
        }
        if (t > fin_orden_s) {
            v_ordenada = Vec3{};
        }

        v_dron += (v_ordenada - v_dron) * (kPaso_s / esc.constante_tiempo_dron_s);
        dron += v_dron * kPaso_s;
        v_piedra += kGravedadNed * kPaso_s;
        piedra += v_piedra * kPaso_s;

        const double d = (piedra - dron).norma();
        if (d < res.distancia_minima_m) {
            res.distancia_minima_m = d;
        }
    }
    res.impacto = res.distancia_minima_m < esc.semienvergadura_m;
    return res;
}

}  // namespace evasion
