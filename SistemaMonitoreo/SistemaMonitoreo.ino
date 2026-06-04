/**
 * @file SistemaMonitoreo.ino
 * @brief Archivo principal del sistema de monitoreo ambiental y de seguridad.
 *
 * Inicializa los periféricos (LCD, RFID, teclado, sensores, LEDs, buzzer),
 * instancia las tareas asíncronas (AsyncTask) y arranca la FSM en #Inicio.
 * El loop() se limita a actualizar tareas y FSM; la lógica de estados reside
 * en statemachine.ino.
 *
 * @authors Juan Sebastian Muñoz Ruiz, Andrea Fernanda Gomez,
 *          Daniel Alexander Chaguendo
 * @date Arquitectura Computacional 2026-1
 * @hardware Arduino Mega 2560
 */

#include "pinout.h"
#include "fsm.h"

// ─── Librerias ───────────────────────────────────────────────────────────────
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>
#include "AsyncTaskLib.h"
#include "StateMachineLib.h"
#include <string.h>

// ─── Perifericos ─────────────────────────────────────────────────────────────
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// Teclado matricial 4x4
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {KP_ROW0, KP_ROW1, KP_ROW2, KP_ROW3};
byte colPins[COLS] = {KP_COL0, KP_COL1, KP_COL2, KP_COL3};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ─── Maquina de estados ───────────────────────────────────────────────────────
// 7 estados, 13 transiciones
StateMachine stateMachine(7, 13);
Input currentInput = SIG_UNKNOWN;

// ─── Estado global del sistema ───────────────────────────────────────────────
char     bufferTeclado[5]  = "";
byte     idxTeclado        = 0;
byte     intentosClave     = 0;
byte     activacionesIntrusos = 0;
byte     activacionesAlarma   = 0;
unsigned long tiempoEntradaAlarma = 0;
bool     alarmaVieneDeIntrusos    = false; // true = vino de MonitorPuertas
byte     lastAlarmSource         = 0;    // 0=ninguno, 1=ambiental, 2=puertas
const byte ROLE_NONE   = 0;
const byte ROLE_1      = 1;
const byte ROLE_2      = 2;
const byte ROLE_MASTER = 3;
const byte SRC_NONE      = 0;
const byte SRC_AMBIENTAL = 1;
const byte SRC_PUERTAS   = 2;
byte     currentRole = ROLE_NONE;
bool     accessByPassword = false;
// Umbrales de alarma 
const byte UMBRAL_TEMP_ALARMA = 32;   // Temperatura > 32C -> Alarma
const int  UMBRAL_LUZ_ALARMA = 100;   // Luz > 100 (ADC) -> Alarma
const int  UMBRAL_HALL_ALARMA = 470;  // Hall < 400 -> Alarma (se activa con lectura menor)
const int  UMBRAL_MIC_ALARMA = 150;   // Mic > 150 -> Alarma
// Lectura de sensores analógicos KY
float    tempC                  = 0;
int      luzVal                 = 0;
int      hallVal                = 0;
int      micVal                 = 0;
// Umbrales en tiempo de ejecucion (se cargan de EEPROM o de los #define)
byte     umbralTemp               = UMBRAL_TEMP_ALARMA;
int      umbralLuz                = UMBRAL_LUZ_ALARMA;
int      umbralHall               = UMBRAL_HALL_ALARMA;
int      umbralMic                = UMBRAL_MIC_ALARMA;

// Estado del menu interno de Gestion
byte     gestionSubstado          = 0;    // 0=menu, 2=umbral1, 3=umbral2, 4=acceso
byte     gestionTarget            = SRC_NONE;
char     bufferGestion[5]         = "";
byte     idxGestion               = 0;

// UID autorizado: llenar los 4 bytes con los de tu tarjeta RFID
// (obtenerlos corriendo el sketch de lectura de UID incluido en el repo)
const byte RFID_ROLE1_UID[4] = {
  RFID_ROLE1_UID_BYTE0, RFID_ROLE1_UID_BYTE1, RFID_ROLE1_UID_BYTE2, RFID_ROLE1_UID_BYTE3
};
const byte RFID_ROLE2_UID[4] = {
  RFID_ROLE2_UID_BYTE0, RFID_ROLE2_UID_BYTE1, RFID_ROLE2_UID_BYTE2, RFID_ROLE2_UID_BYTE3
};

