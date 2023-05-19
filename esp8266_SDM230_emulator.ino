#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

SoftwareSerial SoftSerial(D3, D2);


// Update these with values suitable for your network.

const char* ssid = "Your SSID";
const char* password = "YourWiFiPassword";
const char* mqtt_server = "192.168.0.1"; //Your MQTT Server's IP

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

float gridpower = 100;
bool receivedMessage = false;


uint16_t calc_crc(uint8_t* data, uint8_t length) {
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < length; pos++) {
    crc ^= data[pos];
    for (int i = 0; i < 8; i++) {
      if ((crc & 1) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.hostname("SDM230_growatt");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  payload[length] = '\0';
  String message_buff = String((char*)payload);
  gridpower = message_buff.toInt();
  receivedMessage = true;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("garage/temp", "start");
      // ... and resubscribe
      client.subscribe("shellies/contatore/emeter/0/power");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(9600);

  setup_wifi();

    client.setServer(mqtt_server, 1883);
  client.setCallback(callback);


  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  String IP = WiFi.localIP().toString();
  char IP_char[IP.length() + 1];
  IP.toCharArray(IP_char, IP.length() + 1);
  client.publish("SDM230_growatt/IP", IP_char);

  SoftSerial.begin(9600, SWSERIAL_8N1);
  pinMode(D0, OUTPUT);  //HIGH TX, LOW RX
  digitalWrite(D0, LOW);
}

void sendPowerToGrowatt() {
  pinMode(D0, OUTPUT);  //HIGH TX, LOW RX
  digitalWrite(D0, LOW);
  delay(10);
  if (SoftSerial.available()) {
    static uint8_t recbuffer[8];
    static uint8_t recbufferIndex = 0;

    recbuffer[recbufferIndex++] = SoftSerial.read();
    recbufferIndex %= 8;

    if (calc_crc(recbuffer, 8) != 0) {
      return;
    }

    Serial.print(millis());
    Serial.print(" ");
    for (int i = 0; i < 8; i++) {
      Serial.print(recbuffer[i], HEX);
    }
    //Serial.println();

    uint8_t slaveadr = recbuffer[0];
    uint8_t functioncode = recbuffer[1];
    uint16_t address = (recbuffer[2] << 8) | recbuffer[3];
    uint16_t wordsize = (recbuffer[4] << 8) | recbuffer[5];

    //if (slaveadr == 2 && address == 53) {
    Serial.print(" process register read ");
    Serial.print(slaveadr);
    Serial.print(" ");
    Serial.print(functioncode);
    Serial.print(" ");
    Serial.print(address);
    Serial.print(" ");
    Serial.println(wordsize);


    //gridpower = -gridpower - 50 ;
    Serial.print("gridpower: ");
    Serial.println(gridpower);

    wordsize = 2;
    uint8_t response[9];
    response[0] = 0x02;
    response[1] = 0x04;
    response[2] = 0x04;

    byte powerArray[4];

    memcpy(powerArray, &gridpower, sizeof(gridpower));

// Inverti l'ordine dei byte se necessario (big endian)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    {
      byte temp;
      temp = powerArray[0];
      powerArray[0] = powerArray[3];
      powerArray[3] = temp;
      temp = powerArray[1];
      powerArray[1] = powerArray[2];
      powerArray[2] = temp;
    }
#endif
    response[3] = powerArray[0];
    response[4] = powerArray[1];
    response[5] = 0;
    response[6] = 0;

    uint16_t crc = calc_crc(response, 7);
    response[7] = crc & 0xFF;
    response[8] = (crc >> 8) & 0xFF;

    Serial.print(" response");
    for (int i = 0; i < 9; i++) {
      Serial.print(" ");
      Serial.print(response[i], HEX);
    }
    Serial.println();
    pinMode(D0, OUTPUT);  //HIGH TX, LOW RX
    digitalWrite(D0, HIGH);
    delay(10);
    SoftSerial.write(response, 9);
    //}

    memset(recbuffer, 0, sizeof(recbuffer));
  }
}


unsigned long previousMillis = 0;

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();

  sendPowerToGrowatt();

  if (currentMillis - previousMillis >= 60000) {
    previousMillis = currentMillis;

    if (!receivedMessage) {
      gridpower = -100; //stops inverter if no communication from Shelly Device
    }

    receivedMessage = false;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("Reconnecting to WiFi...");
      WiFi.disconnect();
      delay(500);
      WiFi.reconnect();
      delay(500);
    }


    String IP = WiFi.localIP().toString();
    char IP_char[IP.length() + 1];
    IP.toCharArray(IP_char, IP.length() + 1);
    client.publish("SDM230_growatt/IP", IP_char);
  }
}
