#include <Arduino.h>

/*   Sistema com MQTT e Heartbeat                           18/03/2024
    
usar <ETHClass.h> do fornecedor!
Hardware: LilyGO T-Internet-POE
Ethernet: LAN8720
CHIP:     ESP32 Dev Module

https://github.com/Xinyuan-LilyGO/LilyGO-T-ETH-Series/tree/master

*/

#include <ETHClass.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <SD.h>
#include "HeartBeat.h"

HeartBeat HB;
//int HBstatus;

//Change to IP and DNS corresponding to your network, gateway
IPAddress staticIP  (192, 168, 15, 151);
IPAddress gateway   (192, 168, 15, 1);
IPAddress subnet    (255, 255, 255, 0);
IPAddress dns       (192, 168, 15, 1); // ajustar DNS para usar IP estatico via internet (linha 121)
//IPAddress dns       (8, 8, 4, 4); // Google
// ETH MAC: 40:22:D8:72:CF:97

#define ETH_CLK_MODE                    ETH_CLOCK_GPIO17_OUT
#define ETH_ADDR                        0
#define ETH_TYPE                        ETH_PHY_LAN8720
#define ETH_RESET_PIN                   5
#define ETH_MDC_PIN                     23
#define ETH_MDIO_PIN                    18
//#define SD_MISO_PIN                     2
//#define SD_MOSI_PIN                     15
//#define SD_SCLK_PIN                     14
//#define SD_CS_PIN                       13

#define OUT1                            2
#define OUT2                            4 // usasdo no hearbeat

static bool eth_connected = false;
uint32_t runETH = 0;

WiFiClient ethClient;
PubSubClient client(ethClient);

void WiFiEvent(WiFiEvent_t event)
{
    switch (event) {
    case ARDUINO_EVENT_ETH_START:
        Serial.println("ETH Started");
        //set eth hostname here
        ETH.setHostname("esp32-ethernet");
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        Serial.println("ETH Connected");
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        Serial.print("ETH MAC: ");
        Serial.print(ETH.macAddress());
        Serial.print(", IPv4: ");
        Serial.print(ETH.localIP());
        if (ETH.fullDuplex()) {
            Serial.print(", FULL_DUPLEX");
        }
        Serial.print(", ");
        Serial.print(ETH.linkSpeed());
        Serial.print("Mbps");
        Serial.print(", ");
        Serial.print("GatewayIP:");
        Serial.println(ETH.gatewayIP());
        eth_connected = true;
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        Serial.println("ETH Disconnected");
        eth_connected = false;
        break;
    case ARDUINO_EVENT_ETH_STOP:
        Serial.println("ETH Stopped");
        eth_connected = false;
        break;
    default:
        break;
    }
}

void callback(char *topic, uint8_t *payload, uint32_t length)
{
    Serial.print("Mensagem recebida no tópco [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(OUT1, HIGH);
    Serial.println("OUT1 ON");
  } else {
    digitalWrite(OUT1, LOW);
    Serial.println("OUT1 OFF");
  }

}

void setup()
{
    pinMode(OUT1, OUTPUT);
    digitalWrite(OUT1, LOW);

    HB.begin(OUT2, 1);  // (PIN  , frequency 3)

    Serial.begin(115200);

    WiFi.onEvent(WiFiEvent);

    if (!ETH.begin(ETH_ADDR, ETH_RESET_PIN, ETH_MDC_PIN,
                   ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE)) {
        Serial.println("ETH start Failed!");
    }
      //  Seta os parametros de IP estatico
 // if (ETH.config(staticIP, gateway, subnet, dns, dns) == false) {Serial.println("Configuration static IP failed.");}

    // Note - the default maximum packet size is 128 bytes. If the
    // combined length of clientId, username and password exceed this use the
    // following to increase the buffer size:
    // client.setBufferSize(255);
    while (!eth_connected) {
        Serial.println("Wait eth connect..."); delay(2000);
    }

    //set server and port
    client.setServer("broker-cn.emqx.io", 1883);

    //set callback
    client.setCallback(callback);

    const char *clien_id = "esp32eth";
    const char *username = "empty";
    const char *password = "empty";

    while (!client.connect(clien_id, username, password)) {
        delay(200);
        Serial.println("Connecting....");
    }

    client.subscribe("/esp32eth/led");

    // publish example
    client.publish("/esp32eth/home", "Start...");
    Serial.println("MQTT Connected!");
}

void loop(){

HB.beat();

if (millis() - runETH >= 5000) {
        if (eth_connected) {
            client.publish("/esp32eth/home", "HW MQTT OK");
            Serial.println("Mensagem publicada");
            //Serial.println(HBstatus);
        }
        runETH = millis();}
        
client.loop();
}