// Contrasena activa (leida de EEPROM o default)
char     passwordActual[5] = DEFAULT_PASSWORD;

// UID leido en el ultimo escaneo RFID
byte     uidLeida[4]  = {0, 0, 0, 0};
bool     rfidValido   = false;

// Hora del sistema (sin RTC: valor fijo definido en pinout.h)
// Para cambiar la hora, modificar DEFAULT_HORA_ACTUAL en pinout.h
byte     horaActual   = DEFAULT_HORA_ACTUAL;

// ─── Prototipos de tareas ────────────────────────────────────────────────────
void task_LeerTeclado();
void task_LeerRFID();
void task_VerificarConfig();
void task_LeerAmbiental();
void task_TimeoutAmbiental();
void task_LeerIntrusos();
void task_TimeoutIntrusos();
void task_ParpadeoAlarma();
void task_TimeoutAlarmaAmb();
void task_TimeoutAlarmaInt();
void task_ParpadeoBloqueo();
void task_LeerBoton();
void task_VentanaActivaciones();
void task_TimeoutBloqueo();
void task_LeerTecladoGestion();
void task_AlarmaLedOFF();
void task_BloqueoLedOFF();
extern AsyncTask taskAlarmaLedOFF;
extern AsyncTask taskBloqueoLedOFF;

// ─── Instancias de AsyncTask ─────────────────────────────────────────────────
AsyncTask taskLeerTeclado       (100,  true,  task_LeerTeclado);
AsyncTask taskLeerRFID          (200,  true,  task_LeerRFID);
AsyncTask taskVerificarConfig   (200,  true,  task_VerificarConfig);
AsyncTask taskLeerAmbiental     (T_LECTURA_SENSORES, true, task_LeerAmbiental);
AsyncTask taskTimeoutAmbiental  (T_MONITOR_AMBIENTAL, false, task_TimeoutAmbiental);
AsyncTask taskLeerIntrusos      (T_LECTURA_INTRUSOS, true, task_LeerIntrusos);
AsyncTask taskTimeoutIntrusos   (T_MONITOR_INTRUSOS, false, task_TimeoutIntrusos);
AsyncTask taskParpadeoAlarma    (T_LED_RED_ON, true, task_ParpadeoAlarma);
AsyncTask taskTimeoutAlarmaAmb  (T_ALARMA_A_AMBIENT,  false, task_TimeoutAlarmaAmb);
AsyncTask taskTimeoutAlarmaInt  (T_ALARMA_A_INTRUSOS, false, task_TimeoutAlarmaInt);
AsyncTask taskParpadeoBloqueo   (T_LED_BLOQUEO_ON, true, task_ParpadeoBloqueo);
AsyncTask taskLeerBoton         (50,   true,  task_LeerBoton);
AsyncTask taskVentanaActivaciones(T_ACTIVACION_VENTANA, false, task_VentanaActivaciones);
AsyncTask taskTimeoutBloqueo      (T_BLOQUEO,            false, task_TimeoutBloqueo);
AsyncTask taskLeerTecladoGestion  (100,                  true,  task_LeerTecladoGestion);

// ─── Variable auxiliar parpadeo ───────────────────────────────────────────────
// (eliminadas: ledAlarmaEstado y ledBloqueoEstado ya no son necesarias)

// ════════════════════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════════════════════
/**
 * @brief Inicializa pines, LCD, RFID, EEPROM y arranca la FSM en estado #Inicio.
 */
