// Pruebas de geometría, percepción y supervisor. Arnés mínimo sin dependencias.

#include <cmath>
#include <cstdio>
#include <deque>
#include <functional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "evasion/geometria.hpp"
#include "evasion/percepcion.hpp"
#include "evasion/supervisor.hpp"

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

constexpr double kPi = 3.14159265358979323846;

// ---------------------------------------------------------------- geometría

void prueba_rotacion_yaw() {
    // Con yaw = 90° el frente del avión apunta al este.
    const Vec3 v = cuerpo_a_ned({0.0, 0.0, kPi / 2}) * Vec3{1.0, 0.0, 0.0};
    CERCA(v.x, 0.0, 1e-12);
    CERCA(v.y, 1.0, 1e-12);
}

void prueba_rotacion_pitch() {
    // Nariz arriba: el frente apunta hacia arriba, que en NED es z negativa.
    const Vec3 v = cuerpo_a_ned({0.0, kPi / 6, 0.0}) * Vec3{1.0, 0.0, 0.0};
    CERCA(v.z, -0.5, 1e-12);
}

void prueba_rotacion_roll() {
    // Alabeo a la derecha: el costado derecho baja.
    const Vec3 v = cuerpo_a_ned({kPi / 6, 0.0, 0.0}) * Vec3{0.0, 1.0, 0.0};
    CERCA(v.z, 0.5, 1e-12);
}

void prueba_camara_eje_optico() {
    ModeloCamara cam;
    // Sin inclinación el centro de la imagen mira al frente.
    const Vec3 al_frente = cam.direccion_ned(cam.cx, cam.cy, {});
    CERCA(al_frente.x, 1.0, 1e-12);
    // Inclinada 30° hacia abajo, el centro mira 30° bajo el horizonte.
    cam.inclinacion_abajo_rad = kPi / 6;
    const Vec3 abajo = cam.direccion_ned(cam.cx, cam.cy, {});
    CERCA(abajo.z, 0.5, 1e-12);
    CERCA(abajo.x, std::cos(kPi / 6), 1e-12);
}

void prueba_camara_lados_de_la_imagen() {
    ModeloCamara cam;
    // La derecha de la imagen apunta al lado derecho del avión (este con yaw 0 ... norte->este).
    COMPROBAR(cam.direccion_ned(cam.cx + 200, cam.cy, {}).y > 0.0);
    // La parte alta de la imagen apunta hacia arriba (z negativa).
    COMPROBAR(cam.direccion_ned(cam.cx, cam.cy - 200, {}).z < 0.0);
}

void prueba_proyeccion_ida_y_vuelta() {
    ModeloCamara cam;
    cam.inclinacion_abajo_rad = kPi / 6;
    const Actitud actitud{0.1, -0.05, 1.2};
    for (const auto& [u, v] : {std::pair{300.0, 200.0}, {728.0, 544.0}, {1200.0, 900.0}}) {
        const Vec3 d = cam.direccion_ned(u, v, actitud);
        const auto px = cam.proyectar(d, actitud);
        COMPROBAR(px.has_value());
        if (px) {
            CERCA(px->first, u, 1e-6);
            CERCA(px->second, v, 1e-6);
        }
    }
    // Lo que queda detrás del avión no se ve.
    COMPROBAR(!cam.proyectar(Vec3{-1.0, 0.0, 0.0}, {}).has_value());
}

// -------------------------------------------------------------- percepción

// Detección de una esfera de `diametro_m` en `pos` visto desde `dron`, o nada si no se ve.
std::optional<Deteccion> ver(const ModeloCamara& cam, const Actitud& act, const Vec3& dron, const Vec3& pos,
                             double diametro_m, std::mt19937* ruido = nullptr, double sigma_px = 0.0) {
    const Vec3 rel = pos - dron;
    const auto px = cam.proyectar(rel.unitario(), act);
    if (!px) {
        return std::nullopt;
    }
    std::normal_distribution<double> n(0.0, 1.0);
    auto r = [&] { return ruido ? n(*ruido) * sigma_px : 0.0; };
    Deteccion d;
    d.u_px = px->first + r();
    d.v_px = px->second + r();
    d.ancho_px = d.alto_px = cam.fx * diametro_m / rel.norma() + r();
    return d;
}

