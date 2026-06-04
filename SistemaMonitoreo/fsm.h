/**
 * @file fsm.h
 * @brief Declaración de estados, señales de entrada y prototipos de la FSM.
 *
 * Define los 7 estados (#State), las 13 señales de entrada (#Input) que
 * disparan transiciones, y los prototipos de los callbacks de entrada/salida
 * de cada estado.
 *
 * @authors Juan Sebastian Muñoz Ruiz, Andrea Fernanda Gomez,
 *          Daniel Alexander Chaguendo
 * @date Arquitectura Computacional 2026-1
 */

#ifndef FSM_H
#define FSM_H

/**
 * @brief Estados posibles de la máquina de estados finita (FSM).
 */
enum State {
  Inicio           = 0, ///< Estado inicial: solicita clave y/o tarjeta RFID.
  Config           = 1, ///< Modo administrador: activado por botón físico.
  MonitorAmbiental = 2, ///< Monitoreo continuo de temperatura y luz.
  MonitorPuertas   = 3, ///< Monitoreo de intrusos (sensor Hall y micrófono).
  Alarma           = 4, ///< Alarma activa: LED rojo parpadeante y buzzer.
  Bloqueo          = 5, ///< Sistema bloqueado tras 3 intentos fallidos.
  Gestion          = 6, ///< Gestión: se alcanzaron 3 alarmas consecutivas en 12 s.
};

/**
 * @brief Señales de entrada que disparan transiciones en la FSM.
 */
enum Input {
  SIG_CLAVE_CORRECTA     = 0,  ///< Clave + UID válido + horario correcto → MonitorAmbiental.
  SIG_BOTON              = 1,  ///< Botón físico presionado: Inicio → Config.
  SIG_TECLA_HASH         = 2,  ///< Tecla '#': Config → Inicio.
  SIG_TECLA_ASTERISCO    = 3,  ///< Tecla '*': Gestion → Inicio.
  SIG_ALARMA_COND        = 4,  ///< Temp > umbral o Luz > umbral → Alarma.
  SIG_INTRUSOS           = 5,  ///< Hall o Mic activo (activación detectada) → Alarma.
  SIG_TIMEOUT_AMBIENT    = 6,  ///< Timeout 3 s en Alarma → MonitorAmbiental.
  SIG_TIMEOUT_INTRUS     = 7,  ///< Timeout 4 s en Alarma → MonitorPuertas.
  SIG_ACTIVACIONES       = 8,  ///< 3 alarmas consecutivas en 12 s → Gestion.
  SIG_TIMEOUT_INTRUS2    = 9,  ///< Timeout 2 s en MonitorPuertas → MonitorAmbiental.
  SIG_TIMEOUT_AMB_TO_INT = 10, ///< Timeout 5 s en MonitorAmbiental → MonitorPuertas.
  SIG_BLOQUEADO          = 11, ///< 3 intentos fallidos de clave → Bloqueo.
  SIG_TIMEOUT_BLOQUEO    = 12, ///< Timeout 7 s en Bloqueo → Inicio.
  SIG_UNKNOWN            = 13, ///< Señal nula / sin evento pendiente.
};

/** @brief Registra las 13 transiciones y los callbacks de entrada/salida en la FSM.
 *  Debe llamarse desde setup() antes de iniciar la máquina de estados. */
void setup_State_Machine();

/** @brief Callback al entrar a #Inicio: muestra prompt, inicia teclado, RFID y botón. */
void on_enter_Inicio();
/** @brief Callback al entrar a #Config: muestra menú de administrador. */
void on_enter_Config();
/** @brief Callback al entrar a #MonitorAmbiental: inicia lectura de sensores ambientales. */
void on_enter_MonitorAmbiental();
/** @brief Callback al entrar a #MonitorPuertas: inicia lectura de Hall y micrófono. */
void on_enter_MonitorPuertas();
/** @brief Callback al entrar a #Alarma: activa buzzer, LED rojo y contadores. */
void on_enter_Alarma();
/** @brief Callback al entrar a #Bloqueo: activa parpadeo de LED y timeout de 7 s. */
void on_enter_Bloqueo();
/** @brief Callback al entrar a #Gestion: muestra menú de ajuste de umbrales y clave. */
void on_enter_Gestion();

/** @brief Callback al salir de #Inicio: detiene tareas y apaga LED verde. */
void on_exit_Inicio();
/** @brief Callback al salir de #Config: detiene la tarea de verificación de teclado. */
void on_exit_Config();
/** @brief Callback al salir de #MonitorAmbiental: detiene lectura de sensores. */
void on_exit_MonitorAmbiental();
/** @brief Callback al salir de #MonitorPuertas: detiene lectura de intrusos. */
void on_exit_MonitorPuertas();
/** @brief Callback al salir de #Alarma: apaga buzzer y LED rojo. */
void on_exit_Alarma();
/** @brief Callback al salir de #Bloqueo: apaga LED rojo y detiene timeout. */
void on_exit_Bloqueo();
/** @brief Callback al salir de #Gestion: detiene la tarea de teclado de gestión. */
void on_exit_Gestion();

#endif // FSM_H
