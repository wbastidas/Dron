#include "evasion/supervisor.hpp"

namespace evasion {

namespace {

bool es_modo_de_hover(ModoArdupilot modo) {
    return modo == ModoArdupilot::QLoiter || modo == ModoArdupilot::QHover;
}

}  // namespace

Supervisor::Supervisor(EnlaceVuelo& enlace, PlanificadorEvasion planificador, ConfigSupervisor config)
    : enlace_(enlace), planificador_(planificador), config_(config) {}

void Supervisor::iniciar(double t_s, const Amenaza& amenaza) {
    if (!es_modo_de_hover(enlace_.modo())) {
        ++estadisticas_.fotogramas_con_peligro_ignorado;
        return;
    }
    const ComandoEvasion cmd = planificador_.planificar(amenaza, {ModoVuelo::Hover, enlace_.altura_sobre_suelo_m()});
    if (!cmd.activo) {
        return;
    }
    modo_previo_ = enlace_.modo();
    if (!enlace_.cambiar_modo(ModoArdupilot::Guided)) {
        ++estadisticas_.cambios_de_modo_rechazados;
        return;
    }
    comando_ = cmd;
    estado_ = EstadoSupervisor::Evadiendo;
    t_inicio_s_ = t_s;
    t_fin_maniobra_s_ = t_s + cmd.duracion_s;
    ++estadisticas_.evasiones;
    enlace_.enviar_velocidad_ned(cmd.direccion * cmd.velocidad_ms);
}

void Supervisor::terminar() {
    enlace_.enviar_velocidad_ned(Vec3{});
    enlace_.cambiar_modo(modo_previo_);
    estado_ = EstadoSupervisor::Inactivo;
}

void Supervisor::actualizar(double t_s, const std::optional<Amenaza>& critica) {
    const bool hay_peligro = critica && critica->peligro;

    if (estado_ == EstadoSupervisor::Inactivo) {
        if (hay_peligro) {
            iniciar(t_s, *critica);
        }
        return;
    }

    if (enlace_.modo() != ModoArdupilot::Guided) {
        // El piloto (o un failsafe) tomó el control: no se le disputa.
        ++estadisticas_.cedidas_al_piloto;
        estado_ = EstadoSupervisor::Inactivo;
        return;
    }
    if (t_s - t_inicio_s_ > config_.max_evasion_s) {
        terminar();
        return;
    }

    if (estado_ == EstadoSupervisor::Enfriando && hay_peligro) {
        const ComandoEvasion cmd =
            planificador_.planificar(*critica, {ModoVuelo::Hover, enlace_.altura_sobre_suelo_m()});
        if (cmd.activo) {
            comando_ = cmd;
            t_fin_maniobra_s_ = t_s + cmd.duracion_s;
            estado_ = EstadoSupervisor::Evadiendo;
        }
    }

    if (estado_ == EstadoSupervisor::Evadiendo) {
        if (t_s >= t_fin_maniobra_s_) {
            enlace_.enviar_velocidad_ned(Vec3{});  // frena y mantiene la posición
            estado_ = EstadoSupervisor::Enfriando;
            t_fin_calma_s_ = t_s + config_.calma_s;
        } else {
            enlace_.enviar_velocidad_ned(comando_.direccion * comando_.velocidad_ms);
        }
    } else if (t_s >= t_fin_calma_s_) {
        terminar();
    } else {
        enlace_.enviar_velocidad_ned(Vec3{});
    }
}

}  // namespace evasion
