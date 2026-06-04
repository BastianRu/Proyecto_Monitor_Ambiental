/**
 * @file statemachine.ino
 * @brief Configuración de transiciones y callbacks de entrada/salida de la FSM.
 *
 * Registra las 13 transiciones mediante StateMachineLib e implementa los
 * callbacks on_enter_* / on_exit_* de cada estado. Arduino IDE concatena
 * automáticamente todos los .ino de la misma carpeta, por lo que este archivo
 * puede referenciar variables declaradas en SistemaMonitoreo.ino sin includes.
 *
 * @authors Juan Sebastian Muñoz Ruiz, Andrea Fernanda Gomez,
 *          Daniel Alexander Chaguendo
 * @date Arquitectura Computacional 2026-1
 */
/**
 * @brief Registra las 13 transiciones y los callbacks de entrada/salida en la FSM.
 *
 * Debe llamarse desde setup() antes de SetState(). Las condiciones de transición
 * evalúan la variable global #currentInput en cada llamada a stateMachine.Update().
 */// ─── Configuracion de transiciones ──────────────────────────────────────────
void setup_State_Machine() {

  // Inicio -> MonitorAmbiental : clave + UID + horario validos
  stateMachine.AddTransition(Inicio, MonitorAmbiental, []() {
    return currentInput == SIG_CLAVE_CORRECTA;
  });

  // Inicio -> Config : boton fisico (modo admin)
  stateMachine.AddTransition(Inicio, Config, []() {
    return currentInput == SIG_BOTON;
  });

  // Inicio -> Bloqueo : 3 intentos fallidos
  stateMachine.AddTransition(Inicio, Bloqueo, []() {
    return currentInput == SIG_BLOQUEADO;
  });

  // Config -> Inicio : Tecla #
  stateMachine.AddTransition(Config, Inicio, []() {
    return currentInput == SIG_TECLA_HASH;
  });

  // MonitorAmbiental -> MonitorPuertas : timeout 5s
  stateMachine.AddTransition(MonitorAmbiental, MonitorPuertas, []() {
    return currentInput == SIG_TIMEOUT_AMB_TO_INT;
  });

  // MonitorAmbiental -> Alarma : Temp > umbralTemp y Luz > umbralLuz
  stateMachine.AddTransition(MonitorAmbiental, Alarma, []() {
    return currentInput == SIG_ALARMA_COND;
  });

  // MonitorPuertas -> MonitorAmbiental : timeout 2s
  stateMachine.AddTransition(MonitorPuertas, MonitorAmbiental, []() {
    return currentInput == SIG_TIMEOUT_INTRUS2;
  });

  // MonitorPuertas -> Alarma : Hall o Mic activado 3 veces
  stateMachine.AddTransition(MonitorPuertas, Alarma, []() {
    return currentInput == SIG_INTRUSOS;
  });

  // Alarma -> MonitorAmbiental : timeout 3s (si vino de MonitorAmbiental)
  stateMachine.AddTransition(Alarma, MonitorAmbiental, []() {
    return currentInput == SIG_TIMEOUT_AMBIENT;
  });

  // Alarma -> MonitorPuertas : timeout 4s (si vino de MonitorPuertas)
  stateMachine.AddTransition(Alarma, MonitorPuertas, []() {
    return currentInput == SIG_TIMEOUT_INTRUS;
  });

  // Alarma -> Gestion : 3 alarmas consecutivas en menos de 12s
  stateMachine.AddTransition(Alarma, Gestion, []() {
    return currentInput == SIG_ACTIVACIONES;
  });

  // Gestion -> Inicio : Tecla *
  stateMachine.AddTransition(Gestion, Inicio, []() {
    return currentInput == SIG_TECLA_ASTERISCO;
  });

  // Bloqueo -> Inicio : timeout automatico 7s
  stateMachine.AddTransition(Bloqueo, Inicio, []() {
    return currentInput == SIG_TIMEOUT_BLOQUEO;
  });

  // ─── Callbacks de entrada ─────────────────────────────────────────────────
  stateMachine.SetOnEntering(Inicio,           on_enter_Inicio);
  stateMachine.SetOnEntering(Config,           on_enter_Config);
  stateMachine.SetOnEntering(MonitorAmbiental, on_enter_MonitorAmbiental);
  stateMachine.SetOnEntering(MonitorPuertas,   on_enter_MonitorPuertas);
  stateMachine.SetOnEntering(Alarma,           on_enter_Alarma);
  stateMachine.SetOnEntering(Bloqueo,          on_enter_Bloqueo);
  stateMachine.SetOnEntering(Gestion,          on_enter_Gestion);

  // ─── Callbacks de salida ──────────────────────────────────────────────────
  stateMachine.SetOnLeaving(Inicio,           on_exit_Inicio);
  stateMachine.SetOnLeaving(Config,           on_exit_Config);
  stateMachine.SetOnLeaving(MonitorAmbiental, on_exit_MonitorAmbiental);
  stateMachine.SetOnLeaving(MonitorPuertas,   on_exit_MonitorPuertas);
  stateMachine.SetOnLeaving(Alarma,           on_exit_Alarma);
  stateMachine.SetOnLeaving(Bloqueo,          on_exit_Bloqueo);
  stateMachine.SetOnLeaving(Gestion,          on_exit_Gestion);
}

