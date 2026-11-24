/* =====================================================================
   sensores.h  -  cualquier cantidad, cualquier tipo
   ---------------------------------------------------------------------
   Tu declaras una tabla de sensores en configuracion.h y el resto del
   programa trabaja por SECTORES (frente, izquierda, derecha, atras),
   no por sensores sueltos. Asi puedes usar 2 sensores o 10 y la
   estrategia no cambia ni una linea.

   NO necesitas editar este archivo.
   ===================================================================== */
#ifndef SENSORES_H
#define SENSORES_H

#include "constantes.h"
#include "placas.h"

/* la tabla la defines tu en configuracion.h */
extern Sensor SENSORES[];
extern const uint8_t N_SENSORES;

/* resultados de la ultima lectura */
extern int16_t valor[];        // lectura cruda en la unidad del sensor
extern bool    detecta[];      // true si ese sensor cuenta como detectando (ya filtrado)
extern bool    detectaCrudo[]; // lo que dio el sensor antes de los filtros

/* ---------- cuanto es "muy lejos" para cada tipo ---------- */
#define QTR_RC_TOPE      3000   // us: mas de esto ya es negro seguro
#define DIST_LEJOS       9999   // mm: no ve nada

inline const char* unidadDe(uint8_t tipo) {
  switch (tipo) {
    case SENS_SHARP_41: case SENS_SHARP_21: case SENS_HCSR04: return "mm";
    case SENS_QTR_RC:   return "us";
    case SENS_ANA_MAYOR: case SENS_ANA_MENOR: case SENS_QTR_ANA: return "adc";
    default: return "";
  }
}
inline const char* nombreTipo(uint8_t tipo) {
  switch (tipo) {
    case SENS_DIG_ALTO:  return "digital-alto";
    case SENS_DIG_BAJO:  return "digital-bajo";
    case SENS_ANA_MAYOR: return "analogico-mayor";
    case SENS_ANA_MENOR: return "analogico-menor";
    case SENS_SHARP_41:  return "sharp-41";
    case SENS_SHARP_21:  return "sharp-21";
    case SENS_QTR_RC:    return "qtr-rc";
    case SENS_QTR_ANA:   return "qtr-analogico";
    case SENS_HCSR04:    return "ultrasonico";
    default:             return "?";
  }
}
/* true si el sensor entrega una distancia de verdad (en mm) */
inline bool daDistancia(uint8_t tipo) {
  return tipo == SENS_SHARP_41 || tipo == SENS_SHARP_21 || tipo == SENS_HCSR04;
}

/* =====================================================================
   Lectura de cada tipo
   ===================================================================== */

/* QTR-1RC: carga el capacitor y mide cuanto tarda en descargarse.
   Blanco = rapido (pocos us). Negro = lento. */
inline int16_t leerQtrRc(int16_t pin, int16_t tope) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delayMicroseconds(10);
  pinMode(pin, INPUT);
  uint32_t t0 = micros();
  int16_t t = 0;
  while (digitalRead(pin) == HIGH && t < tope) t = (int16_t)(micros() - t0);
  return t;
}

/* Sharp: de volts a milimetros, con la curva de cada modelo */
inline int16_t leerSharp(int16_t pin, uint8_t tipo) {
  int crudo = analogRead(pin);
  float v = (crudo * MS_VREF) / (float)MS_ADC_MAX;
  if (v < 0.05f) return DIST_LEJOS;
  float cm, minCm, maxCm;
  if (tipo == SENS_SHARP_41) { cm = 12.08f * pow(v, -1.058f); minCm = 4;  maxCm = 30; }
  else                       { cm = 27.86f * pow(v, -1.15f);  minCm = 10; maxCm = 80; }
  if (cm > maxCm) return DIST_LEJOS;          // fuera de rango: lo tomamos como "no ve"
  if (cm < minCm) cm = minCm;                 // mas cerca del minimo, la curva miente
  return (int16_t)(cm * 10.0f);
}

/* HC-SR04: dispara y mide el eco. Timeout de 6 ms = poco mas de 1 m. */
inline int16_t leerUltra(int16_t pinTrig, int16_t pinEcho) {
  digitalWrite(pinTrig, LOW);  delayMicroseconds(3);
  digitalWrite(pinTrig, HIGH); delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);
  uint32_t us = pulseIn(pinEcho, HIGH, 6000UL);
  if (us == 0) return DIST_LEJOS;
  return (int16_t)(us * 0.1715f);             // ida y vuelta -> mm
}

