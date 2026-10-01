"""Línea de comandos: ``python -m mapeo {gsd,planificar,camaras}``."""

import argparse
import json
import sys
from pathlib import Path

from .camara import CAMARAS
from .mision import ParametrosMapeo, exportar_geojson, exportar_waypoints, planificar


def leer_area_geojson(ruta: Path) -> list[tuple[float, float]]:
    """Lee el primer polígono de un GeoJSON (por ejemplo dibujado en geojson.io)."""
    datos = json.loads(ruta.read_text(encoding="utf-8"))
    geometrias = []
    if datos.get("type") == "FeatureCollection":
        geometrias = [f["geometry"] for f in datos["features"]]
    elif datos.get("type") == "Feature":
        geometrias = [datos["geometry"]]
    else:
        geometrias = [datos]
    for geometria in geometrias:
        if geometria and geometria.get("type") == "Polygon":
            anillo = geometria["coordinates"][0]
            if len(anillo) > 1 and anillo[0] == anillo[-1]:
                anillo = anillo[:-1]
            return [(float(lat), float(lon)) for lon, lat, *_ in anillo]
    raise ValueError(f"No se encontró ningún polígono en {ruta}")


def _lat_lon(texto: str) -> tuple[float, float]:
    try:
        lat, lon = (float(v) for v in texto.split(","))
    except ValueError as error:
        raise argparse.ArgumentTypeError("use el formato lat,lon (ej. -0.1807,-78.4678)") from error
    return lat, lon


def _porcentaje(texto: str) -> float:
    return float(texto) / 100.0


def _cmd_camaras(_args: argparse.Namespace) -> int:
    for clave, cam in CAMARAS.items():
        print(f"{clave:12s} {cam.nombre}  ({cam.ancho_px}x{cam.alto_px}, f={cam.focal_mm} mm)")
    return 0


def _cmd_gsd(args: argparse.Namespace) -> int:
    camara = CAMARAS[args.camara]
    altitud = args.altitud if args.altitud is not None else camara.altitud_para_gsd(args.gsd)
    transversal, longitudinal = camara.huella_m(altitud)
    print(f"Cámara:              {camara.nombre}")
    print(f"Altitud:             {altitud:.1f} m sobre el terreno")
    print(f"GSD:                 {camara.gsd_cm(altitud):.2f} cm/píxel")
    print(f"Huella por foto:     {transversal:.1f} m x {longitudinal:.1f} m")
    print(f"Exposición máxima:   1/{1 / camara.exposicion_maxima_s(altitud, args.velocidad):.0f} s a {args.velocidad} m/s")
    return 0


def _cmd_planificar(args: argparse.Namespace) -> int:
    camara = CAMARAS[args.camara]
    altitud = args.altitud if args.altitud is not None else camara.altitud_para_gsd(args.gsd)
    area = leer_area_geojson(args.area)
    parametros = ParametrosMapeo(
        camara=camara,
        altitud_m=altitud,
        traslape_frontal=args.traslape_frontal,
        traslape_lateral=args.traslape_lateral,
        velocidad_ms=args.velocidad,
        extension_m=args.extension,
        altitud_despegue_m=args.altitud_despegue,
        rumbo_grados=args.rumbo,
        seguir_terreno=args.terreno,
        autonomia_min=args.autonomia,
    )
    plan = planificar(area, args.despegue, parametros)

    args.salida.write_text(exportar_waypoints(plan.items), encoding="utf-8")
    vista = args.salida.with_suffix(".geojson")
    vista.write_text(exportar_geojson(area, plan), encoding="utf-8")

    print(f"Área:                {plan.area_ha:.1f} ha")
    print(f"Altitud / GSD:       {altitud:.0f} m / {plan.gsd_cm:.2f} cm/píxel")
    print(f"Rumbo de líneas:     {plan.rumbo_grados:.0f}°  ({len(plan.lineas_geo)} líneas)")
    print(f"Espaciado líneas:    {plan.espaciado_lineas_m:.1f} m")
    print(f"Foto cada:           {plan.distancia_disparo_m:.1f} m")
    print(f"Fotos estimadas:     {plan.fotos_estimadas}")
    print(f"Distancia total:     {plan.distancia_total_m / 1000:.2f} km")
    print(f"Tiempo estimado:     {plan.tiempo_estimado_min:.1f} min")
    print(f"Misión:              {args.salida}  ({len(plan.items)} ítems)")
    print(f"Vista previa:        {vista}")
    for aviso in plan.advertencias:
        print(f"ADVERTENCIA: {aviso}", file=sys.stderr)
    return 0


def construir_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="mapeo", description="Planificador de misiones de fotogrametría")
    sub = parser.add_subparsers(dest="comando", required=True)

    sub.add_parser("camaras", help="lista las cámaras disponibles").set_defaults(func=_cmd_camaras)

    def opciones_altura(p: argparse.ArgumentParser) -> None:
        p.add_argument("--camara", choices=CAMARAS, default="pi-hq-6mm")
        grupo = p.add_mutually_exclusive_group(required=True)
        grupo.add_argument("--altitud", type=float, help="altitud sobre el terreno en metros")
        grupo.add_argument("--gsd", type=float, help="GSD deseado en cm/píxel (calcula la altitud)")
        p.add_argument("--velocidad", type=float, default=16.0, help="velocidad de crucero en m/s")

    gsd = sub.add_parser("gsd", help="calcula GSD, altitud y huella de la cámara")
    opciones_altura(gsd)
    gsd.set_defaults(func=_cmd_gsd)

    plan = sub.add_parser("planificar", help="genera la misión de mapeo")
    opciones_altura(plan)
    plan.add_argument("--area", type=Path, required=True, help="GeoJSON con el polígono a mapear")
    plan.add_argument("--despegue", type=_lat_lon, required=True, help="punto de despegue lat,lon")
    plan.add_argument("--salida", type=Path, default=Path("mision.waypoints"))
    plan.add_argument("--traslape-frontal", type=_porcentaje, default=0.75, help="en %% (def. 75)")
    plan.add_argument("--traslape-lateral", type=_porcentaje, default=0.65, help="en %% (def. 65)")
    plan.add_argument("--rumbo", type=float, help="rumbo de las líneas en grados (def. automático)")
    plan.add_argument("--extension", type=float, default=60.0, help="tramo de aproximación en m (def. 60)")
    plan.add_argument("--altitud-despegue", type=float, default=40.0, help="altura del despegue vertical")
    plan.add_argument("--autonomia", type=float, default=40.0, help="minutos de vuelo útiles (def. 40)")
    plan.add_argument("--terreno", action="store_true", help="altitud sobre el terreno (TERRAIN_ENABLE=1)")
    plan.set_defaults(func=_cmd_planificar)
    return parser


def _unir_coordenadas(argv: list[str]) -> list[str]:
    """argparse confunde "-0.53,-78.58" con una opción; lo une como --despegue=valor."""
    resultado = []
    i = 0
    while i < len(argv):
        if argv[i] == "--despegue" and i + 1 < len(argv):
            resultado.append(f"--despegue={argv[i + 1]}")
            i += 2
        else:
            resultado.append(argv[i])
            i += 1
    return resultado


def main(argv: list[str] | None = None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    args = construir_parser().parse_args(_unir_coordenadas(argv))
    try:
        return args.func(args)
    except (ValueError, OSError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