struct Lanzamiento {
    Vec3 inicio;
    Vec3 velocidad;
    Vec3 en(double t) const { return inicio + velocidad * t + Vec3{0.0, 0.0, kGravedad} * (0.5 * t * t); }
};

Lanzamiento lanzar_a(const Vec3& desde, const Vec3& objetivo, double tiempo_de_vuelo) {
    return {desde, (objetivo - desde - Vec3{0.0, 0.0, kGravedad} * (0.5 * tiempo_de_vuelo * tiempo_de_vuelo)) /
                       tiempo_de_vuelo};
}

ModeloCamara camara_de_pruebas() {
    ModeloCamara cam;
    cam.inclinacion_abajo_rad = kPi / 6;
    return cam;
}

void prueba_percepcion_detecta_piedra_dirigida() {
    const ModeloCamara cam = camara_de_pruebas();
    const Vec3 dron{0.0, 0.0, -15.0};
    const Vec3 lanzador{10.0, 6.0, -1.5};
    const double yaw = std::atan2(lanzador.y - dron.y, lanzador.x - dron.x);
    const Actitud actitud{0.0, 0.0, yaw};
    const Lanzamiento piedra = lanzar_a(lanzador, dron, 1.2);

    Percepcion percepcion(cam);
    double primera_alerta = -1.0;
    double tiempo_al_cruce = 0.0;
    for (int i = 0; i * (1.0 / 30.0) < 1.0; ++i) {
        const double t = i / 30.0;
        std::vector<Deteccion> dets;
        if (const auto d = ver(cam, actitud, dron, piedra.en(t), 0.08)) {
            dets.push_back(*d);
        }
        const ResultadoPercepcion r = percepcion.procesar(t, dets, actitud);
        if (r.critica && primera_alerta < 0.0) {
            primera_alerta = t;
            tiempo_al_cruce = r.critica->tiempo_al_cruce_s;
        }
    }
    COMPROBAR(primera_alerta >= 0.0);
    COMPROBAR(primera_alerta < 0.4);
    // En ese instante la piedra llega al dron en (1.2 - t) s.
    CERCA(tiempo_al_cruce, 1.2 - primera_alerta, 0.25);
}

void prueba_percepcion_funciona_con_cualquier_rumbo() {
    // Misma escena girada: el resultado no debe depender del rumbo del avión.
    const ModeloCamara cam = camara_de_pruebas();
    const Vec3 dron{0.0, 0.0, -15.0};
    for (double giro : {0.0, 1.0, 2.5, -2.0}) {
        const Vec3 lanzador{12.0 * std::cos(giro), 12.0 * std::sin(giro), -1.5};
        const Actitud actitud{0.0, 0.0, giro};
        const Lanzamiento piedra = lanzar_a(lanzador, dron, 1.2);
        Percepcion percepcion(cam);
        bool alerta = false;
        for (int i = 0; i < 24; ++i) {
            const double t = i / 30.0;
            std::vector<Deteccion> dets;
            if (const auto d = ver(cam, actitud, dron, piedra.en(t), 0.08)) {
                dets.push_back(*d);
            }
            alerta = alerta || percepcion.procesar(t, dets, actitud).critica.has_value();
        }
        COMPROBAR(alerta);
    }
}

void prueba_percepcion_ignora_objeto_que_pasa_lejos() {
    const ModeloCamara cam = camara_de_pruebas();
    const Vec3 dron{0.0, 0.0, -15.0};
    const Vec3 lanzador{10.0, 6.0, -1.5};
    const Actitud actitud{0.0, 0.0, std::atan2(6.0, 10.0)};
    // Apuntada a un punto 8 m a un lado del dron.
    const Lanzamiento piedra = lanzar_a(lanzador, dron + Vec3{-6.0, 6.0, 0.0}, 1.2);
    Percepcion percepcion(cam);
    int pistas_vistas = 0;
    for (int i = 0; i < 30; ++i) {
        const double t = i / 30.0;
        std::vector<Deteccion> dets;
        if (const auto d = ver(cam, actitud, dron, piedra.en(t), 0.08)) {
            dets.push_back(*d);
        }
        const ResultadoPercepcion r = percepcion.procesar(t, dets, actitud);
        COMPROBAR(!r.critica.has_value());
        pistas_vistas += static_cast<int>(r.pistas.size());
    }
    COMPROBAR(pistas_vistas > 0);  // la rastreó, pero no la consideró peligrosa
}

