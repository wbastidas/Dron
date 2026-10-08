#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "evasion/amenaza.hpp"
#include "evasion/geometria.hpp"

namespace evasion {

// Una detección en un fotograma, tal como la entrega el detector neuronal
// (cuadro delimitador en píxeles).
struct Deteccion {
    double u_px{0.0};  // centro
    double v_px{0.0};
    double ancho_px{0.0};
    double alto_px{0.0};
    double confianza{1.0};
};

struct ConfigPercepcion {
    double confianza_minima{0.35};
    // Distancia angular máxima entre la predicción de una pista y una detección.
    // Una piedra a 15 m/s vista desde 8 m se mueve ~0.06 rad por fotograma a 30 fps.
    double puerta_angular_rad{0.25};
    // Detecciones consecutivas necesarias para tratar una pista como real; evita
    // reaccionar a una falsa alarma de un solo fotograma.
    int confirmaciones{3};
    int perdidas_maximas{3};
    ConfigAmenaza amenaza;
};

struct AmenazaRastreada {
    int id{0};
    Amenaza amenaza;
};

struct ResultadoPercepcion {
    std::vector<AmenazaRastreada> pistas;  // pistas confirmadas vistas en este fotograma
    std::optional<Amenaza> critica;        // la más urgente que sea peligro, si la hay
};

// Convierte detecciones en píxeles a amenazas: pasa cada detección a una
// dirección NED con la actitud del avión (lo que compensa el giro del avión),
// las asocia con pistas de fotogramas anteriores y evalúa cada pista confirmada.
class Percepcion {
public:
    explicit Percepcion(ModeloCamara camara = {}, ConfigPercepcion config = {});

    ResultadoPercepcion procesar(double t_s, const std::vector<Deteccion>& detecciones, const Actitud& actitud);
    void reiniciar();
    std::size_t pistas_activas() const { return pistas_.size(); }

private:
    struct Pista {
        int id{0};
        int aciertos{0};
        int perdidas{0};
        double t_s{0.0};
        Vec3 direccion;
        Vec3 velocidad_angular;  // derivada de la dirección, 1/s
        EvaluadorAmenaza evaluador;
        Amenaza amenaza;
    };

    ModeloCamara camara_;
    ConfigPercepcion config_;
    std::vector<Pista> pistas_;
    int proximo_id_{1};
};

}  // namespace evasion
