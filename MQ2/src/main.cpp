#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

//Khai báo nguyên mẫu các hàm
void BeginOrCreateNewDevice();
void IRAM_ATTR ChangeWiFiAndEdgeIP();
void DetermineTheNumberOfProvisionTimes();
void DetermineValueChangeWiFiAndEdgeIP();
void GetInforPRVOrNewWiFiAndNewIPEdge();
void ScanWiFi();
void APModeWifi();
void SendWiFiListPByUDP();
void ReceiveInforPRVOrNewWiFiAndNewIPEdgeByUDP();
void SaveWiFiAndEdgeIP();
void ConnectToWiFi();
void SetupProvision();
void ProvisionWithEdgeOrThings();
void Provision(const char*);
void BlinkLedProvision();
void SendProvisionRequest(String, const char*);
void HandleProvisionRespone(const char*, byte*, unsigned int);
void SaveCredentialsAndNumberProvision();
void DisconnectAfterProvision();
void ReconnectThingsBoardDemo();
void ConnectEdge();
void ReadGas();
void SendGasThingsBoard();
void SendGasEdge();
void OTA();
String ipToString(IPAddress);

//Khai báo hằng số, cờ, biến
#define ThingsBoardDemoHost "demo.thingsboard.io"
#define Port 1883
#define UsenameProvision "provision"
#define DeviceID "MQ2"
#define Pass "123"
#define LedRed 19
#define LedGreen 18
#define Button 17
#define Coi 16
#define MQ2A 39
#define MQ2D 36

#define DevicePort 65001
#define AppPort 65001
#define ApSSID "ESP32WiFi"
#define ApPassword "12345678"
String wifiSSID;
String wifiPassword;
String EdgeIP;
String DeviceName;
String DeviceKey;
String DeviceSecret;
String StatusUdp = "Nothing";
String WiFiList;

int n;
int FlagEdgeIP = 3;
int ReconnectTB;
uint16_t MQ2Analog;
uint16_t MQ2Digital;
String StatusGas;
String StatusPRV;
String Access;
char MQ2[100];
char RSSIArray[50];
int ReconnectThingsBoard;
volatile int Change;

//Khai báo đối tượng Wifi, PubSubClient, dht, lcd, preferences
WiFiUDP udp;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Preferences preferences;
AsyncWebServer server(80);
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
/*------------------------------------*/

void setup() {
  Serial.begin(115200);
  pinMode(Button, INPUT_PULLUP);
  pinMode(LedRed, OUTPUT);
  pinMode(LedGreen, OUTPUT);
  pinMode(Coi, OUTPUT);
  pinMode(MQ2D, INPUT);
  BeginOrCreateNewDevice();
  DetermineTheNumberOfProvisionTimes();
  DetermineValueChangeWiFiAndEdgeIP();
  GetInforPRVOrNewWiFiAndNewIPEdge();
  ConnectToWiFi();
  OTA ();
  SetupProvision();
  mqttClient.setCallback(HandleProvisionRespone);
  attachInterrupt(Button, ChangeWiFiAndEdgeIP, FALLING);
}

void loop() {
  mqttClient.loop();
  if (n == 1) {
    SaveCredentialsAndNumberProvision();
  }
  if ((FlagEdgeIP == 0) || (FlagEdgeIP == 1)) {
     DisconnectAfterProvision();
  }
  if(((n == 2) && (FlagEdgeIP == 2)) || ((n == 2) && (FlagEdgeIP == 3))) {
    ReconnectThingsBoardDemo();
  }
  //mqttClient.loop();
}


