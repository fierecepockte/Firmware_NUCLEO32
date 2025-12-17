#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CONFIGURACIÓN OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- CONFIGURACIÓN SERIAL (ESP32) ---
// PA10 = RX (D0), PA9 = TX (D1)
HardwareSerial Serial1(PA10, PA9); 

// --- PINES SENSORES ---
const int trig1 = D2;  const int echo1 = D3;   // S1 - Puerta
const int trig2 = D11; const int echo2 = D12;  // S2 - Ventana
#define PIN_ALARMA LED_BUILTIN 

// --- VARIABLES DE ESTADO ---
String estadoPuertaAnt = "";
String estadoVentanaAnt = "";

float obtenerDistancia(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 25000);
  if (duration == 0) return 0.0;
  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);  // Debug USB
  Serial1.begin(115200); // Comunicación con ESP32

  // Configuración I2C (D4 y D5)
  Wire.setSDA(PB7); 
  Wire.setSCL(PB6);
  Wire.begin();

  pinMode(trig1, OUTPUT); pinMode(echo1, INPUT);
  pinMode(trig2, OUTPUT); pinMode(echo2, INPUT);
  pinMode(PIN_ALARMA, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }
  display.clearDisplay();
  Serial.println("--- STM32 INICIADA CON SENSORES Y OLED ---");
}

void loop() {
  // 1. OBTENER DISTANCIAS
  float d1 = obtenerDistancia(trig1, echo1);
  delay(35); 
  float d2 = obtenerDistancia(trig2, echo2);

  // 2. LÓGICA DE ESTADO - PUERTA
  String estadoPuertaAct;
  if (d1 == 0.0 || d1 >= 11.0) estadoPuertaAct = "CERRADA";
  else if (d1 <= 8.0)          estadoPuertaAct = "ABIERTA";
  else                         estadoPuertaAct = "ESPERA";

  // Enviar a ESP32 solo si el estado de la puerta cambió
  if (estadoPuertaAct != estadoPuertaAnt && estadoPuertaAct != "ESPERA") {
    estadoPuertaAnt = estadoPuertaAct;
    Serial1.println(estadoPuertaAct); // Envía "ABIERTA" o "CERRADA"
    Serial.print("Puerta enviada a ESP32: "); Serial.println(estadoPuertaAct);
  }

  // 3. LÓGICA DE ESTADO - VENTANA (Preparado para el futuro)
  String estadoVentanaAct;
  if (d2 == 0.0 || d2 >= 12.0) estadoVentanaAct = "CERRADA";
  else if (d2 <= 10.0)         estadoVentanaAct = "ABIERTA";
  else                         estadoVentanaAct = "ESPERA";

  // 4. ACTUALIZAR OLED
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("PUERTA (S1):");
  display.setTextSize(2);
  display.setCursor(0, 12);
  display.print(estadoPuertaAct);

  display.drawFastHLine(0, 31, 128, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("VENTANA (S2):");
  display.setTextSize(2);
  display.setCursor(0, 47);
  display.print(estadoVentanaAct);

  display.display();

  // 5. ESCUCHAR COMANDOS DE LA ESP32 (Alarma)
  if (Serial1.available()) {
    String comando = Serial1.readStringUntil('\n');
    comando.trim();
    if (comando == "ALARMA:ON") {
      digitalWrite(PIN_ALARMA, HIGH);
      Serial.println("¡ALARMA ACTIVADA!");
    } 
    else if (comando == "ALARMA:OFF") {
      digitalWrite(PIN_ALARMA, LOW);
      Serial.println("Alarma desactivada");
    }
  }

  delay(200);
}