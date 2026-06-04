/**
 * @file pinout.h
 * @brief Definición de pines, constantes de hardware y temporización para Arduino Mega 2560.
 *
 * Centraliza todos los números de pin, umbrales de sensor, tiempos de tarea
 * y límites lógicos del sistema. Modificar este archivo es suficiente para
 * adaptar el firmware a un hardware diferente.
 *
 * @authors Juan Sebastian Muñoz Ruiz, Andrea Fernanda Gomez,
 *          Daniel Alexander Chaguendo
 * @date Arquitectura Computacional 2026-1
 */

#ifndef PINOUT_H
#define PINOUT_H

/** @defgroup PINES_LED LEDs indicadores de estado
 *  @note LCD_EN usa el pin 11, por eso los LEDs se asignan a otros pines.
 *  @{ */
#define LED_RED_PIN     28 ///< LED rojo: alarma y bloqueo (pin digital).
#define LED_GREEN_PIN   30 ///< LED verde: acceso válido (pin digital).
#define LED_BLUE_PIN    32 ///< LED azul: reservado (pin digital).
/** @} */

/** @defgroup PINES_BUZZER Buzzer
 *  Compatible con buzzer activo (HIGH=ON) y pasivo (tone()).
 *  @{ */
#define BUZZER_PIN          6    ///< Pin PWM del buzzer.
#define BUZZER_FREQ_ALARMA  1000 ///< Frecuencia (Hz) del buzzer en estado #Alarma.
#define BUZZER_FREQ_BLOQUEO 500  ///< Frecuencia (Hz) del buzzer en estado #Bloqueo.
/** @} */

/** @defgroup PINES_SENSORES Sensores analógicos KY
 *  @{ */
#define PIN_TEMP        A7   ///< KY-013: sensor de temperatura (NTC).
#define PHOTOCELL_PIN   A4   ///< KY-018: fotorresistencia (sensor de luz).
#define SENSOR_HALL     A5   ///< KY-035: sensor de efecto Hall (detección magnética).
#define SENSOR_MIC      A6   ///< KY-037: micrófono / sensor de sonido.
/** @} */

/** @defgroup PINES_TECLADO Teclado matricial 4×4
 *  @{ */
#define KP_ROW0         33 ///< Fila 0 del teclado.
#define KP_ROW1         35 ///< Fila 1 del teclado.
#define KP_ROW2         37 ///< Fila 2 del teclado.
#define KP_ROW3         39 ///< Fila 3 del teclado.
#define KP_COL0         41 ///< Columna 0 del teclado.
#define KP_COL1         43 ///< Columna 1 del teclado.
#define KP_COL2         45 ///< Columna 2 del teclado.
#define KP_COL3         47 ///< Columna 3 del teclado.
/** @} */

/** @defgroup PINES_LCD LCD 16×2 en modo 4 bits
 *  @{ */
#define LCD_RS          12 ///< Pin RS del LCD.
#define LCD_EN          11 ///< Pin Enable del LCD.
#define LCD_D4          5  ///< Pin de datos D4 del LCD.
#define LCD_D5          4  ///< Pin de datos D5 del LCD.
#define LCD_D6          3  ///< Pin de datos D6 del LCD.
#define LCD_D7          2  ///< Pin de datos D7 del LCD.
/** @} */

/** @defgroup PINES_RFID Módulo RFID MFRC522 (SPI)
 *  Mega 2560: MOSI=51, MISO=50, SCK=52 (SPI hardware). SS y RST configurables.
 *  @{ */
#define RFID_SS_PIN     53 ///< Pin Slave Select (SS) del MFRC522.
#define RFID_RST_PIN    9  ///< Pin de reset del MFRC522.
/** @name UID de tarjetas RFID autorizadas
 *  Reemplazar los bytes de cada rol con el UID real leído con el sketch de ejemplo.
 *  @{ */
#define RFID_ROLE1_UID_BYTE0  0xB3  ///< Byte 0 del UID del rol 1.
#define RFID_ROLE1_UID_BYTE1  0xEF  ///< Byte 1 del UID del rol 1.
#define RFID_ROLE1_UID_BYTE2  0x7F  ///< Byte 2 del UID del rol 1.
#define RFID_ROLE1_UID_BYTE3  0xE4  ///< Byte 3 del UID del rol 1.

#define RFID_ROLE2_UID_BYTE0  0x0D  ///< Byte 0 del UID del rol 2 (placeholder).
#define RFID_ROLE2_UID_BYTE1  0x7C  ///< Byte 1 del UID del rol 2 (placeholder).
#define RFID_ROLE2_UID_BYTE2  0x84  ///< Byte 2 del UID del rol 2 (placeholder).
#define RFID_ROLE2_UID_BYTE3  0x5B  ///< Byte 3 del UID del rol 2 (placeholder).
/** @} */ // end UID
/** @} */ // end PINES_RFID

