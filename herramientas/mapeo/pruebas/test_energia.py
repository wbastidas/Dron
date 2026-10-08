import unittest

from mapeo.cli import _unir_coordenadas, main
from mapeo.energia import GRAVEDAD, ModeloEnergia, autonomia, bateria_para, densidad_relativa, potencia_crucero_w

MODELO = ModeloEnergia(masa_sin_bateria_kg=2.1)


class PruebasEnergia(unittest.TestCase):
    def test_potencia_de_crucero_a_mano(self):
        # 3 kg a 16 m/s, rendimiento 0.55, L/D 6: 3*9.80665*16/(0.55*6) = 142.65 W (+12 W de electrónica)
        modelo = ModeloEnergia(masa_sin_bateria_kg=3.0, finura=6.0, rendimiento=0.55)
        esperado = 3.0 * GRAVEDAD * 16.0 / (0.55 * 6.0) + 12.0
        self.assertAlmostEqual(potencia_crucero_w(3.0, modelo), esperado, places=6)

    def test_autonomia_a_mano(self):
        modelo = ModeloEnergia(masa_sin_bateria_kg=2.1)
        r = autonomia(194.0, modelo)
        masa = 2.1 + 194.0 / 200.0
        util = 194.0 * 0.85 * 0.80
        hover = (180.0 * masa + 12.0) * 120.0 / 3600.0
        p = masa * GRAVEDAD * 16.0 / (0.55 * 6.0) + 12.0
        self.assertAlmostEqual(r.minutos_crucero, (util - hover) / p * 60.0, places=6)

    def test_mas_bateria_da_mas_tiempo(self):
        self.assertGreater(autonomia(250, MODELO).minutos_crucero, autonomia(150, MODELO).minutos_crucero)

    def test_mejor_finura_da_mas_tiempo(self):
        malo = ModeloEnergia(masa_sin_bateria_kg=2.1, finura=5.0)
        bueno = ModeloEnergia(masa_sin_bateria_kg=2.1, finura=8.0)
        self.assertGreater(autonomia(194, bueno).minutos_crucero, autonomia(194, malo).minutos_crucero)

    def test_bateria_muy_pequena_no_alcanza_ni_para_el_hover(self):
        self.assertEqual(autonomia(5.0, MODELO).minutos_crucero, 0.0)

    def test_ida_y_vuelta(self):
        necesaria = bateria_para(60.0, MODELO)
        self.assertAlmostEqual(necesaria.minutos_crucero, 60.0, places=1)
        self.assertAlmostEqual(autonomia(necesaria.bateria_wh, MODELO).minutos_crucero, 60.0, places=1)

    def test_objetivo_mayor_exige_mas_bateria(self):
        self.assertGreater(bateria_para(90, MODELO).bateria_wh, bateria_para(60, MODELO).bateria_wh)

    def test_masa_maxima(self):
        with self.assertRaises(ValueError):
            bateria_para(60.0, MODELO, masa_maxima_kg=3.0)

    def test_no_converge_si_es_imposible(self):
        pesado = ModeloEnergia(masa_sin_bateria_kg=2.0, finura=0.3, energia_especifica_wh_kg=20.0)
        with self.assertRaises(ValueError):
            bateria_para(600.0, pesado)

    def test_densidad_de_la_atmosfera_estandar(self):
        self.assertAlmostEqual(densidad_relativa(0.0), 1.0, places=9)
        # ISA: a 3000 m la densidad es 0.9093 kg/m3 frente a 1.225 kg/m3 (0.742)
        self.assertAlmostEqual(densidad_relativa(3000.0), 0.742, places=3)

    def test_altitud_reduce_la_autonomia(self):
        alto = ModeloEnergia(masa_sin_bateria_kg=2.1, altitud_msnm=3000.0)
        nivel_del_mar = autonomia(194, MODELO).minutos_crucero
        en_altura = autonomia(194, alto).minutos_crucero
        self.assertLess(en_altura, nivel_del_mar)
        # La parte mecánica sube 1/sqrt(0.742) = 1.161: la autonomía baja entre 10 % y 20 %.
        self.assertGreater(en_altura / nivel_del_mar, 0.80)
        self.assertLess(en_altura / nivel_del_mar, 0.90)

    def test_validaciones(self):
        for mal in (
            ModeloEnergia(masa_sin_bateria_kg=0),
            ModeloEnergia(masa_sin_bateria_kg=2, finura=-1),
            ModeloEnergia(masa_sin_bateria_kg=2, rendimiento=1.5),
            ModeloEnergia(masa_sin_bateria_kg=2, reserva=1.0),
        ):
            with self.assertRaises(ValueError):
                autonomia(100, mal)
        with self.assertRaises(ValueError):
            autonomia(0, MODELO)
        with self.assertRaises(ValueError):
            bateria_para(0, MODELO)


class PruebasCliAutonomia(unittest.TestCase):
    def test_comando(self):
        self.assertEqual(main(["autonomia", "--masa-sin-bateria", "2.1", "--bateria-wh", "194"]), 0)
        self.assertEqual(main(["autonomia", "--masa-sin-bateria", "2.1", "--objetivo", "60"]), 0)

    def test_error_se_reporta_sin_traza(self):
        self.assertEqual(main(["autonomia", "--masa-sin-bateria", "2.1", "--objetivo", "60", "--masa-maxima", "2.5"]), 1)

    def test_coordenadas_de_powershell(self):
        # PowerShell entrega "-0.53,-78.58" sin comillas como dos argumentos.
        self.assertEqual(
            _unir_coordenadas(["planificar", "--despegue", "-0.53", "-78.58", "--gsd", "3"]),
            ["planificar", "--despegue=-0.53,-78.58", "--gsd", "3"],
        )
        self.assertEqual(
            _unir_coordenadas(["planificar", "--despegue", "-0.53,-78.58", "--gsd", "3"]),
            ["planificar", "--despegue=-0.53,-78.58", "--gsd", "3"],
        )


if __name__ == "__main__":
    unittest.main()
