# 08 · Simulación con ArduPilot SITL

SITL (*Software In The Loop*) ejecuta el **mismo firmware** de ArduPilot en tu computadora, con un modelo físico del avión. Sirve para aprender a usarlo, probar misiones y, más adelante, probar el software de a bordo sin arriesgar el avión.

## 1. Instalación (Ubuntu 22.04/24.04, o WSL2 en Windows)

```bash
git clone --recurse-submodules https://github.com/ArduPilot/ardupilot.git
cd ardupilot
Tools/environment_install/install-prereqs-ubuntu.sh -y
. ~/.profile
```

En Windows instala primero WSL2 con Ubuntu (`wsl --install` en PowerShell como administrador) y sigue los mismos pasos dentro de Ubuntu.

## 2. Lanzar un QuadPlane (VTOL) simulado

```bash
cd ardupilot
sim_vehicle.py -v ArduPlane -f quadplane --console --map \
  --custom-location=-0.5300,-78.5800,2900,0
```

`--custom-location` es latitud, longitud, altitud (msnm) y rumbo del punto de inicio. Pon el de tu zona.

Para conectar Mission Planner o QGroundControl, agrega `--out=udp:127.0.0.1:14550` (o la IP de Windows si usas WSL2) y conéctate por UDP en el puerto 14550.

## 3. Primer vuelo en la consola de MAVProxy

```
mode qloiter
arm throttle
rc 3 1800          # sube (acelerador)
rc 3 1500          # mantiene altura
mode fbwa          # transición a avión
mode rtl           # regreso a casa
```

## 4. Volar una misión de mapeo generada con `mapeo`

```bash
cd Dron/herramientas/mapeo
python3 -m mapeo planificar --area ejemplos/area_ejemplo.geojson \
  --despegue -0.5300,-78.5800 --gsd 3 --terreno --salida /tmp/mision.waypoints
```

En MAVProxy:

```
wp load /tmp/mision.waypoints
arm throttle
mode auto
```

Observa en el mapa cómo despega en vertical, hace la transición, recorre las líneas y vuelve a aterrizar en vertical. Revisa que el disparo de cámara empiece y termine en el borde del área (en la consola aparecen los mensajes de cámara).

## 5. Ejercicios recomendados
1. Simula **pérdida de radio** durante la misión: `param set SIM_RC_FAIL 1`. El avión debe volver solo.
2. Simula **viento**: `param set SIM_WIND_SPD 8` y `param set SIM_WIND_DIR 90`. Mira cómo cambia la velocidad sobre el suelo en cada línea.
3. Simula **batería baja** y comprueba la acción de `BATT_FS_LOW_ACT`.
4. Prueba una misión con el punto más lejano a más de 5 km y revisa el consumo estimado.

## 6. Simulador del núcleo de evasión (sin ArduPilot)

```bash
cd Dron/companion
cmake -S . -B build && cmake --build build
./build/pruebas_evasion
./build/simular_piedra 300 12 0.15   # lanzamientos, alcance de detección (m), latencia (s)
```

En la fase 6, el `puente_mavlink` se conectará a SITL igual que se conectará al avión real.

## Referencias
- [QuadPlane Simulation (ArduPilot)](https://ardupilot.org/plane/docs/quadplane-simulation.html)
- [Using SITL (ArduPilot)](https://ardupilot.org/dev/docs/using-sitl-for-ardupilot-testing.html)
- [Plane SITL/MAVProxy Tutorial](https://ardupilot.org/dev/docs/plane-sitlmavproxy-tutorial.html)
