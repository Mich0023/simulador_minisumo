/* =====================================================================
   MINISUMO - firmware multiplaca
   =====================================================================
   Funciona en Arduino (AVR), ESP32, STM32 y cualquier placa con el core
   de Arduino. Soporta la cantidad de sensores que quieras, de los tipos
   que quieras, y manda telemetria por USB a la plataforma web.

   >>> Edita solo configuracion.h <<<

   Repositorio y manual: README.md
   ===================================================================== */

#include "configuracion.h"
#include "placas.h"
#include "sensores.h"
#include "motores.h"
#include "telemetria.h"

/* ---------- memoria de los sensores ---------- */
#define NS (sizeof(SENSORES) / sizeof(SENSORES[0]))
int16_t valor[NS];
bool    detecta[NS];
bool    detectaCrudo[NS];
uint8_t  cuenta[NS];        // lecturas seguidas detectando, para CONFIRMAR
uint32_t desdeCrudo[NS];    // desde cuando no suelta, para T_SENSOR_PEGADO
bool     pegado[NS];        // sensor descartado por quedarse pegado
bool    verSector[N_SECTORES];
int16_t distSector[N_SECTORES];
bool    lineaSector[N_SECTORES];
bool    hayLinea;

/* ---------- estado general ---------- */
int16_t velIzq = 0, velDer = 0;
uint8_t estado = 0, modo = 0;
bool    peleando = false, abortado = false;
int16_t ultimoLado = 1;              // -1 izquierda, +1 derecha
int16_t rumbo = 0;                   // grados hacia el rival (+ izquierda)
bool    veRival = false;
uint32_t tArranque = 0;
uint32_t tEstado = 0;         // desde cuando esta en el estado actual
uint32_t tCambio = 0;         // ultima vez que cambio de maniobra
uint32_t tLineaDesde = 0;     // desde cuando ve linea sin soltar
uint32_t ignoraLineaHasta = 0;
int16_t  ultimoGiro = 1;      // hacia donde giraba cuando se trabo

#define IZQ -1
#define DER  1
#define E_ESPERA    0
#define E_APERTURA  1
#define E_LINEA     2
#define E_ATAQUE    3
#define E_GIRO_LADO 4
#define E_BUSQUEDA  5
#define E_PRUEBA    6
#define E_DESTRABE  7
#define E_ROMPE     8
#define E_SIN_LINEA 9

/* a que distancia suponemos que esta el rival cuando el sensor no la mide */
#ifndef DIST_NOMINAL
  #define DIST_NOMINAL 150
#endif
/* con este angulo de error el robot ya corrige al maximo */
#ifndef ANG_CORRECCION_PLENA
  #define ANG_CORRECCION_PLENA 20
#endif
/* arriba de este angulo deja de embestir y gira en su lugar */
#ifndef ANG_ATAQUE
  #define ANG_ATAQUE 60
#endif

/* =====================================================================
   ARRANCADOR
   ===================================================================== */
bool arrancadorOk() {
#if PIN_START >= 0
  return digitalRead(PIN_START) == HIGH;
#else
  return millis() - tArranque >= ESPERA_SIN_ARRANCADOR;
#endif
}
bool killOk() {
#if PIN_KILL >= 0
  return digitalRead(PIN_KILL) == HIGH;
#else
  return true;
#endif
}
bool permitido() { return arrancadorOk() && killOk(); }

uint8_t leerDip() {
  uint8_t d = 0;
#if PIN_DIP1 >= 0
  if (digitalRead(PIN_DIP1) == LOW) d |= 1;
#endif
#if PIN_DIP2 >= 0
  if (digitalRead(PIN_DIP2) == LOW) d |= 2;
#endif
#if PIN_DIP3 >= 0
  if (digitalRead(PIN_DIP3) == LOW) d |= 4;
#endif
  return d;
}

/* =====================================================================
   SERVO (sin la libreria Servo, que pelea con el PWM de los motores)
   ===================================================================== */
void servoTick() {
#if PIN_SERVO >= 0
  static uint32_t tPulso = 0;
  if (millis() - tPulso < 20) return;
  tPulso = millis();
  bool abriendo = peleando && (millis() - tArranque < T_SERVO);
  if (peleando && !abriendo) return;          // ya abrio: deja de gastar pulsos
  uint16_t us = abriendo ? SERVO_US_ABIERTO : SERVO_US_CERRADO;
  digitalWrite(PIN_SERVO, HIGH);
  delayMicroseconds(us);
  digitalWrite(PIN_SERVO, LOW);
#endif
}

