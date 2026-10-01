// Simula piedras lanzadas contra el dron en hover y compara con/sin evasión.
// Uso: ./simular_piedra [lanzamientos] [alcance_deteccion_m] [latencia_s]
//
// La detección de la simulación es idealizada (la piedra se ve en todos los
// fotogramas, en cualquier dirección). Baje el alcance de detección y suba la
// latencia para ver cómo se degrada el resultado en condiciones reales.

#include <cstdio>
#include <cstdlib>
#include <random>

#include "evasion/simulacion.hpp"

int main(int argc, char** argv) {
    using namespace evasion;
    const int lanzamientos = argc > 1 ? std::atoi(argv[1]) : 200;
    const double alcance_m = argc > 2 ? std::atof(argv[2]) : 30.0;
    const double latencia_s = argc > 3 ? std::atof(argv[3]) : 0.15;

    std::mt19937 generador(7);
    std::uniform_real_distribution<double> distancia_horizontal(6.0, 25.0);
    std::uniform_real_distribution<double> angulo(0.0, 6.283185307);
    std::uniform_real_distribution<double> altura(8.0, 25.0);
    std::uniform_real_distribution<double> tiempo(0.8, 1.6);

    int impactos_sin = 0;
    int impactos_con = 0;
    int detectadas = 0;
    for (int i = 0; i < lanzamientos; ++i) {
        EscenarioLanzamiento esc;
        const double r = distancia_horizontal(generador);
        const double a = angulo(generador);
        esc.posicion_dron = {0.0, 0.0, -altura(generador)};
        esc.posicion_lanzador = {r * std::cos(a), r * std::sin(a), -1.5};
        esc.tiempo_de_vuelo_s = tiempo(generador);
        esc.semilla = static_cast<unsigned>(i + 1);
        esc.alcance_deteccion_m = alcance_m;
        esc.latencia_actuacion_s = latencia_s;

        const ResultadoSimulacion sin = simular_lanzamiento(esc, false);
        const ResultadoSimulacion con = simular_lanzamiento(esc, true);
        impactos_sin += sin.impacto ? 1 : 0;
        impactos_con += con.impacto ? 1 : 0;
        detectadas += con.evasion_activada ? 1 : 0;
        if (i < 5) {
            std::printf("Lanzamiento %d: %.1f m/s, alerta en t=%.2f s, paso sin evasión %.2f m, con evasión %.2f m\n",
                        i + 1, con.velocidad_lanzamiento_ms, con.tiempo_primera_alerta_s,
                        sin.distancia_minima_m, con.distancia_minima_m);
        }
    }
    std::printf("\n%d lanzamientos dirigidos al dron en hover (8-25 m de altura)\n", lanzamientos);
    std::printf("  Alcance de detección %.0f m, latencia de actuación %.2f s\n", alcance_m, latencia_s);
    std::printf("  Amenazas detectadas:   %d\n", detectadas);
    std::printf("  Impactos sin evasión:  %d\n", impactos_sin);
    std::printf("  Impactos con evasión:  %d\n", impactos_con);
    return 0;
}
