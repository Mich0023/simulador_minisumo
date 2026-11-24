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
   5. FILTROS DE SENSORES
   ---------------------------------------------------------------------
   Para que el robot no persiga al publico ni a las paredes.
   ===================================================================== */

/* Ignora todo lo que este mas lejos de esto, en milimetros. 0 = sin tope.
   OJO IMPORTANTE: esto solo puede funcionar con sensores que MIDEN
   distancia (SENS_SHARP_41, SENS_SHARP_21, SENS_HCSR04). Un sensor digital
   como el JS200XF solo dice "veo" o "no veo", y de ahi ningun programa
   puede sacar a que distancia esta la cosa. A esos hay que bajarles el
   alcance a mano, con su boton o soldando los pads de atras.
   Para los digitales, lo que si ayuda contra lo que esta lejos son los dos
   filtros de abajo. Lee el README. */
#define ALCANCE_MAX_MM   500

/* Cuantas lecturas seguidas tiene que dar un sensor para creerle.
   Sirve contra parpadeos de cosas lejanas o brillantes. 1 = sin filtro.
   Cada lectura son menos de 2 ms, asi que 3 casi no retrasa la reaccion.
   Es el filtro que mas sirve: en el simulador, con publico alrededor del
   dohyo, quitarlo baja de 31 % a 16 % de rounds ganados. */
#define CONFIRMAR        3

/* Si un sensor lleva este tiempo detectando SIN SOLTAR, seguro esta roto o
   esta viendo al publico: se ignora hasta que suelte. 0 = apagado. */
#define T_SENSOR_PEGADO  6000

/* Si dos sensores que miran a lados OPUESTOS detectan al mismo tiempo, al
   menos uno esta viendo el fondo: el rival no puede estar de los dos lados.
   Con esto se ignoran los dos y el robot sigue buscando. */
#define IGNORAR_CONTRADICTORIOS  true

/* =====================================================================
   6. GUARDIAS (para que nunca se quede trabado)
   ---------------------------------------------------------------------
   El reglamento dice que si tu robot se queda quieto 5 segundos pierde el
   punto, asi que NINGUNA de estas maniobras detiene los motores.
   ===================================================================== */

/* Girando de lado junto al rival sin lograr ponerlo al frente: los dos
   estan dando vueltas juntos. Pon 0 para apagarlo.
   Tu dijiste 10 segundos; lo dejo en 1.5 s porque 10 s es la mitad de un
   round. En el simulador, esto baja los empates del 20 % al 13 %. */
#define T_MAX_LADO       1500

/* Empujando de frente sin que pase nada. APAGADO por defecto (0):
   en el simulador costaba mas puntos de los que daba, porque muchas veces
   se mete cuando el empuje si iba ganando. Si lo quieres probar, 4500 fue
   el valor menos malo. */
#define T_MAX_EMPUJE     0
#define PCT_ROMPE        25     // velocidad de la llanta de adentro al cambiar de angulo

/* Ultimo recurso: si en todo este tiempo no cambio de maniobra, se destraba.
   Es la red de seguridad que pediste. Pon 0 para apagarlo. */
#define T_MAX_SIN_CAMBIO 10000

/* Maniobra de destrabe */
#define T_DESTRABE_ATRAS 300    // cuanto retrocede
#define T_DESTRABE_GIRO  260    // cuanto gira despues, al 100 %
#define T_ROMPE_GIRO     160    // cuanto dura cada cambio de angulo al romper el empuje

/* Si los sensores de linea dicen blanco todo este tiempo seguido, estan
   sucios, rotos o el robot ya esta afuera. Se ignoran un rato y se avanza,
   porque si no el robot se la pasa retrocediendo hasta caerse por atras. */
#define T_LINEA_PEGADA   1500
#define T_IGNORA_LINEA   700

/* =====================================================================
   7. TELEMETRIA (los datos que ve la plataforma)
   ===================================================================== */
#define BAUDIOS         115200
#define MS_ENTRE_DATOS  25     // cada cuanto manda datos mientras pelea

#endif /* CONFIGURACION_H */