/*---------Định nghĩa hàm-----------*/
void BeginOrCreateNewDevice() {
  digitalWrite(LedGreen, HIGH);
  Serial.println("Thiết lập thiết bị, chờ trong giây lát");
  uint32_t Time = millis();
  while (millis() - Time < 20000) {
    if (digitalRead(Button) == LOW) {
      delay(5000);
      if (digitalRead(Button) == LOW) {
        digitalWrite(LedRed, HIGH);
        Serial.println("Xóa hết dữ liệu được lưu trữ trong Flash");
        preferences.begin("Provision", false);
        preferences.clear();
        delay(2000);
        preferences.end();
        Serial.println("Đã xóa hết dữ liệu được lưu trữ trong Flash. Tạo mới thiết bị");
        digitalWrite(LedRed, LOW);
        ESP.restart();
      }
    }
  }
  Serial.println("Quá trình thiết lập hoàn tất, Khởi động thiết bị");
  digitalWrite(LedGreen, LOW);
}



void IRAM_ATTR ChangeWiFiAndEdgeIP() {
  portENTER_CRITICAL_ISR(&mux);
  //int State = HIGH;
  while (digitalRead(Button) != HIGH) {
    uint32_t StartTime = millis();
    while (millis() - StartTime <50) {
    }
  }
  Serial.println("Vào hàm ngắt");
  Change = 1;
  portEXIT_CRITICAL_ISR(&mux);
}



void DetermineValueChangeWiFiAndEdgeIP() {
 Serial.println("Get value Change from Flash");
 preferences.begin("Changeee", false);
 Change = preferences.getInt("Change", 0);
 preferences.end();
 Serial.println("Gia tri cua Change la: ");
 Serial.println(Change);
 //delay(100);
}



void DetermineTheNumberOfProvisionTimes() {
 Serial.println("Get the number of provision times from Flash memory");
 preferences.begin("Provision", false);
 n = preferences.getInt("n", 1);
 preferences.end();
 Serial.println("Gia tri cua n la: ");
 Serial.println(n);
 delay(1000);
}



void GetInforPRVOrNewWiFiAndNewIPEdge() {
  if (((n == 1) && (Change == 0))) {
    Serial.println("Get WiFi, EdgeIP, DeviceKey, DeviceScret from App");
    ScanWiFi();
    APModeWifi();
    SendWiFiListPByUDP();
    ReceiveInforPRVOrNewWiFiAndNewIPEdgeByUDP();
    SaveWiFiAndEdgeIP();
  } else if ((n == 2) && (Change == 0)) {
    Serial.println("Get WiFi, EdgeIP, AcessToken from Flash");

    preferences.begin("WiFi", false);
    wifiSSID = preferences.getString("wifiSSID", "N");
    wifiPassword = preferences.getString("wifiPassword", "N");
    preferences.end();

    preferences.begin("InforEdge", false);
    EdgeIP = preferences.getString("EdgeIP", "N");
    preferences.end();

    preferences.begin("Credentials", false);
    Access = preferences.getString("Access", "N");
    preferences.end();

    Serial.println(wifiSSID);
    Serial.println(wifiPassword);
    Serial.println(EdgeIP);
    Serial.println(Access);
    attachInterrupt(Button, ChangeWiFiAndEdgeIP, FALLING);
  } else if ((n == 1) && (Change == 1)) {
    Change = 0;
    preferences.begin("Changeee", false);
    preferences.putInt("Change", Change);
    preferences.end();
    Serial.println("Save Change = 0 vào Flash");
    ESP.restart();
  } else if ((n == 2) && (Change == 1)) {
    Serial.println("Get New WiFi and New EdgeIP from App");
    ScanWiFi();
    APModeWifi();
    SendWiFiListPByUDP();
    ReceiveInforPRVOrNewWiFiAndNewIPEdgeByUDP();
    SaveWiFiAndEdgeIP();
    attachInterrupt(Button, ChangeWiFiAndEdgeIP, FALLING);
  }
}



void ScanWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);
  Serial.println("Scan start");
  digitalWrite(LedGreen, HIGH);
  int NumberNetwork = 0;
  while (NumberNetwork == 0) {
    Serial.println("Scanning WiFi");
    NumberNetwork = WiFi.scanNetworks();
    delay(1000);
  }
  digitalWrite(LedGreen, LOW);
  Serial.println("Scan done");
  Serial.print(NumberNetwork);
  Serial.println(" Networks found");
  for (int i = 0; i < NumberNetwork; ++i) {
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print(")");
    WiFiList += WiFi.SSID(i) + ",";
  }
  WiFiList = WiFiList + "WIFI";
  Serial.println("\n");
  Serial.println("WiFiList thu được là:");
  Serial.println(WiFiList);
  delay(2000);
}



void APModeWifi () { 
  Serial.println("Start WiFi AP Mode");
  digitalWrite(LedGreen, HIGH);
  WiFi.softAP(ApSSID, ApPassword);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());
  DeviceName = DeviceID + WiFi.macAddress();
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Device Name: ");
  Serial.println(DeviceName);
  digitalWrite(LedRed, HIGH);
  while (WiFi.softAPgetStationNum() == 0) {
    delay(1000);
  }
  digitalWrite(LedRed, LOW);
  Serial.print("Number of Connected Devices: ");
  Serial.println(WiFi.softAPgetStationNum());
  //delay(100);
}



void SendWiFiListPByUDP() {
  udp.begin(DevicePort);
  uint32_t StartTime = millis();
  while((millis() - StartTime) < 20000) {
  }
  digitalWrite(LedRed, HIGH);
  Serial.println("Send WiFi list");
  uint8_t BufferUDPSend[WiFiList.length() + 1];
  WiFiList.getBytes(BufferUDPSend, WiFiList.length() + 1);
  udp.beginPacket("255.255.255.255", AppPort);
  udp.write(BufferUDPSend, sizeof(BufferUDPSend));
  udp.endPacket();
  Serial.println("Send WiFi list Done");
  delay(1000);
  digitalWrite(LedRed, LOW);
  delay(1000);
}



void ReceiveInforPRVOrNewWiFiAndNewIPEdgeByUDP() {
  Serial.println("Receiving Information Provision");
  digitalWrite(LedRed, HIGH);
  while (!udp.parsePacket()) {
    Serial.println("Have No Packet");
    delay(1000);
  }
  digitalWrite(LedRed, LOW);
  Serial.println("Have Packet");
  char BufferUDPReceive[800];
  int len = udp.read(BufferUDPReceive, sizeof(BufferUDPReceive));
  BufferUDPReceive[len] = '\0'; 

  StaticJsonDocument<500> dataUdp;
  DeserializationError error = deserializeJson (dataUdp, BufferUDPReceive);

  if(error){  
   Serial.println("Error deserialize Json");
   return;
  }
  
  if ((n == 1) && (Change == 0)) {
    wifiSSID = dataUdp["SSID"].as<String>();
    wifiPassword = dataUdp["PASS"].as<String>();
    EdgeIP = dataUdp["EDGEIP"].as<String>();
    DeviceKey = dataUdp["DEVICEKEY"].as<String>();
    DeviceSecret = dataUdp["DEVICESECRET"].as<String>();
    StatusUdp = dataUdp["STATUS"].as<String>();
  } else if ((n == 2) && (Change == 1)) {
    wifiSSID = dataUdp["SSID"].as<String>();
    wifiPassword = dataUdp["PASS"].as<String>();
    EdgeIP = dataUdp["EDGEIP"].as<String>();
    preferences.begin("Credentials", false);
    Access = preferences.getString("Access", "N");
    DeviceKey = preferences.getString("DeviceKey", "N");
    DeviceSecret = preferences.getString("DeviceSecret", "N");
    preferences.end();
    StatusUdp = dataUdp["STATUS"].as<String>();
  }
  while ((!(StatusUdp == "SUCCESS")) || (EdgeIP.length() > 15) || (DeviceKey.length() != 20) || (DeviceSecret.length() != 20)) {
    Serial.println("Thông tin Provision không chính xác, thực hiện gửi nhận lại");
    digitalWrite(LedRed, HIGH);
    delay(1000);
    digitalWrite(LedGreen, LOW);
    digitalWrite(LedRed, LOW);
    delay(1000);
    digitalWrite(LedGreen, HIGH);
    digitalWrite(LedRed, HIGH);
    delay(1000);
    digitalWrite(LedGreen, LOW);
    digitalWrite(LedRed, LOW);
    delay(500);
    digitalWrite(LedGreen, HIGH);
    ESP.restart();
  }
  digitalWrite(LedGreen, LOW);
  delay(100);
  if ((n == 1) && (Change == 0)) {
    Serial.println("Information was received from App");
    Serial.println(wifiSSID);
    Serial.println(wifiPassword);
    Serial.println(EdgeIP);
    Serial.println(DeviceKey);
    Serial.println(DeviceSecret);
    Serial.println(StatusUdp);
    WiFi.softAPdisconnect();
    Serial.println("Ended WiFi AP Mode");
    //delay(100);
  } else if ((n == 2) && (Change == 1)) {
    Serial.println("New WiFi and New IP Edge was received from App");
    Serial.println(wifiSSID);
    Serial.println(wifiPassword);
    Serial.println(EdgeIP);
    Serial.println(StatusUdp);
    if (Change == 1) {
    Change = 0;
    preferences.begin("Changeee", false);
    preferences.putInt("Change", Change);
    preferences.end();
    Serial.println("Save Change = 0 vào Flash");
    }
    WiFi.softAPdisconnect();
    Serial.println("Ended WiFi AP Mode");
    //delay(100);
  }
}



