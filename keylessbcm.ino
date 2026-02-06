#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

/*
 * Project: Keyless Entry Body Control Module (BCM)
 * Platform: Arduino
 * Description:
 * RFID-based keyless access system with FSM-controlled
 * climate control, headlight automation and parking assist.
 */

/* ===== PIN DEFINITIONS ===== */
#define RST_PIN     9
#define SS_PIN      10
#define FAN_PIN     3
#define FAR_PIN     4
#define SERVO_PIN   5
#define TRIG_PIN    6
#define ECHO_PIN    7
#define BUZZER_PIN  8
#define LDR_PIN     A0
#define TEMP_PIN    A1

/* ===== OBJECTS ===== */
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo kapiKilidi;

/* ===== FSM STATES ===== */
enum BCM_State {
  BCM_LOCKED,
  BCM_ACTIVE
};

BCM_State bcmState = BCM_LOCKED;

/* ===== VARIABLES ===== */
byte yetkiliKart[4] = {97, 76, 67, 9};

unsigned long lastRFIDTime = 0;
unsigned long lastLCDUpdate = 0;
const unsigned long lcdInterval = 300;

float sicaklik = 0;
int ortamIsigi = 0;
long sure, mesafe;

/* ===== SETUP ===== */
void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  lcd.init();
  lcd.backlight();
  lcd.print("BCM Sistem");
  lcd.setCursor(0, 1);
  lcd.print("Kilitli");

  kapiKilidi.attach(SERVO_PIN);
  kapiKilidi.write(0); // Kapı kilitli

  pinMode(FAN_PIN, OUTPUT);
  pinMode(FAR_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

/* ===== MAIN LOOP ===== */
void loop() {
  kimlikDogrulama();

  if (bcmState == BCM_ACTIVE) {
    klimaSistemi();
    farSistemi();
    parkAsistani();
  }
}

/* ===== RFID AUTH / TOGGLE ===== */
void kimlikDogrulama() {
  if (millis() - lastRFIDTime < 500) return;

  if (!mfrc522.PICC_IsNewCardPresent() ||
      !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  lastRFIDTime = millis();
  bool yetki = true;

  for (byte i = 0; i < 4; i++) {
    if (mfrc522.uid.uidByte[i] != yetkiliKart[i]) {
      yetki = false;
      break;
    }
  }

  lcd.clear();

  if (yetki) {
    if (bcmState == BCM_LOCKED) {
      // UNLOCK
      bcmState = BCM_ACTIVE;
      lcd.print("Sistem Aktif");
      kapiKilidi.write(90);
      tone(BUZZER_PIN, 1000, 200);
    } else {
      // LOCK
      bcmState = BCM_LOCKED;
      lcd.print("Sistem Kilitli");
      kapiKilidi.write(0);

      digitalWrite(FAN_PIN, LOW);
      digitalWrite(FAR_PIN, LOW);
      noTone(BUZZER_PIN);

      tone(BUZZER_PIN, 1000, 100);
      tone(BUZZER_PIN, 1000, 100);
    }
  } else {
    lcd.print("Gecersiz Kart");
    tone(BUZZER_PIN, 3000, 400);
  }

  mfrc522.PICC_HaltA();
}

/* ===== CLIMATE CONTROL ===== */
void klimaSistemi() {
  int okunan = analogRead(TEMP_PIN);
  sicaklik = (okunan * 5.0 / 1024.0) * 100.0;

  digitalWrite(FAN_PIN, (sicaklik > 25) ? HIGH : LOW);
}

/* ===== HEADLIGHT CONTROL ===== */
void farSistemi() {
  ortamIsigi = analogRead(LDR_PIN);
  digitalWrite(FAR_PIN, (ortamIsigi < 300) ? HIGH : LOW);
}

/* ===== PARK ASSIST ===== */
void parkAsistani() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  sure = pulseIn(ECHO_PIN, HIGH, 25000);
  mesafe = sure * 0.034 / 2;

  if (millis() - lastLCDUpdate < lcdInterval) return;
  lastLCDUpdate = millis();

  lcd.setCursor(0, 1);

  if (mesafe > 0 && mesafe < 10) {
    lcd.print("DUR! Cok Yakin ");
    tone(BUZZER_PIN, 2000);
  } 
  else if (mesafe < 30) {
    lcd.print("Park: Yakin   ");
    tone(BUZZER_PIN, 2000, 100);
  } 
  else {
    lcd.print("Park: Guvenli ");
    noTone(BUZZER_PIN);
  }
}
