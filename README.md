# Dron VTOL de cartografía para zonas de difícil acceso

Proyecto para construir, programar y operar un dron de **despegue vertical y vuelo de avión (VTOL)** que levanta cartografía (ortomosaicos y modelos 3D) en lugares inseguros o de difícil acceso en Ecuador, sin que el operador tenga que entrar a la zona.

## Resumen del diseño

| | |
|---|---|
| **Plataforma** | Ala fija VTOL (HEEWING T2 Cruza VTOL, 1.2 m de envergadura) |
| **Alcance** | Más de 5 km (radio ELRS 900 MHz). Requisito: ≥ 1 h de vuelo; con el T2 Cruza es el límite superior (31–56 min de crucero según la aerodinámica y la altura), ver [doc 11](docs/11-autonomia-1-hora.md) |
| **Piloto automático** | ArduPilot (QuadPlane), configurado, no reescrito |
| **Computadora de a bordo** | Raspberry Pi 5 + acelerador de IA Hailo |
| **Mapeo** | Cámara nadir 12 MP, ~3 cm/píxel a 116 m; procesamiento en WebODM |
| **IA** | Detección de objetos lanzados + predicción de trayectoria + maniobra evasiva (C++) |
| **Presupuesto** | USD 1 700–2 000 en dos etapas |

## Documentación (léela en orden)

1. [Requisitos y decisiones](docs/01-requisitos-y-decisiones.md): por qué VTOL, por qué ArduPilot, Python o C++
2. [Arquitectura](docs/02-arquitectura.md): componentes, flujo de datos, procesos
3. [Lista de materiales](docs/03-lista-de-materiales.md): qué comprar, modelos, precios y dónde
4. [Compras y aduanas en Ecuador](docs/04-compras-y-aduanas-ecuador.md): régimen 4x4 y plan de paquetes
5. [Normativa y seguridad](docs/05-normativa-y-seguridad.md): DGAC, BVLOS, seguridad personal
6. [Planos de integración y cableado](docs/06-planos-de-cableado.md)
7. [**Hoja de ruta paso a paso**](docs/07-hoja-de-ruta.md): fases con criterios de salida
8. [Simulación con ArduPilot SITL](docs/08-simulacion-sitl.md)
9. [Mapeo y procesamiento](docs/09-mapeo-y-procesamiento.md)
10. [IA: no chocar y esquivar objetos lanzados](docs/10-ia-y-evasion.md)
11. [**Autonomía: ¿se puede volar 1 hora?**](docs/11-autonomia-1-hora.md): modelo de energía, opciones y qué medir

Configuración del piloto automático y lista de verificación previa al vuelo: [firmware/ardupilot](firmware/ardupilot/README.md).

## Software

| Carpeta | Lenguaje | Qué hace | Estado |
|---|---|---|---|
| [`herramientas/mapeo`](herramientas/mapeo) | Python (solo biblioteca estándar) | Planifica misiones de fotogrametría para VTOL y las exporta a Mission Planner/QGC | ✅ Funcional, con pruebas |
| [`companion`](companion) | C++17 | Evasión de objetos lanzados: trayectoria, percepción (píxel→NED, rastreo), supervisor de modo y simulador | ✅ Funcional, con pruebas (incluye cadena completa simulada) |
| `companion` (visión, MAVLink) | C++ | Captura + detector YOLO en Hailo; enlace MAVSDK con ArduPilot | ⏳ Fase 6 (requiere hardware) |
| `captura` | Python | Disparo y geoetiquetado de fotos | ⏳ Fase 5 |

### Probar rápido

```bash
# Planificar una misión de ejemplo
cd herramientas/mapeo
python3 -m mapeo planificar --area ejemplos/area_ejemplo.geojson --despegue -0.53,-78.58 --gsd 3 --terreno

# Compilar y probar el núcleo de evasión
cd ../../companion
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
./build/simular_piedra 300 12 0.15
```

## Estado actual

**Fase 0** (preparación y simulación). Próximo paso: comprar la radio y empezar a practicar en el simulador mientras llegan las demás piezas. Ver la [hoja de ruta](docs/07-hoja-de-ruta.md).

> ⚠️ Volar más allá de la línea de vista requiere autorización de la DGAC. Las pruebas de evasión se hacen **solo con objetos blandos** y nunca con personas debajo del dron.
