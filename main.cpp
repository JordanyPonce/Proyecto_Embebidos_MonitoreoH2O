#include <Arduino.h>
#include <WiFi.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>

/* Definición credenciales de la red WiFi */
const char* ssid = "NETLIFE-JOREMY";
const char* pass = "095508023925";
const char* token = "6529364790:AAHdc-YTV6TILXihjFMEQexZRxw5xqv3GqM"; 

// Restablecer el estado de los mensajes
bool mensajeEnviado = false;
bool mensajeEnviadoOptimo = false;

WiFiClientSecure client;
UniversalTelegramBot bot(token, client);

#define COLUMS 16
#define ROWS   2

#define PAGE   ((COLUMS) * (ROWS))

LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);

const int LED_VERDE_PIN = 15;
const int LED_AMARILLO_PIN = 2;
const int LED_ROJO_PIN = 4;
const int TDS_PIN = 36;
const int TURBIDEZ_PIN = 39;
const int RELE_PIN = 12; 

const int NUM_LECTURAS = 10;
bool precaucionVisible = false;

float obtenerPromedioTDS() {
  float promedio = 0;
  for (int i = 0; i < NUM_LECTURAS; i++) {
    promedio += analogRead(TDS_PIN);
    delay(10);
  }
  promedio /= NUM_LECTURAS;
  return promedio;
}

float obtenerTurbidez() {
  int turbidezValue = analogRead(TURBIDEZ_PIN);
  float turbidez = map(turbidezValue, 0, 4095/2, 0, 100);
  return turbidez;
}

void MostrarLCD(float TDS, float Turbity) {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Turbidez: ");
  lcd.print(int(Turbity));
  lcd.print("%");

  lcd.setCursor(0, 0);
  lcd.print("TDS: ");
  lcd.print(TDS);
  lcd.print(" ppm");
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(30);

  // Conecta a la red Wi-Fi
  Serial.print("Conectar a la red WiFi");
  Serial.print(ssid);
  WiFi.begin(ssid, pass);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Conexión Wi-Fi exitosa!. Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Envia un mensaje al bot de Telegram indicando que el sistema inció
  String chatId = "1375625648"; 
  String message = "El módulo ESP32 se encuentra en línea  y en constante monitoreo 👷‍♂️✅.\n📌 Puede conocer los valores de TDS y Turbidez mediante los siguientes comandos:\n/TDS y /Turbidez"; 

  if (bot.sendMessage(chatId, message, "")) {
    Serial.println("Mensaje enviado con éxito.");
  } else {
    Serial.println("Error al enviar el mensaje.");
  }

  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_AMARILLO_PIN, OUTPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, LOW);

  while (lcd.begin(COLUMS, ROWS) != 1) {
    Serial.println(F("No hay conexion con la LCD."));
    delay(5000);
  }

  lcd.print(F("Monitoreo..."));
  delay(2000);

  lcd.clear();
}

void loop() {
  
  float tdsValue, Turbidez, messageTDS;
  tdsValue = obtenerPromedioTDS();
  messageTDS=tdsValue;
  Turbidez = obtenerTurbidez();
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  String chatId = "1375625648";

  if (tdsValue >= 0 && tdsValue < 600) {
    digitalWrite(LED_VERDE_PIN, HIGH);
    digitalWrite(LED_AMARILLO_PIN, LOW);
    digitalWrite(LED_ROJO_PIN, LOW);
    MostrarLCD(tdsValue,Turbidez);
    // Borrar mensaje de precaución
    if (precaucionVisible) {
      lcd.clear();
      precaucionVisible = false;
    }

  } else if (tdsValue >= 600 && tdsValue < 900) {
    digitalWrite(LED_VERDE_PIN, LOW);
    digitalWrite(LED_AMARILLO_PIN, HIGH);
    digitalWrite(LED_ROJO_PIN, LOW);
    MostrarLCD(tdsValue,Turbidez);
    // Borrar mensaje de precaución
    if (precaucionVisible) {
      lcd.clear();
      precaucionVisible = false;
    }

  } else if (tdsValue >= 900 && tdsValue < 1200) {
    digitalWrite(LED_VERDE_PIN, LOW);
    digitalWrite(LED_AMARILLO_PIN, LOW);
    digitalWrite(LED_ROJO_PIN, HIGH);
    MostrarLCD(tdsValue,Turbidez);
    // Borrar mensaje de precaución
    if (precaucionVisible) {
      lcd.clear();
      precaucionVisible = false;
    }

  } else if (tdsValue >= 1200) {
    digitalWrite(LED_VERDE_PIN, LOW);
    digitalWrite(LED_AMARILLO_PIN, LOW);
    digitalWrite(LED_ROJO_PIN, HIGH);
    // Mostrar mensaje de precaución intermitentemente
      if (precaucionVisible) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("   PRECAUCION   ");
        lcd.setCursor(0, 1);
        lcd.print("  Agua no apta  ");
        precaucionVisible = false;
      }   else {
        lcd.clear();
        MostrarLCD(tdsValue,Turbidez);
        precaucionVisible = true;}
  }
    
    if (messageTDS>=1200){
    if (!mensajeEnviado) {
        mensajeEnviadoOptimo = false;
        String message = "⚠️⚠️⚠️ ¡ALERTA! ⚠️⚠️⚠️\nLos niveles de TDS se encuentran fuera del rango configurado, se debe cortar el suministro de agua.\n¿Desea modificar el estado de la Electroválvula?\n/Abrir o /Cerrar";
        if (bot.sendMessage(chatId, message, "")) {
          Serial.println("Mensaje enviado con éxito.");
        } else {
          Serial.println("Error al enviar el mensaje.");
        }
        mensajeEnviado = true; // Marcar el mensaje como enviado
      }}
  else { // Cuando tdsValue es menor a 1200
    // Restablecer el estado de los mensajes
      mensajeEnviado = false;
    // Verificar si el mensaje de niveles óptimos no ha sido enviado
    if (!mensajeEnviadoOptimo) {
      String message = "Los niveles de TDS son óptimos para la potabilización. ✅✅✅\n¿Desea modificar el estado de la Electroválvula?\n/Abrir o /Cerrar";
      if (bot.sendMessage(chatId, message, "")) {
        Serial.println("Mensaje de niveles óptimos enviado con éxito.");
      } else {
        Serial.println("Error al enviar el mensaje de niveles óptimos.");
      }
      mensajeEnviadoOptimo = true; // Marcar el mensaje de niveles óptimos como enviado
    }}


  for (int i = 0; i < numNewMessages; i++) {
    String chatId = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    // Procesa el mensaje
    if (text == "/Abrir") {
      digitalWrite(RELE_PIN, LOW); // Abre la electroválvula
      String message = "🟩 Electroválvula abierta.";
      bot.sendMessage(chatId, message, "");
    } else if (text == "/Cerrar") {
      digitalWrite(RELE_PIN, HIGH); // Cierra la electroválvula
      String message = "🟥 Electroválvula cerrada.";
      bot.sendMessage(chatId, message, "");
    } else if (text == "/TDS") {
      String message = "📌 Valor de TDS: " + String(tdsValue);
      bot.sendMessage(chatId, message, "");
    } else if (text == "/Turbidez") {
      String message = "📌 Valor de Turbidez: " + String(Turbidez);
      bot.sendMessage(chatId, message, "");
    }
  }

  delay(1000);
}