/* =====================================================================
   HACIA DONDE ESTA EL RIVAL
   ---------------------------------------------------------------------
   Cada sensor que detecta apunta a un punto: el que esta sobre su rayo,
   a la distancia que midio (o a una distancia supuesta si no la mide).
   El promedio de esos puntos, visto desde el centro del robot, da el
   rumbo. Asi funciona igual con 2 sensores que con 8, y en el acomodo
   que sea: lo unico que importa son las posiciones y angulos que pusiste
   en configuracion.h.
   ===================================================================== */
void calcularRumbo() {
  float sx = 0, sy = 0; uint8_t n = 0;
  for (uint8_t i = 0; i < N_SENSORES; i++) {
    const Sensor& s = SENSORES[i];
    if (s.rol != ROL_OPONENTE || !detecta[i]) continue;
    float d = daDistancia(s.tipo) ? (float)valor[i] : (float)DIST_NOMINAL;
    float a = s.ang * 0.0174533f;
    sx += s.x + d * cos(a);
    sy += s.y + d * sin(a);
    n++;
  }
  veRival = n > 0;
  if (!n) return;
  rumbo = (int16_t)(atan2(sy / n, sx / n) * 57.2958f);
  if (rumbo > 10)  ultimoLado = IZQ;
  if (rumbo < -10) ultimoLado = DER;
}

void leds() {
#if PIN_LED1 >= 0
  digitalWrite(PIN_LED1, (verSector[SEC_IZQ] || lineaSector[SEC_IZQ] || (veRival && rumbo > 0)) ? HIGH : LOW);
#endif
#if PIN_LED2 >= 0
  digitalWrite(PIN_LED2, (verSector[SEC_DER] || lineaSector[SEC_DER] || (veRival && rumbo < 0)) ? HIGH : LOW);
#endif
}

/* =====================================================================
   FILTROS
   ---------------------------------------------------------------------
   Entre leer el hardware y decidir que hacer, se descartan las lecturas
   que no son de fiar. Ver la seccion 5 de configuracion.h.
   ===================================================================== */
void filtrarSensores() {
  uint32_t ahora = millis();

  for (uint8_t i = 0; i < N_SENSORES; i++) {
    bool c = detectaCrudo[i];

    // sensor pegado: lleva demasiado tiempo detectando sin soltar
    if (T_SENSOR_PEGADO > 0 && SENSORES[i].rol == ROL_OPONENTE) {
      if (c) {
        if (!desdeCrudo[i]) desdeCrudo[i] = ahora;
        else if (!pegado[i] && ahora - desdeCrudo[i] > T_SENSOR_PEGADO) {
          pegado[i] = true;
          evento("SENSOR PEGADO");
        }
      } else { desdeCrudo[i] = 0; pegado[i] = false; }
      if (pegado[i]) c = false;
    }

    // confirmacion: tiene que detectar varias lecturas seguidas
    if (CONFIRMAR > 1) {
      if (c) { if (cuenta[i] < CONFIRMAR) cuenta[i]++; }
      else cuenta[i] = 0;
      c = (cuenta[i] >= CONFIRMAR);
    }
    detecta[i] = c;
  }

  // dos sensores que miran a lados opuestos no pueden ver al mismo rival
  if (IGNORAR_CONTRADICTORIOS) {
    for (uint8_t i = 0; i < N_SENSORES; i++) {
      if (!detecta[i] || SENSORES[i].rol != ROL_OPONENTE) continue;
      for (uint8_t j = i + 1; j < N_SENSORES; j++) {
        if (!detecta[j] || SENSORES[j].rol != ROL_OPONENTE) continue;
        int16_t dif = SENSORES[i].ang - SENSORES[j].ang;
        while (dif > 180) dif -= 360;
        while (dif < -180) dif += 360;
        if (dif < 0) dif = -dif;
        if (dif >= 120) { detecta[i] = false; detecta[j] = false; }
      }
    }
  }

  recalcularSectores();

  // linea pegada: dice blanco desde hace demasiado
  if (hayLinea) {
    if (!tLineaDesde) tLineaDesde = ahora;
    else if (T_LINEA_PEGADA > 0 && ahora - tLineaDesde > T_LINEA_PEGADA && ahora > ignoraLineaHasta) {
      ignoraLineaHasta = ahora + T_IGNORA_LINEA;
      evento("LINEA PEGADA");
    }
  } else tLineaDesde = 0;

  if (ahora < ignoraLineaHasta) {
    hayLinea = false;
    for (uint8_t k = 0; k < N_SECTORES; k++) lineaSector[k] = false;
  }
}

