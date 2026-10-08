"""Balance de energía de un VTOL de ala fija: ¿cuánto vuela con una batería dada?

Es un modelo de primer orden, útil para decidir qué avión y qué batería comprar
y para comparar opciones. NO sustituye una medición: los parámetros por defecto
son supuestos razonables que la fase 3 de la hoja de ruta debe reemplazar por
valores medidos (mAh por minuto en crucero y en hover).
"""

from dataclasses import dataclass

GRAVEDAD = 9.80665


@dataclass(frozen=True)
class ModeloEnergia:
    masa_sin_bateria_kg: float
    velocidad_ms: float = 16.0
    # Finura (L/D) en crucero. Incluye el arrastre de los rotores de elevación.
    # Un ala de espuma con 4 rotores fijos suele estar en 5–8; el resultado es
    # muy sensible a este valor, por eso hay que medirlo.
    finura: float = 6.0
    # Rendimiento conjunto motor + variador + hélice en crucero.
    rendimiento: float = 0.55
    # Potencia en hover por kg de masa total (rotores de elevación).
    hover_w_por_kg: float = 180.0
    # Segundos totales en hover: ascenso, transiciones, descenso y margen.
    tiempo_hover_s: float = 120.0
    # Energía por kg de la batería (paquete completo con cableado).
    energia_especifica_wh_kg: float = 200.0
    # Fracción de la energía nominal que se puede usar sin dañar la batería.
    fraccion_util: float = 0.85
    # Fracción de la energía útil que se deja de reserva.
    reserva: float = 0.20
    # Potencia constante de la electrónica de a bordo (Pi, cámaras, controladora).
    potencia_electronica_w: float = 12.0
    # Altitud del área de operación sobre el nivel del mar. El aire menos denso
    # obliga a volar más rápido (a igual velocidad indicada) y a mover más aire
    # en hover: ambas potencias suben como 1/sqrt(densidad relativa).
    altitud_msnm: float = 0.0

    def validar(self) -> None:
        positivos = {
            "masa_sin_bateria_kg": self.masa_sin_bateria_kg,
            "velocidad_ms": self.velocidad_ms,
            "finura": self.finura,
            "rendimiento": self.rendimiento,
            "hover_w_por_kg": self.hover_w_por_kg,
            "energia_especifica_wh_kg": self.energia_especifica_wh_kg,
            "fraccion_util": self.fraccion_util,
        }
        for nombre, valor in positivos.items():
            if valor <= 0:
                raise ValueError(f"{nombre} debe ser mayor que 0 (recibido {valor})")
        if self.rendimiento > 1 or self.fraccion_util > 1:
            raise ValueError("rendimiento y fraccion_util no pueden superar 1")
        if not 0 <= self.reserva < 1:
            raise ValueError("la reserva debe estar entre 0 y 0.99")
        if self.tiempo_hover_s < 0 or self.potencia_electronica_w < 0:
            raise ValueError("tiempo_hover_s y potencia_electronica_w no pueden ser negativos")
        if not -400 <= self.altitud_msnm <= 11000:
            raise ValueError("altitud_msnm debe estar entre -400 y 11000 m (modelo de troposfera)")


@dataclass(frozen=True)
class Autonomia:
    bateria_wh: float
    masa_total_kg: float
    potencia_crucero_w: float
    energia_hover_wh: float
    minutos_crucero: float  # tiempo en crucero tras descontar hover y reserva


def densidad_relativa(altitud_msnm: float) -> float:
    """Densidad del aire respecto al nivel del mar (atmósfera estándar ISA, troposfera)."""
    return (1.0 - 2.25577e-5 * altitud_msnm) ** 4.2559


def factor_altitud(modelo: ModeloEnergia) -> float:
    """Multiplicador de potencia por volar en aire menos denso."""
    return 1.0 / densidad_relativa(modelo.altitud_msnm) ** 0.5


def masa_total_kg(bateria_wh: float, modelo: ModeloEnergia) -> float:
    return modelo.masa_sin_bateria_kg + bateria_wh / modelo.energia_especifica_wh_kg


def potencia_crucero_w(masa_kg: float, modelo: ModeloEnergia) -> float:
    """Potencia eléctrica en crucero: P = m·g·v / (rendimiento · L/D) · k_altitud + electrónica."""
    mecanica = masa_kg * GRAVEDAD * modelo.velocidad_ms / (modelo.rendimiento * modelo.finura)
    return mecanica * factor_altitud(modelo) + modelo.potencia_electronica_w


def energia_hover_wh(masa_kg: float, modelo: ModeloEnergia) -> float:
    potencia = modelo.hover_w_por_kg * masa_kg * factor_altitud(modelo) + modelo.potencia_electronica_w
    return potencia * modelo.tiempo_hover_s / 3600.0


def autonomia(bateria_wh: float, modelo: ModeloEnergia) -> Autonomia:
    """Minutos de crucero disponibles con una batería de `bateria_wh` (nominal)."""
    modelo.validar()
    if bateria_wh <= 0:
        raise ValueError("bateria_wh debe ser mayor que 0")
    masa = masa_total_kg(bateria_wh, modelo)
    p_crucero = potencia_crucero_w(masa, modelo)
    e_hover = energia_hover_wh(masa, modelo)
    util = bateria_wh * modelo.fraccion_util * (1.0 - modelo.reserva)
    minutos = max(0.0, (util - e_hover) / p_crucero * 60.0)
    return Autonomia(bateria_wh, masa, p_crucero, e_hover, minutos)


def bateria_para(objetivo_min: float, modelo: ModeloEnergia, masa_maxima_kg: float | None = None) -> Autonomia:
    """Batería mínima (Wh nominales) para `objetivo_min` minutos de crucero.

    Cada Wh agregado pesa, lo que sube la potencia: se itera hasta converger.
    Lanza ValueError si no converge o si la masa supera `masa_maxima_kg`.
    """
    modelo.validar()
    if objetivo_min <= 0:
        raise ValueError("objetivo_min debe ser mayor que 0")
    factor = modelo.fraccion_util * (1.0 - modelo.reserva)
    wh = 100.0
    for _ in range(200):
        masa = masa_total_kg(wh, modelo)
        necesaria = (potencia_crucero_w(masa, modelo) * objetivo_min / 60.0 + energia_hover_wh(masa, modelo)) / factor
        if abs(necesaria - wh) < 0.01:
            wh = necesaria
            break
        wh = necesaria
        if wh > 5000:
            break
    else:
        raise ValueError("la batería necesaria no converge: el avión es demasiado pesado para ese objetivo")
    if wh > 5000:
        raise ValueError("la batería necesaria no converge: el avión es demasiado pesado para ese objetivo")
    resultado = autonomia(wh, modelo)
    if masa_maxima_kg is not None and resultado.masa_total_kg > masa_maxima_kg:
        raise ValueError(
            f"se necesitan {wh:.0f} Wh y el avión pesaría {resultado.masa_total_kg:.2f} kg, "
            f"más que el máximo de {masa_maxima_kg:.2f} kg"
        )
    return resultado