void setup() {
  Serial.begin(9600);

  // Pines de salida
  pinMode(LED_RED_PIN,   OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN,  OUTPUT);
  pinMode(BUZZER_PIN,    OUTPUT);
  pinMode(BOTON_PIN,     INPUT_PULLUP);

  // Pines de entrada
  pinMode(SENSOR_HALL, INPUT);
  pinMode(SENSOR_MIC,  INPUT);

  // Apagar todo al inicio
  digitalWrite(LED_RED_PIN,   LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_BLUE_PIN,  LOW);
  digitalWrite(BUZZER_PIN,    LOW);

  // LCD
  lcd.begin(16, 2);

    // RFID
  SPI.begin();
  rfid.PCD_Init();

  // Leer contrasena y horario desde EEPROM
  cargar_EEPROM();

  // Configurar e iniciar la FSM
  setup_State_Machine();
  stateMachine.SetState(State::Inicio, true, true);
}

// ════════════════════════════════════════════════════════════════════════════
//  LOOP
// ════════════════════════════════════════════════════════════════════════════
/**
 * @brief Ciclo principal: actualiza todas las tareas asíncronas y la FSM en cada iteración.
 */
void loop() {
  // Actualizar todas las tareas
  taskLeerTeclado.Update();
  taskLeerRFID.Update();
  taskVerificarConfig.Update();
  taskLeerAmbiental.Update();
  taskTimeoutAmbiental.Update();
  taskLeerIntrusos.Update();
  taskTimeoutIntrusos.Update();
  taskParpadeoAlarma.Update();
  taskAlarmaLedOFF.Update();
  taskTimeoutAlarmaAmb.Update();
  taskTimeoutAlarmaInt.Update();
  taskParpadeoBloqueo.Update();
  taskBloqueoLedOFF.Update();
  taskLeerBoton.Update();
  taskVentanaActivaciones.Update();
  taskTimeoutBloqueo.Update();
  taskLeerTecladoGestion.Update();

  // Actualizar maquina de estados
  stateMachine.Update();
}

// ════════════════════════════════════════════════════════════════════════════
//  EEPROM
// ════════════════════════════════════════════════════════════════════════════
/**
 * @brief Lee contraseña y horario desde EEPROM; usa valores por defecto si está sin inicializar.
 */
void cargar_EEPROM() {
  // Leer contrasena (4 digitos)
  byte i;
  bool virgen = false;
  for (i = 0; i < 4; i++) {
    byte val = EEPROM.read(EEPROM_ADDR_PASSWORD + i);
    if (val == 0xFF) { virgen = true; break; }
    passwordActual[i] = (char)('0' + (val & 0x0F));
  }
  if (virgen) {
    strncpy(passwordActual, DEFAULT_PASSWORD, 4);
  }
  passwordActual[4] = '\0';

  // Leer horario
  byte hIni = EEPROM.read(EEPROM_ADDR_HORA_INI);
  byte hFin = EEPROM.read(EEPROM_ADDR_HORA_FIN);
  bool horarioValido = (hIni != 0xFF && hFin != 0xFF && hIni < 24 && hFin < 24);
  if (!horarioValido) {
    EEPROM.write(EEPROM_ADDR_HORA_INI, DEFAULT_HORA_INI);
    EEPROM.write(EEPROM_ADDR_HORA_FIN, DEFAULT_HORA_FIN);
  }

  // Umbrales de sensores: se usan los valores definidos en el código, no EEPROM
  umbralTemp = UMBRAL_TEMP_ALARMA;
  umbralLuz = UMBRAL_LUZ_ALARMA;

  umbralHall = UMBRAL_HALL_ALARMA;

  umbralMic = UMBRAL_MIC_ALARMA;

}

void guardar_EEPROM_password(const char* pwd) {
  for (byte i = 0; i < 4; i++) {
    EEPROM.write(EEPROM_ADDR_PASSWORD + i, (byte)(pwd[i] - '0'));
  }
  strncpy(passwordActual, pwd, 4);
  passwordActual[4] = '\0';
}

/**
 * @brief Compara un UID RFID leído contra los roles autorizados y asigna #currentRole.
 * @param uid Puntero a un arreglo de 4 bytes con el UID leído por el lector.
 * @return true si el UID corresponde a un rol autorizado; false en caso contrario.
 */
