#pragma once

#include <optional>

#include "evasion/amenaza.hpp"
#include "evasion/planificador.hpp"

namespace evasion {

// Modos de ArduPlane (QuadPlane) que el supervisor necesita distinguir.
enum class ModoArdupilot {
    QStabilize,
    QHover,
    QLoiter,
    QLand,
    Guided,
    Auto,
    Rtl,
    Fbwa,
    Otro,
};

// Lo que el supervisor necesita del autopiloto. La implementación real (MAVSDK /
// MAVLink) va en otro módulo; las pruebas usan una simulada.
class EnlaceVuelo {
public:
    virtual ~EnlaceVuelo() = default;
    virtual ModoArdupilot modo() const = 0;
    // Devuelve false si el autopiloto rechazó el cambio.
    virtual bool cambiar_modo(ModoArdupilot modo) = 0;
    // Velocidad objetivo en NED (m/s). En GUIDED debe enviarse de forma continua:
    // ArduPilot la abandona si deja de recibirla durante unos segundos.
    virtual void enviar_velocidad_ned(const Vec3& velocidad) = 0;
    virtual double altura_sobre_suelo_m() const = 0;
};

enum class EstadoSupervisor {
    Inactivo,
    Evadiendo,
    Enfriando,  // terminó la maniobra; mantiene la posición un momento antes de devolver el control
};

struct ConfigSupervisor {
    double calma_s{1.0};         // sin peligro durante este tiempo antes de devolver el modo
    double max_evasion_s{6.0};   // tope de tiempo en GUIDED por evento, pase lo que pase
};

struct EstadisticasSupervisor {
    int evasiones{0};
    int cambios_de_modo_rechazados{0};
    int cedidas_al_piloto{0};
    int fotogramas_con_peligro_ignorado{0};  // el avión no estaba en un modo de hover
};

// Decide cuándo tomar el control del avión para esquivar y, sobre todo, cuándo
// soltarlo. Reglas de seguridad:
//  - Solo actúa en QLOITER y QHOVER (hover con la altura controlada). En vuelo de
//    crucero, en AUTO o en cualquier otro modo, nunca cambia de modo.
//  - Si el modo deja de ser GUIDED durante la maniobra (el piloto con el
//    interruptor, o un failsafe), cede el control y no lo recupera.
//  - Pase lo que pase, devuelve el modo anterior pasados `max_evasion_s`.
class Supervisor {
public:
    Supervisor(EnlaceVuelo& enlace, PlanificadorEvasion planificador = PlanificadorEvasion{},
               ConfigSupervisor config = {});

    // Llamar en cada fotograma, haya o no amenaza.
    void actualizar(double t_s, const std::optional<Amenaza>& critica);

    EstadoSupervisor estado() const { return estado_; }
    const EstadisticasSupervisor& estadisticas() const { return estadisticas_; }

private:
    void iniciar(double t_s, const Amenaza& amenaza);
    void terminar();

    EnlaceVuelo& enlace_;
    PlanificadorEvasion planificador_;
    ConfigSupervisor config_;
    EstadisticasSupervisor estadisticas_;
    EstadoSupervisor estado_{EstadoSupervisor::Inactivo};
    ModoArdupilot modo_previo_{ModoArdupilot::Otro};
    ComandoEvasion comando_;
    double t_inicio_s_{0.0};
    double t_fin_maniobra_s_{0.0};
    double t_fin_calma_s_{0.0};
};

}  // namespace evasion
