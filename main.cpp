/*  Mesh Connectivity         Active
    GSM Connectivity          Active
    UID                       Active
    Masoud
    June 9, 2021
*/

#include <Arduino.h>
#include "painlessMesh.h"
#include <Wire.h>
#include <SoftwareSerial.h>

// SerialControl pins of ESP32
#ifndef D5
#if defined(ESP8266)
#define D5 (14)
#define D6 (12)
#elif defined(ESP32)
#define D5 (18)
#define D6 (19)
#endif
#endif

#define   MESH_PREFIX     "ToossabAP"
#define   MESH_PASSWORD   "qwerty2580"
#define   MESH_PORT       5555

//Declaraing Vaiables
String SMS, part01, part02, part03, part04, part05, part06, part07, part08, part09;
String message, toDigit, nodeCodename;
String nodeIdentifier;
float airTemperature01, airHumidity01, airTemperature02, airHumidity02; String serverPower, doorLock, hvac01, hvac02;
int delivered = 0;
int signal, shouldResync = 0;

//Prototypying
String getValue(String data, char separator, int index);
char AsciiToChar( byte asciiCode );
void nodeTimeAdjustedCallback(int32_t offset);
void changedConnectionCallback();
void newConnectionCallback(uint32_t nodeId);
void receivedCallback( uint32_t from, String &msg );
void sendMessage();
void SendSMS(String Phone);
void parseSignal(int decimalToBinary);

//Create software serial object to communicate with SIM800
SoftwareSerial serialSIM800(D6,D5); // (Rx, Tx)
Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;
Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void setup() {
  //Begin serial comunication with Arduino and Arduino IDE (Serial Monitor) 
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();

  //Begin serial comunication with Arduino and Arduino IDE (Serial Monitor) 
  Serial.begin(115200);
  while(!Serial);
  //Being serial communication witj Arduino and SIM800
  serialSIM800.begin(9600);
  serialSIM800.println("AT");
  serialSIM800.println("AT+CREG=1");
  serialSIM800.println("AT+CMGF=1");
  serialSIM800.println("AT+CNMI=1,2,0,0,0");
  serialSIM800.println("AT+CSCS=\"GSM\"");
  //serialSIM800.println("AT+CSCS=\"HEX\"");
  //serialSIM800.println("AT+CSMP=49,167,0,8");
}

void loop() {
  char receivedSMS;
  char decodedSMS;
  
  if(serialSIM800.available()){
    receivedSMS = serialSIM800.read();  
    Serial.println(receivedSMS);
    decodedSMS = AsciiToChar( receivedSMS );
    SMS += decodedSMS;
  }
  
  constexpr  uint32_t interval    = 15000;      // interval at which to blink (milliseconds)
  static uint32_t nextMilli  = millis() + 15000;     // will store next time LED will updated
  if (millis() > nextMilli) {
    nextMilli += interval;             // save the next time you blinked the LED
    Serial.println(SMS);
    part01 = getValue(SMS,':',1);
    part02 = getValue(part01,'"',1);
    part03 = getValue(SMS,'"',5);
    part04 = getValue(SMS,'"',6);
    part05 = getValue(SMS,':',0);
    part06 = getValue(part05,'=',0);
    part07 = getValue(SMS,'=',1);
    part08 = getValue(part06,'+',1);
    Serial.println("Message is: ");
    Serial.println(SMS);
    Serial.println("Part One is: ");
    Serial.println(part01);
    Serial.println("Part Two is: ");
    Serial.println(part02);
    Serial.println("Part Three is: ");
    Serial.println(part03);
    Serial.println("Part Four is: ");
    Serial.println(part04);
    Serial.println("Part Five is: ");
    Serial.println(part05);
    Serial.println("Part Six is: ");
    Serial.println(part06);
    Serial.println("Part Seven is: ");
    Serial.println(part07);
    Serial.println("Part Eight is: ");
    Serial.println(part08);
    if(part04 == "\r\n1\r\n"){
        Serial.println("Good Job!");
        SendSMS(part02);
    }
    if(part04 == "\r\n0031\r\n"){
        Serial.println("Good Job!");
        SendSMS(part02);
    }
    if(part04 == "\r\n06F1\r\n"){
        Serial.println("Good Job!");
        SendSMS(part02);
    }
    if(part08 == "CREG"){
        Serial.println("Bootstraped!\nClearig Buffer...");
        SMS = "";
    }
    if(part08 == "CMGF"){
        Serial.println("Message Sent!\nClearig Buffer...");
        SMS = "";
    }
  SMS = "";
  //Serial.println(array);
  }
  // it will run the user scheduler as well
  mesh.update();
}