// ════════════════════════════════════════════════════════════════════════════
//  CALLBACKS DE ENTRADA
// ════════════════════════════════════════════════════════════════════════════
//  CALLBACKS DE ENTRADA
// ════════════════════════════════════════════════════════════════════════════
/** @brief Muestra el prompt de clave, resetea contadores e inicia tareas de teclado, RFID y botón. */
void on_enter_Inicio() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ingrese clave:");
  intentosClave        = 0;
  idxTeclado           = 0;
  activacionesAlarma   = 0;
  activacionesIntrusos = 0;
  currentRole          = ROLE_NONE;
  accessByPassword     = false;
  rfidValido           = false;
  memset(bufferTeclado, 0, sizeof(bufferTeclado));
  taskLeerTeclado.Start();
  taskLeerRFID.Start();
  taskLeerBoton.Start();
}
/** @brief Detiene las tareas de teclado, RFID y botón; apaga el LED verde y limpia el LCD. */
void on_exit_Inicio() {
  taskLeerTeclado.Stop();
  taskLeerRFID.Stop();
  taskLeerBoton.Stop();
  digitalWrite(LED_GREEN_PIN, LOW);
  lcd.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
/** @brief Muestra el menú de administrador e inicia la tarea de verificación de teclado. */
void on_enter_Config() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Config Admin");
  lcd.setCursor(0, 1);
  lcd.print("# para salir");
  taskVerificarConfig.Start();
}
/** @brief Detiene la tarea de verificación de teclado y limpia el LCD. */
void on_exit_Config() {
  taskVerificarConfig.Stop();
  lcd.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
/** @brief Inicia la lectura periódica de temperatura/luz y el timeout de 5 s hacia #MonitorPuertas. */
void on_enter_MonitorAmbiental() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Amb: T=  L=");
  taskLeerAmbiental.Start();
  taskTimeoutAmbiental.Start();
}
/** @brief Detiene la lectura de sensores ambientales y el timeout. */
void on_exit_MonitorAmbiental() {
  taskLeerAmbiental.Stop();
  taskTimeoutAmbiental.Stop();
  lcd.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
/** @brief Inicia la lectura de Hall y Mic, resetea el contador de intrusos y arranca el timeout de 2 s. */
void on_enter_MonitorPuertas() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Puertas:");
  lcd.setCursor(0, 1);
  lcd.print("Hall:- Mic:-");
  activacionesIntrusos = 0;
  taskLeerIntrusos.Start();
  taskTimeoutIntrusos.Start();
}
/** @brief Detiene la lectura de intrusos y el timeout. */
void on_exit_MonitorPuertas() {
  taskLeerIntrusos.Stop();
  taskTimeoutIntrusos.Stop();
  lcd.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
/** @brief Activa buzzer, LED rojo parpadeante, incrementa el contador y arranca los timeouts de salida. */
void on_enter_Alarma() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("!! ALARMA !!");
  activacionesAlarma++;
  Serial.print("[DEBUG] Alarma activa numero: ");
  Serial.println(activacionesAlarma);
  // Iniciar ventana de 12s para contar activaciones consecutivas
  taskVentanaActivaciones.Stop();
  taskVentanaActivaciones.Start();
  taskParpadeoAlarma.Start();
  taskAlarmaLedOFF.Stop();
  tone(BUZZER_PIN, BUZZER_FREQ_ALARMA);
  taskTimeoutAlarmaAmb.Start();
  taskTimeoutAlarmaInt.Start();
}
/** @brief Detiene buzzer, LED rojo y todas las tareas relacionadas con el estado de alarma. */
void on_exit_Alarma() {
  taskParpadeoAlarma.Stop();
  taskAlarmaLedOFF.Stop();
  taskTimeoutAlarmaAmb.Stop();
  taskTimeoutAlarmaInt.Stop();
  digitalWrite(LED_RED_PIN, LOW);
  noTone(BUZZER_PIN);
  lcd.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
/** @brief Activa el parpadeo lento del LED rojo y el timeout de 7 s hacia #Inicio. */
void on_enter_Bloqueo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SISTEMA");
  lcd.setCursor(0, 1);
  lcd.print("BLOQUEADO");
  taskParpadeoBloqueo.Start();
  taskBloqueoLedOFF.Stop();
  taskTimeoutBloqueo.Start();
}
/** @brief Detiene el parpadeo de bloqueo, apaga el LED rojo y detiene el timeout. */
void on_exit_Bloqueo() {
  taskParpadeoBloqueo.Stop();
  taskBloqueoLedOFF.Stop();
  taskTimeoutBloqueo.Stop();
  digitalWrite(LED_RED_PIN, LOW);
  lcd.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
/** @brief Muestra el menú de gestión según la fuente de alarma e inicia la tarea de teclado. */
void on_enter_Gestion() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (lastAlarmSource == SRC_PUERTAS) {
    lcd.print("GESTION PUERTAS");
  } else {
    lcd.print("GESTION AMBIENT.");
  }
  lcd.setCursor(0, 1);
  lcd.print("1:Umbral 2:Acc *");
  gestionSubstado = 0;
  idxGestion      = 0;
  memset(bufferGestion, 0, sizeof(bufferGestion));
  gestionTarget = (lastAlarmSource == SRC_PUERTAS) ? SRC_PUERTAS : SRC_AMBIENTAL;
  taskLeerTecladoGestion.Start();
}
/** @brief Detiene la tarea de teclado de gestión y limpia el LCD. */
void on_exit_Gestion() {
  taskLeerTecladoGestion.Stop();
  lcd.clear();
}
