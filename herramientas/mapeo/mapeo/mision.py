"""Construcción de la misión de mapeo para ArduPlane (QuadPlane/VTOL).

Exporta al formato de texto ``QGC WPL 110`` que abren Mission Planner y
QGroundControl.
"""

import json
import math
from dataclasses import dataclass, field

from .camara import Camara
from .geo import Origen, area_m2, centroide
from .rejilla import Linea, generar_lineas, mejor_rumbo

# Comandos MAVLink (MAV_CMD)
NAV_WAYPOINT = 16
NAV_VTOL_TAKEOFF = 84
NAV_VTOL_LAND = 85
DO_CHANGE_SPEED = 178
DO_SET_CAM_TRIGG_DIST = 206

# Marcos de referencia (MAV_FRAME)
FRAME_GLOBAL = 0
FRAME_RELATIVA = 3  # altitud relativa al punto de despegue
FRAME_TERRENO = 10  # altitud sobre el terreno (requiere TERRAIN_ENABLE=1)

ALTITUD_MAXIMA_LEGAL_M = 122.0  # límite DGAC Ecuador sobre el terreno


@dataclass(frozen=True)
class ItemMision:
    comando: int
    marco: int = FRAME_RELATIVA
    p1: float = 0.0
    p2: float = 0.0
    p3: float = 0.0
    p4: float = 0.0
    lat: float = 0.0
    lon: float = 0.0
    alt: float = 0.0


@dataclass(frozen=True)
class ParametrosMapeo:
    camara: Camara
    altitud_m: float
    traslape_frontal: float = 0.75
    traslape_lateral: float = 0.65
    velocidad_ms: float = 16.0
    extension_m: float = 60.0
    altitud_despegue_m: float = 40.0
    rumbo_grados: float | None = None  # None = automático
    seguir_terreno: bool = False
    autonomia_min: float = 40.0  # tiempo de vuelo útil de la batería (validar en pruebas reales)

    def validar(self) -> None:
        for nombre in ("traslape_frontal", "traslape_lateral"):
            valor = getattr(self, nombre)
            if not 0.0 <= valor < 1.0:
                raise ValueError(f"{nombre} debe estar entre 0 y 0.99 (recibido {valor})")
        if self.velocidad_ms <= 0:
            raise ValueError("La velocidad debe ser mayor que 0")
        if self.altitud_despegue_m <= 0:
            raise ValueError("La altitud de despegue debe ser mayor que 0")


@dataclass
class PlanMapeo:
    items: list[ItemMision]
    lineas_geo: list[list[tuple[float, float]]]
    rumbo_grados: float
    espaciado_lineas_m: float
    distancia_disparo_m: float
    gsd_cm: float
    area_ha: float
    distancia_total_m: float
    tiempo_estimado_min: float
    fotos_estimadas: int
    advertencias: list[str] = field(default_factory=list)


def planificar(
    area: list[tuple[float, float]],
    despegue: tuple[float, float],
    parametros: ParametrosMapeo,
) -> PlanMapeo:
    """Planifica la misión completa.

    ``area`` y ``despegue`` van en (lat, lon) grados.
    """
    parametros.validar()
    camara = parametros.camara
    origen = Origen(*centroide(area))
    poligono = [origen.a_local(lat, lon) for lat, lon in area]

    transversal_m, longitudinal_m = camara.huella_m(parametros.altitud_m)
    espaciado = transversal_m * (1.0 - parametros.traslape_lateral)
    disparo = longitudinal_m * (1.0 - parametros.traslape_frontal)
    rumbo = parametros.rumbo_grados if parametros.rumbo_grados is not None else mejor_rumbo(poligono, espaciado)
    lineas = generar_lineas(poligono, rumbo, espaciado, parametros.extension_m)
    if not lineas:
        raise ValueError("No se generó ninguna línea: revise el polígono del área")

    items = _construir_items(lineas, origen, despegue, parametros, disparo)
    despegue_local = origen.a_local(*despegue)
    distancia = _distancia_recorrida(lineas, despegue_local)
    fotos = sum(math.floor(l.longitud_util_m / disparo) + 1 for l in lineas)
    # +20 % por giros, aceleraciones y viento; más ~2 min de despegue/aterrizaje VTOL.
    tiempo_min = distancia / parametros.velocidad_ms * 1.2 / 60.0 + 2.0
    advertencias = _advertencias(parametros, disparo, lineas, despegue_local)
    if tiempo_min > 0.8 * parametros.autonomia_min:
        advertencias.append(
            f"La misión dura ~{tiempo_min:.0f} min y deja menos del 20 % de reserva sobre una "
            f"autonomía de {parametros.autonomia_min:.0f} min: divida el área en varios vuelos."
        )

    return PlanMapeo(
        items=items,
        lineas_geo=[[origen.a_geo(*p) for p in (l.inicio, l.fin)] for l in lineas],
        rumbo_grados=rumbo,
        espaciado_lineas_m=espaciado,
        distancia_disparo_m=disparo,
        gsd_cm=camara.gsd_cm(parametros.altitud_m),
        area_ha=area_m2(poligono) / 10_000.0,
        distancia_total_m=distancia,
        tiempo_estimado_min=tiempo_min,
        fotos_estimadas=fotos,
        advertencias=advertencias,
    )


