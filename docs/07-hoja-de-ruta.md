# 07 · Hoja de ruta paso a paso

Cada fase tiene un **criterio de salida**: no pases a la siguiente hasta cumplirlo. Los tiempos son orientativos, para alguien que dedica unas 10 horas por semana.

## Fase 0 · Preparación y simulación (semanas 1–4)
- [x] Definir requisitos y arquitectura ([01](01-requisitos-y-decisiones.md), [02](02-arquitectura.md))
- [x] Planificador de misiones de mapeo (`herramientas/mapeo`)
- [x] Núcleo de evasión de amenazas con simulador (`companion/`)
- [ ] **Comprar solo la radio TX16S** y practicar en un simulador de aeromodelismo (PicaSim es gratuito; RealFlight es de pago y más realista). Meta: 10 horas de vuelo simulado en ala fija y en hover.
- [ ] Instalar ArduPilot SITL y volar el QuadPlane simulado ([08](08-simulacion-sitl.md))
- [ ] Cargar en SITL una misión generada con `mapeo` y verla completa
- [ ] Practicar soldadura en una placa de prueba (20 uniones limpias de pads y cables 14–22 AWG)
- [ ] Contactar a la DGAC para el registro, el seguro y la consulta BVLOS ([05](05-normativa-y-seguridad.md))
- [ ] Buscar un club de aeromodelismo en tu ciudad: un instructor acorta muchísimo la curva de aprendizaje

**Criterio de salida:** hiciste en SITL despegue VTOL, transición, misión AUTO y aterrizaje, y sabes recuperar el avión en QLOITER si algo falla.

## Fase 1 · Compras de la etapa 1 (semanas 3–8, en paralelo)
- [ ] Repartir las compras según [04](04-compras-y-aduanas-ecuador.md) (paquetes de USD 400 / 4 kg como máximo)
- [ ] Baterías y cargador comprados en Ecuador
- [ ] Revisar cada paquete al llegar: daños de transporte, piezas faltantes

## Fase 2 · Ensamblaje y configuración en banco (1–2 semanas)
**Siempre sin hélices.**
- [ ] Ensamblar según el manual del fabricante; conectar el receptor y el GPS ([06](06-planos-de-cableado.md))
- [ ] Vincular la radio con el receptor ELRS (misma *binding phrase*)
- [ ] En Mission Planner: calibrar acelerómetro, brújula y radio; revisar el sentido de servos y motores
- [ ] Aplicar la [lista de verificación de firmware](../firmware/ardupilot/README.md): modos de vuelo, failsafes, geocerca
- [ ] Probar el failsafe: apagar la radio → el avión debe pasar a RTL
- [ ] Verificar el centro de gravedad con la batería instalada

**Criterio de salida:** todas las superficies y motores responden en el sentido correcto, y los failsafes funcionan en banco.

## Fase 3 · Primeros vuelos (3–6 semanas)
En campo abierto, sin personas cerca, sin viento fuerte, idealmente con un instructor.
- [ ] Hover en **QLOITER** a 5–10 m: 5 vuelos cortos
- [ ] Prueba de RTL en hover
- [ ] Transición a avión a 60+ m y vuelo en **FBWA**; volver a hover (QLOITER) y aterrizar
- [ ] Misión AUTO corta: despegue VTOL, 4 waypoints en línea de vista, aterrizaje VTOL
- [ ] **Medir la autonomía real**: consumo (mAh por minuto) en crucero y en hover. Pasar el dato a `mapeo --autonomia`
- [ ] Revisar los registros (logs) de cada vuelo: vibraciones, consumo, calidad del GPS

**Criterio de salida:** 10 misiones AUTO completas sin intervención manual.

## Fase 4 · Primer mapeo (2–3 semanas)
- [ ] Elegir un área segura de 10–20 ha en línea de vista
- [ ] Cámara provisional: cualquier cámara que dispare por intervalo de tiempo, o la HQ Camera si ya tienes la etapa 2
- [ ] Colocar 5 puntos de control en tierra (marcas visibles) y medir sus coordenadas
- [ ] Volar la misión de `mapeo` y procesar con WebODM ([09](09-mapeo-y-procesamiento.md))
- [ ] Comparar el GSD y la precisión con lo planificado

**Criterio de salida:** un ortomosaico sin huecos ni deformaciones.

## Fase 5 · Computadora de a bordo (etapa 2 de compras, 4–6 semanas)
- [ ] Cambiar a la controladora Matek H743-WING (soldadura) y reconfigurar; repetir las pruebas de banco y un vuelo de hover
- [ ] Instalar Raspberry Pi OS (64 bits) y conectar la Pi a la controladora por MAVLink
- [ ] **Programar `captura`**: recibir la orden de disparo de ArduPilot, tomar la foto y guardar posición y actitud en el EXIF
- [ ] Activar el seguimiento de terreno (`TERRAIN_ENABLE=1`, `mapeo --terreno`)
- [ ] Instalar el telémetro láser para aterrizajes precisos

**Criterio de salida:** una misión de mapeo completa con fotos geoetiquetadas por la Pi.

## Fase 6 · Visión y evasión (6–10 semanas)
- [ ] Grabar video con la cámara de amenazas (primero en tierra, sobre un trípode) de **pelotas de espuma** lanzadas
- [ ] Etiquetar el dataset y entrenar YOLO; compilarlo para Hailo ([10](10-ia-y-evasion.md))
- [x] Rastreador, conversión píxel→NED y supervisor de la maniobra (`companion/`, probados con enlace simulado)
- [ ] Programar `vision` (captura + inferencia en Hailo) y `puente_mavlink` (MAVSDK)
- [ ] Probar todo contra ArduPilot SITL inyectando detecciones simuladas
- [ ] Prueba real: hover a 15 m sobre campo abierto, pelotas de espuma lanzadas desde lejos. **Nunca piedras reales**

**Criterio de salida:** la evasión se activa con pelotas lanzadas y nunca se activa sin motivo durante 5 vuelos normales.

## Fase 7 · Operación de largo alcance (continua)
- [ ] Autorización BVLOS de la DGAC (o la que corresponda)
- [ ] Pruebas de alcance progresivas: 1 km → 3 km → 5 km → más, siempre con RTL probado
- [ ] Procedimiento operativo escrito: chequeos previos, roles, plan de emergencia, retirada
- [ ] Primera operación real en una zona de interés, despegando desde un punto seguro
