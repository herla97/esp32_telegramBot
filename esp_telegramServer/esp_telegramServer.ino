/*ESP32 Telegram Bot
  By. Salvador Hernández Lara
  20-Oct-2019
  
  Collaborations Used:
  Random Nerd Tutorials blog - Rui Santos
  Random Nerd Tutorials blog - Sara Santos
  Instructables Circuits - Fernando Koyanagi

  Esta versión utiliza DHT11
  Versión sin optimizar y en sucio.
*/

#include "WiFi.h"
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <UniversalTelegramBot.h>
//https://api.telegram.org/bot890166056:AAFXZevwIrBWAMTNzJCx3ukw0qq12B6axlU/getUpdates

//Definimos pines de salida
//#define RELAY_PIN 5
#define INPUT_PIN 15 // Sensor Magnetico
#define DHTPIN1 4 //Pin del DTH11
#define DHTTYPE    DHT11     // DHT 11
//Intervalo entre nuevas verificaciones de mensajes
#define INTERVAL 500

#define arraySize 30
//Definimos token del bot creado
#define BOT_TOKEN "890166056:AAFXZevwIrBWAMTNzJCx3ukw0qq12B6axlU"

//Host donde se mandan los datos
const char* serverName = "http://myservidor.com/post_data.php";
//Key de acesso y seguridad
String apiKeyValue = "4xfyaWcRNE";

String sensorName = "145";
String sensorLocation = "Office";

//Comandos aceptados
const String DOOR = "puerta";
const String ACTIVE = "alarma";
const String CLIMATE = "clima";
const String STATS = "estado";
const String START = "/start";
//const String REBOOT = "/reboot";

//Objeto que realiza la lectura del sensor 
DHT dht1(DHTPIN1, DHTTYPE);
//DHT dht2(DHTPIN2, DHTTYPE);

float temperatura1, humedad1, temperatura2, humedad2;
float temp1Vals[arraySize];
int sendMessage, activeAlarm, i, j = 0;
int inputState;
 
//Cliente para conexiones seguras
WiFiClientSecure client;
WiFiMulti wifiMulti;
//Objeto con métodos para comunicarse por Telegram
UniversalTelegramBot bot(BOT_TOKEN, client);
//Hora de la última verificación
uint32_t lastCheckTime = 0;
 
//Número de usuarios que pueden interactuar con el bot
#define SENDER_ID_COUNT 3
// ID de usuario que pueden interactuar con el bot.
// Puede verificar su identificación desde el monitor en serie enviando un mensaje al bot
String validSenderIds[SENDER_ID_COUNT] = {"449619763", "506603243", "-355651911"};

void setup() {
  Serial.begin(115200);
 
  //Inicialización de sensorDHT1
  dht1.begin();
  //dht2.begin();
  //Pin de sensor magnetico 
  pinMode(INPUT_PIN, INPUT_PULLUP);
  //Inicialización de WiFi
  setupWiFi();
}

void setupWiFi() {
  
    wifiMulti.addAP("RootSHL", "sd9aDUxecK*");
    wifiMulti.addAP("iPhone", "12345678");
    wifiMulti.addAP("SAMSUNG S12", "manzanilla");
    wifiMulti.addAP("AndroidAP8FA0", "sdpc0068");
    
    Serial.println("Conectando");
      
    while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
      delay(500);
      Serial.print('.');
    }
    Serial.println('\n');
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());            
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());           
  }
  
void loop() {
  //Tiempo de ahora desde el boot
  uint32_t now = millis();
  inputState = digitalRead(INPUT_PIN);
  temperatura1 = dht1.readTemperature();
  humedad1 = dht1.readHumidity();
//----
  if(inputState == 0 && i <= 5 && j == 1){
    Serial.print(".");
    i++;
  }else if(inputState == 1){
      if(sendMessage == 0){
        Serial.println("Puerta Abierta");
        if(activeAlarm == 0){
          sendMessageAll();
        }
        sendMessage = 1;
        j = 0;
      }
      Serial.println("");
      sensor_data();
  }else{
      if(sendMessage == 1){
        Serial.println("Puerta Cerrada");
        sendMessage = 0;
        j = 1;
      }
      
    Serial.println("");
    sensor_data();
    i = 0;
  }
 
  //Si el tiempo transcurrido desde la última verificación es más largo que el intervalo especificado
  if (now - lastCheckTime > INTERVAL) {
    //Pone la última hora de verificación como ahora y busqueda de mensajes
    lastCheckTime = now;
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    handleNewMessages(numNewMessages);
  }
}

void handleNewMessages(int numNewMessages) {
  for (int i=0; i < numNewMessages; i++){ //para cada mensaje nuevo
    String chatId = String(bot.messages[i].chat_id); //id del chat
    String senderId = String(bot.messages[i].from_id); //id do contato
     
    Serial.println("senderId: " + senderId); //mostra no monitor serial o id de quem mandou a mensagem
 
    boolean validSender = validateSender(senderId); //comprueba si es una identificación de remitente de la lista de remitentes válidos
 
    if(!validSender){ //Si no tienes autorización se ejecuta
      bot.sendMessage(chatId, "Lo siento pero no tienes permiso para pedir información", "HTML"); //envía un mensaje que no tienes permiso y regresa sin hacer nada más
      continue; //continúe con la próxima iteración de for (vaya al siguiente mensaje, no ejecute el código a continuación)
    }
     
    String text = bot.messages[i].text; //Texto Extraido
    
    if (text.equalsIgnoreCase(START)){
      handleStart(chatId, bot.messages[i].from_name); //mostrar opciones
    }else if(text.equalsIgnoreCase(CLIMATE)){
      handleClimate(chatId); //envia un mensaje con la temperatura y humedad
    }else if(text.equalsIgnoreCase(DOOR)){
      handleDoor(chatId); //envia un mensaje del estado de la puerta
    }else if (text.equalsIgnoreCase(STATS)){
      handleStatus(chatId); //enviar mensaje con estado de la temperatura y humedad
    }else if (text.equalsIgnoreCase(ACTIVE)){
      handleActive(chatId); //Cambia el estado de alarma
    }else{
      handleNotFound(chatId); //mostrar mensaje de que la opción no es válida y mostrar opciones
    }
  }
}


