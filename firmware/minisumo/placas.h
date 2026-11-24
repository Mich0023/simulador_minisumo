/* =====================================================================
   placas.h  -  perfiles de placa
   ---------------------------------------------------------------------
   Aqui esta lo que cambia de una tarjeta a otra: como se maneja el PWM,
   cuantos bits tiene el convertidor analogico y a cuantos volts trabaja.

   NO necesitas editar este archivo para usar el programa.
   Elige tu placa en configuracion.h y listo. Solo vuelve aqui si quieres
   agregar una placa que no esta en la lista.
   ===================================================================== */
#ifndef PLACAS_H
#define PLACAS_H

#include "constantes.h"
#include "configuracion.h"

/* =====================================================================
   Caracteristicas del micro: resolucion del ADC y voltaje de referencia.
   Se detectan solas; si tu placa no esta, cae en los valores de AVR.
   ===================================================================== */
#if defined(ARDUINO_ARCH_ESP32)
  #define MS_ADC_MAX      4095
  #define MS_VREF         3.3f
  #define MS_NOMBRE_ARQ   "ESP32"
#elif defined(ARDUINO_ARCH_STM32)
  #define MS_ADC_MAX      4095
  #define MS_VREF         3.3f
  #define MS_NOMBRE_ARQ   "STM32"
#elif defined(ARDUINO_ARCH_RP2040)
  #define MS_ADC_MAX      1023
  #define MS_VREF         3.3f
  #define MS_NOMBRE_ARQ   "RP2040"
#elif defined(ARDUINO_ARCH_SAMD)
  #define MS_ADC_MAX      1023
  #define MS_VREF         3.3f
  #define MS_NOMBRE_ARQ   "SAMD"
#else
  #define MS_ADC_MAX      1023
  #define MS_VREF         5.0f
  #define MS_NOMBRE_ARQ   "AVR"
#endif

/* =====================================================================
   PWM: en casi todas las placas sirve analogWrite(). El ESP32 necesita
   su propio manejo (LEDC), y cambio entre las versiones 2.x y 3.x del
   core, asi que las dos quedan cubiertas aqui.
   En todas las placas el rango de velocidad es 0 a 255.
   ===================================================================== */
#ifndef MS_PWM_FREQ
  #define MS_PWM_FREQ 20000     // 20 kHz: arriba de lo que se oye (motores sin chillido)
#endif

#if defined(ARDUINO_ARCH_ESP32)
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    inline void msPwmInit(uint8_t pin) { ledcAttach(pin, MS_PWM_FREQ, 8); }
    inline void msPwmWrite(uint8_t pin, uint8_t v) { ledcWrite(pin, v); }
  #else
    inline uint8_t msCanal(uint8_t pin) {
      static uint8_t pines[16]; static uint8_t n = 0;
      for (uint8_t i = 0; i < n; i++) if (pines[i] == pin) return i;
      pines[n] = pin; return n++;
    }
    inline void msPwmInit(uint8_t pin) {
      uint8_t ch = msCanal(pin);
      ledcSetup(ch, MS_PWM_FREQ, 8);
      ledcAttachPin(pin, ch);
    }
    inline void msPwmWrite(uint8_t pin, uint8_t v) { ledcWrite(msCanal(pin), v); }
  #endif
#else
  inline void msPwmInit(uint8_t pin) { pinMode(pin, OUTPUT); }
  inline void msPwmWrite(uint8_t pin, uint8_t v) { analogWrite(pin, v); }
#endif

/* =====================================================================
   Pines por defecto de cada placa. Cualquiera de estos se puede pisar
   desde configuracion.h: si ahi defines el valor antes de incluir este
   archivo, se respeta el tuyo.
   ===================================================================== */

/* ---------- JSumo XMotion V3 ---------- */
#if PLACA == PLACA_XMOTION_V3
  #define MS_PLACA_NOMBRE "XMotion V3"
  #define MS_DRIVER       DRV_PWM_DIR
  #ifndef PIN_PWM_IZQ
    #define PIN_PWM_IZQ   10
    #define PIN_DIR_IZQ   12
    #define PIN_PWM_DER   11
    #define PIN_DIR_DER   13
  #endif
  #ifndef PIN_START
    #define PIN_START     A0
  #endif
  #ifndef PIN_KILL
    #define PIN_KILL      1
  #endif
  #ifndef PIN_LED1
    #define PIN_LED1      8
    #define PIN_LED2      9
  #endif
  #ifndef PIN_DIP1
    #define PIN_DIP1      5
    #define PIN_DIP2      6
    #define PIN_DIP3      7
  #endif

