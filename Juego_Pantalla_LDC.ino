#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const int boton = 9;
const int buzzer = 10;

// ================= BOTÓN =================
bool botonEstado = HIGH;
bool botonEstadoAnt = HIGH;
unsigned long botonCambio = 0;
const unsigned long debounceTime = 80;

// ================= HUMANO =================
const int humanoCol = 7;
int humanoFila = 0;
int humanoFilaAnt = 0;
unsigned long humanoUlt = 0;
const unsigned long humanoIntervalo = 120;

// ================= VIGILANTE =================
int vFila = 1, vCol = 0, vDir = 1;
int vFilaAnt = 1, vColAnt = 0;
unsigned long vUlt = 0;
unsigned long vIntervalo = 400;
const unsigned long intervaloMin = 60;
const unsigned long vIntervaloMaxVel = 80;

// ======== IA DEL VIGILANTE ========
bool vigilanteMirando = false;
int visionPos = 0;
int visionDir = 1;
unsigned long ultimoAvanceVision = 0;
const unsigned long intervaloVision = 150;
unsigned long ultimoCambioEstado = 0;
unsigned long intervaloAccion = 1000;

// ================= PARCHE BUG CAMBIO FILA =================
// Evita vibraciones, bucles de visión y subida/bajada infinita
bool enCambioFila = false;
unsigned long inicioCambioFila = 0;
const unsigned long duracionCambioFila = 180;

// ================= BLOQUEO DE CAMBIO DE FILA =================
bool bloqueoCambioFila = false;

// ================= JUEGO =================
bool gameOver = false;
int pantallaFin = 0;
int pantallaAnterior = -1;

// ================= JUGADORES =================
int jugadorActual = 0;
unsigned long puntos = 0;
unsigned long ultimaPuntuacion[2] = {0,0};
unsigned long record[2] = {0,0};

// ================= DIFICULTAD =================
int multiplicador = 1;
int cambiosFila = 0;

// ================= MODOS ESPECIALES =================
bool modoPanico = false;
bool modoCaos = false;
unsigned long inicioPanico = 0;
unsigned long inicioCaos = 0;
const unsigned long duracionPanico = 60000;
const unsigned long duracionCaos = 60000;

// ================= SONIDO =================
unsigned long pulsoUlt = 0;
const unsigned long pulsoIntervalo = 700;

// ================= ICONOS =================
byte humanoChar[8] = {
  B01110,
  B01010,
  B01110,
  B00100,
  B01110,
  B01010,
  B00000,
  B00000
};

byte vigilanteChar[8] = {
  B01110,
  B11111,
  B01110,
  B00100,
  B01111,
  B00101,
  B00000,
  B00000
};

// ================= MELODÍAS =================
int gameOverMel[] = {392,330,294,262};
int gameOverDur[] = {300,300,300,600};

int scoreMel[] = {262,330,392,330};
int scoreDur[] = {250,250,250,400};

int transMel[] = {523,659};
int transDur[] = {150,250};

int panicoMel[] = {784,880,988,1047};
int panicoDur[] = {150,150,150,200};

int caosMel[] = {1047,988,880,784,659};
int caosDur[] = {100,100,100,100,150};

int melIdx = 0;
unsigned long melUlt = 0;

// ================= SELECCIÓN JUGADOR =================
bool seleccionJugador = false;
unsigned long inicioSeleccion = 0;
int cuentaAtras = 5;

// ================= TIEMPO =================
unsigned long inicioJuego = 0;
unsigned long ultimoSegundo = 0;

// ================= FUNCIONES =================
void tocarMelodia(int* mel, int* dur, int len, unsigned long ahora) {
  if (ahora - melUlt >= dur[melIdx]) {
    melUlt = ahora;
    tone(buzzer, mel[melIdx], dur[melIdx]);
    melIdx++;
    if (melIdx >= len) melIdx = 0;
  }
}

void limpiarPantalla() {
  lcd.clear();
}

void limpiarFila(byte f) {
  lcd.setCursor(0, f);
  lcd.print("                ");
}

void printPuntos(unsigned long val) {
  if (val >= 1000000) {
    float v = val / 1000000.0;
    lcd.print(v, 1);
    lcd.print("M");
  }
  else if (val >= 1000) {
    float v = val / 1000.0;
    lcd.print(v, 1);
    lcd.print("K");
  }
  else {
    lcd.print(val);
  }
}

// ================= ALERTA VIGILANTE =================
void alertaVigilante(unsigned long ahora) {
  int mel[] = {659, 784, 880};
  int dur[] = {80, 80, 80};
  static int idx = 0;
  static unsigned long ult = 0;

  if (ahora - ult >= dur[idx]) {
    ult = ahora;
    tone(buzzer, mel[idx], dur[idx]);
    idx++;
    if (idx >= 3) idx = 0;
  }
}

