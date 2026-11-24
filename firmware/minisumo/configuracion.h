/* =====================================================================
   configuracion.h   <<<<<  ESTE ES EL UNICO ARCHIVO QUE DEBES EDITAR
   ---------------------------------------------------------------------
   Aqui dices que placa usas, que sensores tienes, en que pines estan y
   como quieres que pelee el robot. Todo lo demas se acomoda solo.

   Esta configuracion de ejemplo es la de un minisumo con XMotion V3,
   4 sensores JS200XF y 2 sensores de linea QTR-1RC.
   ===================================================================== */
#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include "constantes.h"

/* =====================================================================
   1. TU PLACA
   ---------------------------------------------------------------------
   Opciones:
     PLACA_XMOTION_V3     JSumo XMotion V3   (en el IDE elige "Arduino Leonardo")
     PLACA_AVR_TB6612     Arduino Uno / Nano / Pro Mini con driver TB6612FNG
     PLACA_AVR_L298       Arduino Uno / Nano con driver L298N
     PLACA_ESP32_TB6612   ESP32 DevKit con TB6612FNG
     PLACA_STM32_TB6612   STM32 Blue Pill con TB6612FNG
     PLACA_PERSONAL       otra cosa (abajo te explico que definir)
   ===================================================================== */
#define PLACA  PLACA_XMOTION_V3

/* --- Si tu placa es una de la lista, los pines de los motores ya vienen
       puestos. Para cambiar alguno, quitale el // y pon el tuyo:        */
// #define PIN_PWM_IZQ   10
// #define PIN_DIR_IZQ   12
// #define PIN_PWM_DER   11
// #define PIN_DIR_DER   13

/* --- Si usas PLACA_PERSONAL tienes que definir, como minimo:
       #define MS_DRIVER  DRV_PWM_2DIR     (o DRV_PWM_DIR, o DRV_2PWM)
       y los pines que pida ese driver:
         DRV_PWM_DIR   -> PIN_PWM_IZQ, PIN_DIR_IZQ, PIN_PWM_DER, PIN_DIR_DER
         DRV_PWM_2DIR  -> PIN_PWM_IZQ, PIN_IN1_IZQ, PIN_IN2_IZQ,
                          PIN_PWM_DER, PIN_IN1_DER, PIN_IN2_DER  (+ PIN_STBY si lo tiene)
         DRV_2PWM      -> PIN_PWM_IZQ, PIN_PWM2_IZQ, PIN_PWM_DER, PIN_PWM2_DER   */

/* --- Pines opcionales. Pon -1 en los que no tengas. --- */
// #define PIN_START   A0    // modulo arrancador: ALTO = pelea. -1 = no tengo (arranca solo)
// #define PIN_KILL     1    // segunda señal del arrancador. -1 = no la uso
// #define PIN_LED1     8
// #define PIN_LED2     9
// #define PIN_DIP1     5    // -1 = no tengo dipswitch (se usa APERTURA_FIJA)
// #define PIN_DIP2     6
// #define PIN_DIP3     7
#define PIN_SERVO   -1       // para soltar alas o cuchillas. -1 = no uso servo

/* Si no tienes arrancador, cuanto espera al encender antes de pelear */
#define ESPERA_SIN_ARRANCADOR  5000

/* =====================================================================
   2. MOTORES
   ---------------------------------------------------------------------
   Prueba los motores desde la plataforma. Si uno gira al reves, pon su
   linea en true. Si al pedir "adelante" el robot gira, es que los
   motores estan cruzados: pon INTERCAMBIAR_MOTORES en true.
   ===================================================================== */
#define INVERTIR_IZQ           false
#define INVERTIR_DER           false
#define INTERCAMBIAR_MOTORES   false

/* =====================================================================
   3. TUS SENSORES
   ---------------------------------------------------------------------
   Pon una linea por sensor. Puedes tener los que quieras, de los tipos
   que quieras y mezclados.

   Cada linea es:
     { "id", "Nombre largo", TIPO, ROL, SECTOR, pin, pin2, umbral, x, y, angulo }

   TIPO:
     SENS_DIG_ALTO    digital, detecta en ALTO    (JS200XF, JS40F, JS60F)
     SENS_DIG_BAJO    digital, detecta en BAJO    (E18-D80NK y parecidos)
     SENS_ANA_MAYOR   analogico, detecta si la lectura SUBE del umbral
     SENS_ANA_MENOR   analogico, detecta si la lectura BAJA del umbral
     SENS_SHARP_41    Sharp GP2Y0A41SK0F  (4 a 30 cm)   -> te da milimetros
     SENS_SHARP_21    Sharp GP2Y0A21YK0F  (10 a 80 cm)  -> te da milimetros
     SENS_QTR_RC      QTR-1RC / QTR-8RC   (mide microsegundos)
     SENS_QTR_ANA     QTR-1A, CNY70, TCRT5000 analogico
     SENS_HCSR04      ultrasonico HC-SR04 -> te da milimetros

   ROL:     ROL_OPONENTE (ver al rival)  o  ROL_LINEA (ver el borde blanco)
   SECTOR:  SEC_FRENTE, SEC_IZQ, SEC_DER, SEC_ATRAS

   pin2:    solo para el HC-SR04, es el pin de disparo (trigger). Si no, -1.

   umbral:  en la unidad del sensor.
              digitales      -> se ignora, pon 0
              Sharp y HC-SR04-> milimetros: detecta si el rival esta MAS CERCA
              QTR-1RC        -> microsegundos: es blanco si mide MENOS
              analogicos     -> cuentas del convertidor
            Todos los puedes ajustar en caliente desde la plataforma.

   x, y:    donde esta el sensor en el robot, en milimetros.
            x hacia adelante, y hacia la izquierda, (0,0) = centro del robot.
   angulo:  hacia donde apunta. 0 = al frente, 90 = izquierda, -90 = derecha,
            180 = atras.
            Esto no es decoracion: con los angulos el robot calcula hacia
            donde esta el rival y corrige el rumbo mientras lo embiste.
   ===================================================================== */
