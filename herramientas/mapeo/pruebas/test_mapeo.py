import json
import math
import tempfile
import unittest
from pathlib import Path

from mapeo.camara import CAMARAS, Camara
from mapeo.cli import leer_area_geojson, main
from mapeo.geo import Origen, area_m2
from mapeo.mision import (
    DO_SET_CAM_TRIGG_DIST,
    FRAME_TERRENO,
    NAV_VTOL_LAND,
    NAV_VTOL_TAKEOFF,
    NAV_WAYPOINT,
    ParametrosMapeo,
    exportar_waypoints,
    planificar,
)
from mapeo.rejilla import generar_lineas, mejor_rumbo

EJEMPLO = Path(__file__).resolve().parent.parent / "ejemplos" / "area_ejemplo.geojson"
CUADRADO = [(0.0, 0.0), (1000.0, 0.0), (1000.0, 1000.0), (0.0, 1000.0)]


class PruebasCamara(unittest.TestCase):
    def test_gsd_y_altitud_son_inversas(self):
        cam = CAMARAS["pi-hq-6mm"]
        self.assertAlmostEqual(cam.altitud_para_gsd(cam.gsd_cm(87.0)), 87.0)

    def test_gsd_conocido(self):
        # 10 mm de sensor, 10 mm de focal, 1000 px: a 100 m cada píxel cubre 10 cm.
        cam = Camara("prueba", 10.0, 7.5, 10.0, 1000, 750)
        self.assertAlmostEqual(cam.gsd_cm(100.0), 10.0)
        self.assertEqual(tuple(round(v, 6) for v in cam.huella_m(100.0)), (100.0, 75.0))

    def test_altitud_invalida(self):
        with self.assertRaises(ValueError):
            CAMARAS["pi3"].gsd_cm(0)


class PruebasGeo(unittest.TestCase):
    def test_ida_y_vuelta(self):
        origen = Origen(-0.52, -78.57)
        lat, lon = origen.a_geo(*origen.a_local(-0.53, -78.56))
        self.assertAlmostEqual(lat, -0.53, places=9)
        self.assertAlmostEqual(lon, -78.56, places=9)

    def test_un_grado_de_latitud(self):
        _, y = Origen(0.0, 0.0).a_local(1.0, 0.0)
        self.assertAlmostEqual(y, 111_195, delta=5)

    def test_area(self):
        self.assertAlmostEqual(area_m2(CUADRADO), 1_000_000.0)


class PruebasRejilla(unittest.TestCase):
    def test_cuadrado_rumbo_norte(self):
        lineas = generar_lineas(CUADRADO, rumbo_grados=0, espaciado_m=100)
        self.assertEqual(len(lineas), 10)
        xs = sorted(l.entrada[0] for l in lineas)
        for esperado, obtenido in zip(range(50, 1000, 100), xs):
            self.assertAlmostEqual(obtenido, esperado)
        for l in lineas:
            self.assertAlmostEqual(l.longitud_util_m, 1000.0)

    def test_sentido_alterno(self):
        lineas = generar_lineas(CUADRADO, rumbo_grados=0, espaciado_m=250)
        self.assertGreater(lineas[0].salida[1], lineas[0].entrada[1])
        self.assertLess(lineas[1].salida[1], lineas[1].entrada[1])

    def test_extension_fuera_del_area(self):
        lineas = generar_lineas(CUADRADO, rumbo_grados=90, espaciado_m=200, extension_m=60)
        for l in lineas:
            self.assertAlmostEqual(l.longitud_total_m, l.longitud_util_m + 120)

    def test_mejor_rumbo_rectangulo_alargado(self):
        rectangulo = [(0.0, 0.0), (3000.0, 0.0), (3000.0, 500.0), (0.0, 500.0)]
        self.assertIn(mejor_rumbo(rectangulo, 100), (89.0, 90.0, 91.0))

    def test_poligono_invalido(self):
        with self.assertRaises(ValueError):
            generar_lineas(CUADRADO[:2], 0, 100)