void leer() {
  sensoresLeer();
  filtrarSensores();
  calcularRumbo();
  leds();
  servoTick();
  telemetria();
}

/* cambia de estado y apunta desde cuando */
void pasaA(uint8_t e) {
  if (estado != e) { estado = e; tEstado = millis(); tCambio = millis(); }
}

/* =====================================================================
   MOVIMIENTOS CON TIEMPO
   ===================================================================== */
#define FIN_TIEMPO 0
#define FIN_RIVAL  1
#define FIN_LINEA  2
#define FIN_PARO   3

bool seguir() {
  if (!permitido()) { frenar(); abortado = true; }
  return !abortado;
}

int moverPor(int16_t vL, int16_t vR, uint32_t ms, bool mirarLinea, int32_t verDesde) {
  uint32_t t0 = millis();
  motores(vL, vR);
  while (millis() - t0 < ms) {
    if (!seguir()) return FIN_PARO;
    leer();
    if (mirarLinea && hayLinea) return FIN_LINEA;
    if (verDesde >= 0 && (int32_t)(millis() - t0) >= verDesde && verSector[SEC_FRENTE]) return FIN_RIVAL;
  }
  return FIN_TIEMPO;
}

/* =====================================================================
   LINEA BLANCA: la prioridad mas alta
   ===================================================================== */
void escaparDeLinea() {
  pasaA(E_LINEA);
  // hacia donde conviene salir, segun por donde vio el blanco
  int16_t h = ultimoLado;
  if (lineaSector[SEC_IZQ] && !lineaSector[SEC_DER]) h = DER;
  else if (lineaSector[SEC_DER] && !lineaSector[SEC_IZQ]) h = IZQ;

  bool atras = !lineaSector[SEC_ATRAS];       // si el blanco esta atras, hay que ir al frente
  int16_t v = atras ? -VEL_ESCAPE : VEL_ESCAPE;
  if (moverPor(v, v, T_RETROCESO, false, -1) == FIN_PARO) return;
  moverPor(h * VEL_ESCAPE, -h * VEL_ESCAPE, T_ESCAPE, false, T_ESCAPE / 2);
}

/* =====================================================================
   COMBATE
   ===================================================================== */
/* =====================================================================
   DESTRABE
   ---------------------------------------------------------------------
   Los dos robots girando uno junto al otro sin pegarse nunca, o empujando
   de frente sin que ninguno avance. Hay que salir de ahi y volver a entrar
   desde otro angulo. Nunca se detienen los motores.
   ===================================================================== */
uint8_t  nDestrabes = 0;
uint32_t tUltDestrabe = 0;

void destrabar() {
  pasaA(E_DESTRABE);
  // si se vuelve a trabar luego luego, cada intento gira mas que el anterior
  uint32_t ahora = millis();
  if (ahora - tUltDestrabe < 3000 && nDestrabes < 4) nDestrabes++; else nDestrabes = 0;
  tUltDestrabe = ahora;
  evento("DESTRABE");

  uint32_t giro = T_DESTRABE_GIRO + (uint32_t)T_DESTRABE_GIRO * nDestrabes / 2;
  if (moverPor(-VEL_ESCAPE, -VEL_ESCAPE, T_DESTRABE_ATRAS, true, -1) == FIN_PARO) return;
  // gira al reves de como estaba girando, para romper el circulo
  int16_t h = -ultimoGiro;
  ultimoLado = h;
  if (moverPor(h * VEL_ESCAPE, -h * VEL_ESCAPE, giro, true, giro / 3) == FIN_TIEMPO) {
    // si al terminar el giro sigue sin verlo, tira una embestida a ciegas
    moverPor(VEL_ATAQUE, VEL_ATAQUE, 220, true, 0);
  }
  tCambio = millis();
}

/* Empuje trabado. NO se retrocede: eso seria regalarle terreno al rival
   justo cuando lo tienes contra la orilla. Se sigue empujando pero
   cambiando el angulo de un lado al otro, para meter la cuchilla debajo. */