/* ---------- Arduino Uno / Nano + TB6612FNG ---------- */
#elif PLACA == PLACA_AVR_TB6612
  #define MS_PLACA_NOMBRE "Arduino + TB6612"
  #define MS_DRIVER       DRV_PWM_2DIR
  #ifndef PIN_PWM_IZQ
    #define PIN_PWM_IZQ   10
    #define PIN_IN1_IZQ   8
    #define PIN_IN2_IZQ   9
    #define PIN_PWM_DER   5
    #define PIN_IN1_DER   7
    #define PIN_IN2_DER   6
    #define PIN_STBY      3
  #endif
  #ifndef PIN_START
    #define PIN_START     11
  #endif

/* ---------- Arduino Uno / Nano + L298N ---------- */
#elif PLACA == PLACA_AVR_L298
  #define MS_PLACA_NOMBRE "Arduino + L298N"
  #define MS_DRIVER       DRV_PWM_2DIR
  #ifndef PIN_PWM_IZQ
    #define PIN_PWM_IZQ   10        // ENA
    #define PIN_IN1_IZQ   8
    #define PIN_IN2_IZQ   9
    #define PIN_PWM_DER   5         // ENB
    #define PIN_IN1_DER   7
    #define PIN_IN2_DER   6
  #endif
  #ifndef PIN_START
    #define PIN_START     11
  #endif

/* ---------- ESP32 DevKit + TB6612FNG ---------- */
#elif PLACA == PLACA_ESP32_TB6612
  #define MS_PLACA_NOMBRE "ESP32 + TB6612"
  #define MS_DRIVER       DRV_PWM_2DIR
  #ifndef PIN_PWM_IZQ
    #define PIN_PWM_IZQ   25
    #define PIN_IN1_IZQ   26
    #define PIN_IN2_IZQ   27
    #define PIN_PWM_DER   33
    #define PIN_IN1_DER   32
    #define PIN_IN2_DER   14
    #define PIN_STBY      13
  #endif
  #ifndef PIN_START
    #define PIN_START     4
  #endif

/* ---------- STM32 Blue Pill + TB6612FNG ---------- */
#elif PLACA == PLACA_STM32_TB6612
  #define MS_PLACA_NOMBRE "STM32 + TB6612"
  #define MS_DRIVER       DRV_PWM_2DIR
  #ifndef PIN_PWM_IZQ
    #define PIN_PWM_IZQ   PA8
    #define PIN_IN1_IZQ   PB12
    #define PIN_IN2_IZQ   PB13
    #define PIN_PWM_DER   PA9
    #define PIN_IN1_DER   PB14
    #define PIN_IN2_DER   PB15
    #define PIN_STBY      PB1
  #endif
  #ifndef PIN_START
    #define PIN_START     PA0
  #endif

/* ---------- la tuya ---------- */
#elif PLACA == PLACA_PERSONAL
  #ifndef MS_PLACA_NOMBRE
    #define MS_PLACA_NOMBRE "Personalizada"
  #endif
  #ifndef MS_DRIVER
    #error "Con PLACA_PERSONAL tienes que definir MS_DRIVER y los pines de los motores en configuracion.h"
  #endif

#else
  #error "No elegiste una placa valida en configuracion.h (revisa la constante PLACA)"
#endif

/* ---------- cosas opcionales: si no las definiste, no existen ---------- */
#ifndef PIN_KILL
  #define PIN_KILL   -1
#endif
#ifndef PIN_STBY
  #define PIN_STBY   -1
#endif
#ifndef PIN_LED1
  #define PIN_LED1   -1
#endif
#ifndef PIN_LED2
  #define PIN_LED2   -1
#endif
#ifndef PIN_DIP1
  #define PIN_DIP1   -1
#endif
#ifndef PIN_DIP2
  #define PIN_DIP2   -1
#endif
#ifndef PIN_DIP3
  #define PIN_DIP3   -1
#endif
#ifndef PIN_SERVO
  #define PIN_SERVO  -1
#endif
#ifndef MS_PUERTO
  #define MS_PUERTO  Serial
#endif

#endif /* PLACAS_H */