class PruebasMision(unittest.TestCase):
    def setUp(self):
        self.area = leer_area_geojson(EJEMPLO)
        self.despegue = (-0.5300, -78.5800)

    def test_estructura(self):
        plan = planificar(self.area, self.despegue, ParametrosMapeo(CAMARAS["pi-hq-6mm"], 100))
        comandos = [it.comando for it in plan.items]
        self.assertEqual(comandos[1], NAV_VTOL_TAKEOFF)
        self.assertEqual(comandos[-1], NAV_VTOL_LAND)
        disparos = [it.p1 for it in plan.items if it.comando == DO_SET_CAM_TRIGG_DIST]
        self.assertEqual(len(disparos), 2 * len(plan.lineas_geo))
        self.assertTrue(all(d > 0 for d in disparos[0::2]))
        self.assertTrue(all(d == 0 for d in disparos[1::2]))
        self.assertGreater(plan.area_ha, 100)
        self.assertGreater(plan.fotos_estimadas, 0)

    def test_terreno(self):
        plan = planificar(self.area, self.despegue, ParametrosMapeo(CAMARAS["pi3"], 100, seguir_terreno=True))
        puntos = [it for it in plan.items[1:] if it.comando == NAV_WAYPOINT]
        self.assertTrue(all(it.marco == FRAME_TERRENO for it in puntos))
        self.assertFalse(any("relativa al despegue" in a for a in plan.advertencias))

    def test_advierte_altitud_ilegal(self):
        plan = planificar(self.area, self.despegue, ParametrosMapeo(CAMARAS["pi3"], 150))
        self.assertTrue(any("DGAC" in a for a in plan.advertencias))

    def test_advierte_autonomia(self):
        plan = planificar(self.area, self.despegue, ParametrosMapeo(CAMARAS["pi3"], 60, autonomia_min=10))
        self.assertTrue(any("autonomía" in a for a in plan.advertencias))

    def test_traslape_invalido(self):
        with self.assertRaises(ValueError):
            planificar(self.area, self.despegue, ParametrosMapeo(CAMARAS["pi3"], 100, traslape_frontal=1.2))

    def test_formato_waypoints(self):
        plan = planificar(self.area, self.despegue, ParametrosMapeo(CAMARAS["pi3"], 100))
        filas = exportar_waypoints(plan.items).splitlines()
        self.assertEqual(filas[0], "QGC WPL 110")
        self.assertEqual(len(filas), len(plan.items) + 1)
        for i, fila in enumerate(filas[1:]):
            campos = fila.split("\t")
            self.assertEqual(len(campos), 12)
            self.assertEqual(int(campos[0]), i)
            self.assertEqual(campos[1], "1" if i == 0 else "0")
            self.assertTrue(math.isfinite(float(campos[10])))


class PruebasCli(unittest.TestCase):
    def test_planificar_escribe_archivos(self):
        with tempfile.TemporaryDirectory() as tmp:
            salida = Path(tmp) / "m.waypoints"
            codigo = main([
                "planificar", "--area", str(EJEMPLO), "--despegue", "-0.53,-78.58",
                "--gsd", "3", "--salida", str(salida),
            ])
            self.assertEqual(codigo, 0)
            self.assertTrue(salida.read_text().startswith("QGC WPL 110"))
            vista = json.loads(salida.with_suffix(".geojson").read_text())
            self.assertEqual(vista["features"][0]["geometry"]["type"], "Polygon")

    def test_area_sin_poligono(self):
        with tempfile.TemporaryDirectory() as tmp:
            ruta = Path(tmp) / "x.geojson"
            ruta.write_text('{"type": "Point", "coordinates": [0, 0]}')
            with self.assertRaises(ValueError):
                leer_area_geojson(ruta)


if __name__ == "__main__":
    unittest.main()
