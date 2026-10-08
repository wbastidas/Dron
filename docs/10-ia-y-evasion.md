# 10 · IA: no chocar y esquivar objetos lanzados

## Tres capas de protección

| Capa | Qué evita | Cómo | Estado |
|---|---|---|---|
| 1. Planificación | Chocar con cerros, árboles, antenas | Misiones con altitud sobre el terreno (`mapeo --terreno` + datos SRTM en ArduPilot) y margen de altura | Listo en el planificador |
| 2. Aterrizaje | Golpear el suelo o rocas al bajar | Telémetro láser hacia abajo; aterrizaje lento los últimos metros | Fase 5 |
| 3. Reacción | Objetos lanzados, aves | Cámara + red neuronal + estimación de trayectoria + maniobra | **Núcleo listo**, visión en fase 6 |

La capa 1 es la que más protege: a 100–120 m sobre el terreno, el avión queda fuera del alcance de una piedra lanzada a mano. Las capas 2 y 3 cubren el despegue y el aterrizaje, cuando el dron está bajo.

## Cómo funciona la capa 3

```
cámara (60 fps) → detector YOLO (Hailo) → rastreador → EvaluadorAmenaza → PlanificadorEvasion → ArduPilot
                   "hay un objeto aquí"    "es el mismo     "¿dónde estará y     "¿hacia dónde y a
                                            de antes"       a qué distancia      qué velocidad me
                                                            pasará de mí?"       aparto?"
```

### EvaluadorAmenaza (`companion/src/amenaza.cpp`)
1. Con cada detección calcula la **posición relativa** del objeto: dirección (desde la cámara y la actitud) × distancia. La distancia se mide (cámara de profundidad) o se estima por el tamaño aparente, suponiendo un objeto de 6 cm.
2. Con las últimas 4–10 posiciones ajusta, por **mínimos cuadrados**, una trayectoria **parabólica**: los objetos lanzados caen por la gravedad.
3. Busca el instante de **máximo acercamiento** en los próximos 2.5 s.
4. Si el objeto pasará a menos de **1.2 m** (semienvergadura de 0.6 m más margen), es un peligro.

### PlanificadorEvasion (`companion/src/planificador.cpp`)
- Se mueve **en sentido contrario** al lado por donde pasará el objeto.
- Si viene directo, se mueve en perpendicular a la línea de visión, **prefiriendo subir**.
- **Nunca baja** cerca del suelo (menos de 9 m) ni pica en modo avión.
- Calcula la velocidad necesaria descontando la latencia, y avisa si no hay tiempo físico para esquivar.

## Resultados de la simulación

`./build/simular_piedra 300 <alcance> <latencia>`: 300 piedras de 8 cm lanzadas a 15–30 m/s, apuntadas al dron en hover entre 8 y 25 m de altura.

| Distancia a la que se detecta la piedra | Latencia de actuación | Impactos sin evasión | Impactos con evasión |
|---|---|---|---|
| 30 m | 0.15 s | 300 | **0** |
| 12 m | 0.15 s | 300 | **52** (se evita el 83 %) |
| 8 m | 0.25 s | 300 | **241** (se evita el 20 %) |
| 5 m | 0.30 s | 300 | **294** (se evita el 2 %) |

**Conclusión:** el algoritmo funciona; lo que decide el resultado es **a qué distancia se detecta el objeto**. Una piedra de 8 cm a 12 m ocupa unos 6–7 píxeles en una cámara gran angular de 1 456 píxeles, y a 30 m apenas 2–3. La simulación también es optimista en otros puntos: ve la piedra en todas las direcciones y en todos los fotogramas. En la realidad, la cámara de amenazas cubre solo hacia adelante y abajo. Por eso la capa 1 (altura) es la defensa principal.

## Plan de la fase 6: visión

### 1. Datos
- Graba con la **Global Shutter Camera** (sin deformación en objetos rápidos) a 60 fps, primero en tierra sobre un trípode y apuntando al cielo y al terreno.
- Lanza **pelotas de espuma** y objetos blandos de 5–10 cm a distintas distancias, con distintos fondos, luz y horas.
- Meta: 2 000–5 000 imágenes etiquetadas. Etiqueta con [CVAT](https://www.cvat.ai/) o Roboflow (clase única: `objeto_lanzado`; opcional: `ave`).

### 2. Entrenamiento (Python, en una PC con GPU o en Google Colab)
```bash
pip install ultralytics
yolo detect train data=objetos.yaml model=yolov8n.pt imgsz=640 epochs=150
```
Modelo pequeño (`n`): en el Hailo-8L corre a decenas de fps.

### 3. Compilar para Hailo
El modelo se exporta a ONNX y se compila al formato `.hef` con el *Hailo Dataflow Compiler* (necesita una PC con Linux x86). Raspberry Pi y Hailo publican ejemplos de YOLO listos (`hailo-rpi5-examples`).

### 4. Qué ya está resuelto en `companion/`
Entre la salida del detector y ArduPilot ya existe código probado:
- **Geometría** (`geometria.hpp`): pasa cada detección a una dirección NED usando la actitud del avión. Así el giro del avión no se confunde con movimiento del objeto.
- **Percepción** (`percepcion.cpp`): asocia detecciones entre fotogramas, exige 3 seguidas para confirmar una pista (una falsa alarma de un fotograma se ignora) y entrega la amenaza más urgente.
- **Supervisor** (`supervisor.cpp`): solo actúa en QLOITER/QHOVER, cede el control si el piloto cambia de modo y devuelve el modo anterior al terminar o a los 6 s.

La prueba de cadena completa (piedra de 8 cm → píxeles con ruido de 0.3 px → percepción → supervisor → dron simulado con 0.15 s de latencia) esquiva 24 de 24 lanzamientos. **Con 0.6 s de latencia esquiva solo la mitad**: la latencia total (cámara + inferencia + MAVLink + respuesta del dron) es el número que hay que medir en el avión real. Esa prueba asume un detector perfecto que ve la piedra desde el lanzamiento; el detector real es la parte que falta validar.

### 5. Detección por movimiento (complemento)
A más de 15 m, una piedra ocupa tan pocos píxeles que la red neuronal no la reconoce. Se agrega un detector clásico: diferencia entre fotogramas compensada por el movimiento del dron, que encuentra objetos pequeños **que se mueven distinto del fondo**. Es más rápido que la red y detecta antes; la red confirma y reduce las falsas alarmas.

### 6. Integración y pruebas
1. En SITL: inyectar detecciones simuladas y verificar que `puente_mavlink` cambia a GUIDED, envía la velocidad y vuelve al modo anterior.
2. En vuelo: hover a 15 m sobre campo abierto, pelotas de espuma lanzadas desde lejos por una persona protegida. **Nunca con piedras reales ni con personas debajo del dron.**

## ¿Por qué no aprendizaje por refuerzo?
"Que aprenda a no chocarse" suena a que el dron aprende solo volando, pero entrenar así en un avión real significa estrellarlo miles de veces. El enfoque profesional es este:
- **Aprendizaje supervisado** para *percibir* (la red neuronal reconoce objetos a partir de ejemplos etiquetados).
- **Física y control clásico** para *decidir y actuar* (predicción de trayectoria y maniobra): es predecible, verificable con pruebas y no hace cosas raras.

El aprendizaje por refuerzo se puede explorar más adelante **solo en simulación** (Gazebo + SITL) como proyecto de investigación.