void prueba_percepcion_falsa_alarma_de_un_fotograma() {
    Percepcion percepcion(camara_de_pruebas());
    Deteccion falsa;
    falsa.u_px = 700;
    falsa.v_px = 500;
    falsa.ancho_px = falsa.alto_px = 40;  // grande y cercana: sería peligrosa si se creyera
    ResultadoPercepcion r = percepcion.procesar(0.0, {falsa}, {});
    COMPROBAR(r.pistas.empty() && !r.critica);
    r = percepcion.procesar(1.0 / 30.0, {}, {});
    COMPROBAR(r.pistas.empty() && !r.critica);
    // La pista sin confirmar termina descartada tras varios fotogramas sin verla.
    for (int i = 2; i < 8; ++i) {
        percepcion.procesar(i / 30.0, {}, {});
    }
    COMPROBAR(percepcion.pistas_activas() == 0);
}

void prueba_percepcion_baja_confianza() {
    Percepcion percepcion(camara_de_pruebas());
    Deteccion d;
    d.u_px = 700;
    d.v_px = 500;
    d.ancho_px = d.alto_px = 10;
    d.confianza = 0.1;
    percepcion.procesar(0.0, {d}, {});
    COMPROBAR(percepcion.pistas_activas() == 0);
}

void prueba_percepcion_dos_objetos_sin_cruzar_identidades() {
    const ModeloCamara cam = camara_de_pruebas();
    Percepcion percepcion(cam);
    const Actitud actitud{};
    // Dos objetos que se mueven en sentidos opuestos, separados en la imagen.
    int id_a = -1, id_b = -1;
    for (int i = 0; i < 12; ++i) {
        const double t = i / 30.0;
        Deteccion a, b;
        a.u_px = 400 + 15 * i;
        a.v_px = 300;
        a.ancho_px = a.alto_px = 8;
        b.u_px = 1000 - 15 * i;
        b.v_px = 500;
        b.ancho_px = b.alto_px = 8;
        const ResultadoPercepcion r = percepcion.procesar(t, {a, b}, actitud);
        if (i == 5) {
            COMPROBAR(r.pistas.size() == 2);
            id_a = r.pistas[0].id;
            id_b = r.pistas[1].id;
        }
        if (i == 11) {
            COMPROBAR(r.pistas.size() == 2);
            COMPROBAR(r.pistas[0].id == id_a && r.pistas[1].id == id_b);
        }
    }
}

// -------------------------------------------------------------- supervisor

class EnlaceFalso : public EnlaceVuelo {
public:
    ModoArdupilot modo_actual{ModoArdupilot::QLoiter};
    double altura{15.0};
    bool aceptar_cambio{true};
    int cambios{0};
    int velocidades_enviadas{0};
    Vec3 ultima_velocidad;

    ModoArdupilot modo() const override { return modo_actual; }
    bool cambiar_modo(ModoArdupilot m) override {
        if (!aceptar_cambio) {
            return false;
        }
        modo_actual = m;
        ++cambios;
        return true;
    }
    void enviar_velocidad_ned(const Vec3& v) override {
        ultima_velocidad = v;
        ++velocidades_enviadas;
    }
    double altura_sobre_suelo_m() const override { return altura; }
};

Amenaza amenaza_directa(double tiempo_al_cruce_s = 1.0) {
    Amenaza a;
    a.peligro = true;
    a.direccion = {1.0, 0.0, 0.0};
    a.distancia_de_paso_m = 0.0;
    a.tiempo_al_cruce_s = tiempo_al_cruce_s;
    return a;
}

