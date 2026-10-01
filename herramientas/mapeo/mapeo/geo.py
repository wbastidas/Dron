"""Proyección local plana (metros) alrededor de un origen geográfico.

Usa una proyección equirectangular. Para áreas de hasta ~20 km el error es de
centímetros a pocos metros, suficiente para planificar líneas de vuelo.
"""

import math
from dataclasses import dataclass

RADIO_TIERRA_M = 6_371_008.8


@dataclass(frozen=True)
class Origen:
    lat: float
    lon: float

    def a_local(self, lat: float, lon: float) -> tuple[float, float]:
        """Convierte (lat, lon) en grados a (este, norte) en metros."""
        cos_lat0 = math.cos(math.radians(self.lat))
        x = math.radians(lon - self.lon) * RADIO_TIERRA_M * cos_lat0
        y = math.radians(lat - self.lat) * RADIO_TIERRA_M
        return x, y

    def a_geo(self, x: float, y: float) -> tuple[float, float]:
        """Convierte (este, norte) en metros a (lat, lon) en grados."""
        cos_lat0 = math.cos(math.radians(self.lat))
        lat = self.lat + math.degrees(y / RADIO_TIERRA_M)
        lon = self.lon + math.degrees(x / (RADIO_TIERRA_M * cos_lat0))
        return lat, lon


def centroide(puntos: list[tuple[float, float]]) -> tuple[float, float]:
    """Promedio simple de vértices (suficiente como origen de proyección)."""
    n = len(puntos)
    return sum(p[0] for p in puntos) / n, sum(p[1] for p in puntos) / n


def rotar(x: float, y: float, angulo_rad: float) -> tuple[float, float]:
    c, s = math.cos(angulo_rad), math.sin(angulo_rad)
    return x * c - y * s, x * s + y * c


def area_m2(poligono: list[tuple[float, float]]) -> float:
    """Área de un polígono en coordenadas locales (fórmula del lazo)."""
    total = 0.0
    for (x1, y1), (x2, y2) in zip(poligono, poligono[1:] + poligono[:1]):
        total += x1 * y2 - x2 * y1
    return abs(total) / 2.0
