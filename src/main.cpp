#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CONFIGURACIÓN OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- CONFIGURACIÓN COMUNICACIÓN CON ESP32 ---
// PA10 = RX (Conectar al TX del ESP32)
// PA9  = TX (Conectar al RX del ESP32)
HardwareSerial SerialESP(PA10, PA9); 

// --- PINES SENSORES ---
const int trig1 = D2;  const int echo1 = D3;   // S1 - Puerta
const int trig2 = D11; const int echo2 = D12;  // S2 - Ventana

// Pin para el LED o Buzzer de la alarma
#define PIN_ALARMA LED_BUILTIN 

// --- VARIABLES DE ESTADO ---
String estadoPuertaAnt = "";
String estadoVentanaAnt = "";
String ultimoComandoAlarma = "OFF"; 

// Función para medir distancia
float obtenerDistancia(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  // Timeout de 30ms (suficiente para medir hasta ~5 metros)
  long duration = pulseIn(echo, HIGH, 30000); 
  
  if (duration == 0) return 0.0;
  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);  
  SerialESP.begin(115200); 

  // Configuración pines I2C
  Wire.setSDA(PB7); 
  Wire.setSCL(PB6);
  Wire.begin();

  pinMode(trig1, OUTPUT); pinMode(echo1, INPUT);
  pinMode(trig2, OUTPUT); pinMode(echo2, INPUT);
  pinMode(PIN_ALARMA, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Error: No se encuentra la pantalla OLED"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("STM32 INICIANDO...");
  display.display();
  delay(1000);

  Serial.println("--- STM32 LISTA CON NUEVAS MEDIDAS ---");
}

void loop() {
  // 1. LECTURA DE SENSORES
  float d1 = obtenerDistancia(trig1, echo1);
  delay(40); // Pequeña pausa
  float d2 = obtenerDistancia(trig2, echo2);

  // ====================================================
  // 2. LÓGICA DE PUERTA (S1) - (Valores anteriores)
  // ====================================================
  String estadoPuertaAct;
  if (d1 == 0.0 || d1 >= 11.0) estadoPuertaAct = "CERRADA";
  else if (d1 <= 8.0)          estadoPuertaAct = "ABIERTA";
  else                         estadoPuertaAct = "ESPERA";

  if (estadoPuertaAct != "ESPERA" && estadoPuertaAct != estadoPuertaAnt) {
    estadoPuertaAnt = estadoPuertaAct;
    SerialESP.println("PUERTA:" + estadoPuertaAct); 
    Serial.println("Cambio Puerta -> " + estadoPuertaAct);
  }

  // ====================================================
  // 3. LÓGICA DE VENTANA (S2) - ¡¡AJUSTADA!!
  // ====================================================
  // CERRADO: 12.5 cm o más
  // ABIERTO: 9.0 cm o menos
  // MENOS SENSIBLE: Hay un hueco grande entre 9 y 12.5
  String estadoVentanaAct;
  
  if (d2 == 0.0 || d2 >= 12.5) {
      estadoVentanaAct = "CERRADA";
  } 
  else if (d2 <= 9.0) {
      estadoVentanaAct = "ABIERTA";
  } 
  else {
      // Si está entre 9.1 y 12.4 cm, no cambia (zona muerta)
      estadoVentanaAct = "ESPERA";
  }

  if (estadoVentanaAct != "ESPERA" && estadoVentanaAct != estadoVentanaAnt) {
    estadoVentanaAnt = estadoVentanaAct;
    SerialESP.println("VENTANA:" + estadoVentanaAct); 
    Serial.println("Cambio Ventana -> " + estadoVentanaAct + " (" + String(d2) + "cm)");
  }

  // ==========================================
  // 4. RECIBIR DATOS DEL ESP32 (ALARMA)
  // ==========================================
  if (SerialESP.available()) {
    String comando = SerialESP.readStringUntil('\n');
    comando.trim(); 
    
    if (comando == "ALARMA:ON") {
      digitalWrite(PIN_ALARMA, HIGH);
      ultimoComandoAlarma = "ON";
    } 
    else if (comando == "ALARMA:OFF") {
      digitalWrite(PIN_ALARMA, LOW);
      ultimoComandoAlarma = "OFF";
    }
  }

  // ==========================================
  // 5. ACTUALIZAR PANTALLA OLED
  // ==========================================
  display.clearDisplay();
  display.drawLine(0, 0, 128, 0, SSD1306_WHITE);
  
  // -- PUERTA --
  display.setTextSize(1);
  display.setCursor(0, 4);
  display.print("PTA (d1): ");
  display.print((int)d1); // Muestro distancia para calibrar
  display.print("cm");
  
  display.setTextSize(2);
  display.setCursor(20, 15);
  if(estadoPuertaAnt == "ABIERTA") display.print("ABIERTA");
  else display.print("CERRADA");

  // -- VENTANA --
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("VEN (d2): ");
  display.print(d2, 1); // Muestro distancia con decimal para calibrar fino
  display.print("cm");

  display.setTextSize(2); 
  display.setCursor(20, 46);
  if(estadoVentanaAnt == "ABIERTA") display.print("ABIERTA");
  else display.print("CERRADA");

  display.display();
  
  delay(150);
}