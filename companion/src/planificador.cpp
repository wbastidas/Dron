#include "evasion/planificador.hpp"

#include <algorithm>
#include <cmath>

namespace evasion {

namespace {

// Por debajo de esta distancia de paso la dirección "lado_de_paso" es puro
// ruido: se usa una dirección fija para no cambiar de lado entre fotogramas.
constexpr double kPasoCentrado_m = 0.2;
// Margen sobre la altura mínima a partir del cual se prohíbe bajar.
constexpr double kMargenAltura_m = 3.0;
// Aceleración lateral de un ala fija con ~45° de alabeo.
constexpr double kAceleracionLateralAla_ms2 = kGravedad;

Vec3 perpendicular_horizontal(const Vec3& u) {
    const Vec3 h{-u.y, u.x, 0.0};
    return h.norma() > 1e-6 ? h.unitario() : Vec3{0.0, 1.0, 0.0};
}

}  // namespace

PlanificadorEvasion::PlanificadorEvasion(ConfigEvasion config) : config_(config) {}

Vec3 PlanificadorEvasion::direccion_de_escape(const Amenaza& amenaza, const EstadoVehiculo& estado) const {
    const Vec3 u = amenaza.direccion;
    Vec3 d;
    if (amenaza.distancia_de_paso_m > kPasoCentrado_m && amenaza.lado_de_paso.norma() > 0.0) {
        // Alejarse del punto por donde pasará el objeto.
        d = (amenaza.lado_de_paso * -1.0).unitario();
    } else {
        // Viene directo: moverse perpendicular a la línea de visión, prefiriendo subir.
        d = Vec3{0.0, 0.0, -1.0}.perpendicular_a(u);
        d = d.norma() > 0.3 ? d.unitario() : perpendicular_horizontal(u);
    }

    const bool cerca_del_suelo = estado.altura_sobre_suelo_m < config_.altura_minima_m + kMargenAltura_m;
    if (d.z > 0.0 && (cerca_del_suelo || estado.modo == ModoVuelo::Crucero)) {
        d.z = 0.0;  // nunca bajar cerca del suelo ni picar en crucero
        d = d.norma() > 1e-6 ? d.unitario() : perpendicular_horizontal(u);
    }
    return d;
}

ComandoEvasion PlanificadorEvasion::planificar(const Amenaza& amenaza, const EstadoVehiculo& estado) const {
    ComandoEvasion cmd;
    if (!amenaza.peligro) {
        return cmd;
    }
    cmd.activo = true;
    cmd.direccion = direccion_de_escape(amenaza, estado);
    cmd.duracion_s = amenaza.tiempo_al_cruce_s + 0.5;

    const double desplazamiento = config_.radio_seguridad_m + config_.margen_m - amenaza.distancia_de_paso_m;
    const double tiempo_util = amenaza.tiempo_al_cruce_s - config_.latencia_s;

    if (estado.modo == ModoVuelo::Hover) {
        // Perfil triangular desde reposo: la velocidad media es la mitad de la final.
        const double requerida = tiempo_util > 0.0 ? 2.0 * desplazamiento / tiempo_util : INFINITY;
        cmd.velocidad_ms = std::min(requerida, config_.velocidad_maxima_hover_ms);
        cmd.alcanzable = requerida <= config_.velocidad_maxima_hover_ms;
    } else {
        // En crucero el autopiloto vira/trepa al máximo; solo se evalúa si alcanza.
        const double tiempo_necesario = std::sqrt(2.0 * desplazamiento / kAceleracionLateralAla_ms2);
        cmd.alcanzable = tiempo_util >= tiempo_necesario;
    }
    return cmd;
}

}  // namespace evasion