void prueba_supervisor_sin_amenaza_no_hace_nada() {
    EnlaceFalso enlace;
    Supervisor sup(enlace);
    for (int i = 0; i < 30; ++i) {
        sup.actualizar(i / 30.0, std::nullopt);
    }
    COMPROBAR(enlace.cambios == 0 && enlace.velocidades_enviadas == 0);
    COMPROBAR(sup.estado() == EstadoSupervisor::Inactivo);
}

void prueba_supervisor_ciclo_completo() {
    EnlaceFalso enlace;
    Supervisor sup(enlace);
    sup.actualizar(0.0, amenaza_directa());
    COMPROBAR(enlace.modo_actual == ModoArdupilot::Guided);
    COMPROBAR(sup.estado() == EstadoSupervisor::Evadiendo);
    COMPROBAR(enlace.ultima_velocidad.norma() > 1.0);
    COMPROBAR(enlace.ultima_velocidad.z < 0.0);  // sube

    // La maniobra dura tiempo_al_cruce + 0.5 s = 1.5 s; avanzamos hasta t = 1.67 s.
    for (int i = 1; i <= 50; ++i) {
        sup.actualizar(i / 30.0, std::nullopt);
    }
    COMPROBAR(sup.estado() == EstadoSupervisor::Enfriando);
    COMPROBAR(enlace.ultima_velocidad.norma() == 0.0);
    COMPROBAR(enlace.modo_actual == ModoArdupilot::Guided);

    // Tras 1 s de calma (hasta t = 2.5 s) devuelve el modo original.
    for (int i = 51; i <= 90; ++i) {
        sup.actualizar(i / 30.0, std::nullopt);
    }
    COMPROBAR(sup.estado() == EstadoSupervisor::Inactivo);
    COMPROBAR(enlace.modo_actual == ModoArdupilot::QLoiter);
    COMPROBAR(sup.estadisticas().evasiones == 1);
}

void prueba_supervisor_no_actua_fuera_de_hover() {
    for (ModoArdupilot modo : {ModoArdupilot::Auto, ModoArdupilot::Fbwa, ModoArdupilot::Rtl, ModoArdupilot::QLand,
                               ModoArdupilot::QStabilize, ModoArdupilot::Otro}) {
        EnlaceFalso enlace;
        enlace.modo_actual = modo;
        Supervisor sup(enlace);
        sup.actualizar(0.0, amenaza_directa());
        COMPROBAR(enlace.modo_actual == modo);
        COMPROBAR(enlace.cambios == 0 && enlace.velocidades_enviadas == 0);
        COMPROBAR(sup.estadisticas().fotogramas_con_peligro_ignorado == 1);
    }
}

void prueba_supervisor_cede_al_piloto() {
    EnlaceFalso enlace;
    Supervisor sup(enlace);
    sup.actualizar(0.0, amenaza_directa());
    COMPROBAR(enlace.modo_actual == ModoArdupilot::Guided);
    // El piloto cambia a QSTABILIZE a mitad de la maniobra.
    enlace.modo_actual = ModoArdupilot::QStabilize;
    const int cambios_antes = enlace.cambios;
    const int enviadas_antes = enlace.velocidades_enviadas;
    sup.actualizar(0.2, amenaza_directa());
    sup.actualizar(1.0, std::nullopt);
    sup.actualizar(5.0, std::nullopt);
    COMPROBAR(enlace.modo_actual == ModoArdupilot::QStabilize);  // no se le quitó
    COMPROBAR(enlace.cambios == cambios_antes);
    COMPROBAR(enlace.velocidades_enviadas == enviadas_antes);
    COMPROBAR(sup.estado() == EstadoSupervisor::Inactivo);
    COMPROBAR(sup.estadisticas().cedidas_al_piloto == 1);
}

void prueba_supervisor_cambio_de_modo_rechazado() {
    EnlaceFalso enlace;
    enlace.aceptar_cambio = false;
    Supervisor sup(enlace);
    sup.actualizar(0.0, amenaza_directa());
    COMPROBAR(sup.estado() == EstadoSupervisor::Inactivo);
    COMPROBAR(enlace.velocidades_enviadas == 0);
    COMPROBAR(sup.estadisticas().cambios_de_modo_rechazados == 1);
}

