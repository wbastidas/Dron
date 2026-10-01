# 03 · Lista de materiales

> **Precios orientativos en USD** (referencias de 2025–2026, sin envío ni impuestos). Cambian seguido: confírmalos antes de comprar. Busca siempre la **tienda oficial** de la marca en AliExpress ("RadioMaster Official Store", "BETAFPV Official Store", "HEEWING Official Store", "Mateksys") para evitar falsificaciones.
>
> Antes de comprar, lee [04 · Compras y aduanas](04-compras-y-aduanas-ecuador.md): en Ecuador el régimen 4x4 limita cada paquete a USD 400 y 4 kg, con un cupo de USD 1600 al año por persona.

La compra se divide en **dos etapas**. La etapa 1 permite aprender a volar y hacer las primeras misiones de mapeo. La etapa 2 agrega la computadora de a bordo y la IA. Así el gasto se reparte y no se compra nada antes de necesitarlo.

## Etapa 1 · Volar y mapear (≈ USD 1 250–1 450)

| # | Componente | Modelo sugerido | Precio aprox. | Dónde buscar | Notas |
|---|---|---|---|---|---|
| 1 | Radio de control | **RadioMaster TX16S MKII** (versión ELRS) | 200 | AliExpress (RadioMaster Official), Amazon | **Cómprala primero**: sirve como mando del simulador en la fase 0 |
| 2 | Módulo de radio largo alcance | **ELRS 900 MHz, formato JR/Micro** (BetaFPV Micro TX 900 MHz o RadioMaster equivalente) | 50–70 | AliExpress (BETAFPV Official) | Banda 902–928 MHz. Elige la versión "915/FCC" |
| 3 | Receptor | **ELRS 900 MHz con diversidad** (BetaFPV SuperD 900 MHz o similar) | 30–40 | AliExpress (BETAFPV Official) | Debe ser de la misma banda que el módulo |
| 4 | Avión VTOL | **HEEWING T2 Cruza VTOL PNP** (con controladora de fábrica) | 650–780 | AliExpress (HEEWING Official), tiendas FPV | Envergadura 1200 mm, batería 6S. Confirma con el vendedor si incluye GPS y sensor de velocidad. **Supera USD 400**: ver aduanas |
| 5 | GPS + brújula | Matek **M10Q-5883** (u-blox M10) | 35–45 | AliExpress, Amazon | Solo si el kit no lo incluye |
| 6 | Sensor de velocidad aerodinámica | Matek **ASPD-4525** + tubo pitot | 35–45 | AliExpress | Muy recomendado en ala fija: evita entradas en pérdida con viento |
| 7 | Baterías (×2) | **6S2P Li-ion 21700** (celdas Molicel P45B, ~9 000 mAh) o LiPo 6S 5 000 mAh para empezar | 100–140 c/u | **Comprar en Ecuador** de preferencia | Los couriers restringen el envío de baterías de litio |
| 8 | Cargador | ToolkitRC M7AC o ISDT 608AC (con fuente incluida) | 60–80 | AliExpress, Amazon | Que cargue 6S y balancee |
| 9 | Herramientas de soldadura | Cautín **Pinecil V2** + estaño 63/37 con flux + flux en pasta + termorretráctil | 50–60 | AliExpress, Amazon | Practica en una placa de prueba antes de tocar el avión |
| 10 | Seguridad y montaje | Multímetro, bolsa ignífuga para baterías, conectores XT60/XT90, velcro, bridas, llaves Allen | 50–70 | Ferretería local, Amazon | |
| 11 | Repuestos | Hélices de crucero y VTOL (2 juegos), 2 servos | 30–50 | Misma tienda del avión | Algo se rompe en las primeras pruebas |

## Etapa 2 · Computadora de a bordo e IA (≈ USD 450–550)

| # | Componente | Modelo sugerido | Precio aprox. | Dónde buscar | Notas |
|---|---|---|---|---|---|
| 12 | Controladora de vuelo (mejora) | **Matek H743-WING V3** + microSD 32 GB | 100–110 | AliExpress (Mateksys), GetFPV | Más memoria y puertos: seguimiento de terreno, MAVLink a la Pi, telémetro. Requiere soldar |
| 13 | Computadora de a bordo | **Raspberry Pi 5 (8 GB)** + disipador activo + microSD 64 GB A2 | 95–110 | Distribuidores en Ecuador, Amazon | |
| 14 | Acelerador de IA | **Raspberry Pi AI HAT+** (Hailo-8L, 13 TOPS) | 70–80 | Distribuidores autorizados Raspberry Pi | Ejecuta YOLO a 30+ fps sin cargar la CPU |
| 15 | Cámara de mapeo | **Raspberry Pi HQ Camera** + lente de **6 mm** (montura CS) | 75–90 | Distribuidores Raspberry Pi | 12 MP; mirando hacia abajo |
| 16 | Cámara de amenazas | **Raspberry Pi Global Shutter Camera** + lente gran angular | 60–75 | Distribuidores Raspberry Pi | El obturador global no deforma objetos rápidos |
| 17 | Cables de cámara | 2 cables CSI para Pi 5 (22→15 pines, 300 mm) | 10 | Amazon, AliExpress | La Pi 5 usa el conector pequeño |
| 18 | Alimentación de la Pi | **BEC 5 V / 5 A** de entrada 2–6S | 12–20 | AliExpress (Matek, Holybro) | La Pi 5 se reinicia si le falta corriente |
| 19 | Telémetro láser | **Benewake TF02-Pro** (40 m) o TFmini-S (12 m, más barato) | 40–75 | AliExpress (Benewake Official) | Aterrizajes precisos sobre terreno irregular |

## Opcionales (cuando el proyecto madure)

| Componente | Para qué | Precio aprox. |
|---|---|---|
| Radios de telemetría dedicados (Holybro SiK 915 MHz o RFD900x) | Telemetría más robusta que la de ELRS | 60–300 |
| Antena direccional (patch) para la estación | Más alcance en montaña | 30–60 |
| Gafas/emisor de video digital | Ver en vivo lo que ve el dron | 200+ |
| GPS RTK/PPK | Precisión centimétrica sin puntos de control | 300+ |

## Estación en tierra (lo que ya deberías tener)
- Una laptop (Windows recomendado para Mission Planner; QGroundControl corre en cualquier sistema).
- Para WebODM: 16 GB de RAM o más (32 GB para más de 1 000 fotos), o usar el servicio en la nube.

## Presupuesto total
| Etapa | Rango |
|---|---|
| Etapa 1 | USD 1 250–1 450 |
| Etapa 2 | USD 450–550 |
| **Total** | **USD 1 700–2 000** (más envíos y aranceles) |

Si el presupuesto se ajusta, lo primero que puede esperar es la etapa 2 completa: el avión ya mapea solo con una cámara que dispare por intervalo de tiempo.

## Estimación de peso (a verificar con balanza)

| Elemento | Peso aprox. |
|---|---|
| T2 Cruza VTOL listo para volar, sin batería | 1 600–1 900 g (confirmar) |
| Batería 6S2P 21700 | 850–900 g |
| Pi 5 + AI HAT+ + disipador | ~90 g |
| 2 cámaras con lentes | ~110 g |
| BEC, telémetro, cables, soportes | ~120 g |
| **Total** | **≈ 2 800–3 100 g** (máximo del fabricante: 3 500 g) |

Si te pasas del peso, usa una batería más pequeña: pierdes autonomía, pero el avión vuela seguro. Cualquier componente nuevo obliga a **revisar el centro de gravedad** (CG) antes de volar.
