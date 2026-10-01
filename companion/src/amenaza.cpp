#include "evasion/amenaza.hpp"

#include <cmath>
#include <vector>

namespace evasion {

namespace {

constexpr double kPasoBusqueda_s = 0.005;

struct AjusteLineal {
    Vec3 posicion;   // posición estimada en el instante de la última muestra
    Vec3 velocidad;  // velocidad relativa estimada
    bool valido{false};
};

// Ajuste por mínimos cuadrados de p(τ) = p0 + v·τ, con τ relativo a la última muestra.
AjusteLineal ajustar(const std::vector<double>& tau, const std::vector<Vec3>& p) {
    const auto n = static_cast<double>(tau.size());
    double tau_medio = 0.0;
    Vec3 p_medio;
    for (std::size_t i = 0; i < tau.size(); ++i) {
        tau_medio += tau[i];
        p_medio += p[i];
    }
    tau_medio /= n;
    p_medio = p_medio / n;

    double varianza = 0.0;
    Vec3 covarianza;
    for (std::size_t i = 0; i < tau.size(); ++i) {
        const double dt = tau[i] - tau_medio;
        varianza += dt * dt;
        covarianza += (p[i] - p_medio) * dt;
    }
    if (varianza < 1e-12) {
        return {};
    }
    AjusteLineal ajuste;
    ajuste.velocidad = covarianza / varianza;
    ajuste.posicion = p_medio - ajuste.velocidad * tau_medio;
    ajuste.valido = true;
    return ajuste;
}

}  // namespace

EvaluadorAmenaza::EvaluadorAmenaza(ConfigAmenaza config) : config_(config) {}

void EvaluadorAmenaza::reiniciar() { historial_.clear(); }

double EvaluadorAmenaza::distancia_de(const Observacion& obs) const {
    if (obs.distancia_m) {
        return *obs.distancia_m;
    }
    return config_.tamano_supuesto_m / (2.0 * std::tan(obs.tamano_angular_rad / 2.0));
}

Amenaza EvaluadorAmenaza::actualizar(const Observacion& obs) {
    Amenaza resultado;
    resultado.direccion = obs.direccion.unitario();

    const bool medible = obs.distancia_m ? *obs.distancia_m > 0.0 : obs.tamano_angular_rad > 0.0;
    if (!medible || resultado.direccion.norma() == 0.0) {
        return resultado;  // observación inutilizable: no se incorpora
    }
    if (!historial_.empty() && obs.t_s <= historial_.back().t_s) {
        reiniciar();  // el tiempo retrocedió: se descarta la historia
    }
    historial_.push_back(obs);
    while (historial_.size() > config_.muestras_maximas) {
        historial_.pop_front();
    }
    resultado.distancia_actual_m = distancia_de(obs);
    if (historial_.size() < config_.muestras_minimas) {
        return resultado;
    }

    // Aceleración relativa del objeto: gravedad (el dron se supone casi quieto
    // en la ventana de unos cientos de milisegundos).
    const Vec3 aceleracion = config_.considerar_gravedad ? Vec3{0.0, 0.0, kGravedad} : Vec3{};

    std::vector<double> tau;
    std::vector<Vec3> posiciones;
    for (const auto& o : historial_) {
        const double t = o.t_s - obs.t_s;
        tau.push_back(t);
        // Se resta la parte parabólica para ajustar solo posición y velocidad.
        posiciones.push_back(o.direccion.unitario() * distancia_de(o) - aceleracion * (0.5 * t * t));
    }
    const AjusteLineal ajuste = ajustar(tau, posiciones);
    if (!ajuste.valido) {
        return resultado;
    }

    // Busca el instante de máximo acercamiento dentro del horizonte.
    double mejor_t = 0.0;
    double mejor_d = ajuste.posicion.norma();
    for (double t = kPasoBusqueda_s; t <= config_.horizonte_s; t += kPasoBusqueda_s) {
        const Vec3 p = ajuste.posicion + ajuste.velocidad * t + aceleracion * (0.5 * t * t);
        const double d = p.norma();
        if (d < mejor_d) {
            mejor_d = d;
            mejor_t = t;
        }
    }

    if (mejor_t > 0.0) {
        const Vec3 p = ajuste.posicion + ajuste.velocidad * mejor_t + aceleracion * (0.5 * mejor_t * mejor_t);
        resultado.tiempo_al_cruce_s = mejor_t;
        resultado.distancia_de_paso_m = mejor_d;
        resultado.lado_de_paso = p.unitario();
        resultado.peligro = mejor_d < config_.radio_seguridad_m;
    }
    return resultado;
}

}  // namespace evasion