void SaveWiFiAndEdgeIP () {
  preferences.begin("WiFi", false);
  preferences.putString("wifiSSID", wifiSSID);
  preferences.putString("wifiPassword", wifiPassword);
  preferences.end();
  preferences.begin("InforEdge", false);
  preferences.putString("EdgeIP", EdgeIP);
  preferences.end();
  Serial.println("Save WiFi, EdgeIP Done");
  delay(1000);
}



void ConnectToWiFi() {
  Serial.printf("Connecting to %s...\n", wifiSSID);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LedRed, HIGH);
    delay(3000);
    digitalWrite(LedRed, LOW);
    WiFi.begin(wifiSSID, wifiPassword);
    if (Change == 1) {
    Serial.println("Bắt đầu thay đổi WiFi, EdgeIP");
    preferences.begin("Changeee", false);
    preferences.putInt("Change", Change);
    preferences.end();
    Serial.println("Save Change = 1 vào Flash");
    delay(500);
    ESP.restart();
    }
    delay(1000);
  }
  Serial.printf("Connected to ");
  Serial.println(wifiSSID);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LedGreen, HIGH);
  delay(3000);
  digitalWrite(LedGreen, LOW);
  delay(1000);
}



void SetupProvision() {
if (n == 1) {
  while (digitalRead(Button) == HIGH) {
    digitalWrite(LedRed, HIGH);
    delay (1000);
    digitalWrite(LedRed, LOW);
    delay (1000);
    digitalWrite(LedGreen, HIGH);
    delay (1000);
    digitalWrite(LedGreen, LOW);
    delay (1000);
   }
  ProvisionWithEdgeOrThings();
 } else if (n == 2) {
    digitalWrite(LedGreen, HIGH);
    digitalWrite(LedRed, HIGH);
    delay(5000);
    digitalWrite(LedGreen, LOW);
    digitalWrite(LedRed, LOW);
    delay(1000);
    mqttClient.setCallback(HandleProvisionRespone);
    ReconnectThingsBoardDemo();
 }
}