def _construir_items(
    lineas: list[Linea],
    origen: Origen,
    despegue: tuple[float, float],
    parametros: ParametrosMapeo,
    disparo_m: float,
) -> list[ItemMision]:
    marco = FRAME_TERRENO if parametros.seguir_terreno else FRAME_RELATIVA
    alt = parametros.altitud_m
    items = [
        # Ítem 0: posición "home". El autopiloto lo reemplaza al armar.
        ItemMision(NAV_WAYPOINT, FRAME_GLOBAL, lat=despegue[0], lon=despegue[1]),
        ItemMision(NAV_VTOL_TAKEOFF, FRAME_RELATIVA, alt=parametros.altitud_despegue_m),
        # p1=0 velocidad aerodinámica, p2=valor m/s, p3=-1 sin cambiar acelerador
        ItemMision(DO_CHANGE_SPEED, p1=0, p2=parametros.velocidad_ms, p3=-1),
    ]

    def punto(xy: tuple[float, float]) -> ItemMision:
        lat, lon = origen.a_geo(*xy)
        return ItemMision(NAV_WAYPOINT, marco, lat=lat, lon=lon, alt=alt)

    for linea in lineas:
        if parametros.extension_m > 0:
            items.append(punto(linea.inicio))
        items.append(punto(linea.entrada))
        # p3=1: dispara una foto inmediatamente al activar
        items.append(ItemMision(DO_SET_CAM_TRIGG_DIST, p1=round(disparo_m, 2), p3=1))
        items.append(punto(linea.salida))
        items.append(ItemMision(DO_SET_CAM_TRIGG_DIST, p1=0))
        if parametros.extension_m > 0:
            items.append(punto(linea.fin))

    # Regreso y aterrizaje vertical en el punto de despegue.
    items.append(ItemMision(NAV_VTOL_LAND, FRAME_RELATIVA, lat=despegue[0], lon=despegue[1], alt=0))
    return items


def _distancia_recorrida(lineas: list[Linea], despegue: tuple[float, float]) -> float:
    total = math.dist(despegue, lineas[0].inicio)
    for actual, siguiente in zip(lineas, lineas[1:]):
        total += actual.longitud_total_m + math.dist(actual.fin, siguiente.inicio)
    total += lineas[-1].longitud_total_m + math.dist(lineas[-1].fin, despegue)
    return total


def _advertencias(
    parametros: ParametrosMapeo,
    disparo_m: float,
    lineas: list[Linea],
    despegue_local: tuple[float, float],
) -> list[str]:
    avisos = []
    if parametros.altitud_m > ALTITUD_MAXIMA_LEGAL_M:
        avisos.append(
            f"Altitud {parametros.altitud_m:.0f} m supera el máximo de {ALTITUD_MAXIMA_LEGAL_M:.0f} m "
            "sobre el terreno permitido por la DGAC."
        )
    if not parametros.seguir_terreno:
        avisos.append(
            "La altitud es relativa al despegue: en terreno montañoso el dron puede quedar muy bajo "
            "sobre las lomas. Use --terreno (TERRAIN_ENABLE=1) en zonas con desnivel."
        )
    intervalo = disparo_m / parametros.velocidad_ms
    if intervalo < parametros.camara.intervalo_minimo_s:
        avisos.append(
            f"Una foto cada {intervalo:.2f} s es más rápido que el mínimo de la cámara "
            f"({parametros.camara.intervalo_minimo_s:.1f} s): suba la altitud, baje la velocidad "
            "o reduzca el traslape frontal."
        )
    distancia_max = max(math.dist(despegue_local, p) for l in lineas for p in (l.inicio, l.fin))
    if distancia_max > 5000:
        avisos.append(
            f"El punto más lejano está a {distancia_max / 1000:.1f} km: vuelo más allá de la línea de "
            "vista (BVLOS). Requiere autorización específica de la DGAC y enlace de radio de largo alcance."
        )
    return avisos


def exportar_waypoints(items: list[ItemMision]) -> str:
    """Texto en formato QGC WPL 110 (Mission Planner / QGroundControl)."""
    filas = ["QGC WPL 110"]
    for i, it in enumerate(items):
        actual = 1 if i == 0 else 0
        campos = [i, actual, it.marco, it.comando, it.p1, it.p2, it.p3, it.p4]
        texto = [str(c) if isinstance(c, int) else _num(c) for c in campos]
        texto += [f"{it.lat:.8f}", f"{it.lon:.8f}", _num(it.alt), "1"]
        filas.append("\t".join(texto))
    return "\n".join(filas) + "\n"


def exportar_geojson(area: list[tuple[float, float]], plan: PlanMapeo) -> str:
    """GeoJSON con el área y las líneas, para revisarlo en geojson.io o QGIS."""
    anillo = [[lon, lat] for lat, lon in area] + [[area[0][1], area[0][0]]]
    entidades = [
        {"type": "Feature", "properties": {"tipo": "area"}, "geometry": {"type": "Polygon", "coordinates": [anillo]}}
    ]
    for i, linea in enumerate(plan.lineas_geo):
        entidades.append(
            {
                "type": "Feature",
                "properties": {"tipo": "linea", "numero": i + 1},
                "geometry": {"type": "LineString", "coordinates": [[lon, lat] for lat, lon in linea]},
            }
        )
    return json.dumps({"type": "FeatureCollection", "features": entidades}, ensure_ascii=False, indent=2)


def _num(valor: float) -> str:
    return f"{valor:.6f}".rstrip("0").rstrip(".") or "0"