/** @defgroup HORARIOS Horarios de acceso por rol (formato 24 h)
 *  @{ */
#define ROLE1_HORA_INI   6  ///< Hora de inicio de acceso para el rol 1.
#define ROLE1_HORA_FIN   22 ///< Hora de fin de acceso para el rol 1.
#define ROLE2_HORA_INI   7  ///< Hora de inicio de acceso para el rol 2.
#define ROLE2_HORA_FIN   20 ///< Hora de fin de acceso para el rol 2.
/** @} */

// ─── Boton de desbloqueo ─────────────────────────────────────────────────────
/** @brief Pin del botón físico de acceso al modo administrador (#Config). Usa INPUT_PULLUP. */
#define BOTON_PIN       40

/** @defgroup EEPROM_ADDR Direcciones de almacenamiento en EEPROM interna
 *  @{ */
#define EEPROM_ADDR_PASSWORD    0 ///< Dirección base: contraseña numérica (4 bytes).
#define EEPROM_ADDR_HORA_INI    4 ///< Hora de inicio de acceso, rango 0-23 (1 byte).
#define EEPROM_ADDR_HORA_FIN    5 ///< Hora de fin de acceso, rango 0-23 (1 byte).
#define EEPROM_ADDR_UID         6 ///< UID del tag RFID autorizado (4 bytes).
/** @} */

// ─── Umbrales de sensores ─────────────────────────────────────────────────────────
// Definidos directamente en SistemaMonitoreo.ino

// ─── Temporización de tareas (ms) ────────────────────────────────────────────
/** @defgroup TIEMPOS Temporización de tareas asíncronas (ms)
 *  @{ */
#define T_LECTURA_SENSORES      500   ///< Período de muestreo de sensores analógicos.
#define T_LECTURA_INTRUSOS      300   ///< Período de muestreo de Hall y Mic.
#define T_MONITOR_AMBIENTAL     5000  ///< Tiempo en #MonitorAmbiental antes de pasar a #MonitorPuertas.
#define T_MONITOR_INTRUSOS      2000  ///< Tiempo en #MonitorPuertas antes de volver a #MonitorAmbiental.
#define T_ALARMA_A_AMBIENT      3000  ///< Timeout en #Alarma para regresar a #MonitorAmbiental.
#define T_ALARMA_A_INTRUSOS     4000  ///< Timeout en #Alarma para regresar a #MonitorPuertas.
#define T_BLOQUEO               7000  ///< Duración del estado #Bloqueo antes de volver a #Inicio.
#define T_LED_RED_ON            100   ///< Tiempo de encendido del LED rojo en parpadeo de alarma.
#define T_LED_RED_OFF           200   ///< Tiempo de apagado del LED rojo en parpadeo de alarma.
#define T_LED_BLOQUEO_ON        300   ///< Tiempo de encendido del LED rojo en parpadeo de bloqueo.
#define T_LED_BLOQUEO_OFF       700   ///< Tiempo de apagado del LED rojo en parpadeo de bloqueo.
#define T_ACTIVACION_VENTANA    12000 ///< Ventana de tiempo para contar 3 alarmas consecutivas.
/** @} */

/** @defgroup LIMITES Límites lógicos del sistema
 *  @{ */
#define MAX_INTENTOS_CLAVE      3 ///< Intentos de clave fallidos antes de pasar a #Bloqueo.
#define MAX_ACTIVACIONES_ALARM  3 ///< Alarmas consecutivas en @ref T_ACTIVACION_VENTANA ms para ir a #Gestion.
#define MAX_ACTIVACIONES_INTRUS 3 ///< Activaciones de Hall/Mic para disparar #SIG_INTRUSOS.
/** @} */

/** @brief Contraseña por defecto cuando la EEPROM está sin inicializar (0xFF). */
#define DEFAULT_PASSWORD        "1231"

/** @defgroup HORA_SISTEMA Hora del sistema (valor fijo, sin módulo RTC)
 *  Sin módulo RTC se usa un valor fijo compilado. Modificar antes de cargar
 *  si se requiere validar un horario de acceso distinto.
 *  @{ */
#define DEFAULT_HORA_ACTUAL     21 ///< Hora actual del sistema (formato 24 h).
#define DEFAULT_HORA_INI        6  ///< Inicio de la ventana de acceso predeterminada (6 AM).
#define DEFAULT_HORA_FIN        22 ///< Fin de la ventana de acceso predeterminada (10 PM).
/** @} */

#endif // PINOUT_H