void romperEmpuje() {
  pasaA(E_ROMPE);
  evento("ROMPE EMPUJE");
  int16_t poco = (int16_t)((int32_t)VEL_ATAQUE * PCT_ROMPE / 100);
  moverPor(VEL_ATAQUE, poco, T_ROMPE_GIRO, true, -1);
  moverPor(poco, VEL_ATAQUE, T_ROMPE_GIRO, true, -1);
  tCambio = millis();
}

void combate() {
  tEstado = tCambio = millis();
  while (seguir()) {
    leer();
    uint32_t ahora = millis();

    // 0) guardia general: lleva demasiado sin cambiar de maniobra
    if (T_MAX_SIN_CAMBIO > 0 && ahora - tCambio > T_MAX_SIN_CAMBIO) { destrabar(); continue; }

    // 1) linea blanca: manda siempre
    if (hayLinea) { pasaA(E_LINEA); escaparDeLinea(); tCambio = millis(); continue; }

    // 2) rival a la vista
    if (veRival) {
      int16_t a = rumbo < 0 ? -rumbo : rumbo;
      if (a <= ANG_ATAQUE) {
        // lleva mucho empujando sin resultado: cambia de angulo
        if (T_MAX_EMPUJE > 0 && estado == E_ATAQUE && ahora - tEstado > T_MAX_EMPUJE) { romperEmpuje(); continue; }
        pasaA(E_ATAQUE);
        int32_t k = a;
        if (k > ANG_CORRECCION_PLENA) k = ANG_CORRECCION_PLENA;
        // la llanta de adentro baja de 100 % hasta PCT_CORRECCION
        int16_t dentro = (int16_t)(VEL_ATAQUE * (100 - (100 - PCT_CORRECCION) * k / ANG_CORRECCION_PLENA) / 100);
        if (rumbo > 0) motores(dentro, VEL_ATAQUE);     // esta a la izquierda
        else           motores(VEL_ATAQUE, dentro);     // a la derecha
      } else {
        // de lado o atras: girar hacia el. Si lleva mucho girando, los dos
        // estan dando vueltas juntos y hay que destrabarse.
        if (T_MAX_LADO > 0 && estado == E_GIRO_LADO && ahora - tEstado > T_MAX_LADO) { destrabar(); continue; }
        pasaA(E_GIRO_LADO);
        ultimoGiro = rumbo > 0 ? IZQ : DER;
        girar(ultimoGiro, VEL_GIRO);
      }
      continue;
    }

    // 3) no ve nada: buscar
    pasaA(E_BUSQUEDA);
    if (BUSCAR_GIRANDO) { ultimoGiro = ultimoLado; girar(ultimoLado, VEL_BUSQUEDA); }
    else                motores(VEL_AVANCE, VEL_AVANCE);
  }
}

/* =====================================================================
   APERTURAS
   ===================================================================== */
void giroApertura(int16_t h, uint32_t t) {
  ultimoLado = h;
  moverPor(h * VEL_GIRO, -h * VEL_GIRO, t, true, (int32_t)(t * PCT_GIRO_CIEGO / 100));
}

void flanqueo(int16_t h) {
  ultimoLado = -h;
  if (moverPor(h * VEL_GIRO, -h * VEL_GIRO, T_GIRO_45, true, -1) != FIN_TIEMPO) return;
  int16_t interior = (int16_t)((int32_t)VEL_ATAQUE * PCT_ARCO_INTERIOR / 100);
  if (h == IZQ) moverPor(VEL_ATAQUE, interior, T_ARCO, true, 0);
  else          moverPor(interior, VEL_ATAQUE, T_ARCO, true, 0);
}

void apertura() {
  pasaA(E_APERTURA);
  switch (modo) {
    case 1: moverPor(VEL_ATAQUE, VEL_ATAQUE, T_DIRECTO, true, -1); return;
    case 2: giroApertura(IZQ, T_GIRO_90);  return;
    case 3: giroApertura(DER, T_GIRO_90);  return;
    case 4: giroApertura(DER, T_GIRO_180); return;
    case 5: flanqueo(IZQ); return;
    case 6: flanqueo(DER); return;
    case 7: giroApertura(IZQ, T_GIRO_180); return;
  }
  // 0 = automatica: decide con lo que ve al arrancar
  leer();
  if (verSector[SEC_FRENTE]) return;                       // ya lo tiene enfrente
  if (verSector[SEC_IZQ] && !verSector[SEC_DER]) giroApertura(IZQ, T_GIRO_90);
  else if (verSector[SEC_DER] && !verSector[SEC_IZQ]) giroApertura(DER, T_GIRO_90);
  else if (verSector[SEC_ATRAS]) giroApertura(DER, T_GIRO_180);
  else giroApertura(DER, T_GIRO_180);                      // no ve nada: estan de espaldas
}

