# Configuración de ArduPilot (ArduPlane QuadPlane)

> El T2 Cruza VTOL viene **afinado de fábrica**. No cambies parámetros de control (PID, `Q_A_*`, `PTCH_*`, `RLL_*`, transiciones) hasta tener experiencia. Antes de tocar nada, **guarda una copia de todos los parámetros**: *Config → Full Parameter List → Save*.
>
> Los nombres y valores pueden cambiar entre versiones de ArduPilot. Confirma cada uno en *Full Parameter List* de Mission Planner, que muestra la descripción y los valores válidos de **tu** versión.

## 1. Modos de vuelo (interruptor de 6 posiciones de la radio)

| Posición | Parámetro | Modo | Valor | Para qué |
|---|---|---|---|---|
| 1 | `FLTMODE1` | QLOITER | 19 | Hover con posición fija por GPS: el modo "seguro" |
| 2 | `FLTMODE2` | QHOVER | 18 | Hover manteniendo altura, sin GPS |
| 3 | `FLTMODE3` | FBWA | 5 | Avión asistido: no deja inclinar demasiado |
| 4 | `FLTMODE4` | AUTO | 10 | Ejecuta la misión |
| 5 | `FLTMODE5` | RTL | 11 | Regresa a casa |
| 6 | `FLTMODE6` | QLAND | 20 | Aterrizaje vertical donde está |

## 2. Seguridad (failsafes)

| Parámetro | Valor sugerido | Efecto |
|---|---|---|
| `THR_FAILSAFE` | 1 | Detecta la pérdida de la señal de radio |
| `FS_LONG_ACTN` | 1 | RTL tras una pérdida larga de radio |
| `Q_RTL_MODE` | revisar opciones | Que el RTL termine con **aterrizaje vertical** en casa |
| `BATT_FS_LOW_ACT` | RTL | Regresa con batería baja |
| `BATT_FS_CRT_ACT` | QLAND | Aterriza donde esté con batería crítica |
| `BATT_LOW_VOLT` | 19.8 (Li-ion 6S) / 21.0 (LiPo 6S) | 3.3 V/celda Li-ion; 3.5 V/celda LiPo |
| `BATT_CRT_VOLT` | 18.6 (Li-ion 6S) / 19.8 (LiPo 6S) | 3.1 V/celda Li-ion; 3.3 V/celda LiPo |
| `FENCE_ENABLE` | 1 | Activa la geocerca (dibuja el polígono en Mission Planner) |
| `FENCE_ACTION` | 1 (RTL) | Qué hacer al salir de la geocerca |

> **Geocerca de altura en montaña:** `FENCE_ALT_MAX` se mide **respecto al punto de despegue**, no al terreno. Si despegas en un valle y mapeas una ladera más alta, el avión puede superar el límite aunque esté a 100 m del suelo. Calcula el valor según el desnivel de la zona.

## 3. Mapeo y terreno

| Parámetro | Valor | Para qué |
|---|---|---|
| `TERRAIN_ENABLE` | 1 | Usa el relieve SRTM (requiere tarjeta SD en la controladora; lo descarga la estación de tierra) |
| `ARSPD_USE` | 1 | Usa el sensor de velocidad aerodinámica |
| `CAM1_TYPE` | MAVLink | Envía la orden de disparo a la Raspberry Pi (fase 5) |

## 4. Puertos serie (etapa 2, Matek H743-WING)

Mira la tabla de UART de la placa en la documentación de ArduPilot y completa la columna `SERIALn`:

| Dispositivo | SERIALn | `SERIALn_PROTOCOL` | `SERIALn_BAUD` |
|---|---|---|---|
| Receptor ELRS | | 23 (RCIN) | — (más `RSSI_TYPE = 3`) |
| GPS | | 5 (GPS) | — (autodetecta) |
| Telémetro TF02-Pro | | 9 (Rangefinder) | 115 |
| Raspberry Pi por UART (opcional) | | 2 (MAVLink2) | 921 |

Por USB no hace falta configurar nada: es `SERIAL0` con MAVLink.

## 5. Lista de verificación antes de cada vuelo

**En casa**
- [ ] Baterías cargadas y balanceadas; sin golpes ni hinchazón
- [ ] Batería de la radio cargada
- [ ] Misión planificada y revisada en el mapa (`mapeo` sin advertencias críticas)
- [ ] Pronóstico: viento menor a 8 m/s, sin lluvia
- [ ] Permisos y documentos a mano

**En el campo, antes de armar**
- [ ] Inspección visual: hélices sin fisuras, servos firmes, alas bien trabadas
- [ ] Centro de gravedad correcto con la batería instalada
- [ ] Encender la radio **antes** que el avión
- [ ] GPS con 3D fix y 12+ satélites; HDOP < 1.0
- [ ] Superficies de control: mover los sticks y ver el sentido correcto
- [ ] Sensor de velocidad: soplar el pitot y ver que sube la lectura
- [ ] Misión cargada en el avión (*Read WPs* para confirmar)
- [ ] Zona de despegue despejada, sin personas a menos de 15 m

**Después de despegar**
- [ ] Hover estable en QLOITER a 10 m durante 10 s antes de pasar a AUTO
- [ ] Voltaje y consumo dentro de lo esperado

**Después de aterrizar**
- [ ] Desarmar, desconectar la batería
- [ ] Descargar el log (*DataFlash Logs*) y anotar el tiempo de vuelo y los mAh consumidos

## Referencias
- [QuadPlane (ArduPilot)](https://ardupilot.org/plane/docs/quadplane-support.html)
- [Opciones de puertos serie](https://ardupilot.org/plane/docs/common-serial-options.html)
- [Crossfire y ELRS en ArduPilot](https://ardupilot.org/plane/docs/common-tbs-rc.html)
- [Controles de cámara](https://ardupilot.org/copter/docs/common-camera-controls.html)
