/* =====================================================================
   motores.h  -  maneja los motores sin importar el driver
   ---------------------------------------------------------------------
   La velocidad siempre va de -255 (todo atras) a +255 (todo adelante),
   en cualquier placa y con cualquier driver.

   NO necesitas editar este archivo.
   ===================================================================== */
#ifndef MOTORES_H
#define MOTORES_H

#include "placas.h"

/* velocidad real que se mando, para la telemetria */
extern int16_t velIzq, velDer;

inline void motoresInit() {
  msPwmInit(PIN_PWM_IZQ);
  msPwmInit(PIN_PWM_DER);
#if MS_DRIVER == DRV_PWM_DIR
  pinMode(PIN_DIR_IZQ, OUTPUT);
  pinMode(PIN_DIR_DER, OUTPUT);
#elif MS_DRIVER == DRV_PWM_2DIR
  pinMode(PIN_IN1_IZQ, OUTPUT); pinMode(PIN_IN2_IZQ, OUTPUT);
  pinMode(PIN_IN1_DER, OUTPUT); pinMode(PIN_IN2_DER, OUTPUT);
#elif MS_DRIVER == DRV_2PWM
  msPwmInit(PIN_PWM2_IZQ);
  msPwmInit(PIN_PWM2_DER);
#endif
#if PIN_STBY >= 0
  pinMode(PIN_STBY, OUTPUT);
  digitalWrite(PIN_STBY, HIGH);
#endif
}

static void __attribute__((noinline)) unMotor(bool izquierdo, int16_t v) {
  if (v > 255) v = 255;
  if (v < -255) v = -255;
  uint8_t mag = (uint8_t)(v < 0 ? -v : v);
  bool adelante = v >= 0;
#if MS_DRIVER == DRV_PWM_DIR
  digitalWrite(izquierdo ? PIN_DIR_IZQ : PIN_DIR_DER, adelante ? HIGH : LOW);
  msPwmWrite(izquierdo ? PIN_PWM_IZQ : PIN_PWM_DER, mag);
#elif MS_DRIVER == DRV_PWM_2DIR
  int in1 = izquierdo ? PIN_IN1_IZQ : PIN_IN1_DER;
  int in2 = izquierdo ? PIN_IN2_IZQ : PIN_IN2_DER;
  digitalWrite(in1, adelante ? HIGH : LOW);
  digitalWrite(in2, adelante ? LOW : HIGH);
  msPwmWrite(izquierdo ? PIN_PWM_IZQ : PIN_PWM_DER, mag);
#elif MS_DRIVER == DRV_2PWM
  int pa = izquierdo ? PIN_PWM_IZQ  : PIN_PWM_DER;
  int pb = izquierdo ? PIN_PWM2_IZQ : PIN_PWM2_DER;
  msPwmWrite(adelante ? pa : pb, mag);
  msPwmWrite(adelante ? pb : pa, 0);
#endif
}

static void __attribute__((noinline)) motores(int16_t vL, int16_t vR) {
  if (INVERTIR_IZQ) vL = -vL;
  if (INVERTIR_DER) vR = -vR;
  if (INTERCAMBIAR_MOTORES) { int16_t t = vL; vL = vR; vR = t; }
  velIzq = vL; velDer = vR;
  unMotor(true, vL);
  unMotor(false, vR);
}

static void __attribute__((noinline)) frenar() {
  velIzq = velDer = 0;
#if MS_DRIVER == DRV_PWM_2DIR
  // freno corto: las dos entradas en alto
  digitalWrite(PIN_IN1_IZQ, HIGH); digitalWrite(PIN_IN2_IZQ, HIGH);
  digitalWrite(PIN_IN1_DER, HIGH); digitalWrite(PIN_IN2_DER, HIGH);
  msPwmWrite(PIN_PWM_IZQ, 255);    msPwmWrite(PIN_PWM_DER, 255);
#elif MS_DRIVER == DRV_2PWM
  msPwmWrite(PIN_PWM_IZQ, 0);  msPwmWrite(PIN_PWM2_IZQ, 0);
  msPwmWrite(PIN_PWM_DER, 0);  msPwmWrite(PIN_PWM2_DER, 0);
#else
  msPwmWrite(PIN_PWM_IZQ, 0);
  msPwmWrite(PIN_PWM_DER, 0);
#endif
}

/* gira sobre su propio eje: h = -1 izquierda, +1 derecha */
inline void girar(int16_t h, int16_t v) { motores((int16_t)(h * v), (int16_t)(-h * v)); }

#endif /* MOTORES_H */
