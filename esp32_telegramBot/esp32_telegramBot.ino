/*ESP32 Telegram Bot
  By. Salvador Hernández Lara
  20-Oct-2019
  
  Collaborations Used:
  Random Nerd Tutorials blog - Rui Santos
  Random Nerd Tutorials blog - Sara Santos
  Instructables Circuits - Fernando Koyanagi
*/
//Libraries
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define arraySize 30
#define BOT_TOKEN "826117763:AAGSn9FwsZHgPkkiGiiDbgoVesCbJfBdHl8"
#define INTERVAL 500
#define marginTemp (0.5)
#define SEALEVELPRESSURE_HPA (1013.25)
#define SENDER_ID_COUNT 1

Adafruit_BME280 bme;

const String CLIMATE = "clima";
const String STATS = "estado";
const String START = "/start";

float temp1Vals[arraySize];
float temperatura1, humedad1, presion1, altitud1;

int sendMessage, activeAlarm, i, j, p, ini = 0;
int inputState;

uint32_t lastCheckTime = 0;

String validSenderIds[SENDER_ID_COUNT] = {"449619763"};

WiFiClientSecure client;
WiFiMulti wifiMulti;

UniversalTelegramBot bot(BOT_TOKEN, client);

void setup() {
  Serial.begin(115200);
  
  setupWiFi();
  
  bool status;
  status = bme.begin(0x76);  
  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  Serial.println("-- ESP-Telegram-Control --");
  Serial.println();
}

void loop() { 
  uint32_t now = millis();
  float maxValue, minValue, diff;
  
  analyzerValues(maxValue, minValue, diff);

  if (now - lastCheckTime > INTERVAL) {
    lastCheckTime = now;
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    handleNewMessages(numNewMessages);
  }
}

void setupWiFi() {
  
    wifiMulti.addAP("AXTEL XTREMO-F002", "0369F002");
    //wifiMulti.addAP("ssid", "password");
    
    Serial.println("Connecting...");
      
    while (wifiMulti.run() != WL_CONNECTED) {
      delay(500);
      Serial.print('.');
    }
    Serial.println('\n');
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());            
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());           
}

/*----------------*/
void handleNewMessages(int numNewMessages) {
  for (int i=0; i < numNewMessages; i++){
    String chatId = String(bot.messages[i].chat_id);
    String senderId = String(bot.messages[i].from_id);
     
    Serial.println("senderId: " + senderId);
 
    boolean validSender = validateSender(senderId);
 
    if(!validSender){
      bot.sendMessage(chatId, "Lo siento pero no tienes autorización para pedir información", "HTML"); 
      continue;
    }
     
    String text = bot.messages[i].text;
    
    if (text.equalsIgnoreCase(START)){
      handleStart(chatId, bot.messages[i].from_name);
    }else if(text.equalsIgnoreCase(CLIMATE)){
      handleClimate(chatId);
    }else if (text.equalsIgnoreCase(STATS)){
      handleStatus(chatId);
    }else{
      handleNotFound(chatId);
    }
  }
}

boolean validateSender(String senderId){
  for(int i=0; i < SENDER_ID_COUNT; i++) {
    if(senderId == validSenderIds[i]) {
      return true;
    }
  }
  return false;
}

void handleStart(String chatId, String fromName) {
  String message = "<b>Hola " + fromName + ".</b>\n";

  message += getCommands();
  bot.sendMessage(chatId, message, "HTML");
}

void sendMessageAll(String msgCase){
  for(int i=0; i < SENDER_ID_COUNT; i++) {
      String senderId = validSenderIds[i];
      String message = "";
      message += "<b>Notificación de Sensor Inteligente</b>\n";
      message += msgCase;
      message += getClimateMessage() + "\n";
      
      bot.sendMessage(senderId, message, "HTML");
    }
 }
 
void handleClimate(String chatId){
  bot.sendMessage(chatId, getClimateMessage(), "");
}

void handleStatus(String chatId) {
  String message = "";
  message += getClimateMessage();
  bot.sendMessage(chatId, message, "HTML");
}

void handleNotFound(String chatId) {
  String message = "Comando no encontrado\n";
  message += getCommands();
  //Serial.println(message);
  bot.sendMessage(chatId, message, "HTML");
}

/*-----------------*/
void analyzerValues(float & maxValue, float & minValue, float & diff){
  /*
   * Función de analisis de arreglos de datos de temperatura
   * Obtenie el maximo, minimo y la diferencia del buffer de datos
   * By. Salvador Hernández Lara
  */
  byte maxIndex, minIndex = 0; //Esta varibales solo pertenecen a la función
  maxValue = temp1Vals[maxIndex];
  minValue = temp1Vals[minIndex];
  
  if (p < arraySize){
    temp1Vals[p] = bme.readTemperature();
    p++;
  }else{
    p = 0;
  }
  
  for (int j = 0; j < arraySize; j++){
    if (temp1Vals[j] > maxValue) {
      maxValue = temp1Vals[j];
      maxIndex = j;
    }else if (temp1Vals[j] < minValue){
      minValue = temp1Vals[j];
      minIndex = j;
    }
  }
  diff = maxValue - minValue;
  
  if (abs(diff) >= marginTemp && diff != maxValue){
    if(sendMessage == 0){
      sendMessageAll("La temperatura a sobrepasado el limite establecido 0.5ºC\n");
      sendMessage = 1;
    }
  }else if (diff == maxValue){
     if(sendMessage == 0){
      sendMessageAll("Inicializando, llenando buffer de datos...\n");
      sendMessage = 1;
    }
  }else {
    if(ini == 0){
      sendMessageAll("Buffer de datos completo, el sistema funciona correctamente.\n");
      ini = 1;
    } 
    if(sendMessage == 1){
      sendMessage = 0;
    }
  }
  
}

//------------------------------------
String getClimateMessage() {
  temperatura1 = bme.readTemperature();
  humedad1 = bme.readHumidity();
  presion1 = bme.readPressure() / 100.0F;
  altitud1 = bme.readAltitude(SEALEVELPRESSURE_HPA);
  
  if (isnan(temperatura1)){
     return "Error en la lectura de la temperatura";
  }else{
        
    String message = "";
    message += "La temperatura es de: " + String(temperatura1)+ " °C \n";
    message += "La humedad es de: " + String(humedad1) + "% \n";
    message += "La presión es de: " + String(presion1) + " hPa \n";
    message += "La altitud aprox. es de: " + String(altitud1) + " msm \n";
    return message;
  
   }
   
}

String getCommands() {
  String message = "Los Comandos disponibles son:\n\n";
  message += "<b>" + CLIMATE + "</b>: Comando para verificar la temperatura y humedad del sensor\n";
  message += "<b>" + STATS + "</b>: Comando para verificar el estado de lo sensores"; 
  return message;
}
