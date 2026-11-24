/* =====================================================================
   telemetria.h  -  manda los datos a la plataforma web por USB
   ---------------------------------------------------------------------
   Nunca se queda esperando al puerto: si no hay nadie conectado o el
   buffer de salida esta lleno, se brinca el envio y sigue peleando.
   Por eso este firmware tambien sirve para competir tal cual.

   PROTOCOLO (texto, una linea por mensaje)
     robot -> PC
       I,nombre,version,placa,arquitectura,nSensores,adcMax
       S,idx,id,nombre,tipo,unidad,rol,sector,umbral,x,y,ang     (uno por sensor)
       P,clave=valor,...                                         (constantes)
       D,ms,estado,modo,start,kill,dip,vIzq,vDer,v0,v1,...       (datos)
       E,texto                                                   (eventos)
     PC -> robot
       ?              manda I + S + P
       M,vL,vR,ms     prueba de motores (solo con el arrancador apagado)
       X              frena
       U,idx,umbral   cambia el umbral de un sensor en caliente

   NO necesitas editar este archivo.
   ===================================================================== */
#ifndef TELEMETRIA_H
#define TELEMETRIA_H

#include "placas.h"
#include "sensores.h"

#define VERSION_FW "2.0"

extern int16_t velIzq, velDer;
extern uint8_t estado, modo;
extern bool peleando;
extern bool arrancadorOk();
extern bool killOk();
extern uint8_t leerDip();

inline void telemetriaBombea();
inline void evento(const char* t) {
  if (!MS_PUERTO) return;
  MS_PUERTO.print(F("E,")); MS_PUERTO.println(t);   // los eventos son cortos y poco frecuentes
}

inline void mandaInfo() {
  if (!MS_PUERTO) return;
  MS_PUERTO.print(F("I,minisumo,")); MS_PUERTO.print(F(VERSION_FW));
  MS_PUERTO.print(','); MS_PUERTO.print(F(MS_PLACA_NOMBRE));
  MS_PUERTO.print(','); MS_PUERTO.print(F(MS_NOMBRE_ARQ));
  MS_PUERTO.print(','); MS_PUERTO.print(N_SENSORES);
  MS_PUERTO.print(','); MS_PUERTO.println(MS_ADC_MAX);

  for (uint8_t i = 0; i < N_SENSORES; i++) {
    const Sensor& s = SENSORES[i];
    MS_PUERTO.print(F("S,"));  MS_PUERTO.print(i);
    MS_PUERTO.print(',');      MS_PUERTO.print(s.id);
    MS_PUERTO.print(',');      MS_PUERTO.print(s.nombre);
    MS_PUERTO.print(',');      MS_PUERTO.print(nombreTipo(s.tipo));
    MS_PUERTO.print(',');      MS_PUERTO.print(unidadDe(s.tipo));
    MS_PUERTO.print(',');      MS_PUERTO.print(s.rol);
    MS_PUERTO.print(',');      MS_PUERTO.print(s.sector);
    MS_PUERTO.print(',');      MS_PUERTO.print(s.umbral);
    MS_PUERTO.print(',');      MS_PUERTO.print(s.x);
    MS_PUERTO.print(',');      MS_PUERTO.print(s.y);
    MS_PUERTO.print(',');      MS_PUERTO.println(s.ang);
  }

  MS_PUERTO.print(F("P,VEL_ATAQUE="));      MS_PUERTO.print(VEL_ATAQUE);
  MS_PUERTO.print(F(",VEL_AVANCE="));       MS_PUERTO.print(VEL_AVANCE);
  MS_PUERTO.print(F(",VEL_GIRO="));         MS_PUERTO.print(VEL_GIRO);
  MS_PUERTO.print(F(",VEL_ESCAPE="));       MS_PUERTO.print(VEL_ESCAPE);
  MS_PUERTO.print(F(",VEL_BUSQUEDA="));     MS_PUERTO.print(VEL_BUSQUEDA);
  MS_PUERTO.print(F(",PCT_CORRECCION="));   MS_PUERTO.print(PCT_CORRECCION);
  MS_PUERTO.print(F(",T_GIRO_90="));        MS_PUERTO.print(T_GIRO_90);
  MS_PUERTO.print(F(",T_GIRO_180="));       MS_PUERTO.print(T_GIRO_180);
  MS_PUERTO.print(F(",T_GIRO_45="));        MS_PUERTO.print(T_GIRO_45);
  MS_PUERTO.print(F(",T_RETROCESO="));      MS_PUERTO.print(T_RETROCESO);
  MS_PUERTO.print(F(",T_ESCAPE="));         MS_PUERTO.print(T_ESCAPE);
  MS_PUERTO.print(F(",T_ARCO="));           MS_PUERTO.print(T_ARCO);
  MS_PUERTO.print(F(",PCT_ARCO_INTERIOR=")); MS_PUERTO.print(PCT_ARCO_INTERIOR);
  MS_PUERTO.print(F(",T_DIRECTO="));        MS_PUERTO.print(T_DIRECTO);
  MS_PUERTO.print(F(",PCT_GIRO_CIEGO="));   MS_PUERTO.print(PCT_GIRO_CIEGO);
  MS_PUERTO.print(F(",BUSCAR_GIRANDO="));   MS_PUERTO.println(BUSCAR_GIRANDO ? 1 : 0);
}