void ProvisionWithEdgeOrThings(){
  if (EdgeIP == "NoEdge"){
    FlagEdgeIP = 0;
  } else {
    FlagEdgeIP = 1;
  }
  Serial.print("Gia tri cua FlagEdgeIP la: ");
  Serial.println(FlagEdgeIP);
  /*if (FlagEdgeIP == 1){
    digitalWrite(LedRed, HIGH);
    delay(3000);
    Provision(EdgeIP.c_str());
    digitalWrite(LedGreen, LOW);
    digitalWrite(LedRed, LOW);
    delay(2000);
  } else if (FlagEdgeIP == 0) {
    digitalWrite(LedGreen, HIGH);
    delay(3000);
    Provision(ThingsBoardDemoHost);
    digitalWrite(LedRed, LOW);
    digitalWrite(LedGreen, LOW);
    delay(2000);
  }*/
  digitalWrite(LedGreen, HIGH);
  delay(3000);
  Provision(ThingsBoardDemoHost);
  digitalWrite(LedGreen, LOW);
  delay(2000);
}



void Provision(const char* Host) {
  Serial.printf("Provisioning with: %s\n", Host);

  DynamicJsonDocument provisioningRequestJson(256);
  provisioningRequestJson["deviceName"] = DeviceName;
  provisioningRequestJson["provisionDeviceKey"] = DeviceKey;
  provisioningRequestJson["provisionDeviceSecret"] = DeviceSecret;

  String provisioningRequestString;
  serializeJson(provisioningRequestJson, provisioningRequestString);

  SendProvisionRequest(provisioningRequestString, Host);
  while(!(mqttClient.subscribe("/provision/response"))) {
    Serial.println("Subscribing provisioning topic");
    while(WiFi.status() != WL_CONNECTED) {
      ConnectToWiFi();
    }
    BlinkLedProvision();
  }
  Serial.println("Subscribed provisioning topic ");
}



void SendProvisionRequest(String requestString, const char* Host) {
  Serial.printf("Connecting to %s\n", Host);
  mqttClient.setServer(Host, Port);
  while (!(mqttClient.connect(DeviceID, UsenameProvision, Pass))) {
    while (WiFi.status() != WL_CONNECTED) {
      ConnectToWiFi();
    }
    Serial.printf("Connecting %s\n", Host);
    BlinkLedProvision();
  }
  Serial.printf("Connected to %s\n",Host);
  delay(100);
  while(!(mqttClient.publish("/provision/request", requestString.c_str()))) {
    Serial.println("Sending provisioning request");
    while(WiFi.status() != WL_CONNECTED) {
      ConnectToWiFi();
    }
    BlinkLedProvision();
  }
  Serial.println("Sent provisioning request Successfully");
  delay(100);
}



void HandleProvisionRespone(const char* topic, byte* payload, unsigned int length) { 
  StaticJsonDocument<500> data1;
  StaticJsonDocument<300> data2;
  if (strcmp(topic, "/provision/response") == 0) {
    char MessageJson1[length + 1];
    strncpy(MessageJson1, (char*)payload, length);
    Serial.println("Chuoi JSON nhan duoc la:");
    Serial.println(MessageJson1);
    MessageJson1[length] = '\0';
    DeserializationError error = deserializeJson (data1, MessageJson1);
    if(error){  
    Serial.println("Error deserialize Json");
    return;
    }
    StatusPRV = data1["status"].as<String>();
    Access = data1["credentialsValue"].as<String>();
    if (!(StatusPRV == "SUCCESS")) {
      Serial.println("Provision không thành công, kiểm tra lại thông tin và thực hiện lại quá trình Provision");
      delay(1000);
      ESP.restart();
    }
  } else if (strcmp(topic, "/provision/response") != 0) {
    char MessageJson2[length + 1];
    strncpy(MessageJson2, (char*)payload, length);
    Serial.println("Chuoi JSON nhan duoc la:");
    Serial.println(MessageJson2);
    MessageJson2[length] = '\0';
    DeserializationError error = deserializeJson (data2, MessageJson2);
    if(error){  
    Serial.println("Error deserialize Json");
    return;
    }
    String MethodName = data2["method"].as<String>();
    ReconnectThingsBoard = data2["params"]["relation"];
  }
}