bool verificar_uid(byte* uid) {
  if (memcmp(uid, RFID_ROLE1_UID, 4) == 0) {
    currentRole = ROLE_1;
    return true;
  }
  if (memcmp(uid, RFID_ROLE2_UID, 4) == 0) {
    currentRole = ROLE_2;
    return true;
  }
  currentRole = ROLE_NONE;
  return false;
}

/**
 * @brief Verifica si el rol tiene acceso permitido en la hora actual del sistema.
 * @param role Rol del usuario (#ROLE_1, #ROLE_2 o #ROLE_MASTER).
 * @return true si el acceso está dentro del horario permitido; false en caso contrario.
 */
bool verificar_horario(byte role) {
  if (role == ROLE_MASTER || role == ROLE_NONE) return true;
  if (role == ROLE_1) {
    return (horaActual >= ROLE1_HORA_INI && horaActual < ROLE1_HORA_FIN);
  }
  if (role == ROLE_2) {
    return (horaActual >= ROLE2_HORA_INI && horaActual < ROLE2_HORA_FIN);
  }
  return true;
}

// ════════════════════════════════════════════════════════════════════════════

//  TAREAS - ESTADO: INICIO
// ════════════════════════════════════════════════════════════════════════════
/**
 * @brief Tarea periódica: lee el teclado y acumula dígitos; '#' evalúa la clave, '*' limpia el buffer.
 */
void task_LeerTeclado() {
  char key = keypad.getKey();
  
  // Si no se presionó ninguna tecla, salir
  if (key == NO_KEY) return;

  Serial.print("[KEYPAD] Tecla presionada: ");
  Serial.println(key);

  // Tecla '*': limpiar buffer
  if (key == '*') {
    idxTeclado = 0;
    memset(bufferTeclado, 0, sizeof(bufferTeclado));
    lcd.setCursor(0, 1);
    lcd.print("                ");
    Serial.println("[BUFFER] Limpiado por tecla *");
    return;
  }

  // Tecla '#': procesar clave ingresada
  if (key == '#') {
    // Solo procesar si se ingresaron exactamente 4 dígitos
    if (idxTeclado == 4) {
      bufferTeclado[4] = '\0'; // Terminar string
      
      Serial.print("[CHECK] Comparando: ");
      Serial.print(bufferTeclado);
      Serial.print(" con ");
      Serial.println(passwordActual);

      // Comparar clave maestra.
      if (strcmp(bufferTeclado, passwordActual) == 0) {
        Serial.println("[RESULTADO] -> CLAVE CORRECTA MASTER");
        lcd.clear();
        lcd.print("Acceso OK");
        digitalWrite(LED_GREEN_PIN, HIGH);
        accessByPassword = true;
        currentRole = ROLE_MASTER;
        currentInput = SIG_CLAVE_CORRECTA;
      } else {
        Serial.print("[RESULTADO] -> CLAVE INCORRECTA. Intento: ");
        Serial.println(intentosClave + 1);
        lcd.clear();
        lcd.print("Clave invalida");
        intentosClave++;
        verificar_intentos();
      }

      // Reiniciar buffer después de procesar
      delay(2000); // Pausa para mostrar mensaje
      idxTeclado = 0;
      memset(bufferTeclado, 0, sizeof(bufferTeclado));
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
    return;
  }

  // Acumular caracteres: cifras + letras A-D válidas
  if (idxTeclado < 4) {
    bufferTeclado[idxTeclado] = key;
    idxTeclado++;
    bufferTeclado[idxTeclado] = '\0'; // Mantener string terminado

    // Mostrar asterisco en LCD por cada carácter ingresado
    lcd.setCursor(idxTeclado - 1, 1);
    lcd.print('*');

    Serial.print("[BUFFER] Estado actual: ");
    Serial.println(bufferTeclado);
  }
}

/**
 * @brief Tarea periódica: detecta tarjeta RFID, verifica el UID y el horario del rol asignado.
 */
void task_LeerRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) {
    Serial.println("[RFID] Lectura de tarjeta fallida");
    return;
  }

  Serial.println("[RFID] Tarjeta presente");
  for (byte i = 0; i < 4; i++) {
    uidLeida[i] = rfid.uid.uidByte[i];
  }
  Serial.print("[DEBUG] UID leido: ");
  for (byte i = 0; i < 4; i++) {
    Serial.print(uidLeida[i] < 0x10 ? "0" : "");
    Serial.print(uidLeida[i], HEX);
    if (i < 3) Serial.print(':');
  }
  Serial.println();
  Serial.print("[DEBUG] UID esperado R1: ");
  Serial.print(RFID_ROLE1_UID_BYTE0, HEX);
  Serial.print(':');
  Serial.print(RFID_ROLE1_UID_BYTE1, HEX);
  Serial.print(':');
  Serial.print(RFID_ROLE1_UID_BYTE2, HEX);
  Serial.print(':');
  Serial.println(RFID_ROLE1_UID_BYTE3, HEX);
  Serial.print("[DEBUG] UID esperado R2: ");
  Serial.print(RFID_ROLE2_UID_BYTE0, HEX);
  Serial.print(':');
  Serial.print(RFID_ROLE2_UID_BYTE1, HEX);
  Serial.print(':');
  Serial.print(RFID_ROLE2_UID_BYTE2, HEX);
  Serial.print(':');
  Serial.println(RFID_ROLE2_UID_BYTE3, HEX);

  rfidValido = verificar_uid(uidLeida);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  if (rfidValido) {
    lcd.setCursor(0, 0);
    lcd.print("RFID: OK        ");
    Serial.print("[RFID] Rol: ");
    Serial.println(currentRole == ROLE_1 ? "ROLE1" : "ROLE2");
    if (verificar_horario(currentRole)) {
      lcd.setCursor(0, 1);
      lcd.print("Acceso RFID OK  ");
      currentInput = SIG_CLAVE_CORRECTA;
      accessByPassword = false;
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Fuera de hora   ");
      Serial.println("[RFID] Fuera de horario");
      currentRole = ROLE_NONE;
      rfidValido = false;
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print("RFID: NO REG    ");
    Serial.println("[RFID] NO REGISTRADO");
  }
}