void sendMessage() {
  String msg = "Node535,";
  msg += mesh.getNodeId();
  msg += ",";
  msg += ESP.getChipId();
  msg += ",";
  msg += part02;
  msg += ",";
  msg += part04;
  msg += "\r\n";
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  message = msg;
  nodeIdentifier = getValue(message,',',2);
  Serial.println(nodeIdentifier);
  if (nodeIdentifier == "10207467") {
    toDigit = getValue(message, ',', 3);
    signal = toDigit.toInt();
    parseSignal(signal);
    shouldResync = signal % 2;
    //Serial.println(shouldResync);
    if (shouldResync) ESP.wdtDisable();
  }
  if (nodeIdentifier == "11647896") {
    toDigit = getValue(message,',',3);
    airTemperature01 = toDigit.toFloat();
    toDigit = getValue(message,',',4);
    airHumidity01 = toDigit.toFloat();
    Serial.print(airTemperature01);
    Serial.print(", ");
    Serial.println(airHumidity01);
  }
  if (nodeIdentifier == "14538556") {
    toDigit = getValue(message,',',3);
    airTemperature02 = toDigit.toFloat();
    toDigit = getValue(message,',',4);
    airHumidity02 = toDigit.toFloat();
    Serial.print(airTemperature02);
    Serial.print(", ");
    Serial.println(airHumidity02);
  }
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

char AsciiToChar( byte asciiCode )   { return (asciiCode +'\0' ); }

void parseSignal(int decimalToBinary) {
  uint8_t bitsCount = sizeof(decimalToBinary) * 8;
  char str[ bitsCount + 1 ];
  itoa( decimalToBinary, str, 2 );
  Serial.println( str );
  Serial.println( str[0] );
  Serial.println( str[1] );
  Serial.println( str[2] );
  Serial.println( str[3] );
  if(str[0] == '1'){
    hvac02 = "On";
  } else if(str[0] == '0'){
    hvac02 = "Off";
  }
  Serial.println( str[4] );
  if(str[0] == '1'){
    hvac01 = "On";
  } else if(str[0] == '0'){
    hvac01 = "Off";
  }
  Serial.println( str[5] );
  if(str[0] == '1'){
    doorLock = "Closed";
  } else if(str[0] == '0'){
    doorLock = "Open";
  }
  Serial.println( str[6] );
  if(str[0] == '1'){
    serverPower = "Normal";
  } else if(str[0] == '0'){
    serverPower = "Emergency";
  }
  Serial.println( str[7] );
//  return 0;
}

void SendSMS(String Phone)
{
  Serial.println("Sending SMS...");               //Show this message on serial monitor
  serialSIM800.print("AT+CMGF=1\r");                   //Set the module to SMS mode
  delay(100);
  serialSIM800.print("\n\rAT+CMGS=\"" + Phone + "\"\r\n");  //Your phone number don't forget to include your country code, example +212123456789"
  delay(500);
  serialSIM800.print("Temp01: ");
  delay(500);
  serialSIM800.println(airTemperature01);
  delay(500);
  serialSIM800.print("Humidity01: ");
  delay(500);
  serialSIM800.println(airHumidity01);
  delay(500);
  serialSIM800.print("Temp02: ");
  delay(500);
  serialSIM800.println(airTemperature02);
  delay(500);
  serialSIM800.print("Humidity02: ");
  delay(500);
  serialSIM800.println(airHumidity02);
  delay(500);
  serialSIM800.print("Power: ");
  delay(500);
  serialSIM800.println(serverPower);
  delay(500);
  serialSIM800.print("Door: ");
  delay(500);
  serialSIM800.println(doorLock);
  delay(500);
  serialSIM800.print("HVAC01: ");
  delay(500);
  serialSIM800.println(hvac01);
  delay(500);
  serialSIM800.print("HVAC02: ");
  delay(500);
  serialSIM800.println(hvac02);
  delay(500);
  serialSIM800.print("smart.angizehco.com");
//  serialSIM800.print("\n\rSIM800l is working\n\r");       //This is the text to send to the phone number, don't make it too long or you have to modify the SoftwareSerial buffer
//  delay(500);
//  serialSIM800.write(0x1A);
  serialSIM800.print((char)26);// (required according to the datasheet)
  delay(500);
  serialSIM800.println();
  Serial.println("Text Sent.");
  delay(500);
}

// String  var = getValue( StringVar, ',', 2); // if  a,4,D,r  would return D        
String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length();

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}  // END
//=======================================================================