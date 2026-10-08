#include "evasion/percepcion.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace evasion {

namespace {

struct DeteccionNed {
    Vec3 direccion;
    double tamano_angular_rad{0.0};
};

double angulo_entre(const Vec3& a, const Vec3& b) {
    return std::acos(std::clamp(a.punto(b), -1.0, 1.0));
}

}  // namespace

Percepcion::Percepcion(ModeloCamara camara, ConfigPercepcion config) : camara_(camara), config_(config) {}

void Percepcion::reiniciar() { pistas_.clear(); }

ResultadoPercepcion Percepcion::procesar(double t_s, const std::vector<Deteccion>& detecciones,
                                         const Actitud& actitud) {
    std::vector<DeteccionNed> dets;
    for (const auto& d : detecciones) {
        if (d.confianza < config_.confianza_minima) {
            continue;
        }
        dets.push_back({camara_.direccion_ned(d.u_px, d.v_px, actitud),
                        camara_.tamano_angular_rad(std::max(d.ancho_px, d.alto_px))});
    }

    // Asociación voraz por distancia angular a la posición predicha de cada pista.
    std::vector<std::tuple<double, std::size_t, std::size_t>> candidatos;
    for (std::size_t i = 0; i < pistas_.size(); ++i) {
        const Pista& p = pistas_[i];
        const Vec3 prediccion = (p.direccion + p.velocidad_angular * (t_s - p.t_s)).unitario();
        for (std::size_t j = 0; j < dets.size(); ++j) {
            const double ang = angulo_entre(prediccion, dets[j].direccion);
            if (ang < config_.puerta_angular_rad) {
                candidatos.emplace_back(ang, i, j);
            }
        }
    }
    std::sort(candidatos.begin(), candidatos.end());

    std::vector<bool> pista_usada(pistas_.size(), false);
    std::vector<bool> det_usada(dets.size(), false);
    for (const auto& [ang, i, j] : candidatos) {
        (void)ang;
        if (pista_usada[i] || det_usada[j]) {
            continue;
        }
        pista_usada[i] = det_usada[j] = true;

        Pista& p = pistas_[i];
        const double dt = t_s - p.t_s;
        if (dt > 1e-6) {
            const Vec3 nueva = (dets[j].direccion - p.direccion) / dt;
            p.velocidad_angular = p.aciertos > 1 ? (p.velocidad_angular + nueva) * 0.5 : nueva;
        }
        p.direccion = dets[j].direccion;
        p.t_s = t_s;
        ++p.aciertos;
        p.perdidas = 0;
        Observacion obs;
        obs.t_s = t_s;
        obs.direccion = dets[j].direccion;
        obs.tamano_angular_rad = dets[j].tamano_angular_rad;
        p.amenaza = p.evaluador.actualizar(obs);
    }

    for (std::size_t i = 0; i < pistas_.size(); ++i) {
        if (!pista_usada[i]) {
            ++pistas_[i].perdidas;
        }
    }
    pistas_.erase(std::remove_if(pistas_.begin(), pistas_.end(),
                                 [&](const Pista& p) { return p.perdidas > config_.perdidas_maximas; }),
                  pistas_.end());

    for (std::size_t j = 0; j < dets.size(); ++j) {
        if (det_usada[j]) {
            continue;
        }
        Pista p;
        p.id = proximo_id_++;
        p.aciertos = 1;
        p.t_s = t_s;
        p.direccion = dets[j].direccion;
        p.evaluador = EvaluadorAmenaza(config_.amenaza);
        Observacion obs;
        obs.t_s = t_s;
        obs.direccion = dets[j].direccion;
        obs.tamano_angular_rad = dets[j].tamano_angular_rad;
        p.amenaza = p.evaluador.actualizar(obs);
        pistas_.push_back(std::move(p));
    }

    ResultadoPercepcion res;
    for (const Pista& p : pistas_) {
        if (p.aciertos < config_.confirmaciones || p.perdidas != 0) {
            continue;
        }
        res.pistas.push_back({p.id, p.amenaza});
        if (p.amenaza.peligro &&
            (!res.critica || p.amenaza.tiempo_al_cruce_s < res.critica->tiempo_al_cruce_s)) {
            res.critica = p.amenaza;
        }
    }
    return res;
}

}  // namespace evasion
