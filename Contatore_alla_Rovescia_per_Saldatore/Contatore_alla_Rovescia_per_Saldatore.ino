#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===== LCD I2C 16x4 =====
LiquidCrystal_I2C lcd(0x27, 16, 4);

// ===== PIN =====
const int startBtn = 8;
const int zeroBtn  = 9;
const int relayPin = 10;
const int buzzer   = 11;
const int ledPin   = 12;

// ===== TEMPO =====
const long START_TIME = 1800;   // 30 minuti
long timeLeft = 0;
bool running = false;
unsigned long lastSecondMillis = 0;

// ===== FASE FINALE =====
bool finalPhase = false;
bool melodyPlayed = false;

// ===== LED accelera =====
unsigned long ledMillis = 0;
bool ledState = false;

// ===== LCD anti-flicker =====
long lastShown = -1;

// ===== NOTE =====
#define NOTE_A5  880
#define NOTE_F5  698
#define NOTE_C6  1047
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_GS5 831

// ===== MARCIA IMPERIALE =====
const int melodyLength = 18;
const int melody[melodyLength] = {
  NOTE_A5, NOTE_A5, NOTE_A5,
  NOTE_F5, NOTE_C6, NOTE_A5,
  NOTE_F5, NOTE_C6, NOTE_A5,
  NOTE_E6, NOTE_E6, NOTE_E6,
  NOTE_F6, NOTE_C6, NOTE_GS5,
  NOTE_F5, NOTE_C6, NOTE_A5
};

const int noteDurations[melodyLength] = {
  420, 420, 420,
  300, 120, 420,
  300, 120, 560,
  420, 420, 420,
  300, 120, 420,
  300, 120, 560
};

const int noteGapMs = 35;

// Durata totale melodia = somma note (6160 ms) + 18 pause da 35 ms (630 ms)
const long MELODY_TOTAL_MS = 6790;

void setup() {
  pinMode(startBtn, INPUT_PULLUP);
  pinMode(zeroBtn,  INPUT_PULLUP);
  pinMode(relayPin, OUTPUT);
  pinMode(buzzer,   OUTPUT);
  pinMode(ledPin,   OUTPUT);

  lcd.init();
  lcd.backlight();

  stopAllNoMsg();
  showReady();
}

void loop() {

  // START / RESTART
  if (digitalRead(startBtn) == LOW) {
    startCountdown();
    delay(250);
  }

  // STOP
  if (digitalRead(zeroBtn) == LOW) {
    stopAllWithMsg();
    delay(250);
  }

  // TICK 1 SECONDO
  if (running && millis() - lastSecondMillis >= 1000) {
    lastSecondMillis += 1000;

    if (timeLeft > 0) timeLeft--;

    if (timeLeft == 15) {
      finalPhase = true;
      ledMillis = millis();
      ledState = false;
    }

    if (timeLeft == 0) {
      running = false;
      finalPhase = false;
      digitalWrite(relayPin, LOW);
      digitalWrite(ledPin, LOW);
      noTone(buzzer);
      showEnd();
    }
  }

  // ===== LED =====
  if (finalPhase && timeLeft > 0) {
    handleAcceleratingLed();
  } else if (running) {
    digitalWrite(ledPin, HIGH);
  }

  // ===== BUZZER A 30 SECONDI DALLA FINE =====
  // NB: non dipende da finalPhase (che parte a 15s), altrimenti non suonerebbe mai
  if (running && timeLeft == 30 && !melodyPlayed) {
    melodyPlayed = true;
    playImperialMarch();   // suona solo una volta
    lastSecondMillis = millis();   // evita recupero a raffica dopo il delay della melodia
    timeLeft -= (long)((MELODY_TOTAL_MS + 500) / 1000);
    if (timeLeft < 0) timeLeft = 0;
    lastShown = -1;
  }

  // LCD
  if (running) updateLCD();
}

// ================= FUNZIONI =================

void startCountdown() {
  timeLeft = START_TIME;
  running = true;
  finalPhase = false;
  melodyPlayed = false;

  digitalWrite(relayPin, HIGH);
  digitalWrite(ledPin, HIGH);
  noTone(buzzer);

  lastSecondMillis = millis();
  lastShown = -1;
}

void stopAllNoMsg() {
  running = false;
  finalPhase = false;
  melodyPlayed = false;
  timeLeft = 0;
  digitalWrite(relayPin, LOW);
  digitalWrite(ledPin, LOW);
  noTone(buzzer);
  lastShown = -1;
}

void stopAllWithMsg() {
  stopAllNoMsg();

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Stop            ");
  lcd.setCursor(0,1); lcd.print("Timer a zero    ");
  lcd.setCursor(0,2); lcd.print("Rele/Buzzer OFF ");
  lcd.setCursor(0,3); lcd.print("Premi START     ");
  delay(1200);
  showReady();
}

void handleAcceleratingLed() {
  int interval;
  if (timeLeft > 10) interval = 500;
  else if (timeLeft > 5) interval = 300;
  else if (timeLeft > 3) interval = 150;
  else interval = 80;

  if (millis() - ledMillis >= (unsigned long)interval) {
    ledMillis = millis();
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
  }
}

void updateLCD() {
  if (timeLeft == lastShown) return;
  lastShown = timeLeft;

  lcd.setCursor(0,0);
  if (timeLeft == 30)      lcd.print("- 30 SECONDI -  ");
  else if (!finalPhase)    lcd.print("Conto alla roves");
  else if (timeLeft == 5)  lcd.print("- 5 SECONDI  -  ");
  else if (timeLeft == 3)  lcd.print("!!!   3   !!!  ");
  else if (timeLeft <= 2)  lcd.print("!!! FINALE !!!  ");
  else                     lcd.print("!! ATTENZIONE !!");

  lcd.setCursor(0,1);
  lcd.print("Tempo: ");
  printTimeAt(7,1);
  lcd.print("   ");

  lcd.setCursor(0,2);
  if (!finalPhase) lcd.print("Rele:ON  LED:ON ");
  else             lcd.print("LED finale att. ");

  lcd.setCursor(0,3);
  lcd.print("ReStart    Stop ");
}

void printTimeAt(uint8_t col, uint8_t row) {
  int min = timeLeft / 60;
  int sec = timeLeft % 60;
  lcd.setCursor(col,row);
  if (min < 10) lcd.print("0");
  lcd.print(min);
  lcd.print(":");
  if (sec < 10) lcd.print("0");
  lcd.print(sec);
}

void showReady() {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Sistema pronto  ");
  lcd.setCursor(0,1); lcd.print("START=30 minuti ");
  lcd.setCursor(0,2); lcd.print("ZERO=Stop subito");
  lcd.setCursor(0,3); lcd.print("Rele e LED OFF  ");
}

void showEnd() {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("TEMPO SCADUTO   ");
  lcd.setCursor(0,1); lcd.print("Operazione OFF  ");
  lcd.setCursor(0,2); lcd.print("Rele/LED/Buz OFF");
  lcd.setCursor(0,3); lcd.print("Premi START     ");
}

void playImperialMarch() {
  for (int i = 0; i < melodyLength; i++) {
    if (timeLeft <= 0) return;
    tone(buzzer, melody[i], noteDurations[i]);
    delay(noteDurations[i]);
    noTone(buzzer);
    delay(noteGapMs);
  }
}
