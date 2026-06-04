# Proyecto Sistema de Monitoreo
### Arquitectura Computacional - 2026.1 - Grupo B

- Juan Sebastian Muñoz Ruiz
- Andrea Fernanda Gomez
- Daniel Alexander Chaguendo

---

## Descripción

Sistema de monitoreo ambiental y de seguridad implementado sobre **Arduino Mega 2560**, basado en una **máquina de estados finita (FSM)** con 7 estados y 13 transiciones. Controla acceso mediante clave numérica y tarjeta RFID, monitorea condiciones ambientales (temperatura y luz) y detecta intrusos (efecto Hall y sonido).

## Hardware requerido

| Componente | Descripción |
|---|---|
| Arduino Mega 2560 | Microcontrolador principal |
| MFRC522 | Lector RFID (SPI: SS=53, RST=9) |
| KY-013 | Sensor de temperatura (A7) |
| KY-018 | Fotoresistencia / sensor de luz (A4) |
| KY-035 | Sensor de efecto Hall (A5) |
| KY-037 | Sensor de sonido / micrófono (A6) |
| LCD 16x2 | Pantalla (modo 4 bits, pines 2–5, 11, 12) |
| Teclado 4×4 | Entrada de clave (pines 33–47) |
| LEDs RGB | Indicadores de estado (pines 28, 30, 32) |
| Buzzer | Alertas sonoras (pin 6) |

## Diagrama de estados

```
Inicio ──────────────────────────► Config          (botón físico)
Inicio ──────────────────────────► MonitorAmbiental (clave + RFID + horario válidos)
Inicio ──────────────────────────► Bloqueo          (3 intentos fallidos)
Config ──────────────────────────► Inicio           (tecla #)
MonitorAmbiental ────────────────► MonitorPuertas   (timeout 5 s)
MonitorAmbiental ────────────────► Alarma           (Temp > 24 °C y Luz > 400)
MonitorPuertas ──────────────────► MonitorAmbiental (timeout 2 s)
MonitorPuertas ──────────────────► Alarma           (Hall o Mic × 3)
Alarma ──────────────────────────► MonitorAmbiental (timeout 3 s)
Alarma ──────────────────────────► MonitorPuertas   (timeout 4 s)
Alarma ──────────────────────────► Gestion          (3 alarmas consecutivas en 12 s)
Gestion ─────────────────────────► Inicio           (tecla *)
Bloqueo ─────────────────────────► Inicio           (timeout 7 s)
```

## Estructura del proyecto

```
SistemaMonitoreo/
├── SistemaMonitoreo.ino   # Archivo principal: setup(), loop() y periféricos
├── statemachine.ino       # Definición de callbacks y transiciones de la FSM
├── fsm.h                  # Declaración de estados, entradas y funciones
└── pinout.h               # Definición de pines y constantes de hardware
```

## Librerías requeridas

- `Keypad` — manejo del teclado matricial
- `LiquidCrystal` — control del LCD
- `MFRC522` — lectura de tarjetas RFID
- `AsyncTaskLib` — tareas asíncronas con temporizadores
- `StateMachineLib` — motor de máquina de estados
- `EEPROM`, `SPI` — incluidas en el SDK de Arduino