inline int16_t leerSensor(const Sensor& s) {
  switch (s.tipo) {
    case SENS_DIG_ALTO:
    case SENS_DIG_BAJO:  return digitalRead(s.pin) == HIGH ? 1 : 0;
    case SENS_SHARP_41:
    case SENS_SHARP_21:  return leerSharp(s.pin, s.tipo);
    case SENS_QTR_RC:    return leerQtrRc(s.pin, QTR_RC_TOPE);
    case SENS_HCSR04:    return leerUltra(s.pin2, s.pin);
    default:             return (int16_t)analogRead(s.pin);
  }
}

/* Tope global de alcance para los sensores que SI dan distancia.
   Si no lo definiste en configuracion.h, no limita nada. */
#ifndef ALCANCE_MAX_MM
  #define ALCANCE_MAX_MM 0
#endif

inline bool evaluaSensor(const Sensor& s, int16_t v) {
  switch (s.tipo) {
    case SENS_DIG_ALTO:  return v == 1;
    case SENS_DIG_BAJO:  return v == 0;
    case SENS_ANA_MAYOR: return v >= s.umbral;
    case SENS_SHARP_41:
    case SENS_SHARP_21:
    case SENS_HCSR04: {                             // detecta si esta mas cerca que el umbral
      int16_t lim = s.umbral;
      if (ALCANCE_MAX_MM > 0 && lim > ALCANCE_MAX_MM) lim = ALCANCE_MAX_MM;
      return v <= lim;
    }
    default:             return v <= s.umbral;      // ANA_MENOR, QTR_RC y QTR_ANA: blanco = poco
  }
}

/* =====================================================================
   Lo que usa la estrategia
   ===================================================================== */
extern bool    verSector[N_SECTORES];     // hay rival en ese sector
extern int16_t distSector[N_SECTORES];    // mm al rival (DIST_LEJOS si no se sabe)
extern bool    lineaSector[N_SECTORES];   // se ve la linea blanca por ese lado
extern bool    hayLinea;                  // cualquier sensor de linea

inline void sensoresInit() {
  for (uint8_t i = 0; i < N_SENSORES; i++) {
    const Sensor& s = SENSORES[i];
    if (s.tipo == SENS_HCSR04) { pinMode(s.pin2, OUTPUT); pinMode(s.pin, INPUT); }
    else if (s.tipo == SENS_QTR_RC) { pinMode(s.pin, INPUT); }
    else if (s.tipo == SENS_DIG_ALTO || s.tipo == SENS_DIG_BAJO) {
      // los sensores con salida tipo NPN (E18) necesitan resistencia interna
      pinMode(s.pin, s.tipo == SENS_DIG_BAJO ? INPUT_PULLUP : INPUT);
    } else pinMode(s.pin, INPUT);
  }
}

/* arma los sectores a partir de detecta[], que ya paso por los filtros */
inline void recalcularSectores() {
  for (uint8_t k = 0; k < N_SECTORES; k++) {
    verSector[k] = false; lineaSector[k] = false; distSector[k] = DIST_LEJOS;
  }
  hayLinea = false;
  for (uint8_t i = 0; i < N_SENSORES; i++) {
    if (!detecta[i]) continue;
    const Sensor& s = SENSORES[i];
    if (s.rol == ROL_LINEA) { hayLinea = true; lineaSector[s.sector] = true; }
    else {
      verSector[s.sector] = true;
      if (daDistancia(s.tipo) && valor[i] < distSector[s.sector]) distSector[s.sector] = valor[i];
    }
  }
}

/* solo lee el hardware; los filtros y los sectores van aparte */
inline void sensoresLeer() {
  for (uint8_t i = 0; i < N_SENSORES; i++) {
    int16_t v = leerSensor(SENSORES[i]);
    valor[i] = v;
    detectaCrudo[i] = evaluaSensor(SENSORES[i], v);
    detecta[i] = detectaCrudo[i];
  }
  recalcularSectores();
}

#endif /* SENSORES_H */