/* ---------------------------------------------------------------------
   Transmisor que NO bloquea
   ---------------------------------------------------------------------
   El buffer de salida del puerto es chico (64 bytes en casi todas las
   placas) y una linea de datos con muchos sensores no cabe completa.
   Por eso la linea se arma en memoria y se va soltando a pedazos, tantos
   bytes como quepan en cada vuelta. Asi nunca se detiene el combate,
   aunque tengas 16 sensores.
   --------------------------------------------------------------------- */
#ifndef TX_MAX
  #define TX_MAX 240
#endif
static char     txBuf[TX_MAX];
static uint16_t txLen = 0, txPos = 0;

inline void txChar(char c) { if (txLen < TX_MAX) txBuf[txLen++] = c; }
inline void txStr(const char* s) { while (*s) txChar(*s++); }
inline void txInt(long v) {
  if (v < 0) { txChar('-'); v = -v; }
  char d[12]; uint8_t n = 0;
  do { d[n++] = (char)('0' + (v % 10)); v /= 10; } while (v);
  while (n) txChar(d[--n]);
}

/* suelta lo que quepa; llamalo seguido */
inline void telemetriaBombea() {
  if (txPos >= txLen) return;
  if (!MS_PUERTO) { txPos = txLen = 0; return; }
  int libre = MS_PUERTO.availableForWrite();
  while (libre > 0 && txPos < txLen) { MS_PUERTO.write(txBuf[txPos++]); libre--; }
  if (txPos >= txLen) { txPos = txLen = 0; }
}

inline void telemetria(bool forzar = false) {
  telemetriaBombea();
  if (txLen) return;                      // todavia hay linea pendiente
  static uint32_t tUlt = 0;
  uint16_t periodo = peleando ? MS_ENTRE_DATOS : (MS_ENTRE_DATOS * 2);
  if (!forzar && millis() - tUlt < periodo) return;
  if (!MS_PUERTO) return;
  tUlt = millis();
  txStr("D,");  txInt(millis());
  txChar(','); txInt(estado);
  txChar(','); txInt(modo);
  txChar(','); txInt(arrancadorOk() ? 1 : 0);
  txChar(','); txInt(killOk() ? 1 : 0);
  txChar(','); txInt(leerDip());
  txChar(','); txInt(velIzq);
  txChar(','); txInt(velDer);
  for (uint8_t i = 0; i < N_SENSORES; i++) { txChar(','); txInt(valor[i]); }
  txChar('\n');
  telemetriaBombea();
}

#endif /* TELEMETRIA_H */
