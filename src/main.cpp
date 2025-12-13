#include <Arduino.h>

// --- CONFIGURACIÓN DE PINES ---
#define PIN_SENSOR_PUERTA 2
#define PIN_ALARMA LED_BUILTIN 

// --- VARIABLES ---
int estadoAnterior = -1;

// --- SOLUCIÓN AL ERROR DE LINKER ---
// Creamos manualmente el puerto Serial1 asignando sus pines
// PA10 = RX, PA9 = TX (Son los pines D0 y D1 de la placa)
HardwareSerial Serial1(PA10, PA9); 

void setup() {
  // 1. Debug con PC (USB)
  Serial.begin(115200);

  // 2. Comunicación con ESP32 (Pines D0/D1)
  Serial1.begin(115200); 

  // 3. Pines
  pinMode(PIN_SENSOR_PUERTA, INPUT_PULLUP); 
  pinMode(PIN_ALARMA, OUTPUT);

  Serial.println("--- STM32 LISTA (CON FIX SERIAL1) ---");
}

void loop() {
  // --- TAREA 1: LEER PUERTA ---
  int lecturaActual = digitalRead(PIN_SENSOR_PUERTA);

  if (lecturaActual != estadoAnterior) {
    delay(50); // Debounce
    if (digitalRead(PIN_SENSOR_PUERTA) == lecturaActual) {
      estadoAnterior = lecturaActual;
      
      if (lecturaActual == HIGH) {
        Serial.println("Detectado: ABIERTA");
        Serial1.println("ABIERTA");
      } else {
        Serial.println("Detectado: CERRADA");
        Serial1.println("CERRADA");
      }
    }
  }

  // --- TAREA 2: ESCUCHAR ESP32 ---
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
}