Sensor SENSORES[] = {
  //  id      nombre largo          tipo           rol           sector      pin  pin2 umbral   x    y   ang
  { "fI", "Frente izquierdo",  SENS_DIG_ALTO, ROL_OPONENTE, SEC_FRENTE,  A2,  -1,     0,  46,  16,    0 },
  { "fD", "Frente derecho",    SENS_DIG_ALTO, ROL_OPONENTE, SEC_FRENTE,  A5,  -1,     0,  46, -16,    0 },
  { "lI", "Costado izquierdo", SENS_DIG_ALTO, ROL_OPONENTE, SEC_IZQ,     A1,  -1,     0,   0,  49,   90 },
  { "lD", "Costado derecho",   SENS_DIG_ALTO, ROL_OPONENTE, SEC_DER,      4,  -1,     0,   0, -49,  -90 },
  { "qI", "Linea izquierda",   SENS_QTR_RC,   ROL_LINEA,    SEC_IZQ,     A4,  -1,   600,  40,  35,    0 },
  { "qD", "Linea derecha",     SENS_QTR_RC,   ROL_LINEA,    SEC_DER,      2,  -1,   600,  40, -35,    0 },
};
const uint8_t N_SENSORES = sizeof(SENSORES) / sizeof(SENSORES[0]);

/* =====================================================================
   4. COMO PELEA
   ===================================================================== */

/* --- velocidades (0 a 255) --- */
#define VEL_ATAQUE      255    // cuando ve al rival al frente
#define VEL_AVANCE      204    // buscando hacia adelante (80 %)
#define VEL_GIRO        153    // giros de apertura y de busqueda (60 %)
#define VEL_ESCAPE      255    // al pisar la linea blanca
#define PCT_CORRECCION   55    // que tanto frena la llanta de adentro al corregir
                               // el rumbo. 100 = no corrige, 0 = gira sobre su eje
#define ANG_CORRECCION_PLENA 20 // grados de error con los que ya corrige al maximo
#define ANG_ATAQUE       60    // mas de este angulo, en vez de embestir gira en su lugar
#define DIST_NOMINAL    150    // mm que supone al rival cuando el sensor no mide distancia

/* --- tiempos en milisegundos: CALIBRALOS en tu dohyo --- */
#define T_GIRO_90       230
#define T_GIRO_180      460
#define T_GIRO_45       115
#define T_RETROCESO     250    // cuanto retrocede al ver la linea
#define T_ESCAPE        220    // cuanto gira despues de retroceder
#define T_ARCO          450    // la curva del flanqueo
#define PCT_ARCO_INTERIOR 45
#define T_DIRECTO       600    // ataque directo de salida
#define PCT_GIRO_CIEGO   60    // % del giro de apertura que no hace caso al frente

/* --- busqueda --- */
#define BUSCAR_GIRANDO  false  // false = busca avanzando, true = girando en su lugar
#define VEL_BUSQUEDA    153    // solo si BUSCAR_GIRANDO es true

/* --- apertura cuando NO tienes dipswitch --- */
#define APERTURA_FIJA   0      // 0 automatica, 1 ataque directo, 2 giro 90 izq,
                               // 3 giro 90 der, 4 giro 180 der, 5 flanqueo izq,
                               // 6 flanqueo der, 7 giro 180 izq

/* --- servo (solo si PIN_SERVO no es -1) --- */
#define SERVO_US_CERRADO 1000  // pulso con las alas trabadas
#define SERVO_US_ABIERTO 2000  // pulso que las suelta
#define T_SERVO          500   // cuanto tiempo manda el pulso de abrir

/* =====================================================================
   5. TELEMETRIA (los datos que ve la plataforma)
   ===================================================================== */
#define BAUDIOS         115200
#define MS_ENTRE_DATOS  25     // cada cuanto manda datos mientras pelea

#endif /* CONFIGURACION_H */
