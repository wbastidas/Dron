# Software de a bordo (Raspberry Pi 5)

C++17 con CMake. El núcleo `evasion` no tiene dependencias externas: compila igual en tu PC y en la Raspberry Pi.

## Compilar y probar

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure   # ejecuta pruebas_evasion y pruebas_vision
```

## Simulador de piedras lanzadas

```bash
./build/simular_piedra [lanzamientos] [alcance_deteccion_m] [latencia_s]
./build/simular_piedra 300 12 0.15
```

Compara impactos con y sin evasión. Los resultados y su interpretación están en [docs/10](../docs/10-ia-y-evasion.md).

## Estructura

| Archivo | Contenido |
|---|---|
| `include/evasion/vector.hpp` | Vector 3D en marco NED |
| `include/evasion/amenaza.hpp`, `src/amenaza.cpp` | `EvaluadorAmenaza`: ajusta la trayectoria (con gravedad) y predice el máximo acercamiento |
| `include/evasion/planificador.hpp`, `src/planificador.cpp` | `PlanificadorEvasion`: dirección y velocidad de escape |
| `include/evasion/simulacion.hpp`, `src/simulacion.cpp` | Simulador de lanzamientos con ruido de cámara, latencia y dinámica del dron |
| `include/evasion/geometria.hpp` | Rotaciones cuerpo→NED con la actitud de ArduPilot y modelo de cámara (píxel ↔ dirección NED) |
| `include/evasion/percepcion.hpp`, `src/percepcion.cpp` | `Percepcion`: detecciones en píxeles → pistas confirmadas → amenazas. Compensa el giro del avión |
| `include/evasion/supervisor.hpp`, `src/supervisor.cpp` | `Supervisor`: cuándo tomar el control (GUIDED) y cuándo soltarlo. Habla con el autopiloto mediante la interfaz `EnlaceVuelo` |
| `src/simular_piedra.cpp` | Programa de demostración |
| `pruebas/pruebas_evasion.cpp` | Pruebas del evaluador, el planificador y el simulador |
| `pruebas/pruebas_vision.cpp` | Pruebas de geometría, percepción y supervisor, más una prueba de la cadena completa (piedra → píxeles con ruido → percepción → supervisor → dron simulado) |

## Reglas de seguridad del supervisor

1. Solo actúa en **QLOITER** y **QHOVER**. En AUTO, crucero, RTL o cualquier otro modo no cambia nada.
2. Si durante la maniobra el modo deja de ser GUIDED (el piloto movió el interruptor, o saltó un failsafe), **cede el control y no lo recupera**.
3. Pasados `max_evasion_s` (6 s) devuelve el modo anterior pase lo que pase.
4. Una detección de un solo fotograma no activa nada: la pista debe confirmarse con 3 detecciones seguidas.
5. Si ArduPilot rechaza el cambio a GUIDED, no se envía ninguna velocidad.

## Próximos módulos
- `EnlaceMavsdk` (C++): implementación real de `EnlaceVuelo` con [MAVSDK](https://mavsdk.mavlink.io/). Debe leer también `ATTITUDE` para alimentar a `Percepcion`, y se prueba contra ArduPilot SITL.
- `vision`: captura con libcamera, inferencia en Hailo y conversión de la salida del detector a `Deteccion`.
- `captura` (Python): toma la foto de mapeo cuando ArduPilot da la orden y escribe la posición en el EXIF.