void prueba_supervisor_tope_de_tiempo() {
    EnlaceFalso enlace;
    ConfigSupervisor cfg;
    cfg.max_evasion_s = 3.0;
    Supervisor sup(enlace, PlanificadorEvasion{}, cfg);
    // Una amenaza que no se va nunca: debe soltar igualmente al pasar el tope.
    for (int i = 0; i <= 90; ++i) {  // hasta t = 3.0 s: aún dentro del tope
        sup.actualizar(i / 30.0, amenaza_directa());
    }
    COMPROBAR(enlace.cambios == 1 && enlace.modo_actual == ModoArdupilot::Guided);
    sup.actualizar(3.1, std::nullopt);  // pasado el tope
    COMPROBAR(enlace.cambios == 2 && enlace.modo_actual == ModoArdupilot::QLoiter);
    COMPROBAR(sup.estado() == EstadoSupervisor::Inactivo);
}

void prueba_supervisor_segunda_amenaza_en_enfriamiento() {
    EnlaceFalso enlace;
    Supervisor sup(enlace);
    sup.actualizar(0.0, amenaza_directa());
    for (int i = 1; i <= 50; ++i) {
        sup.actualizar(i / 30.0, std::nullopt);
    }
    COMPROBAR(sup.estado() == EstadoSupervisor::Enfriando);
    sup.actualizar(51 / 30.0, amenaza_directa());
    COMPROBAR(sup.estado() == EstadoSupervisor::Evadiendo);
    COMPROBAR(enlace.cambios == 1);  // sigue en GUIDED, no se cambió de modo otra vez
}

// ------------------------------------------------- cadena completa simulada

// Dron simulado: sigue la velocidad pedida con retardo y respuesta de primer orden.
class DronSimulado : public EnlaceVuelo {
public:
    Vec3 posicion{0.0, 0.0, -15.0};
    Vec3 velocidad;
    ModoArdupilot modo_actual{ModoArdupilot::QLoiter};

    ModoArdupilot modo() const override { return modo_actual; }
    bool cambiar_modo(ModoArdupilot m) override {
        modo_actual = m;
        return true;
    }
    void enviar_velocidad_ned(const Vec3& v) override { pendientes_.emplace_back(t_ + latencia_s, v); }
    double altura_sobre_suelo_m() const override { return -posicion.z; }

    void avanzar(double dt) {
        t_ += dt;
        while (!pendientes_.empty() && pendientes_.front().first <= t_) {
            ordenada_ = pendientes_.front().second;
            pendientes_.pop_front();
        }
        const Vec3 objetivo = modo_actual == ModoArdupilot::Guided ? ordenada_ : Vec3{};
        velocidad += (objetivo - velocidad) * (dt / 0.3);
        posicion += velocidad * dt;
    }

    double latencia_s{0.15};

private:
    double t_{0.0};
    Vec3 ordenada_;
    std::deque<std::pair<double, Vec3>> pendientes_;
};

struct ResultadoCadena {
    double distancia_minima_m;
    int evasiones;
    ModoArdupilot modo_final;
};

ResultadoCadena simular_cadena(double giro, double tiempo_de_vuelo, bool con_supervisor, unsigned semilla) {
    const ModeloCamara cam = camara_de_pruebas();
    const Vec3 lanzador{12.0 * std::cos(giro), 12.0 * std::sin(giro), -1.5};
    const Actitud actitud{0.0, 0.0, giro};
    DronSimulado dron;
    const Lanzamiento piedra = lanzar_a(lanzador, dron.posicion, tiempo_de_vuelo);
    Percepcion percepcion(cam);
    Supervisor supervisor(dron);
    std::mt19937 ruido(semilla);

    double minima = (piedra.inicio - dron.posicion).norma();
    double proximo_fotograma = 0.0;
    constexpr double kPaso = 0.001;
    for (double t = 0.0; t < tiempo_de_vuelo + 4.0; t += kPaso) {
        if (t >= proximo_fotograma) {
            proximo_fotograma += 1.0 / 30.0;
            std::vector<Deteccion> dets;
            if (const auto d = ver(cam, actitud, dron.posicion, piedra.en(t), 0.08, &ruido, 0.3)) {
                dets.push_back(*d);
            }
            const ResultadoPercepcion r = percepcion.procesar(t, dets, actitud);
            if (con_supervisor) {
                supervisor.actualizar(t, r.critica);
            }
        }
        dron.avanzar(kPaso);
        minima = std::fmin(minima, (piedra.en(t) - dron.posicion).norma());
    }
    return {minima, supervisor.estadisticas().evasiones, dron.modo_actual};
}

