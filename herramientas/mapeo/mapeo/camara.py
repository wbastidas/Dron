"""Modelo de cámara para fotogrametría: GSD, huella en el suelo y límites de exposición."""

from dataclasses import dataclass


@dataclass(frozen=True)
class Camara:
    """Cámara montada mirando hacia abajo (nadir).

    Convención: el lado ``ancho`` del sensor queda perpendicular a la línea de
    vuelo (transversal) y el lado ``alto`` queda a lo largo de la línea de vuelo.
    Así se cubre más terreno por pasada.
    """

    nombre: str
    sensor_ancho_mm: float
    sensor_alto_mm: float
    focal_mm: float
    ancho_px: int
    alto_px: int
    intervalo_minimo_s: float = 1.0  # tiempo mínimo entre fotos que soporta la cámara

    def gsd_cm(self, altitud_m: float) -> float:
        """Tamaño de un píxel sobre el terreno (cm/píxel) a una altitud dada."""
        _validar_positivo(altitud_m, "altitud_m")
        return (self.sensor_ancho_mm * altitud_m * 100.0) / (self.focal_mm * self.ancho_px)

    def altitud_para_gsd(self, gsd_cm: float) -> float:
        """Altitud (m sobre el terreno) necesaria para obtener el GSD pedido."""
        _validar_positivo(gsd_cm, "gsd_cm")
        return (gsd_cm * self.focal_mm * self.ancho_px) / (self.sensor_ancho_mm * 100.0)

    def huella_m(self, altitud_m: float) -> tuple[float, float]:
        """Huella de una foto en el suelo: (transversal, longitudinal) en metros."""
        gsd_m = self.gsd_cm(altitud_m) / 100.0
        return gsd_m * self.ancho_px, gsd_m * self.alto_px

    def exposicion_maxima_s(self, altitud_m: float, velocidad_ms: float) -> float:
        """Exposición máxima para que el desenfoque por movimiento sea < 0.5 píxel."""
        _validar_positivo(velocidad_ms, "velocidad_ms")
        return 0.5 * (self.gsd_cm(altitud_m) / 100.0) / velocidad_ms


def _validar_positivo(valor: float, nombre: str) -> None:
    if valor <= 0:
        raise ValueError(f"{nombre} debe ser mayor que 0 (recibido {valor})")


# Datos de fabricante. El sensor en mm = píxeles x tamaño de píxel.
CAMARAS: dict[str, Camara] = {
    # Raspberry Pi Camera Module 3 (Sony IMX708, píxel 1.4 µm, lente 4.74 mm)
    "pi3": Camara("Raspberry Pi Camera Module 3", 6.451, 3.629, 4.74, 4608, 2592),
    # Raspberry Pi Camera Module 3 Wide (IMX708, lente 2.75 mm, ~102° diagonal)
    "pi3-wide": Camara("Raspberry Pi Camera Module 3 Wide", 6.451, 3.629, 2.75, 4608, 2592),
    # Raspberry Pi HQ Camera (Sony IMX477, píxel 1.55 µm) con lente de 6 mm
    "pi-hq-6mm": Camara("Raspberry Pi HQ Camera + lente 6 mm", 6.287, 4.712, 6.0, 4056, 3040),
    # Raspberry Pi HQ Camera con lente de 8 mm (más detalle, menos huella)
    "pi-hq-8mm": Camara("Raspberry Pi HQ Camera + lente 8 mm", 6.287, 4.712, 8.0, 4056, 3040),
}