// ================= REINICIO =================
void reiniciarJuego() {
  humanoFila = humanoFilaAnt = 0;

  vFila = 1;
  vFilaAnt = 1;
  vCol = 0;
  vColAnt = 0;
  vDir = 1;

  vIntervalo = 400;
  vUlt = millis();

  vigilanteMirando = false;
  visionDir = 1;

  enCambioFila = false;
  bloqueoCambioFila = false;

  puntos = 0;
  multiplicador = 1;
  cambiosFila = 0;

  modoPanico = false;
  modoCaos = false;

  gameOver = false;
  pantallaFin = 0;
  pantallaAnterior = -1;
  seleccionJugador = false;

  inicioJuego = millis();
  ultimoSegundo = millis();

  lcd.clear();
}

// ================= SETUP =================
void setup() {
  lcd.begin(16,2);
  pinMode(boton, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);

  lcd.createChar(0, humanoChar);
  lcd.createChar(1, vigilanteChar);

  randomSeed(analogRead(0));
  lcd.clear();
}
// ================= LOOP =================
void loop() {
  unsigned long ahora = millis();

  // -------- BOTÓN DEBOUNCE --------
  bool lectura = digitalRead(boton);
  if (lectura != botonEstadoAnt) botonCambio = ahora;

  bool pulsacion = false;
  if (ahora - botonCambio > debounceTime) {
    if (lectura != botonEstado) {
      botonEstado = lectura;
      if (botonEstado == LOW) pulsacion = true;
    }
  }
  botonEstadoAnt = lectura;

  // ================= GAME OVER / MENÚS =================
  if (gameOver) {
    if (pantallaFin != pantallaAnterior) {
      limpiarPantalla();
      melIdx = 0;
      pantallaAnterior = pantallaFin;
    }

    if (pantallaFin == 0) {
      lcd.setCursor(3,0);
      lcd.print("GAME OVER");
      tocarMelodia(gameOverMel, gameOverDur, 4, ahora);
    }
    else if (pantallaFin == 1) {
      lcd.setCursor(0,0);
      lcd.print("J1 ");
      printPuntos(ultimaPuntuacion[0]);
      lcd.setCursor(9,0);
      lcd.print("RJ1 ");
      printPuntos(record[0]);

      lcd.setCursor(0,1);
      lcd.print("J2 ");
      printPuntos(ultimaPuntuacion[1]);
      lcd.setCursor(9,1);
      lcd.print("RJ2 ");
      printPuntos(record[1]);

      tocarMelodia(scoreMel, scoreDur, 4, ahora);
    }
    else if (pantallaFin == 2) {
      if (!seleccionJugador) {
        seleccionJugador = true;
        inicioSeleccion = ahora;
        cuentaAtras = 5;
      }

      limpiarFila(0);
      limpiarFila(1);

      lcd.setCursor(7,0);
      lcd.print(cuentaAtras);
      lcd.setCursor(0,1);
      lcd.print("J1");
      lcd.setCursor(14,1);
      lcd.print("J2");

      if (ahora - inicioSeleccion >= 1000) {
        inicioSeleccion = ahora;
        cuentaAtras--;
        if (cuentaAtras < 0) {
          jugadorActual = 1;
          pantallaFin = 3;
          seleccionJugador = false;
        }
      }

      if (pulsacion) {
        jugadorActual = 0;
        pantallaFin = 3;
        seleccionJugador = false;
      }
    }
    else if (pantallaFin == 3) {
      limpiarFila(0);
      limpiarFila(1);
      lcd.setCursor(3,0);
      lcd.print("ERES J");
      lcd.print(jugadorActual + 1);
      delay(1000);
      reiniciarJuego();
      return;
    }

    if (pantallaFin < 2 && pulsacion) pantallaFin++;
    return;
  }

  // ================= MODOS PÁNICO / CAOS =================
  unsigned long tiempoJugado = ahora - inicioJuego;

  // PÁNICO cada 5 min
  if (!modoPanico && tiempoJugado >= 300000 && (tiempoJugado % 300000) < 1200) {
    modoPanico = true;
    inicioPanico = ahora;
    melIdx = 0;
  }
  if (modoPanico && ahora - inicioPanico >= duracionPanico) {
    modoPanico = false;
  }

  // CAOS cada 10 min
  if (!modoCaos && tiempoJugado >= 600000 && (tiempoJugado % 600000) < 1200) {
    modoCaos = true;
    inicioCaos = ahora;
    melIdx = 0;
  }
  if (modoCaos && ahora - inicioCaos >= duracionCaos) {
    modoCaos = false;
  }

  // ================= BOTÓN HUMANO =================
  if (pulsacion) {
    humanoFila = 1 - humanoFila;
    tone(buzzer, 880, 80);
  }

  // ================= PULSO TENSIÓN =================
  if (ahora - pulsoUlt >= pulsoIntervalo) {
    pulsoUlt = ahora;
    tone(buzzer, 220, 40);
  }

  // ================= MOVIMIENTO VIGILANTE =================
  if (!vigilanteMirando && !enCambioFila && ahora - vUlt >= vIntervalo) {
    vUlt = ahora;

    vColAnt = vCol;
    vFilaAnt = vFila;
    vCol += vDir;

    // Rebote lateral
    if (vCol <= 0 || vCol >= 15) {
      vDir = -vDir;

      if (!bloqueoCambioFila) {
        enCambioFila = true;
        inicioCambioFila = ahora;
        vFila = 1 - vFila;
        cambiosFila++;
        bloqueoCambioFila = true;
      }

      if (vIntervalo < 220) vIntervalo = 220;
    }

    // IA aleatoria (solo si NO está cambiando fila)
    if (!enCambioFila && ahora - ultimoCambioEstado >= intervaloAccion) {
      ultimoCambioEstado = ahora;
      int accion = random(0,100);

      if (accion < 12) {
        vDir = -vDir;
      }
      else if (accion < 35) {
        vigilanteMirando = true;
        visionPos = vCol;
        visionDir = vDir;
      }
      else if (accion < 55) {
        vIntervalo -= 35;
        if (vIntervalo < vIntervaloMaxVel)
          vIntervalo = vIntervaloMaxVel;
        alertaVigilante(ahora);
      }
      else if (accion < 75) {
        vIntervalo += 25;
      }
    }
  }

  // ================= FIN CAMBIO FILA =================
  if (enCambioFila && ahora - inicioCambioFila >= duracionCambioFila) {
    enCambioFila = false;
    bloqueoCambioFila = false;
  }

  // ================= AUMENTO DIFICULTAD =================
  if (cambiosFila >= 3) {
    cambiosFila = 0;
    multiplicador++;
    if (vIntervalo > intervaloMin) {
      vIntervalo -= 40;
      if (vIntervalo < intervaloMin) vIntervalo = intervaloMin;
    }
  }

  // ================= COLISIÓN DIRECTA =================
  if (vCol == humanoCol && vFila == humanoFila) {
    gameOver = true;
    ultimaPuntuacion[jugadorActual] = puntos;
    if (puntos > record[jugadorActual]) record[jugadorActual] = puntos;
    lcd.clear();
    return;
  }

  // ================= VISIÓN VIGILANTE =================
  if (vigilanteMirando && ahora - ultimoAvanceVision >= intervaloVision) {
    ultimoAvanceVision = ahora;
    visionPos += visionDir;

    if (visionPos < 0) visionPos = 0;
    if (visionPos > 15) visionPos = 15;

    lcd.setCursor(visionPos, vFila);
    lcd.print("-");

    // MUERTE si toca línea
    if (humanoFila == vFila &&
        humanoCol == visionPos) {
      gameOver = true;
      ultimaPuntuacion[jugadorActual] = puntos;
      if (puntos > record[jugadorActual]) record[jugadorActual] = puntos;
      lcd.clear();
      return;
    }

    // Fin visión
    if ((visionDir == 1 && visionPos >= 15) ||
        (visionDir == -1 && visionPos <= 0)) {
      vigilanteMirando = false;
      limpiarFila(vFila);
      ultimoCambioEstado = ahora;
    }
  }

  // ================= DIBUJO =================
  if (modoCaos && ((ahora / 180) % 2 == 0)) {
    lcd.clear();
  }

  lcd.setCursor(vColAnt, vFilaAnt);
  lcd.print(" ");

  if (ahora - humanoUlt >= humanoIntervalo || humanoFila != humanoFilaAnt) {
    lcd.setCursor(humanoCol, humanoFilaAnt);
    lcd.print(" ");
    lcd.setCursor(humanoCol, humanoFila);
    lcd.write(byte(0));
    humanoFilaAnt = humanoFila;
    humanoUlt = ahora;
  }

  lcd.setCursor(vCol, vFila);
  lcd.write(byte(1));

  // ================= PUNTUACIÓN (AJUSTADA) =================
  if (ahora - ultimoSegundo >= 1000) {
    ultimoSegundo = ahora;

    unsigned long base = 3 * multiplicador;

    if (modoPanico) base *= 2;
    if (modoCaos) base *= 3;

    puntos += base;
  }

  // ================= MÚSICA MODOS =================
  if (modoPanico && !modoCaos)
    tocarMelodia(panicoMel, panicoDur, 4, ahora);

  if (modoCaos)
    tocarMelodia(caosMel, caosDur, 5, ahora);
}