/**
 * @brief Incrementa el contador de intentos fallidos y emite #SIG_BLOQUEADO al llegar al límite.
 */
void verificar_intentos() {
  if (intentosClave >= MAX_INTENTOS_CLAVE) {
    currentInput  = SIG_BLOQUEADO;
    intentosClave = 0;
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  TAREAS - ESTADO: CONFIG
// ════════════════════════════════════════════════════════════════════════════
/**
 * @brief Tarea periódica: espera la tecla '#' para emitir #SIG_TECLA_HASH y salir del modo Config.
 */
void task_VerificarConfig() {
  char key = keypad.getKey();
  if (key == NO_KEY) return;

  if (key == '#') {
    // Tecla # -> volver a Inicio
    currentInput = SIG_TECLA_HASH;
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  TAREAS - ESTADO: MONITOR AMBIENTAL
// ════════════════════════════════════════════════════════════════════════════
/**
 * @brief Tarea periódica: lee temperatura (KY-013) y luz (KY-018); emite #SIG_ALARMA_COND si superan umbrales.
 */
void task_LeerAmbiental() {
  int analogValue = analogRead(PIN_TEMP);
  if (analogValue > 0) {
    const float BETA = 3950.0;
    tempC = 1.0 / (log(1.0 / (1023.0 / analogValue - 1.0)) / BETA + 1.0 / 298.15) - 273.15;
  }
  luzVal = analogRead(PHOTOCELL_PIN);

  Serial.print("[DEBUG] Temp (C): ");
  Serial.print(tempC);
  Serial.print("  Luz: ");
  Serial.print(luzVal);
  bool tempAlarma = tempC > (float)umbralTemp;
  bool luzAlarma = luzVal > umbralLuz;

  Serial.print("  umbralTemp: ");
  Serial.print(umbralTemp);
  Serial.print("  umbralLuz: ");
  Serial.print(umbralLuz);
  Serial.print("  cond: ");
  Serial.print(tempAlarma ? "T>" : "T<=");
  Serial.print(" ");
  Serial.println(luzAlarma ? "L>" : "L<=");

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print((int)tempC);
  lcd.print("C L:");
  lcd.print(luzVal);
  lcd.print("   ");

  // Condicion de alarma: Temp > umbralTemp o Luz > umbralLuz
  if (tempAlarma || luzAlarma) {
    alarmaVieneDeIntrusos = false;
    lastAlarmSource = SRC_AMBIENTAL;
    currentInput = SIG_ALARMA_COND;
  }
}

/**
 * @brief Tarea de un solo disparo: emite #SIG_TIMEOUT_AMB_TO_INT para ir a #MonitorPuertas tras 5 s.
 */
void task_TimeoutAmbiental() {
  // Timeout 5s: ir a Monitor Puertas
  currentInput = SIG_TIMEOUT_AMB_TO_INT;
}

// ════════════════════════════════════════════════════════════════════════════
//  TAREAS - ESTADO: MONITOR INTRUSOS
// ════════════════════════════════════════════════════════════════════════════
/**
 * @brief Tarea periódica: lee Hall (KY-035) y Mic (KY-037); emite #SIG_INTRUSOS si se detecta actividad.
 */
void task_LeerIntrusos() {
  hallVal = analogRead(SENSOR_HALL);
  micVal  = analogRead(SENSOR_MIC);
    bool hallActivado = (hallVal < umbralHall);
  bool micActivado  = (micVal > umbralMic);

  Serial.print("[DEBUG] Hall: ");
  Serial.print(hallVal);
  Serial.print("  Mic: ");
  Serial.print(micVal);
  Serial.print("  umbralHall: ");
  Serial.print(umbralHall);
  Serial.print("  umbralMic: ");
  Serial.print(umbralMic);
    Serial.print("  cond: ");
    Serial.print(hallActivado ? "H<" : "H>=");
  Serial.print(" ");
  Serial.println(micVal > umbralMic ? "M>" : "M<=");

  lcd.setCursor(6, 1);
  lcd.print("H:");
  lcd.print(hallVal);
  lcd.print(" ");
  lcd.setCursor(11, 1);
  lcd.print("M:");
  lcd.print(micVal);

  if (hallActivado || micActivado) {
    alarmaVieneDeIntrusos = true;
    lastAlarmSource = SRC_PUERTAS;
    currentInput = SIG_INTRUSOS;
  }
}

/**
 * @brief Tarea de un solo disparo: emite #SIG_TIMEOUT_INTRUS2 para volver a #MonitorAmbiental tras 2 s.
 */
void task_TimeoutIntrusos() {
  // Timeout 5s: volver a Monitor Ambiental
  currentInput = SIG_TIMEOUT_INTRUS2;
}

// ════════════════════════════════════════════════════════════════════════════
//  TAREAS - ESTADO: ALARMA
// ════════════════════════════════════════════════════════════════════════════

// Parpadeo asimetrico Alarma: ON 100ms / OFF 200ms
// Se usan dos tareas separadas que se activan mutuamente.
// El buzzer (tone/noTone) se maneja directamente en on_enter/on_exit_Alarma.
void task_AlarmaLedOFF();
AsyncTask taskAlarmaLedOFF(T_LED_RED_OFF, false, task_AlarmaLedOFF);

/**
 * @brief Tarea periódica: enciende el LED rojo y programa el apagado (parpadeo asimétrico de alarma).
 */
void task_ParpadeoAlarma() {
  // ON: encender LED y programar tarea OFF
  digitalWrite(LED_RED_PIN, HIGH);
  taskAlarmaLedOFF.Stop();
  taskAlarmaLedOFF.Start();
}

/** @brief Tarea de un solo disparo: apaga el LED rojo (parte OFF del parpadeo de alarma). */
void task_AlarmaLedOFF() {
  // OFF: apagar LED
  digitalWrite(LED_RED_PIN, LOW);
}

/**
 * @brief Timeout de alarma ambiental: emite #SIG_ACTIVACIONES o #SIG_TIMEOUT_AMBIENT según el conteo.
 */
void task_TimeoutAlarmaAmb() {
  // Timeout 4s: volver a Monitor Ambiental o ir a Gestion si ya hubo 3 activaciones
  if (activacionesAlarma >= MAX_ACTIVACIONES_ALARM) {
    Serial.println("[DEBUG] Timeout alarma ambiental -> SIG_ACTIVACIONES");
    currentInput = SIG_ACTIVACIONES;
  } else if (!alarmaVieneDeIntrusos) {
    Serial.println("[DEBUG] Timeout alarma ambiental -> SIG_TIMEOUT_AMBIENT");
    currentInput = SIG_TIMEOUT_AMBIENT;
  }
}

/**
 * @brief Timeout de alarma de intrusos: emite #SIG_ACTIVACIONES o #SIG_TIMEOUT_INTRUS según el conteo.
 */
void task_TimeoutAlarmaInt() {
  // Timeout 2s: volver a Monitor Intrusos o ir a Gestion si ya hubo 3 activaciones
  if (activacionesAlarma >= MAX_ACTIVACIONES_ALARM) {
    Serial.println("[DEBUG] Timeout alarma intrusos -> SIG_ACTIVACIONES");
    currentInput = SIG_ACTIVACIONES;
  } else if (alarmaVieneDeIntrusos) {
    Serial.println("[DEBUG] Timeout alarma intrusos -> SIG_TIMEOUT_INTRUS");
    currentInput = SIG_TIMEOUT_INTRUS;
  }
}

/** @brief Tarea de un solo disparo: resetea el contador de alarmas al expirar la ventana de 12 s. */
void task_VentanaActivaciones() {
  // Si la ventana de 12s expira, resetear contador de activaciones
  activacionesAlarma = 0;
}

// ════════════════════════════════════════════════════════════════════════════
//  TAREAS - ESTADO: BLOQUEO
// ════════════════════════════════════════════════════════════════════════════

// Parpadeo asimetrico Bloqueo: ON 300ms / OFF 700ms
void task_BloqueoLedOFF();
AsyncTask taskBloqueoLedOFF(T_LED_BLOQUEO_OFF, false, task_BloqueoLedOFF);

/**
 * @brief Tarea periódica: enciende el LED rojo y programa el apagado (parpadeo lento de bloqueo).
 */
void task_ParpadeoBloqueo() {
  digitalWrite(LED_RED_PIN, HIGH);
  taskBloqueoLedOFF.Stop();
  taskBloqueoLedOFF.Start();
}

/** @brief Tarea de un solo disparo: apaga el LED rojo (parte OFF del parpadeo de bloqueo). */
void task_BloqueoLedOFF() {
  digitalWrite(LED_RED_PIN, LOW);
}

/** @brief Tarea periódica: detecta pulsación del botón físico y emite #SIG_BOTON. */
void task_LeerBoton() {
  if (digitalRead(BOTON_PIN) == LOW) {
    // Boton presionado (INPUT_PULLUP -> LOW = presionado): Inicio -> Config
    currentInput = SIG_BOTON;
  }
}

/** @brief Tarea de un solo disparo: emite #SIG_TIMEOUT_BLOQUEO para salir de #Bloqueo tras 7 s. */
void task_TimeoutBloqueo() {
  // Timeout 7s: volver a Inicio automaticamente
  currentInput = SIG_TIMEOUT_BLOQUEO;
}

// ════════════════════════════════════════════════════════════════════════════
//  TAREAS - ESTADO: GESTION
// ════════════════════════════════════════════════════════════════════════════
/**
 * @brief Tarea periódica en #Gestion: menú de ajuste de umbrales y cambio de clave.
 *
 * Subestados internos:
 * - 0: menú principal (1=umbral, 2=acceso, *=salir)
 * - 2: ingreso del primer umbral (Temp o Hall)
 * - 3: ingreso del segundo umbral (Luz o Mic)
 * - 4: cambio de contraseña
 */
void task_LeerTecladoGestion() {
  char key = keypad.getKey();
  if (key == NO_KEY) return;

  if (gestionSubstado == 0) {
    if (key == '*') {
      currentInput = SIG_TECLA_ASTERISCO;
    } else if (key == '1') {
      gestionTarget = (lastAlarmSource == SRC_PUERTAS) ? SRC_PUERTAS : SRC_AMBIENTAL;
      idxGestion = 0;
      memset(bufferGestion, 0, sizeof(bufferGestion));
      lcd.clear();
      lcd.setCursor(0, 0);
      if (gestionTarget == SRC_AMBIENTAL) {
        lcd.print("Temp umbral?");
      } else {
        lcd.print("Hall umbral?");
      }
      lcd.setCursor(0, 1);
      lcd.print("# = confirmar");
      gestionSubstado = 2;
    } else if (key == '2') {
      gestionSubstado = 4;
      idxGestion = 0;
      memset(bufferGestion, 0, sizeof(bufferGestion));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Nueva clave:");
      lcd.setCursor(0, 1);
      lcd.print("____");
    }

  } else if (gestionSubstado == 2) {
    if (key == '*') {
      gestionSubstado = 0;
      idxGestion = 0;
      memset(bufferGestion, 0, sizeof(bufferGestion));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GESTION:");
      lcd.setCursor(0, 1);
      lcd.print("1:Umbral 2:Acc *");
    } else if (key == '#') {
      if (idxGestion > 0) {
        int valor = atoi(bufferGestion);
        if (gestionTarget == SRC_AMBIENTAL) {
          umbralTemp = (byte)valor;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Luz umbral?");
          lcd.setCursor(0, 1);
          lcd.print("# = confirmar");
          gestionSubstado = 3;
        } else {
          umbralHall = valor;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Mic umbral?");
          lcd.setCursor(0, 1);
          lcd.print("# = confirmar");
          gestionSubstado = 3;
        }
      }
      idxGestion = 0;
      memset(bufferGestion, 0, sizeof(bufferGestion));
    } else if (idxGestion < 4 && key >= '0' && key <= '9') {
      bufferGestion[idxGestion] = key;
      idxGestion++;
      lcd.setCursor(idxGestion - 1, 1);
      lcd.print('*');
    }

  } else if (gestionSubstado == 3) {
    if (key == '*') {
      gestionSubstado = 0;
      idxGestion = 0;
      memset(bufferGestion, 0, sizeof(bufferGestion));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GESTION:");
      lcd.setCursor(0, 1);
      lcd.print("1:Umbral 2:Acc *");
    } else if (key == '#') {
      if (idxGestion > 0) {
        int valor = atoi(bufferGestion);
        if (gestionTarget == SRC_AMBIENTAL) {
          umbralLuz = valor;
        } else {
          umbralMic = valor;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Umbral guardado");
        lcd.setCursor(0, 1);
        lcd.print("* = volver");
        gestionSubstado = 0;
      }
      idxGestion = 0;
      memset(bufferGestion, 0, sizeof(bufferGestion));
    } else if (idxGestion < 4 && key >= '0' && key <= '9') {
      bufferGestion[idxGestion] = key;
      idxGestion++;
      lcd.setCursor(idxGestion - 1, 1);
      lcd.print('*');
    }

  } else if (gestionSubstado == 4) {
    if (key == '*') {
      gestionSubstado = 0;
      idxGestion = 0;
      memset(bufferGestion, 0, sizeof(bufferGestion));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GESTION:");
      lcd.setCursor(0, 1);
      lcd.print("1:Umbral 2:Acc *");
    } else if (key == '#') {
      if (idxGestion == 4) {
        bufferGestion[4] = '\0';
        guardar_EEPROM_password(bufferGestion);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Clave guardada!");
        lcd.setCursor(0, 1);
        lcd.print("* para salir");
        gestionSubstado = 0;
      }
      idxGestion = 0;
      memset(bufferGestion, 0, sizeof(bufferGestion));
    } else if (idxGestion < 4 && key >= '0' && key <= '9') {
      bufferGestion[idxGestion] = key;
      idxGestion++;
      lcd.setCursor(idxGestion - 1, 1);
      lcd.print('*');
    }
  }
}