void SaveCredentialsAndNumberProvision() {
  if((n == 1) && (StatusPRV == "SUCCESS")){
  Serial.println("Thong tin nhan dc sau qua trinh Provision la:");
  Serial.println(StatusPRV);
  Serial.println(Access);
  n = 2;
  Serial.println("Save Number Provision, Access Token, DeviceKey, DeviceScret to Flash");
  preferences.begin("Credentials", false);
  preferences.putString("Access", Access);
  preferences.putString("DeviceKey", DeviceKey);
  preferences.putString("DeviceSecret", DeviceSecret);
  preferences.end();
  preferences.begin("Provision", false);
  preferences.putInt("n", n);
  preferences.end();
  Serial.println("Save Done");
  }
}



void DisconnectAfterProvision() {
  if (((StatusPRV == "SUCCESS") || (StatusPRV == "FAILURE")) && ((FlagEdgeIP == 1) || (FlagEdgeIP == 0))) {
    mqttClient.unsubscribe("/provision/response");
    Serial.println("Unsubscribe Provision Topic");
    mqttClient.disconnect();
    Serial.println("Disconnect after Provision and Reconnect");
    FlagEdgeIP = 2;
    delay(2000);
  }
}



void BlinkLedProvision() {
  if (FlagEdgeIP == 1) {
    digitalWrite(LedGreen, HIGH);
    delay(500);
    digitalWrite(LedGreen, LOW);
    delay(500);
  } else if(FlagEdgeIP == 0) {
    digitalWrite(LedRed, HIGH);
    delay(500);
    digitalWrite(LedRed, LOW);
    delay(500);
  }
}



void ReconnectThingsBoardDemo() {
 digitalWrite(LedGreen, HIGH);
 delay(2000);
 digitalWrite(LedGreen, LOW);
 delay(2000);
 digitalWrite(LedGreen, HIGH);
 delay(2000);
 digitalWrite(LedGreen, LOW);
 delay(2000);
 ReconnectThingsBoard = 0;
 if (Change == 1) {
   Serial.println("Bắt đầu thay đổi WiFi, EdgeIP");
   preferences.begin("Changeee", false);
   preferences.putInt("Change", Change);
   preferences.end();
   Serial.println("Save Change = 1 vào Flash");
   delay(500);
   ESP.restart();
 }
 Serial.println("Reconnect ThingsBoardDemo");
 mqttClient.setServer(ThingsBoardDemoHost, Port);
 int Attempt = 1;
  while ((!mqttClient.connect(DeviceID, Access.c_str(), Pass)) && (Attempt <= 5)) {
    Attempt = Attempt + 1;
    digitalWrite(LedRed, HIGH);
    delay(500);
    digitalWrite(LedRed, LOW);
    delay(500);
    while (WiFi.status() != WL_CONNECTED) {
      ConnectToWiFi();
    }
    //delay(1000);
    Serial.println("Connecting ThingsBoardDemo");
  }
  if(mqttClient.connected()) {
    Serial.println("Connected ThingsBoard");
    digitalWrite(LedGreen, HIGH);
    String MAC = WiFi.macAddress();
    String BSSID = WiFi.BSSIDstr();
    String SSID = wifiSSID;
    IPAddress IP = WiFi.localIP();
    String IPString = ipToString(IP);
    String InforWiFi = "{\"MAC Address\":\"" +MAC+"\""+",\"BSSID\":\"" +BSSID+"\""+",\"SSID\":\"" +SSID+"\""+",\"Local IP\":\""+IPString+"\""+"}";
    Serial.println(InforWiFi);
    char InforWiFiArray[150];
    InforWiFi.toCharArray(InforWiFiArray, InforWiFi.length()+1);
    mqttClient.publish("v1/devices/me/attributes", InforWiFiArray);
    ReadGas();
    SendGasThingsBoard();
  } else if ((!mqttClient.connected()) && (EdgeIP == "NoEdge")) {
    Serial.println("Connect ThingsBoard Failure");
    digitalWrite(LedGreen, LOW);
    mqttClient.disconnect();
    Serial.println("Disconnect ThingsBoardDemo");
    return;
  } else if ((!mqttClient.connected()) && (!(EdgeIP == "NoEdge"))) {
    delay(1000);
    digitalWrite(LedRed, HIGH);
    delay(2000);
    digitalWrite(LedRed, LOW);
    delay(2000);
    digitalWrite(LedRed, HIGH);
    delay(2000);
    digitalWrite(LedRed, LOW);
    delay(2000);
    mqttClient.disconnect();
    Serial.println("Disconnect ThingsBoardDemo");
    if (Change == 1) {
    Serial.println("Bắt đầu thay đổi WiFi, EdgeIP");
    preferences.begin("Changeee", false);
    preferences.putInt("Change", Change);
    preferences.end();
    Serial.println("Save Change = 1 vào Flash");
    delay(500);
    ESP.restart();
    }
    ConnectEdge();
  }
}



