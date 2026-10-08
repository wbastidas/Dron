# 08 · Simulación con ArduPilot SITL

SITL (*Software In The Loop*) ejecuta el **mismo firmware** de ArduPilot en tu computadora, con un modelo físico del avión. Sirve para aprender a usarlo, probar misiones y, más adelante, probar el software de a bordo sin arriesgar el avión.

## 1. Instalación en Windows (WSL2 + Mission Planner)

El simulador corre dentro de Linux (WSL2) y el programa de la estación en tierra, **Mission Planner**, corre en Windows. Mission Planner es la herramienta principal para Windows.

**a) Instalar WSL2 con Ubuntu.** En PowerShell como administrador:

```powershell
wsl --install -d Ubuntu-24.04
```

Reinicia cuando lo pida y crea tu usuario de Linux. Necesita Windows 10 (versión 2004 o posterior) o Windows 11, con la virtualización activada en la BIOS.

**b) Instalar ArduPilot dentro de Ubuntu** (en la terminal de Ubuntu, no en PowerShell). Clona dentro del sistema de archivos de Linux (`~`), no en `/mnt/c`, que es muy lento:

```bash
cd ~
git clone --recurse-submodules https://github.com/ArduPilot/ardupilot.git
cd ardupilot
Tools/environment_install/install-prereqs-ubuntu.sh -y
. ~/.profile
```

**c) Instalar Mission Planner en Windows** desde la sección de descargas de ardupilot.org.

**d) Tu proyecto dentro de WSL.** Para usar el planificador y las pruebas de C++ desde Ubuntu:

```bash
cd ~
git clone https://github.com/wbastidas/Dron.git
sudo apt install -y cmake g++ python3
```

Los archivos de WSL se ven desde Windows en `\\wsl$\Ubuntu-24.04\home\<tu-usuario>`, así que puedes cargar el `.waypoints` generado en Mission Planner desde ahí. El planificador también corre directamente en Windows con `py -m mapeo ...` (ver [README de mapeo](../herramientas/mapeo/README.md)); el código C++ del `companion` solo se ha probado en Linux, por eso se recomienda compilarlo dentro de WSL.

## 2. Lanzar un QuadPlane (VTOL) simulado

En la terminal de Ubuntu:

```bash
cd ~/ardupilot
sim_vehicle.py -v ArduPlane -f quadplane --console --map \
  --custom-location=-0.5300,-78.5800,2900,0
```

`--custom-location` es latitud, longitud, altitud (msnm) y rumbo del punto de inicio. Pon el de tu zona. La altitud importa: a 2 900 m el avión simulado también tiene menos aire ([11](11-autonomia-1-hora.md)).

**Conectar Mission Planner (Windows) con el simulador (WSL2):** en Mission Planner elige **UDP** y el puerto **14550**, y pulsa *Connect*. SITL envía por defecto a ese puerto de la misma máquina; si no conecta, prueba lo siguiente en este orden:

1. Agrega `--out=udpout:127.0.0.1:14550` al comando de `sim_vehicle.py`.
2. Si WSL2 usa el modo de red por defecto (NAT), `127.0.0.1` puede no llegar a Windows. Descubre la IP de Windows desde Ubuntu con `ip route show default | awk '{print $3}'` y úsala: `--out=udpout:<esa-ip>:14550`.
3. Alternativa en Windows 11: crear `C:\Users\<tu-usuario>\.wslconfig` con `[wsl2]` y `networkingMode=mirrored`, luego `wsl --shutdown` y volver a abrir Ubuntu. Con ese modo `127.0.0.1` funciona en ambos sentidos.
4. Si Mission Planner dice que el puerto está en uso, cierra otras estaciones de tierra (QGroundControl, otra copia de Mission Planner) que escuchen en el 14550.
5. Revisa el firewall de Windows: debe permitir UDP entrante en el 14550 para Mission Planner.

> Estos pasos de red están tomados de reportes de usuarios en el foro de ArduPilot ([ejemplo](https://discuss.ardupilot.org/t/missionplanner-is-unable-to-connect-to-sitl-via-udp-tcp/142907)) y no los he podido probar en tu máquina: si alguno falla, dime el mensaje exacto de error.

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