boolean validateSender(String senderId){
  //Para cada ID de usuario que pueda interactuar con este bot
  for(int i=0; i < SENDER_ID_COUNT; i++) {
    //Si la identificación del remitente es parte de la matriz, devolvemos que es válida
    if(senderId == validSenderIds[i]) {
      return true;
    }
  }
 
  //Si llegaste aquí, significa que verificaste todos los identificadores
  //y no lo encontraste en la matriz
  return false;
}

//FUNCIONES PARA ENVIAR MENSAJE
void handleStart(String chatId, String fromName) {
  //Mostrar hola y nombre de contacto seguido de mensajes válidos
  String message = "<b>Hola " + fromName + ".</b>\n";

  message += getCommands();
  //Serial.println(message);
  bot.sendMessage(chatId, message, "HTML");
}


void sendMessageAll(){
  for(int i=0; i < SENDER_ID_COUNT; i++) {
      String senderId = validSenderIds[i];
      String message = "";
      message += "<b>Notificación de Apoyo Técnico</b>\n";
      message += "<b>Equipo: </b> Prototipo V2 Refrigerador \n";
      message += getClimateMessage() + "\n";
      message += "<b>Hospital: </b> HGZ No. 1 Tapachula, Chiapas \n";
      message += "<b>Reporte: </b> Puerta Abierta";
      
      bot.sendMessage(senderId, message, "HTML");
    }
 }

void handleDoor(String chatId){
  //Envia mensaje de estado de puerta
  bot.sendMessage(chatId, getStatusDoor(), "HTML");
}

void handleClimate(String chatId){
  bot.sendMessage(chatId, getClimateMessage(), "");
}

void handleStatus(String chatId) {
  String message = "";
  //Concatena el mensaje de la puerta
  message += getStatusDoor() + "\n";
 
  //Mensaje del clima
  message += getClimateMessage();
 
  //Envia el mensaje para el contacto
  bot.sendMessage(chatId, message, "HTML");
}

void handleActive(String chatId){
  if(activeAlarm == 0){
    activeAlarm = 1;
    bot.sendMessage(chatId, "La alarma de puerta a sido <b>desactivada</b>", "HTML");
   }else{
    activeAlarm = 0;
    bot.sendMessage(chatId, "La alarma de puerta a sido <b>activada</b>", "HTML");
   }
  
  
  }
void handleNotFound(String chatId) {
  //Enviar mensaje diciendo comando no encontrado y mostrar opciones de comando válidas
  String message = "Comando no encontrado\n";
  message += getCommands();
  //Serial.println(message);
  bot.sendMessage(chatId, message, "HTML");
}


//------------------------------------
//FUNCIONES DE OBTENCIÓN

String getClimateMessage() {
  //Funcion para obtener la temperatura y humedad.
  //float temperatura1, humedad1;
  temperatura1 = dht1.readTemperature();
  humedad1 = dht1.readHumidity();
  //int status = dht.read2(DHT_PIN, &temperature, &humidity, NULL);
 
  //Se foi bem sucedido
  if (isnan(temperatura1)){
     return "Error en la lectura de la temperatura";
  }else{
        //Retorna un string con los valores
    String message = "";
    message += "La temperatura es de: " + String(temperatura1)+ " °C \n";
    message += "La humedad es de: " + String(humedad1) + "%";
    return message;
  
   }
   
  //Si no tiene éxito, devuelve un mensaje de error
}

//Funcion para obtener el estado de la puerta
String getStatusDoor() {
  //Lectura del Pin de Puerta
  inputState = digitalRead(INPUT_PIN);
  String message = "";
  
   if(inputState == 1) {
    message += "Puerta <b>Abierta</b>";
   }else{
    message += "Puerta <b>Cerrada</b>";
   }
    return message;
 
}

//Funcion de comandos disponibles
String getCommands() {
  //Lista de comandos disponibles
  String message = "Los Comandos disponibles son:\n\n";
  message += "<b>" + DOOR + "</b>: Comando para verificar el sensor de puerta\n";
  message += "<b>" + ACTIVE + "</b>: Comando para activar o desactivar alarma de Telegram\n";
  message += "<b>" + CLIMATE + "</b>: Comando para verificar la temperatura y humedad del sensor\n";
  message += "<b>" + STATS + "</b>: Comando para verificar el estado de lo sensores";
  
  return message;
}

/*
 * //En pruebas da error.
void handleReboot(String chatId) {
  //Funcion de Reinicio del chip desde telegram
  String message = "Reiniciando Chip...";
  bot.sendMessage(chatId, message, "");
  ESP.restart();
}
*/

//-------------
//Funcion para envio de datos al host

void sensor_data() {
  //Check WiFi connection status
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    
    // Your Domain name with URL path or IP address with path
    http.begin(serverName);
    
    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    // Prepare your HTTP POST request data
    String httpRequestData = "api_key=" + apiKeyValue + "&sensor=" + sensorName
                          + "&location=" + sensorLocation + "&value1=" + String(temperatura1)
                          + "&value2=" + String(humedad1) + "&value3=" + String(temperatura2)
                          + "&value4=" + String(humedad2) + "&value5=" + String(inputState) + "";
    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);
   
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
     
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error de codigo: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Desconectado");
  }
}