void ReadGas() {
  MQ2Analog = analogRead(MQ2A);
  Serial.println(MQ2Analog);
  while (MQ2Analog == 0) {
    Serial.println("Failed to read from MQ2 sensor!");
    digitalWrite(LedRed, HIGH);
    digitalWrite(LedGreen, HIGH);
    delay(3000);
    digitalWrite(LedRed, LOW);
    digitalWrite(LedGreen, LOW);
    delay(3000);
    //dht.begin();
    MQ2Analog= analogRead(MQ2A);
    if (Change == 1) {
    Serial.println("Bắt đầu thay đổi WiFi, EdgeIP");
    preferences.begin("Changeee", false);
    preferences.putInt("Change", Change);
    preferences.end();
    Serial.println("Save Change = 1 vào Flash");
    delay(500);
    ESP.restart();
    }
  }
  if(MQ2Analog >= 1500) {
    digitalWrite(Coi, HIGH);
    StatusGas = "Nguy Hiểm";
  } else {
    digitalWrite(Coi, LOW);
    StatusGas = "An toàn";
  }
  delay(1000);
  int RSSI = WiFi.RSSI();
  String RSSIString = "{\"RSSI\":" + String(RSSI) +"}";
  RSSIString.toCharArray(RSSIArray, RSSIString.length()+1);
  String Gas = "{\"Gas\":\"" + String(StatusGas) + "\"" + "}";
  Gas.toCharArray(MQ2, Gas.length()+1);
}



void SendGasThingsBoard() {
  //mqttClient.connect(WifiDeviceID, Access.c_str(), Pass);
  while((mqttClient.connected()) && (mqttClient.publish("v1/devices/me/telemetry", MQ2))) {
    mqttClient.publish("v1/devices/me/attributes", RSSIArray);
    if (Change == 1) {
    Serial.println("Bắt đầu thay đổi WiFi, EdgeIP");
    preferences.begin("Changeee", false);
    preferences.putInt("Change", Change);
    preferences.end();
    Serial.println("Save Change = 1 vào Flash");
    delay(500);
    ESP.restart();
    }
    //mqttClient.loop();
    //Serial.println(mqttClient.publish("v1/devices/me/telemetry", HumTemJS));
    Serial.println("Sent Gas to ThingsBoardDemo Success");
    uint32_t StartTimee = millis();
    while(millis() - StartTimee < 10000) {
    }
      //chu kì đọc và gửi HumTem
    mqttClient.loop();
    ReadGas();
    //mqttClient.loop();
  }
  Serial.println("Sent Gas to ThingsBoardDemo Failure");
  digitalWrite(LedGreen, LOW);
  mqttClient.disconnect();
  Serial.println("Disconnect ThingsBoardDemo");
  return;
}



