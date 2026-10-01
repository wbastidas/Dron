"""Generación de líneas de vuelo en "cortadora de césped" sobre un polígono."""

import math
from dataclasses import dataclass

from .geo import rotar


@dataclass(frozen=True)
class Linea:
    """Una pasada de mapeo en coordenadas locales (metros).

    ``entrada``/``salida`` delimitan el tramo sobre el área (donde se toman
    fotos). ``inicio``/``fin`` incluyen la extensión de aproximación que
    necesita un ala fija para llegar recta y nivelada al área.
    """

    inicio: tuple[float, float]
    entrada: tuple[float, float]
    salida: tuple[float, float]
    fin: tuple[float, float]

    @property
    def longitud_util_m(self) -> float:
        return math.dist(self.entrada, self.salida)

    @property
    def longitud_total_m(self) -> float:
        return math.dist(self.inicio, self.fin)


def _angulo_matematico(rumbo_grados: float) -> float:
    """Rumbo (0 = norte, horario) a ángulo matemático (0 = este, antihorario)."""
    return math.radians(90.0 - rumbo_grados)


def _cortes_horizontales(poligono: list[tuple[float, float]], y: float) -> list[float]:
    """Abscisas donde la recta horizontal ``y`` cruza los bordes del polígono."""
    cortes = []
    for (x1, y1), (x2, y2) in zip(poligono, poligono[1:] + poligono[:1]):
        # Regla semiabierta: evita contar dos veces un vértice compartido.
        if (y1 <= y < y2) or (y2 <= y < y1):
            t = (y - y1) / (y2 - y1)
            cortes.append(x1 + t * (x2 - x1))
    return cortes


def numero_de_lineas(poligono: list[tuple[float, float]], rumbo_grados: float, espaciado_m: float) -> int:
    angulo = _angulo_matematico(rumbo_grados)
    ys = [rotar(x, y, -angulo)[1] for x, y in poligono]
    # La tolerancia evita una línea extra por errores de redondeo (1000.0000001 / 100).
    return max(1, math.ceil((max(ys) - min(ys)) / espaciado_m - 1e-9))


def mejor_rumbo(poligono: list[tuple[float, float]], espaciado_m: float) -> float:
    """Rumbo (0–179°) que minimiza el número de líneas y por tanto los giros."""
    return float(min(range(180), key=lambda r: (numero_de_lineas(poligono, r, espaciado_m), r)))


def generar_lineas(
    poligono: list[tuple[float, float]],
    rumbo_grados: float,
    espaciado_m: float,
    extension_m: float = 0.0,
) -> list[Linea]:
    """Genera pasadas paralelas que cubren el polígono, alternando el sentido.

    En polígonos cóncavos cada pasada va del primer al último cruce con el
    borde, de modo que puede sobrevolar entrantes del polígono (más cobertura,
    nunca menos).
    """
    if len(poligono) < 3:
        raise ValueError("El área necesita al menos 3 vértices")
    if espaciado_m <= 0:
        raise ValueError("El espaciado entre líneas debe ser mayor que 0")
    if extension_m < 0:
        raise ValueError("La extensión no puede ser negativa")

    angulo = _angulo_matematico(rumbo_grados)
    rotado = [rotar(x, y, -angulo) for x, y in poligono]
    y_min = min(p[1] for p in rotado)
    y_max = max(p[1] for p in rotado)

    n = numero_de_lineas(poligono, rumbo_grados, espaciado_m)
    # Centra el conjunto de líneas sobre el área para cubrir ambos bordes por igual.
    margen = ((y_max - y_min) - (n - 1) * espaciado_m) / 2.0

    lineas = []
    for i in range(n):
        y = y_min + margen + i * espaciado_m
        cortes = _cortes_horizontales(rotado, y)
        if len(cortes) < 2:
            continue
        x0, x1 = min(cortes), max(cortes)
        if x1 - x0 < 1e-6:  # la línea solo roza un vértice
            continue
        if len(lineas) % 2 == 1:  # sentido alterno (boustrofedón)
            x0, x1 = x1, x0
        sentido = 1.0 if x1 >= x0 else -1.0
        puntos = [(x0 - sentido * extension_m, y), (x0, y), (x1, y), (x1 + sentido * extension_m, y)]
        inicio, entrada, salida, fin = (rotar(px, py, angulo) for px, py in puntos)
        lineas.append(Linea(inicio, entrada, salida, fin))
    return lineas
