# 11 · Autonomía: ¿se puede volar 1 hora?

**Requisito:** el dron debe poder volar al menos 1 hora. Para no engañarnos, el criterio estricto que usa este documento es **60 min de crucero con 20 % de batería de reserva**, además del despegue y el aterrizaje verticales.

> ### Corrección a lo que escribí antes
> En la primera versión dije que el T2 Cruza daría "40–60 min" con la batería 6S2P Li-ion. **Era un número sin respaldo.** Lo que sí encontré: un distribuidor europeo estima 15–20 min para el T2 Cruza VTOL con una LiPo 6S pequeña, y 25 min para la versión sin VTOL con 6S 3000 mAh ([fuente](https://www.rotorama.com/product/heewing-t2-cruza-vtol-pnp)). Con la batería grande, el modelo de abajo da **31–56 min de crucero según la aerodinámica y la altura**. Es decir: **1 hora con el T2 Cruza es el límite superior, no el caso normal.**

## Qué dicen las fuentes

| Dato | Fuente | Confiabilidad |
|---|---|---|
| T2 Cruza VTOL: 15–20 min en LiPo 6S, ~2 kg | Distribuidor europeo | Estimación de tienda |
| Ala volante X8 con rotores VTOL: pierde ~20 % de eficiencia frente al ala sin rotores; un Skywalker 1880 convertido voló ~40 min con una batería Li-ion 4S3P | [Foro de ArduPilot](https://discuss.ardupilot.org/t/skywalker-x8-tiltrotor/22691) | Reporte de un constructor |
| Foxtech Nimbus 1800: "1 hora" en teoría con una 6S 10 000 mAh; el propio fabricante recomienda aterrizar a los 45 min | [Foro de ArduPilot](https://discuss.ardupilot.org/t/foxtech-nimbus-1800-vtol-mapping-version/29620) | Fabricante, admite que es teórico |
| ArgusFPV ROC Wing VTOL (1.2 m): "hasta 80 min" con una 6S 6000 mAh | [Ficha del vendedor](https://www.fpv24.com/en/argusfpv/argusfpv-roc-wing-vtol-fpv-glider-airplane-pnp-gray) | Publicidad ("hasta"), sin verificar |

Conclusión: los anuncios de "1 hora o más" son cifras ideales. Los reportes de quienes vuelan estos aviones hablan de 40–45 min reales.

## El modelo de energía

`herramientas/mapeo/mapeo/energia.py` calcula el balance:

```
P_crucero = m·g·v / (rendimiento · L/D) · k_altitud + P_electrónica
E_util    = E_batería · 0.85 (fracción utilizable) · (1 − reserva)
t_crucero = (E_util − E_hover) / P_crucero
```

Es un modelo de primer orden. **Lo que más lo mueve es la finura aerodinámica (L/D)**, que no conocemos para este avión con los rotores instalados: por eso se muestra un rango.

```bash
cd herramientas/mapeo
python -m mapeo autonomia --masa-sin-bateria 2.1 --bateria-wh 194 --finura 6 --altitud-msnm 2900
python -m mapeo autonomia --masa-sin-bateria 2.1 --objetivo 60 --finura 7 --masa-maxima 3.5
```

Supuestos por defecto: 16 m/s, rendimiento motor+hélice 0.55, 180 W/kg en hover durante 120 s, batería de 200 Wh/kg, 12 W de electrónica.

### Resultados: T2 Cruza con la batería 6S2P 21700 (194 Wh, masa total 3.07 kg)

| Finura (L/D) | Potencia de crucero | Crucero a nivel del mar | Crucero a 2 900 msnm |
|---|---|---|---|
| 5 | 187 W | 36 min | 31 min |
| 6 | 158 W | 43 min | 37 min |
| 7 | 137 W | 49 min | 42 min |
| 8 | 121 W | 56 min | 48 min |

(Con 20 % de reserva ya descontada. Sin reserva, L/D 6 a 2 900 m daría 48 min.)

### Batería necesaria para 60 min de crucero con 20 % de reserva

| Avión | Finura | Nivel del mar | A 2 900 msnm |
|---|---|---|---|
| T2 Cruza (2.1 kg sin batería, **máx. 3.5 kg**) | 6 | 303 Wh · 3.61 kg ❌ | 384 Wh · 4.02 kg ❌ |
| | 7 | 248 Wh · 3.34 kg ✅ | 307 Wh · 3.63 kg ❌ |
| | 8 | 212 Wh · 3.16 kg ✅ | 258 Wh · 3.39 kg ✅ |
| Ala hipotética de ~1.8 m (3.2 kg sin batería) | 6 | 446 Wh · 5.43 kg | 567 Wh · 6.04 kg |
| | 8 | 309 Wh · 4.74 kg | 379 Wh · 5.09 kg |

❌ = supera el peso máximo de despegue del fabricante. La fila de 1.8 m es un ejemplo con masa supuesta, no un producto concreto.

### Lo que dicen estos números
1. **Con el T2 Cruza, 1 hora solo se logra con muy buena aerodinámica (L/D ≥ 8) y poca altitud.** En la sierra, ni así con 20 % de reserva.
2. **La altura sobre el nivel del mar importa.** A 2 900 m el aire tiene ~74 % de la densidad del nivel del mar: la potencia sube ~16 %, y además baja el empuje disponible de los rotores para despegar y aterrizar. Un avión que despega bien en la costa puede quedarse sin margen en el hover a 3 000 m. **Esto es un riesgo de seguridad, no solo de autonomía.**
3. **Terreno montañoso** (seguimiento del terreno) obliga a subir y bajar: suma consumo. Reserva 10–20 % más de energía en esas misiones.
4. **Volar más lento ayuda:** a 13 m/s, el T2 Cruza con L/D 6 necesita 232 Wh en lugar de 303 Wh. El límite es la velocidad de pérdida y el desenfoque por movimiento de la cámara.

## Opciones

| Opción | Qué implica | Cuándo conviene |
|---|---|---|
| **A. T2 Cruza + batería grande, y medir** | Se mantiene el presupuesto. Se compra una 6S3P 21700 (≈290 Wh, ~1.3 kg), se vuela en la fase 3 y se mide la finura real. Si da 45–50 min, se dividen las áreas en 2 vuelos | Quieres empezar ya y aceptas que 1 h exacta puede no llegar |
| **B. Avión más eficiente de tamaño similar** | Buscar un VTOL de ~1.2 m optimizado para autonomía (por ejemplo el ROC Wing que anuncia 80 min), verificando antes con pilotos que lo usen | Quieres 1 h con presupuesto similar, pero hay que verificar el anuncio y que haya stock |
| **C. Subir el presupuesto a un ala de 1.8–2.2 m** | Más batería, más margen en altura. Precios de referencia encontrados: Foxtech Loong 2160 a USD 2 899 (otro avión), un MFE HERO 2180 mm a £1 399 (agotado). El Nimbus no tiene precio publicado | El 1 h es obligatorio para la operación o vas a operar sobre 2 500 m |
| **D. Relajar el requisito** | "1 hora" como tiempo total en el aire sin reserva, o dos vuelos de 35–40 min con cambio de batería | El trabajo real permite aterrizar y cambiar de batería |

## Decisión registrada

- **Criterio:** estricto, 60 min de crucero con 20 % de reserva.
- **Presupuesto:** se mantiene en USD 2 000 (opciones A, B o D de la tabla).

**Consecuencia: con ese criterio y ese presupuesto, 1 hora no está garantizada, y en la sierra es improbable.** El único candidato que encontré en ese rango, además del T2 Cruza, es el ArgusFPV ROC Wing VTOL (1.2 m, anuncia "hasta 80 min" con una 6S 6000 mAh LiPo, 133 Wh). Comprobación con el modelo (masa total supuesta de 2.2 kg, LiPo de 150 Wh/kg, nivel del mar):

| Finura | Velocidad | Criterio estricto (con reserva) | Sin reserva ni margen de batería |
|---|---|---|---|
| 6 | 13 m/s | 48 min | 74 min |
| 7 | 13 m/s | 54 min | 85 min |
| 8 | 13 m/s | **61 min** | 95 min |
| 8 | 16 m/s | 51 min | 79 min |

El "hasta 80 min" del anuncio es coherente con L/D 7–8 volando lento y **sin reserva**: no contradice el modelo, pero tampoco cumple tu criterio. Solo lo cumpliría con L/D ≥ 8 a 13 m/s a nivel del mar; a 2 900 m, ni así. (La masa de 2.2 kg es un supuesto mío: la ficha que encontré no la da.)

**Qué hacer antes de comprar el avión (no comprarlo todavía):**
1. Pedir al vendedor o a dueños del avión un **registro de vuelo de ArduPilot** (`.bin`) con la corriente en crucero (`BAT.Curr`), la velocidad y el peso. Con eso se calcula la finura real y se reemplazan los supuestos.
2. Definir la **altitud de la zona de trabajo**. En la costa o el oriente (cerca del nivel del mar) el criterio estricto es mucho más alcanzable que en la sierra.
3. Si el avión real no llega: o se sube el presupuesto (opción C), o se acepta la opción D (dos vuelos con cambio de batería).

## Qué hay que medir en la fase 3 (decide todo)
1. Corriente en crucero a 14, 16 y 18 m/s (el registro de ArduPilot guarda `BAT.Curr`). Con eso se calcula la finura real: `L/D = m·g·v / (η·P)`.
2. Energía consumida en un despegue + aterrizaje vertical.
3. Autonomía real hasta el 20 % de batería, con y sin viento.
4. Repetir las pruebas a la altitud de la zona de trabajo, si es distinta de donde se prueba.

Con esos datos, se reemplazan los supuestos en `mapeo autonomia` y se decide entre A, B, C o D con números propios.

## Efecto en la misión de mapeo
A 3 cm/píxel, el ejemplo del planificador (187 ha) da ~65 min de vuelo, de modo que **1 h de vuelo ≈ 170 ha**. Si la autonomía real es de 40 min, serán ~110 ha por vuelo. `mapeo planificar --autonomia <min>` avisa cuando la misión no cabe con 20 % de reserva.