/* =====================================================================
   COMANDOS DESDE LA PLATAFORMA
   ===================================================================== */
char buf[40]; uint8_t nbuf = 0;

void pruebaMotores(int16_t vL, int16_t vR, uint32_t ms) {
  if (ms > 3000) ms = 3000;
  estado = E_PRUEBA;
  evento("PRUEBA");
  uint32_t t0 = millis();
  motores(vL, vR);
  while (millis() - t0 < ms) {
#if PIN_START >= 0
    if (arrancadorOk()) break;                       // si llega el GO, se corta
#endif
    if (MS_PUERTO.available() && MS_PUERTO.peek() == 'X') { MS_PUERTO.read(); break; }
    leer();
  }
  frenar();
  estado = E_ESPERA;
  evento("FIN PRUEBA");
}

void comandos() {
  while (MS_PUERTO.available()) {
    char c = MS_PUERTO.read();
    if (c == '\r') continue;
    if (c != '\n') { if (nbuf < sizeof(buf) - 1) buf[nbuf++] = c; continue; }
    buf[nbuf] = 0; nbuf = 0;
    if (buf[0] == '?') mandaInfo();
    else if (buf[0] == 'X') frenar();
    else if (buf[0] == 'U') {                        // U,idx,umbral
      int idx = atoi(strtok(buf + 2, ","));
      char* p = strtok(NULL, ",");
      if (p && idx >= 0 && idx < N_SENSORES) {
        SENSORES[idx].umbral = atoi(p);
        evento("UMBRAL");
      }
    }
    else if (buf[0] == 'M') {                        // M,vL,vR,ms
#if PIN_START < 0
      tArranque = millis();                          // sin arrancador: la prueba aplaza el inicio
#else
      if (arrancadorOk()) continue;                  // con GO puesto no se prueban motores
#endif
      int16_t vL = 0, vR = 0; int32_t ms = 0;
      char* p = strtok(buf + 2, ","); if (p) vL = atoi(p);
      p = strtok(NULL, ",");          if (p) vR = atoi(p);
      p = strtok(NULL, ",");          if (p) ms = atol(p);
      pruebaMotores(vL, vR, ms);
    }
  }
}

/* =====================================================================
   SETUP Y LOOP
   ===================================================================== */
void setup() {
  motoresInit();
  frenar();
  sensoresInit();
#if PIN_START >= 0
  pinMode(PIN_START, INPUT);
#endif
#if PIN_KILL >= 0
  pinMode(PIN_KILL, INPUT_PULLUP);
#endif
#if PIN_LED1 >= 0
  pinMode(PIN_LED1, OUTPUT);
#endif
#if PIN_LED2 >= 0
  pinMode(PIN_LED2, OUTPUT);
#endif
#if PIN_DIP1 >= 0
  pinMode(PIN_DIP1, INPUT_PULLUP);
#endif
#if PIN_DIP2 >= 0
  pinMode(PIN_DIP2, INPUT_PULLUP);
#endif
#if PIN_DIP3 >= 0
  pinMode(PIN_DIP3, INPUT_PULLUP);
#endif
#if PIN_SERVO >= 0
  pinMode(PIN_SERVO, OUTPUT);
#endif
  MS_PUERTO.begin(BAUDIOS);
  tArranque = millis();
}

void loop() {
  frenar();
  abortado = false;
  peleando = false;
  estado = E_ESPERA;

#if PIN_DIP1 >= 0
  modo = leerDip();
#else
  modo = APERTURA_FIJA;
#endif

  leer();
  comandos();

  if (!permitido()) return;

  // GO: arranca sin ningun retraso (reglamento Robochallenge 4.2)
  peleando = true;
  tArranque = millis();
  for (uint8_t i = 0; i < N_SENSORES; i++) { cuenta[i] = 0; desdeCrudo[i] = 0; pegado[i] = false; }
  tLineaDesde = ignoraLineaHasta = 0;
  nDestrabes = 0; tUltDestrabe = 0;
  evento("GO");
  apertura();
  if (!abortado) combate();

  frenar();
  peleando = false;
  estado = E_ESPERA;
  evento("STOP");
  // se queda parado hasta que el arbitro quite el GO
  while (arrancadorOk()) { frenar(); leer(); comandos(); }
}
