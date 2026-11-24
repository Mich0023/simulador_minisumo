/* =====================================================================
   constantes.h  -  los nombres que usas en configuracion.h
   ---------------------------------------------------------------------
   Este archivo no hace nada por si solo: nada mas pone los nombres
   (PLACA_..., SENS_..., ROL_..., SEC_...) para que configuracion.h los
   pueda usar. Se carga antes que todo lo demas.

   NO necesitas editar este archivo.
   ===================================================================== */
#ifndef CONSTANTES_H
#define CONSTANTES_H

#include <Arduino.h>

/* ---------- placas ---------- */
#define PLACA_XMOTION_V3   1   // JSumo XMotion V3 (ATmega32U4, "Arduino Leonardo")
#define PLACA_AVR_TB6612   2   // Arduino Uno / Nano / Pro Mini + driver TB6612FNG
#define PLACA_AVR_L298     3   // Arduino Uno / Nano + driver L298N
#define PLACA_ESP32_TB6612 4   // ESP32 DevKit + TB6612FNG
#define PLACA_STM32_TB6612 5   // STM32 "Blue Pill" + TB6612FNG
#define PLACA_PERSONAL     9   // la tuya: defines todo a mano

/* ---------- formas de manejar un motor ---------- */
#define DRV_PWM_DIR    1   // 1 pin de PWM + 1 pin de direccion (XMotion)
#define DRV_PWM_2DIR   2   // 1 pin de PWM + 2 pines de direccion (TB6612, DRV8833, L298N)
#define DRV_2PWM       3   // 2 pines de PWM, uno por sentido

/* ---------- tipos de sensor ---------- */
#define SENS_DIG_ALTO   0   // digital, detecta en ALTO    (JS200XF, JS40F, JS60F)
#define SENS_DIG_BAJO   1   // digital, detecta en BAJO    (E18-D80NK)
#define SENS_ANA_MAYOR  2   // analogico, detecta si SUBE del umbral
#define SENS_ANA_MENOR  3   // analogico, detecta si BAJA del umbral
#define SENS_SHARP_41   4   // Sharp GP2Y0A41SK0F  (4 a 30 cm)  -> milimetros
#define SENS_SHARP_21   5   // Sharp GP2Y0A21YK0F  (10 a 80 cm) -> milimetros
#define SENS_QTR_RC     6   // QTR-1RC / QTR-8RC   -> microsegundos
#define SENS_QTR_ANA    7   // QTR-1A, CNY70, TCRT5000 analogico
#define SENS_HCSR04     8   // ultrasonico HC-SR04 -> milimetros

/* ---------- para que sirve cada sensor ---------- */
#define ROL_OPONENTE    0
#define ROL_LINEA       1

/* ---------- en que parte del robot esta ---------- */
#define SEC_FRENTE      0
#define SEC_IZQ         1
#define SEC_DER         2
#define SEC_ATRAS       3
#define N_SECTORES      4

struct Sensor {
  const char* id;       // nombre corto, sin espacios (ej. "fI")
  const char* nombre;   // nombre que se ve en la plataforma
  uint8_t  tipo;        // SENS_...
  uint8_t  rol;         // ROL_OPONENTE o ROL_LINEA
  uint8_t  sector;      // SEC_...
  int16_t  pin;         // pin de la señal
  int16_t  pin2;        // solo HC-SR04: pin de disparo. Si no, -1
  int16_t  umbral;      // en la unidad del sensor
  int16_t  x, y;        // posicion en el robot, en mm (x al frente, y a la izquierda)
  int16_t  ang;         // hacia donde mira, en grados (0 frente, 90 izquierda)
};

#endif /* CONSTANTES_H */
