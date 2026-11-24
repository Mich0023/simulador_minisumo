/* =====================================================================
   motor_sim.js  -  simulador de combate minisumo (2D, vista de arriba)
   ---------------------------------------------------------------------
   Corre el MISMO programa del firmware (traducido linea por linea) con
   los MISMOS sensores que declaraste en configuracion.h. Si conectas el
   robot, la plataforma le pide su lista de sensores y el simulador la
   usa tal cual.

   Unidades: metros, kilos, segundos. Angulos en radianes salvo donde
   diga grados.
   ===================================================================== */
const SIM = (() => {

  /* ---------- constantes que coinciden con el firmware ---------- */
  const TIPO = { DIG_ALTO:0, DIG_BAJO:1, ANA_MAYOR:2, ANA_MENOR:3, SHARP_41:4,
                 SHARP_21:5, QTR_RC:6, QTR_ANA:7, HCSR04:8 };
  const NOMBRE_TIPO = ["digital-alto","digital-bajo","analogico-mayor","analogico-menor",
                       "sharp-41","sharp-21","qtr-rc","qtr-analogico","ultrasonico"];
  const UNIDAD = ["","","adc","adc","mm","mm","us","adc","mm"];
  const ROL = { OPONENTE:0, LINEA:1 };
  const SEC = { FRENTE:0, IZQ:1, DER:2, ATRAS:3 };
  const NOMBRE_SEC = ["Frente","Izquierda","Derecha","Atrás"];
  const DIST_LEJOS = 9999, QTR_TOPE = 3000;

  const G = 9.81, DT = 0.002, R = 0.385, BORDE = 0.025;
  const IZQ = -1, DER = 1;
  const ESTADOS = ["Esperando","Apertura","Línea blanca","Ataque","Giro al costado","Búsqueda",
                   "Prueba de motores","Destrabándose","Rompiendo empuje","Línea ignorada"];
  const APERTURAS = ["0 Automática","1 Ataque directo","2 Giro 90° izquierda","3 Giro 90° derecha",
                     "4 Giro 180° (derecha)","5 Flanqueo izquierda","6 Flanqueo derecha","7 Giro 180° (izquierda)"];

  const PARAM_DEF = {
    VEL_ATAQUE:255, VEL_AVANCE:204, VEL_GIRO:153, VEL_ESCAPE:255, VEL_BUSQUEDA:153,
    PCT_CORRECCION:55, ANG_CORRECCION_PLENA:20, ANG_ATAQUE:60, DIST_NOMINAL:150,
    T_GIRO_90:230, T_GIRO_180:460, T_GIRO_45:115, T_RETROCESO:250, T_ESCAPE:220,
    T_ARCO:450, PCT_ARCO_INTERIOR:45, T_DIRECTO:600, PCT_GIRO_CIEGO:60, BUSCAR_GIRANDO:0,
    // filtros de sensores
    ALCANCE_MAX_MM:500, CONFIRMAR:3, T_SENSOR_PEGADO:6000, IGNORAR_CONTRADICTORIOS:1,
    // guardias anti-trabas
    T_MAX_LADO:1500, T_MAX_EMPUJE:0, T_MAX_SIN_CAMBIO:10000,
    T_DESTRABE_ATRAS:300, T_DESTRABE_GIRO:260, T_ROMPE_GIRO:160, PCT_ROMPE:25,
    T_LINEA_PEGADA:1500, T_IGNORA_LINEA:700,
  };
  const FIS_DEF = { masa:500, rpm:375, llanta:27.5, fuerza:5.0, mu:0.9, giroEscala:1.0,
                    lado:98, track:80, base:47, alcance:70, cono:8, cuchilla:35 };
  const vmaxDe = f => (f.rpm / 60) * Math.PI * (f.llanta / 1000);

  /* ---------- sensores por defecto (se reemplazan por los del robot) ---------- */
  const SENSORES_DEF = [
    { id:"fI", nombre:"Frente izquierdo", tipo:TIPO.DIG_ALTO, rol:0, sector:0, umbral:0,   x:46,  y:16,  ang:0   },
    { id:"fD", nombre:"Frente derecho",   tipo:TIPO.DIG_ALTO, rol:0, sector:0, umbral:0,   x:46,  y:-16, ang:0   },
    { id:"lI", nombre:"Costado izquierdo",tipo:TIPO.DIG_ALTO, rol:0, sector:1, umbral:0,   x:0,   y:49,  ang:90  },
    { id:"lD", nombre:"Costado derecho",  tipo:TIPO.DIG_ALTO, rol:0, sector:2, umbral:0,   x:0,   y:-49, ang:-90 },
    { id:"qI", nombre:"Línea izquierda",  tipo:TIPO.QTR_RC,   rol:1, sector:1, umbral:600, x:40,  y:35,  ang:0   },
    { id:"qD", nombre:"Línea derecha",    tipo:TIPO.QTR_RC,   rol:1, sector:2, umbral:600, x:40,  y:-35, ang:0   },
  ];

  const daDistancia = t => t === TIPO.SHARP_41 || t === TIPO.SHARP_21 || t === TIPO.HCSR04;
  /* alcance de cada tipo, en metros ("alcance" de la fisica es el de los digitales) */
  function alcanceDe(s, fis) {
    switch (s.tipo) {
      case TIPO.SHARP_41: return 0.30;
      case TIPO.SHARP_21: return 0.80;
      case TIPO.HCSR04:   return 1.00;
      default:            return (fis.alcance || 70) / 100;
    }
  }
  /* misma regla de deteccion que evaluaSensor() del firmware */
  function evalua(s, v, topeMm) {
    switch (s.tipo) {
      case TIPO.DIG_ALTO:  return v === 1;
      case TIPO.DIG_BAJO:  return v === 0;
      case TIPO.ANA_MAYOR: return v >= s.umbral;
      case TIPO.SHARP_41: case TIPO.SHARP_21: case TIPO.HCSR04: {
        let lim = s.umbral;
        if (topeMm > 0 && lim > topeMm) lim = topeMm;
        return v <= lim;
      }
      default:             return v <= s.umbral;
    }
  }

  const clamp = (v,a,b) => Math.max(a, Math.min(b, v));
  const rnd = (a,b) => a + Math.random() * (b - a);

  /* ===================== CUERPOS Y FISICA ===================== */
  function cuerpo(f, nombre, sensores) {
    const L = f.lado / 1000, m = f.masa / 1000;
    return {
      nombre, x:0, y:0, th:0, vx:0, vy:0, w:0, m, I: m * 2 * L * L / 12, L,
      track: f.track/1000, base: f.base/1000, vmax: vmaxDe(f), fuerza: f.fuerza, mu: f.mu,
      giroEscala: f.giroEscala ?? 1, cuchilla: f.cuchilla/100, fis: f, sensores,
      uL:0, uR:0, trac:1, tracSig:1, rastro:[],
      valor: new Array(sensores.length).fill(0), detecta: new Array(sensores.length).fill(false),
      detectaCrudo: new Array(sensores.length).fill(false),
      cuenta: new Array(sensores.length).fill(0), desdeCrudo: new Array(sensores.length).fill(0),
      pegado: new Array(sensores.length).fill(false),
      verSector:[false,false,false,false], distSector:[DIST_LEJOS,DIST_LEJOS,DIST_LEJOS,DIST_LEJOS],
      lineaSector:[false,false,false,false], hayLinea:false, rumbo:0, veRival:false,
    };
  }
  const ejes = b => { const c = Math.cos(b.th), s = Math.sin(b.th); return { fx:c, fy:s, lx:-s, ly:c }; };
  function esquinas(b) {
    const { fx, fy, lx, ly } = ejes(b), h = b.L / 2;
    return [[1,1],[1,-1],[-1,-1],[-1,1]].map(([a,c]) => ({ x: b.x + fx*h*a + lx*h*c, y: b.y + fy*h*a + ly*h*c }));
  }
  const aMundo = (b, px, py) => { const { fx, fy, lx, ly } = ejes(b); return { x: b.x + fx*px + lx*py, y: b.y + fy*px + ly*py }; };

  function fisica(b, dt) {
    const { fx, fy, lx, ly } = ejes(b), ht = b.track / 2;
    const N = b.m * G / 2 * b.trac;
    const vf = b.vx*fx + b.vy*fy;
    const vL = vf - b.w*ht, vR = vf + b.w*ht;
    const fmax = b.mu * N;
    const FL = clamp(b.fuerza * (b.uL/255 - vL/b.vmax), -fmax, fmax);
    const FR = clamp(b.fuerza * (b.uR/255 - vR/b.vmax), -fmax, fmax);
    b.vx += (FL+FR)*fx/b.m*dt; b.vy += (FL+FR)*fy/b.m*dt;
    b.w += (FR-FL)*ht/b.I*dt;
    const vlat = b.vx*lx + b.vy*ly, dvmax = b.mu*G*b.trac*dt;
    const dv = -clamp(vlat, -dvmax, dvmax); b.vx += dv*lx; b.vy += dv*ly;
    const dwmax = b.giroEscala * b.mu*b.m*G*b.trac*(b.base/2)/b.I*dt;
    b.w -= clamp(b.w, -dwmax, dwmax);
    b.vx *= 1 - 0.4*dt; b.vy *= 1 - 0.4*dt;
    b.x += b.vx*dt; b.y += b.vy*dt; b.th += b.w*dt;
  }

  function choque(A, B) {
    const ca = esquinas(A), cb = esquinas(B), ea = ejes(A), eb = ejes(B);
    const axes = [[ea.fx,ea.fy],[ea.lx,ea.ly],[eb.fx,eb.fy],[eb.lx,eb.ly]];
    let dmin = 1e9, n = null;
    for (const [ax, ay] of axes) {
      const pa = ca.map(p => p.x*ax + p.y*ay), pb = cb.map(p => p.x*ax + p.y*ay);
      const o = Math.min(Math.max(...pa) - Math.min(...pb), Math.max(...pb) - Math.min(...pa));
      if (o <= 0) return null;
      if (o < dmin) { dmin = o; n = [ax, ay]; }
    }
    if ((B.x-A.x)*n[0] + (B.y-A.y)*n[1] < 0) n = [-n[0], -n[1]];
    const dentro = (b,p) => { const e = ejes(b), dx = p.x-b.x, dy = p.y-b.y, h = b.L/2 + 1e-6;
      return Math.abs(dx*e.fx + dy*e.fy) <= h && Math.abs(dx*e.lx + dy*e.ly) <= h; };
    let pts = cb.filter(p => dentro(A,p)).concat(ca.filter(p => dentro(B,p)));
    if (!pts.length) pts = [{ x:(A.x+B.x)/2, y:(A.y+B.y)/2 }];
    const p = { x: pts.reduce((s,q)=>s+q.x,0)/pts.length, y: pts.reduce((s,q)=>s+q.y,0)/pts.length };
    const tot = A.m + B.m;
    A.x -= n[0]*dmin*B.m/tot; A.y -= n[1]*dmin*B.m/tot;
    B.x += n[0]*dmin*A.m/tot; B.y += n[1]*dmin*A.m/tot;
    const rA = { x:p.x-A.x, y:p.y-A.y }, rB = { x:p.x-B.x, y:p.y-B.y };
    const vA = { x:A.vx - A.w*rA.y, y:A.vy + A.w*rA.x }, vB = { x:B.vx - B.w*rB.y, y:B.vy + B.w*rB.x };
    const vr = { x:vB.x-vA.x, y:vB.y-vA.y }, vn = vr.x*n[0] + vr.y*n[1];
    const cr = (r,d) => r.x*d[1] - r.y*d[0];
    const aplica = (j,d) => {
      A.vx -= j*d[0]/A.m; A.vy -= j*d[1]/A.m; A.w -= cr(rA,d)*j/A.I;
      B.vx += j*d[0]/B.m; B.vy += j*d[1]/B.m; B.w += cr(rB,d)*j/B.I;
    };
    const k = d => 1/A.m + 1/B.m + cr(rA,d)**2/A.I + cr(rB,d)**2/B.I;
    let jn = 0;
    if (vn < 0) { jn = -(1.05)*vn/k(n); aplica(jn, n); }
    const t = [-n[1], n[0]], vt = vr.x*t[0] + vr.y*t[1];
    aplica(clamp(-vt/k(t), -0.4*jn, 0.4*jn), t);
    const alFrente = b => { const e = ejes(b); return (p.x-b.x)*e.fx + (p.y-b.y)*e.fy > b.L/2 - 0.012; };
    const fa = alFrente(A), fb = alFrente(B);
    if (fa && !fb) { B.tracSig = 1 - A.cuchilla; A.tracSig = 1 + A.cuchilla*0.5; }
    if (fb && !fa) { A.tracSig = 1 - B.cuchilla; B.tracSig = 1 + B.cuchilla*0.5; }
    return p;
  }

  /* ===================== SENSORES SIMULADOS ===================== */
  function rayoCaja(ox, oy, dx, dy, b) {
    const e = ejes(b), rx = ox-b.x, ry = oy-b.y;
    const o = [rx*e.fx + ry*e.fy, rx*e.lx + ry*e.ly], d = [dx*e.fx + dy*e.fy, dx*e.lx + dy*e.ly];
    let t0 = -1e9, t1 = 1e9; const h = b.L/2;
    for (let i = 0; i < 2; i++) {
      if (Math.abs(d[i]) < 1e-12) { if (Math.abs(o[i]) > h) return null; continue; }
      let a = (-h-o[i])/d[i], c = (h-o[i])/d[i]; if (a > c) [a,c] = [c,a];
      t0 = Math.max(t0,a); t1 = Math.min(t1,c); if (t0 > t1) return null;
    }
    return t1 < 0 ? null : Math.max(t0, 0);
  }
  function rayoCirculo(ox, oy, dx, dy, cx, cy, r) {
    const fx = ox-cx, fy = oy-cy, b = fx*dx + fy*dy, c = fx*fx + fy*fy - r*r, h = b*b - c;
    if (h < 0) return null; const t = -b - Math.sqrt(h); return t >= 0 ? t : null;
  }
  const GENTE = Array.from({ length:12 }, (_, i) => ({ a: i*Math.PI/6 + 0.2, r: R + 0.35 }));

  /* lee todos los sensores del cuerpo b y arma los sectores, igual que sensoresLeer() */
  function leerSensores(mundo, b, otro) {
    for (let k = 0; k < 4; k++) { b.verSector[k] = false; b.lineaSector[k] = false; b.distSector[k] = DIST_LEJOS; }
    b.hayLinea = false;
    for (let i = 0; i < b.sensores.length; i++) {
      const s = b.sensores[i];
      let v;
      if (s.rol === ROL.LINEA) {
        const p = aMundo(b, s.x/1000, s.y/1000), d = Math.hypot(p.x, p.y);
        const blanco = d >= R - BORDE;
        if (s.tipo === TIPO.QTR_RC)      v = Math.round((blanco ? 150 : 2500) * rnd(0.9, 1.1));
        else if (s.tipo === TIPO.ANA_MAYOR) v = Math.round((blanco ? 900 : 150) * rnd(0.95, 1.05));
        else                              v = Math.round((blanco ? 150 : 800) * rnd(0.95, 1.05));
      } else {
        const p = aMundo(b, s.x/1000, s.y/1000), alc = alcanceDe(s, b.fis);
        let dist = null;
        const cono = b.fis.cono ?? 8;                        // medio angulo del haz, en grados
        for (const da of [-cono, -cono/2, 0, cono/2, cono]) {
          const a = b.th + (s.ang + da) * Math.PI/180, dx = Math.cos(a), dy = Math.sin(a);
          let t = rayoCaja(p.x, p.y, dx, dy, otro);
          if (mundo.gente) for (const g of GENTE) {
            const tg = rayoCirculo(p.x, p.y, dx, dy, Math.cos(g.a)*g.r, Math.sin(g.a)*g.r, 0.15);
            if (tg !== null && (t === null || tg < t)) t = tg;
          }
          if (t !== null && t <= alc && (dist === null || t < dist)) dist = t;
        }
        if (daDistancia(s.tipo))      v = dist === null ? DIST_LEJOS : Math.round(dist * 1000);
        else if (s.tipo === TIPO.DIG_ALTO)  v = dist === null ? 0 : 1;
        else if (s.tipo === TIPO.DIG_BAJO)  v = dist === null ? 1 : 0;
        else if (s.tipo === TIPO.ANA_MAYOR) v = dist === null ? 40 : Math.round(900 - dist*1000);
        else                                v = dist === null ? 900 : Math.round(dist*1000);
      }
      const d = evalua(s, v, mundo.cfg.P?.ALCANCE_MAX_MM || 0);
      b.valor[i] = v; b.detectaCrudo[i] = d; b.detecta[i] = d;
      if (s.rol === ROL.LINEA) { if (d) { b.hayLinea = true; b.lineaSector[s.sector] = true; } }
      else if (d) {
        b.verSector[s.sector] = true;
        if (daDistancia(s.tipo) && v < b.distSector[s.sector]) b.distSector[s.sector] = v;
      }
    }
    return b;
  }

  /* arma los sectores a partir de detecta[], ya filtrado (igual que el firmware) */
  function recalcSectores(b) {
    for (let k = 0; k < 4; k++) { b.verSector[k] = false; b.lineaSector[k] = false; b.distSector[k] = DIST_LEJOS; }
    b.hayLinea = false;
    for (let i = 0; i < b.sensores.length; i++) {
      if (!b.detecta[i]) continue;
      const s = b.sensores[i];
      if (s.rol === ROL.LINEA) { b.hayLinea = true; b.lineaSector[s.sector] = true; }
      else {
        b.verSector[s.sector] = true;
        if (daDistancia(s.tipo) && b.valor[i] < b.distSector[s.sector]) b.distSector[s.sector] = b.valor[i];
      }
    }
  }

  /* mismo calculo de rumbo que calcularRumbo() del firmware */
  function calcularRumbo(b, P) {
    let sx = 0, sy = 0, n = 0;
    for (let i = 0; i < b.sensores.length; i++) {
      const s = b.sensores[i];
      if (s.rol !== ROL.OPONENTE || !b.detecta[i]) continue;
      const d = daDistancia(s.tipo) ? b.valor[i] : P.DIST_NOMINAL;
      const a = s.ang * Math.PI/180;
      sx += s.x + d*Math.cos(a); sy += s.y + d*Math.sin(a);
      n++;
    }
    b.veRival = n > 0;
    if (n) b.rumbo = Math.round(Math.atan2(sy/n, sx/n) * 180/Math.PI);
  }

  /* ===================== EL PROGRAMA (copia del .ino) ===================== */
  function* programa(io, P, modo) {
    const b = io.b;
    let ultimo = DER, ultimoGiro = DER;
    let tEstado = io.t, tCambio = io.t, nDestrabes = 0, tUltDestrabe = -1e9;
    let tLineaDesde = 0, ignoraLineaHasta = 0;

    const pasaA = e => { if (io.estado !== e) { io.estado = e; tEstado = io.t; tCambio = io.t; } };

    /* ---- filtros, iguales a filtrarSensores() del firmware ---- */
    function filtrar() {
      const ahora = io.t;
      for (let i = 0; i < b.sensores.length; i++) {
        let c = b.detectaCrudo[i];
        if (P.T_SENSOR_PEGADO > 0 && b.sensores[i].rol === ROL.OPONENTE) {
          if (c) {
            if (!b.desdeCrudo[i]) b.desdeCrudo[i] = ahora;
            else if (!b.pegado[i] && ahora - b.desdeCrudo[i] > P.T_SENSOR_PEGADO) b.pegado[i] = true;
          } else { b.desdeCrudo[i] = 0; b.pegado[i] = false; }
          if (b.pegado[i]) c = false;
        }
        if (P.CONFIRMAR > 1) {
          if (c) { if (b.cuenta[i] < P.CONFIRMAR) b.cuenta[i]++; } else b.cuenta[i] = 0;
          c = b.cuenta[i] >= P.CONFIRMAR;
        }
        b.detecta[i] = c;
      }
      if (P.IGNORAR_CONTRADICTORIOS) {
        for (let i = 0; i < b.sensores.length; i++) {
          if (!b.detecta[i] || b.sensores[i].rol !== ROL.OPONENTE) continue;
          for (let j = i + 1; j < b.sensores.length; j++) {
            if (!b.detecta[j] || b.sensores[j].rol !== ROL.OPONENTE) continue;
            let dif = b.sensores[i].ang - b.sensores[j].ang;
            while (dif > 180) dif -= 360;
            while (dif < -180) dif += 360;
            if (Math.abs(dif) >= 120) { b.detecta[i] = false; b.detecta[j] = false; }
          }
        }
      }
      recalcSectores(b);
      if (b.hayLinea) {
        if (!tLineaDesde) tLineaDesde = ahora;
        else if (ahora - tLineaDesde > P.T_LINEA_PEGADA && ahora > ignoraLineaHasta) ignoraLineaHasta = ahora + P.T_IGNORA_LINEA;
      } else tLineaDesde = 0;
      if (ahora < ignoraLineaHasta) { b.hayLinea = false; for (let k = 0; k < 4; k++) b.lineaSector[k] = false; }
    }

    const leer = () => {
      io.sense(); filtrar(); calcularRumbo(b, P);
      if (b.rumbo > 10) ultimo = IZQ;
      if (b.rumbo < -10) ultimo = DER;
    };
    const seguir = () => io.permitido();

    function* moverPor(vL, vR, ms, mirarLinea, verDesde) {
      const t0 = io.t; io.motors(vL, vR);
      while (io.t - t0 < ms) {
        if (!seguir()) return 3;
        leer();
        if (mirarLinea && b.hayLinea) return 2;
        if (verDesde >= 0 && io.t - t0 >= verDesde && b.verSector[SEC.FRENTE]) return 1;
        yield;
      }
      return 0;
    }
    function* escapar() {
      pasaA(2);
      let h = ultimo;
      if (b.lineaSector[SEC.IZQ] && !b.lineaSector[SEC.DER]) h = DER;
      else if (b.lineaSector[SEC.DER] && !b.lineaSector[SEC.IZQ]) h = IZQ;
      const v = b.lineaSector[SEC.ATRAS] ? P.VEL_ESCAPE : -P.VEL_ESCAPE;
      if ((yield* moverPor(v, v, P.T_RETROCESO, false, -1)) === 3) return;
      yield* moverPor(h*P.VEL_ESCAPE, -h*P.VEL_ESCAPE, P.T_ESCAPE, false, P.T_ESCAPE/2);
    }
    function* destrabar() {
      pasaA(7);
      const ahora = io.t;
      if (ahora - tUltDestrabe < 3000 && nDestrabes < 4) nDestrabes++; else nDestrabes = 0;
      tUltDestrabe = ahora;
      io.destrabes = (io.destrabes || 0) + 1;
      const giro = P.T_DESTRABE_GIRO + P.T_DESTRABE_GIRO * nDestrabes / 2;
      if ((yield* moverPor(-P.VEL_ESCAPE, -P.VEL_ESCAPE, P.T_DESTRABE_ATRAS, true, -1)) === 3) return;
      const h = -ultimoGiro;
      ultimo = h;
      if ((yield* moverPor(h*P.VEL_ESCAPE, -h*P.VEL_ESCAPE, giro, true, giro/3)) === 0)
        yield* moverPor(P.VEL_ATAQUE, P.VEL_ATAQUE, 220, true, 0);
      tCambio = io.t;
    }
    /* Empuje trabado: NO se retrocede (seria regalar terreno). Se sigue
       empujando pero cambiando el angulo de un lado al otro, para meter la
       cuchilla debajo del rival. */
    function* romperEmpuje() {
      pasaA(8);
      io.rompes = (io.rompes || 0) + 1;
      const poco = Math.floor(P.VEL_ATAQUE * P.PCT_ROMPE / 100);
      yield* moverPor(P.VEL_ATAQUE, poco, P.T_ROMPE_GIRO, true, -1);
      yield* moverPor(poco, P.VEL_ATAQUE, P.T_ROMPE_GIRO, true, -1);
      tCambio = io.t;
    }
    function* giroApertura(h, t) { ultimo = h; yield* moverPor(h*P.VEL_GIRO, -h*P.VEL_GIRO, t, true, Math.floor(t*P.PCT_GIRO_CIEGO/100)); }
    function* flanqueo(h) {
      ultimo = -h;
      if ((yield* moverPor(h*P.VEL_GIRO, -h*P.VEL_GIRO, P.T_GIRO_45, true, -1)) !== 0) return;
      const inter = Math.floor(P.VEL_ATAQUE*P.PCT_ARCO_INTERIOR/100);
      if (h === IZQ) yield* moverPor(P.VEL_ATAQUE, inter, P.T_ARCO, true, 0);
      else           yield* moverPor(inter, P.VEL_ATAQUE, P.T_ARCO, true, 0);
    }

    pasaA(1);
    switch (modo) {
      case 1: yield* moverPor(P.VEL_ATAQUE, P.VEL_ATAQUE, P.T_DIRECTO, true, -1); break;
      case 2: yield* giroApertura(IZQ, P.T_GIRO_90); break;
      case 3: yield* giroApertura(DER, P.T_GIRO_90); break;
      case 4: yield* giroApertura(DER, P.T_GIRO_180); break;
      case 5: yield* flanqueo(IZQ); break;
      case 6: yield* flanqueo(DER); break;
      case 7: yield* giroApertura(IZQ, P.T_GIRO_180); break;
      default:
        leer();
        if (b.verSector[SEC.FRENTE]) break;
        if (b.verSector[SEC.IZQ] && !b.verSector[SEC.DER]) yield* giroApertura(IZQ, P.T_GIRO_90);
        else if (b.verSector[SEC.DER] && !b.verSector[SEC.IZQ]) yield* giroApertura(DER, P.T_GIRO_90);
        else yield* giroApertura(DER, P.T_GIRO_180);
    }

    tEstado = tCambio = io.t;
    while (seguir()) {
      leer();
      const ahora = io.t;
      if (P.T_MAX_SIN_CAMBIO > 0 && ahora - tCambio > P.T_MAX_SIN_CAMBIO) { yield* destrabar(); continue; }
      if (b.hayLinea) { pasaA(2); yield* escapar(); tCambio = io.t; continue; }
      if (b.veRival) {
        const a = Math.abs(b.rumbo);
        if (a <= P.ANG_ATAQUE) {
          if (P.T_MAX_EMPUJE > 0 && io.estado === 3 && ahora - tEstado > P.T_MAX_EMPUJE) { yield* romperEmpuje(); continue; }
          pasaA(3);
          const k = Math.min(a, P.ANG_CORRECCION_PLENA);
          const dentro = Math.floor(P.VEL_ATAQUE * (100 - (100-P.PCT_CORRECCION)*k/P.ANG_CORRECCION_PLENA) / 100);
          if (b.rumbo > 0) io.motors(dentro, P.VEL_ATAQUE);
          else             io.motors(P.VEL_ATAQUE, dentro);
        } else {
          if (P.T_MAX_LADO > 0 && io.estado === 4 && ahora - tEstado > P.T_MAX_LADO) { yield* destrabar(); continue; }
          pasaA(4);
          ultimoGiro = b.rumbo > 0 ? IZQ : DER;
          io.motors(ultimoGiro*P.VEL_GIRO, -ultimoGiro*P.VEL_GIRO);
        }
        yield; continue;
      }
      pasaA(5);
      if (P.BUSCAR_GIRANDO) { ultimoGiro = ultimo; io.motors(ultimo*P.VEL_BUSQUEDA, -ultimo*P.VEL_BUSQUEDA); }
      else                  io.motors(P.VEL_AVANCE, P.VEL_AVANCE);
      yield;
    }
    io.motors(0, 0);
  }

  /* ---------- rivales de practica ---------- */
  function* rivalQuieto(io) { io.estado = 0; while (true) { io.motors(0,0); yield; } }
  function* rivalEmbestida(io) {
    const b = io.b;
    while (true) {
      io.sense();
      if (b.hayLinea) {
        io.estado = 2; let t0 = io.t; io.motors(-255,-255); while (io.t-t0 < 200) yield;
        t0 = io.t; io.motors(255,-255); while (io.t-t0 < 260) yield;
      } else { io.estado = b.verSector[0] ? 3 : 5; io.motors(255,255); yield; }
    }
  }
  function* rivalGirando(io, P) {
    const b = io.b;
    while (true) {
      io.sense(); calcularRumbo(b, P);
      if (b.hayLinea) { io.estado = 2; const t0 = io.t; io.motors(-255,-255); while (io.t-t0 < 220) yield; }
      else if (b.veRival && Math.abs(b.rumbo) <= 60) { io.estado = 3; io.motors(255,255); yield; }
      else if (b.veRival) { io.estado = 4; const h = b.rumbo > 0 ? -1 : 1; io.motors(h*200,-h*200); yield; }
      else { io.estado = 5; io.motors(170,-170); yield; }
    }
  }
  const RIVALES = { quieto:"Quieto (bloque)", embestida:"Embestida recta", girando:"Busca girando", mismo:"Mismo programa que el tuyo" };

  const POS = {
    frente:   { nombre:"De frente", th:0 },
    izq:      { nombre:"De lado (rival a tu izquierda)", th:-90 },
    der:      { nombre:"De lado (rival a tu derecha)", th:90 },
    espaldas: { nombre:"De espaldas", th:180 },
    azar:     { nombre:"Al azar", th:null },
  };

  /* ===================== MUNDO ===================== */
  function crearMundo(cfg) {
    const sens = cfg.sensores || SENSORES_DEF;
    const m = {
      cfg, t:0, gente: !!cfg.gente, fin:null, sensores: sens,
      A: cuerpo(cfg.fis, "tú", sens), B: cuerpo(cfg.fisRival || cfg.fis, "rival", sens),
    };
    const sep = 0.107, jit = cfg.jitter ? 1 : 0;
    const pos = POS[cfg.pos] || POS.frente;
    m.A.x = -sep + jit*rnd(-0.005,0.005); m.A.y = jit*rnd(-0.02,0.02);
    m.A.th = ((pos.th === null ? rnd(-180,180) : pos.th) + jit*rnd(-6,6)) * Math.PI/180;
    m.B.x = sep + jit*rnd(-0.005,0.005); m.B.y = jit*rnd(-0.02,0.02);
    m.B.th = (180 + jit*rnd(-8,8)) * Math.PI/180;

    const mkIO = (b, otro, retraso) => {
      const io = { t:-retraso, estado:0, b,
        motors: (l,r) => { b.uL = clamp(Math.round(l),-255,255); b.uR = clamp(Math.round(r),-255,255); },
        sense: () => leerSensores(m, b, otro),
        permitido: () => !m.fin };
      return io;
    };
    m.ioA = mkIO(m.A, m.B, jit ? Math.round(rnd(0,12)) : 0);
    m.ioB = mkIO(m.B, m.A, jit ? Math.round(rnd(0,12)) : 0);
    m.gA = programa(m.ioA, cfg.P, cfg.modo);
    const t = cfg.rival;
    m.gB = t === "quieto" ? rivalQuieto(m.ioB) : t === "embestida" ? rivalEmbestida(m.ioB)
         : t === "girando" ? rivalGirando(m.ioB, cfg.P) : programa(m.ioB, cfg.P, cfg.modoRival ?? 0);
    return m;
  }

  function paso(m) {
    if (m.fin) return;
    const ms = DT * 1000;
    for (const [io, g] of [[m.ioA, m.gA], [m.ioB, m.gB]]) {
      if (io.t >= 0) { leerSensores(m, io.b, io.b === m.A ? m.B : m.A); g.next(); }
      io.t += ms;
    }
    for (const b of [m.A, m.B]) { b.trac = b.tracSig; b.tracSig = 1; fisica(b, DT); }
    const p = choque(m.A, m.B);
    if (p) { m.puntoChoque = p; } else m.puntoChoque = null;
    m.t += DT;
    if (Math.round(m.t*1000) % 30 === 0) for (const b of [m.A, m.B]) { b.rastro.push([b.x,b.y]); if (b.rastro.length > 200) b.rastro.shift(); }
    const outA = Math.hypot(m.A.x,m.A.y) > R, outB = Math.hypot(m.B.x,m.B.y) > R;
    if (outA || outB) m.fin = outA && outB ? "empate" : outA ? "pierdes" : "ganas";
    else if (m.t >= (m.cfg.limite || 30)) m.fin = "tiempo";
  }

  function correrRound(cfg) { const m = crearMundo(cfg); while (!m.fin) paso(m); return { fin:m.fin, t:m.t }; }

  return { G, DT, R, BORDE, TIPO, NOMBRE_TIPO, UNIDAD, ROL, SEC, NOMBRE_SEC, DIST_LEJOS, QTR_TOPE,
           ESTADOS, APERTURAS, PARAM_DEF, FIS_DEF, SENSORES_DEF, RIVALES, POS,
           daDistancia, evalua, alcanceDe, recalcSectores, crearMundo, paso, correrRound, esquinas, aMundo, ejes, leerSensores };
})();
if (typeof module !== "undefined") module.exports = SIM;
