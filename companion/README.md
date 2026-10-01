# Software de a bordo (Raspberry Pi 5)

C++17 con CMake. El núcleo `evasion` no tiene dependencias externas: compila igual en tu PC y en la Raspberry Pi.

## Compilar y probar

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure   # o ./build/pruebas_evasion
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
| `src/simular_piedra.cpp` | Programa de demostración |
| `pruebas/pruebas_evasion.cpp` | Pruebas unitarias |

## Próximos módulos (fases 5 y 6)
- `puente_mavlink`: conexión a ArduPilot con [MAVSDK](https://mavsdk.mavlink.io/) (C++). Recibe la actitud y el modo; envía las maniobras en GUIDED.
- `vision`: captura con libcamera, inferencia en Hailo, rastreador y conversión de píxeles a direcciones NED.
- `captura` (Python): toma la foto de mapeo cuando ArduPilot da la orden y escribe la posición en el EXIF.