void ConnectEdge() {
 Serial.println("Connect Edge");
 mqttClient.setServer(EdgeIP.c_str(), Port);
 int Attempt = 1;
  while ((!mqttClient.connect(DeviceID, Access.c_str(), Pass)) && (Attempt <= 5)) {
    Attempt = Attempt + 1;
    digitalWrite(LedGreen, HIGH);
    delay(500);
    digitalWrite(LedGreen, LOW);
    delay(500);
    while (WiFi.status() != WL_CONNECTED) {
      ConnectToWiFi();
    }
    Serial.println("Connecting Edge");
  }
  if(mqttClient.connected()) {
    digitalWrite(LedRed, HIGH);
    while (!mqttClient.subscribe("v1/devices/me/rpc/request/+")) {
      delay(500);
    }
    Serial.println("Subscribe ReconnectThingsBoad topic Done");
    String MACE = WiFi.macAddress();
    String BSSIDE = WiFi.BSSIDstr();
    String SSIDE = wifiSSID;
    IPAddress IPE = WiFi.localIP();
    String IPStringE = ipToString(IPE);
    String InforWiFiE = "{\"MAC Address\":\"" +MACE+"\""+",\"BSSID\":\"" +BSSIDE+"\""+",\"SSID\":\"" +SSIDE+"\""+",\"Local IP\":\""+IPStringE+"\""+"}";
    Serial.println(InforWiFiE);
    char InforWiFiArrayE[150];
    InforWiFiE.toCharArray(InforWiFiArrayE, InforWiFiE.length()+1);
    mqttClient.publish("v1/devices/me/attributes", InforWiFiArrayE);
    ReadGas();
    mqttClient.loop();
    Serial.print("Gia tri cua ReconnectThingsBoard la: ");
    Serial.println(ReconnectThingsBoard);
    if(ReconnectThingsBoard == 1) {
      digitalWrite(LedRed, LOW);
      mqttClient.disconnect();
      Serial.println("Disconnect Edge");
      return;
    } else {
      SendGasEdge();
    }
  } else {
    digitalWrite(LedRed, LOW);
    mqttClient.disconnect();
    Serial.println("Disconnect Edge");
    return;
  }
}



void SendGasEdge() {
  //mqttClient.connect(WifiDeviceID, Access.c_str(), Pass);
  while((mqttClient.connected()) && mqttClient.publish("v1/devices/me/telemetry", MQ2)) {
    mqttClient.publish("v1/devices/me/attributes", RSSIArray);
    if (Change == 1) {
    Serial.println("Bắt đầu thay đổi WiFi, EdgeIP");
    preferences.begin("Changeee", false);
    preferences.putInt("Change", Change);
    preferences.end();
    Serial.println("Save Change = 1 vào Flash");
    delay(500);
    ESP.restart();
    }
    //Serial.println(mqttClient.publish("v1/devices/me/telemetry", HumTemJS));
    Serial.println("Sent Gas to Edge Success");
    uint32_t StartTimeee = millis();
    while (millis() - StartTimeee < 10000) {
    }
    mqttClient.loop();
    ReadGas();
    Serial.print("Gia tri cua ReconnectThingsBoard la: ");
    Serial.println(ReconnectThingsBoard);
    if(ReconnectThingsBoard == 1) {
    digitalWrite(LedRed, LOW);
    mqttClient.disconnect();
    Serial.println("Disconnect Edge");
    return;
    }
  }
  Serial.println("Sent Gas to Edge Failure");
  digitalWrite(LedRed, LOW);
  mqttClient.disconnect();
  Serial.println("Disconnect Edge");
  return;
}



String ipToString(IPAddress ip) {
  String result = "";
  for (int i = 0; i < 4; i++) {
    result += String(ip[i]);
    if (i < 3) result += ".";
  }
  return result;
}



void OTA() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "tren 3.2 Web, co lennn");
  });
  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}