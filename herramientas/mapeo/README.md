# mapeo · Planificador de misiones de fotogrametría

Genera misiones de mapeo para ArduPlane QuadPlane (VTOL): despegue vertical, líneas paralelas con tramos de aproximación para ala fija, disparo de cámara por distancia solo sobre el área, y aterrizaje vertical en el punto de despegue.

Solo usa la biblioteca estándar de Python (3.10 o posterior). No hay que instalar nada.

## Uso

```bash
python3 -m mapeo camaras
python3 -m mapeo gsd --camara pi-hq-6mm --altitud 100
python3 -m mapeo autonomia --masa-sin-bateria 2.1 --bateria-wh 194 --finura 6 --altitud-msnm 2900
python3 -m mapeo planificar --area ejemplos/area_ejemplo.geojson \
    --despegue -0.5300,-78.5800 --gsd 3 --terreno --salida mision.waypoints
```

Genera:
- `mision.waypoints`: formato QGC WPL 110, para Mission Planner o QGroundControl.
- `mision.geojson`: área y líneas, para revisarlas en [geojson.io](https://geojson.io) o QGIS.

| Opción | Por defecto | Descripción |
|---|---|---|
| `--camara` | `pi-hq-6mm` | Cámara (ver `mapeo camaras`) |
| `--altitud` / `--gsd` | — | Altitud sobre el terreno (m), o el GSD deseado (cm/píxel) |
| `--traslape-frontal` | 75 | % de traslape a lo largo de la línea |
| `--traslape-lateral` | 65 | % de traslape entre líneas |
| `--rumbo` | automático | Rumbo de las líneas; el automático minimiza el número de giros |
| `--velocidad` | 16 | Velocidad de crucero (m/s) |
| `--extension` | 60 | Tramo recto antes y después de cada línea (m) |
| `--altitud-despegue` | 40 | Altura del despegue vertical antes de la transición |
| `--autonomia` | 40 | Minutos de vuelo útiles; avisa si no queda un 20 % de reserva |
| `--terreno` | no | Altitudes sobre el terreno (requiere `TERRAIN_ENABLE=1`) |

El programa avisa si la altitud supera el límite legal, si la cámara no alcanza a disparar, si falta autonomía o si la misión pasa de 5 km (BVLOS).

## Autonomía

`autonomia` estima los minutos de crucero para una batería dada (`--bateria-wh`) o la batería necesaria para un objetivo (`--objetivo`). Es un modelo de primer orden cuya incógnita principal es la finura (L/D): ver [docs/11](../../docs/11-autonomia-1-hora.md).

## Windows

En PowerShell use `py -m mapeo ...` y ponga las coordenadas entre comillas: `--despegue "-0.53,-78.58"` (la coma sin comillas se interpreta como lista; el programa acepta también `--despegue -0.53 -78.58`).

## Pruebas

```bash
python3 -m unittest discover -s pruebas -t .
```
