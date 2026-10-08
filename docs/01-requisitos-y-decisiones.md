# 01 · Requisitos y decisiones de diseño

## Requisitos del proyecto (definidos con el usuario)

| Requisito | Valor | Consecuencia en el diseño |
|---|---|---|
| Uso | Levantar cartografía (fotogrametría) en zonas de difícil acceso o inseguras | Cámara nadir, misiones automáticas, procesamiento con OpenDroneMap |
| País | Ecuador | Normativa DGAC, límite de 122 m sobre el terreno, régimen courier 4x4 de SENAE |
| Presupuesto de hardware | USD 900–2000 | Plataforma comercial PNP + electrónica abierta, sin LiDAR ni RTK al inicio |
| Distancia piloto–zona | Más de 5 km | **Ala fija VTOL**, no multirrotor. Radio de 900 MHz. Requiere permiso BVLOS |
| Autonomía | **Al menos 1 hora de vuelo.** Ver [11](11-autonomia-1-hora.md): con el T2 Cruza es el límite superior |
| Terreno | Mayormente plano, a veces montañoso. Seguimiento del terreno siempre que haya desnivel |
| Estación en tierra | Windows |
| Experiencia | Programa en Python/C++; no suelda, no tiene impresora 3D y no ha volado | Avión pre-ensamblado y de fábrica afinado; fase de simulador obligatoria; compra de kit de soldadura |
| IA | Evitar choques y esquivar piedras lanzadas | Detector de objetos + estimación de trayectoria + maniobra evasiva (C++) |

## Decisiones clave y por qué

### 1. VTOL de ala fija en lugar de cuadricóptero
Un cuadricóptero de este presupuesto vuela 20–30 min y gasta casi toda su energía en mantenerse en el aire; llegar a una zona a 5 km y volver le deja muy poco tiempo para mapear. Un ala fija gasta mucho menos por minuto de crucero, pero necesita pista. El **VTOL** (despegue y aterrizaje vertical como un dron, crucero como avión) combina ambas cosas: sale de un sitio pequeño y seguro, y recorre distancias largas.

### 2. No diseñar el fuselaje desde cero (por ahora)
Diseñar una aeronave desde cero (perfil alar, centro de gravedad, superficies de control, estructura) es un proyecto aeronáutico en sí mismo, y los primeros prototipos suelen terminar estrellados. Como aún no hay experiencia de vuelo, se parte de un **fuselaje VTOL comercial "PNP"** (Plug-N-Play: motores, variadores y servos ya instalados) con soporte en ArduPilot. Nosotros diseñamos todo lo demás: la integración de la electrónica, la cámara, la computadora de a bordo, el software de misión y la IA. Los "planos" del proyecto son, por tanto, planos de **integración y cableado** ([06](06-planos-de-cableado.md)). Más adelante, con experiencia, se puede diseñar un fuselaje propio.

**Plataforma recomendada:** HEEWING T2 Cruza VTOL (1200 mm de envergadura, 2–3.5 kg de peso en vuelo, batería 6S, viene con ArduPilot de fábrica). Alternativa más grande si el presupuesto crece: MakeFlyEasy Striver Mini VTOL (2100 mm, documentada en ArduPilot).

### 3. ArduPilot como piloto automático (no escribirlo desde cero)
El control de vuelo (estabilización a 400 Hz, transición VTOL, navegación, seguridad) es lo más crítico y peligroso de programar. **ArduPilot** es de código abierto, está escrito en C++ y lleva más de 10 años en millones de horas de vuelo. Nosotros lo **configuramos** y escribimos el software que va **encima** de él.

### 4. ¿Python o C++? Cada lenguaje donde le corresponde
Python no es "demasiado lento" para todo; el reparto correcto es este:

| Capa | Lenguaje | Por qué |
|---|---|---|
| Control de vuelo (bucle de 400 Hz) | C++ (ArduPilot, ya hecho) | Tiempo real estricto, corre en el microcontrolador |
| Evasión de amenazas en la computadora de a bordo | **C++** | Cada milisegundo cuenta entre ver la piedra y mover el dron |
| Inferencia de la red neuronal | Acelerador Hailo (NPU) | El modelo corre en hardware dedicado, no en la CPU |
| Planificación de misiones, procesamiento, entrenamiento de IA | **Python** | No es tiempo real; Python es más productivo y tiene las librerías (Ultralytics, OpenCV) |
| Comunicación entre todo | MAVLink | Protocolo estándar de ArduPilot |

### 5. La mejor defensa contra piedras es la altura
Física básica: una piedra lanzada a mano sale a 20–30 m/s y alcanza, en el mejor de los casos, 20–40 m de altura; con una honda o resortera puede subir más, pero pierde puntería muy rápido. El mapeo se hace a **80–120 m sobre el terreno**, fuera de ese alcance. El riesgo real está en el **despegue y el aterrizaje**, cuando el dron está bajo y en modo hover. Por eso la estrategia combina:

1. **Operativa** (lo más eficaz): despegar y aterrizar en un lugar seguro y alejado; subir rápido en vertical; no sobrevolar bajo a grupos de personas.
2. **Técnica**: en hover, un detector de objetos estima la trayectoria de lo que se acerca y el dron se aparta. Nuestra simulación ([10](10-ia-y-evasion.md)) muestra que esto funciona si la piedra se detecta a más de ~12 m, y que **deja de funcionar si se detecta a menos de ~8 m**. Es una capa de protección, no una garantía.

## Rendimiento esperado (estimaciones a validar en vuelo)

| Parámetro | Estimación |
|---|---|
| Autonomía de crucero | **31–56 min** con 6S2P Li-ion en el T2 Cruza (modelo, depende de L/D y de la altura; ver [11](11-autonomia-1-hora.md)). Se mide en la fase 3 |
| Velocidad de crucero | 15–18 m/s |
| Área por vuelo | ~170 ha a 3 cm/píxel por cada hora de vuelo; ~110 ha si la autonomía real es de 40 min |
| Alcance de radio | 10–30 km en línea de vista con ELRS 900 MHz; en montaña, lo limita el relieve |
| GSD típico | 2.5–3.5 cm/píxel a 100–120 m con Pi HQ Camera + lente de 6 mm |
