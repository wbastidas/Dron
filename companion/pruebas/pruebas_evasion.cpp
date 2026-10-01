// Pruebas unitarias del núcleo de evasión. Arnés mínimo sin dependencias.

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "evasion/amenaza.hpp"
#include "evasion/planificador.hpp"
#include "evasion/simulacion.hpp"

using namespace evasion;

namespace {

int fallos = 0;

void comprobar(bool condicion, const char* expresion, const char* archivo, int linea) {
    if (!condicion) {
        ++fallos;
        std::printf("  FALLO %s:%d: %s\n", archivo, linea, expresion);
    }
}

#define COMPROBAR(x) comprobar((x), #x, __FILE__, __LINE__)
#define CERCA(a, b, tol) comprobar(std::fabs((a) - (b)) <= (tol), #a " ~= " #b, __FILE__, __LINE__)

// Alimenta al evaluador con un objeto en movimiento rectilíneo uniforme.
Amenaza seguir_objeto(EvaluadorAmenaza& ev, Vec3 inicio, Vec3 velocidad, int muestras, double dt = 1.0 / 30.0) {
    Amenaza a;
    for (int i = 0; i < muestras; ++i) {
        const double t = i * dt;
        const Vec3 p = inicio + velocidad * t;
        Observacion obs;
        obs.t_s = t;
        obs.direccion = p.unitario();
        obs.distancia_m = p.norma();
        a = ev.actualizar(obs);
    }
    return a;
}

ConfigAmenaza sin_gravedad() {
    ConfigAmenaza c;
    c.considerar_gravedad = false;
    return c;
}

void prueba_vector() {
    const Vec3 a{3.0, 4.0, 0.0};
    CERCA(a.norma(), 5.0, 1e-12);
    CERCA(a.unitario().norma(), 1.0, 1e-12);
    COMPROBAR(Vec3{}.unitario().norma() == 0.0);
    const Vec3 perp = Vec3{1.0, 1.0, 0.0}.perpendicular_a(Vec3{1.0, 0.0, 0.0});
    CERCA(perp.x, 0.0, 1e-12);
    CERCA(perp.y, 1.0, 1e-12);
}

void prueba_objeto_directo_es_peligro() {
    EvaluadorAmenaza ev(sin_gravedad());
    const Amenaza a = seguir_objeto(ev, {20.0, 0.0, 0.0}, {-20.0, 0.0, 0.0}, 6);
    COMPROBAR(a.peligro);
    // Tras 5 intervalos de 1/30 s el objeto está a 16.67 m y llega en ~0.83 s.
    CERCA(a.tiempo_al_cruce_s, 0.833, 0.01);
    CERCA(a.distancia_de_paso_m, 0.0, 0.05);
}

void prueba_objeto_lateral_no_es_peligro() {
    EvaluadorAmenaza ev(sin_gravedad());
    const Amenaza a = seguir_objeto(ev, {20.0, 5.0, 0.0}, {-20.0, 0.0, 0.0}, 6);
    COMPROBAR(!a.peligro);
    CERCA(a.distancia_de_paso_m, 5.0, 0.05);
    CERCA(a.lado_de_paso.y, 1.0, 0.01);
}

void prueba_objeto_que_se_aleja() {
    EvaluadorAmenaza ev(sin_gravedad());
    const Amenaza a = seguir_objeto(ev, {5.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, 6);
    COMPROBAR(!a.peligro);
    COMPROBAR(std::isinf(a.tiempo_al_cruce_s));
}

void prueba_pocas_muestras() {
    EvaluadorAmenaza ev(sin_gravedad());
    const Amenaza a = seguir_objeto(ev, {20.0, 0.0, 0.0}, {-20.0, 0.0, 0.0}, 3);
    COMPROBAR(!a.peligro);
}

void prueba_lejano_fuera_de_horizonte() {
    EvaluadorAmenaza ev(sin_gravedad());
    const Amenaza a = seguir_objeto(ev, {100.0, 0.0, 0.0}, {-20.0, 0.0, 0.0}, 6);
    COMPROBAR(!a.peligro);
}

void prueba_parabola_con_gravedad() {
    // Una piedra lanzada desde abajo que, por la gravedad, se queda corta.
    EvaluadorAmenaza ev;
    Amenaza a;
    const Vec3 inicio{10.0, 0.0, 14.0};    // 14 m por debajo del dron
    const Vec3 velocidad{-8.0, 0.0, -14.0}; // subiendo, pero sin fuerza suficiente
    for (int i = 0; i < 6; ++i) {
        const double t = i / 30.0;
        const Vec3 p = inicio + velocidad * t + Vec3{0.0, 0.0, kGravedad} * (0.5 * t * t);
        Observacion obs;
        obs.t_s = t;
        obs.direccion = p.unitario();
        obs.distancia_m = p.norma();
        a = ev.actualizar(obs);
    }
    COMPROBAR(!a.peligro);
    COMPROBAR(a.distancia_de_paso_m > 3.0);
}

void prueba_tiempo_que_retrocede_reinicia() {
    EvaluadorAmenaza ev(sin_gravedad());
    seguir_objeto(ev, {20.0, 0.0, 0.0}, {-20.0, 0.0, 0.0}, 6);
    Observacion obs;
    obs.t_s = 0.0;
    obs.direccion = {1.0, 0.0, 0.0};
    obs.distancia_m = 10.0;
    COMPROBAR(!ev.actualizar(obs).peligro);
}

void prueba_observacion_invalida() {
    EvaluadorAmenaza ev;
    Observacion obs;
    obs.t_s = 0.0;
    obs.direccion = {1.0, 0.0, 0.0};
    obs.tamano_angular_rad = 0.0;  // sin tamaño ni distancia
    COMPROBAR(std::isinf(ev.actualizar(obs).distancia_actual_m));
}

void prueba_planificador_inactivo_sin_peligro() {
    PlanificadorEvasion plan;
    COMPROBAR(!plan.planificar(Amenaza{}, {ModoVuelo::Hover, 20.0}).activo);
}

void prueba_planificador_directo_sube() {
    PlanificadorEvasion plan;
    Amenaza a;
    a.peligro = true;
    a.direccion = {1.0, 0.0, 0.0};
    a.distancia_de_paso_m = 0.0;
    a.tiempo_al_cruce_s = 1.0;
    const ComandoEvasion c = plan.planificar(a, {ModoVuelo::Hover, 20.0});
    COMPROBAR(c.activo);
    CERCA(c.direccion.z, -1.0, 1e-9);  // hacia arriba en NED
    CERCA(c.direccion.punto(a.direccion), 0.0, 1e-9);
    COMPROBAR(c.alcanzable);
    // 1.7 m en 0.75 s útiles con perfil triangular -> 4.53 m/s
    CERCA(c.velocidad_ms, 4.533, 0.01);
}

void prueba_planificador_se_aleja_del_lado_de_paso() {
    PlanificadorEvasion plan;
    Amenaza a;
    a.peligro = true;
    a.direccion = {1.0, 0.0, 0.0};
    a.distancia_de_paso_m = 0.6;
    a.lado_de_paso = {0.0, 1.0, 0.0};  // pasará por la derecha (este)
    a.tiempo_al_cruce_s = 1.0;
    const ComandoEvasion c = plan.planificar(a, {ModoVuelo::Hover, 20.0});
    CERCA(c.direccion.y, -1.0, 1e-9);
}

void prueba_planificador_no_baja_cerca_del_suelo() {
    PlanificadorEvasion plan;
    Amenaza a;
    a.peligro = true;
    a.direccion = {1.0, 0.0, 0.0};
    a.distancia_de_paso_m = 0.5;
    a.lado_de_paso = {0.0, 0.0, -1.0};  // pasará por encima: lo natural sería bajar
    a.tiempo_al_cruce_s = 1.0;
    const ComandoEvasion bajo = plan.planificar(a, {ModoVuelo::Hover, 4.0});
    COMPROBAR(bajo.direccion.z <= 0.0);
    CERCA(bajo.direccion.norma(), 1.0, 1e-9);
    const ComandoEvasion alto = plan.planificar(a, {ModoVuelo::Hover, 30.0});
    CERCA(alto.direccion.z, 1.0, 1e-9);
    const ComandoEvasion crucero = plan.planificar(a, {ModoVuelo::Crucero, 30.0});
    COMPROBAR(crucero.direccion.z <= 0.0);
}

void prueba_planificador_sin_tiempo() {
    PlanificadorEvasion plan;
    Amenaza a;
    a.peligro = true;
    a.direccion = {1.0, 0.0, 0.0};
    a.tiempo_al_cruce_s = 0.1;  // menor que la latencia
    const ComandoEvasion c = plan.planificar(a, {ModoVuelo::Hover, 20.0});
    COMPROBAR(c.activo);
    COMPROBAR(!c.alcanzable);
    CERCA(c.velocidad_ms, plan.config().velocidad_maxima_hover_ms, 1e-9);
}

void prueba_simulacion_escenario_base() {
    EscenarioLanzamiento esc;
    const ResultadoSimulacion sin = simular_lanzamiento(esc, false);
    const ResultadoSimulacion con = simular_lanzamiento(esc, true);
    COMPROBAR(sin.impacto);
    COMPROBAR(con.evasion_activada);
    COMPROBAR(!con.impacto);
    COMPROBAR(con.distancia_minima_m > 1.0);
}

void prueba_simulacion_reduce_impactos() {
    int sin = 0;
    int con = 0;
    for (unsigned semilla = 1; semilla <= 40; ++semilla) {
        EscenarioLanzamiento esc;
        esc.semilla = semilla;
        esc.posicion_lanzador = {6.0 + (semilla % 7) * 2.5, -8.0 + (semilla % 5) * 4.0, -1.5};
        esc.tiempo_de_vuelo_s = 0.9 + (semilla % 4) * 0.2;
        sin += simular_lanzamiento(esc, false).impacto ? 1 : 0;
        con += simular_lanzamiento(esc, true).impacto ? 1 : 0;
    }
    std::printf("  (simulación: %d impactos sin evasión, %d con evasión, de 40)\n", sin, con);
    COMPROBAR(sin >= 35);
    COMPROBAR(con <= 4);
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> pruebas = {
        {"vector", prueba_vector},
        {"objeto directo es peligro", prueba_objeto_directo_es_peligro},
        {"objeto lateral no es peligro", prueba_objeto_lateral_no_es_peligro},
        {"objeto que se aleja", prueba_objeto_que_se_aleja},
        {"pocas muestras", prueba_pocas_muestras},
        {"lejano fuera de horizonte", prueba_lejano_fuera_de_horizonte},
        {"parábola con gravedad", prueba_parabola_con_gravedad},
        {"tiempo que retrocede reinicia", prueba_tiempo_que_retrocede_reinicia},
        {"observación inválida", prueba_observacion_invalida},
        {"planificador inactivo sin peligro", prueba_planificador_inactivo_sin_peligro},
        {"planificador: amenaza directa -> sube", prueba_planificador_directo_sube},
        {"planificador: se aleja del lado de paso", prueba_planificador_se_aleja_del_lado_de_paso},
        {"planificador: no baja cerca del suelo", prueba_planificador_no_baja_cerca_del_suelo},
        {"planificador: sin tiempo", prueba_planificador_sin_tiempo},
        {"simulación: escenario base", prueba_simulacion_escenario_base},
        {"simulación: reduce impactos", prueba_simulacion_reduce_impactos},
    };
    for (const auto& [nombre, prueba] : pruebas) {
        const int antes = fallos;
        prueba();
        std::printf("[%s] %s\n", fallos == antes ? " OK " : "FALLO", nombre.c_str());
    }
    std::printf("\n%s (%d fallos)\n", fallos == 0 ? "TODAS LAS PRUEBAS PASARON" : "HAY FALLOS", fallos);
    return fallos == 0 ? 0 : 1;
}