void prueba_cadena_completa() {
    int impactos_sin = 0;
    int impactos_con = 0;
    int total = 0;
    int sin_devolver_el_modo = 0;
    for (double giro : {0.0, 0.8, -1.2, 2.4}) {
        for (double vuelo : {0.9, 1.2, 1.5}) {
            for (unsigned semilla : {1u, 2u}) {
                const ResultadoCadena sin = simular_cadena(giro, vuelo, false, semilla);
                const ResultadoCadena con = simular_cadena(giro, vuelo, true, semilla);
                impactos_sin += sin.distancia_minima_m < 0.6 ? 1 : 0;
                impactos_con += con.distancia_minima_m < 0.6 ? 1 : 0;
                sin_devolver_el_modo += con.modo_final != ModoArdupilot::QLoiter ? 1 : 0;
                ++total;
            }
        }
    }
    std::printf("  (cadena completa: %d de %d impactos sin evasión, %d con evasión)\n", impactos_sin, total,
                impactos_con);
    COMPROBAR(impactos_sin == total);
    COMPROBAR(impactos_con <= total / 10);
    COMPROBAR(sin_devolver_el_modo == 0);
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> pruebas = {
        {"rotación: yaw", prueba_rotacion_yaw},
        {"rotación: pitch", prueba_rotacion_pitch},
        {"rotación: roll", prueba_rotacion_roll},
        {"cámara: eje óptico e inclinación", prueba_camara_eje_optico},
        {"cámara: lados de la imagen", prueba_camara_lados_de_la_imagen},
        {"cámara: proyección ida y vuelta", prueba_proyeccion_ida_y_vuelta},
        {"percepción: detecta piedra dirigida", prueba_percepcion_detecta_piedra_dirigida},
        {"percepción: independiente del rumbo", prueba_percepcion_funciona_con_cualquier_rumbo},
        {"percepción: ignora objeto que pasa lejos", prueba_percepcion_ignora_objeto_que_pasa_lejos},
        {"percepción: falsa alarma de un fotograma", prueba_percepcion_falsa_alarma_de_un_fotograma},
        {"percepción: baja confianza", prueba_percepcion_baja_confianza},
        {"percepción: dos objetos", prueba_percepcion_dos_objetos_sin_cruzar_identidades},
        {"supervisor: sin amenaza no hace nada", prueba_supervisor_sin_amenaza_no_hace_nada},
        {"supervisor: ciclo completo", prueba_supervisor_ciclo_completo},
        {"supervisor: no actúa fuera de hover", prueba_supervisor_no_actua_fuera_de_hover},
        {"supervisor: cede al piloto", prueba_supervisor_cede_al_piloto},
        {"supervisor: cambio de modo rechazado", prueba_supervisor_cambio_de_modo_rechazado},
        {"supervisor: tope de tiempo", prueba_supervisor_tope_de_tiempo},
        {"supervisor: segunda amenaza en enfriamiento", prueba_supervisor_segunda_amenaza_en_enfriamiento},
        {"cadena completa simulada", prueba_cadena_completa},
    };
    for (const auto& [nombre, prueba] : pruebas) {
        const int antes = fallos;
        prueba();
        std::printf("[%s] %s\n", fallos == antes ? " OK " : "FALLO", nombre.c_str());
    }
    std::printf("\n%s (%d fallos)\n", fallos == 0 ? "TODAS LAS PRUEBAS PASARON" : "HAY FALLOS", fallos);
    return fallos == 0 ? 0 : 1;
}
