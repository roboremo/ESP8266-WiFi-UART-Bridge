// ESP8266 WiFi <-> UART Bridge
// by RoboRemo
// www.roboremo.com

// Disclaimer: Don't use RoboRemo for life support systems
// or any other situations where system failure may affect
// user or environmental safety.

#include <ESP8266WiFi.h>


// config: ////////////////////////////////////////////////////////////

#define UART_BAUD 9600
#define packTimeout 5 // ms (if nothing more on UART, then send packet)
#define bufferSize 8192

//#define MODE_AP // phone connects directly to ESP
#define MODE_STA // ESP connects to WiFi router

#define PROTOCOL_TCP
//#define PROTOCOL_UDP


#ifdef MODE_AP
// For AP mode:
const char *ssid = "mywifi";  // You will connect your phone to this Access Point
const char *pw = "qwerty123"; // and this is the password
IPAddress ip(192, 168, 0, 1); // From RoboRemo app, connect to this IP
IPAddress netmask(255, 255, 255, 0);
const int port = 9876; // and this port
// You must connect the phone to this AP, then:
// menu -> connect -> Internet(TCP) -> 192.168.0.1:9876
#endif


#ifdef MODE_STA
// For STATION mode:
const char *ssid = "myrouter";  // Your ROUTER SSID
const char *pw = "password"; // and WiFi PASSWORD
const int port = 9876;
// You must connect the phone to the same router,
// Then somehow find the IP that the ESP got from router, then:
// menu -> connect -> Internet(TCP) -> [ESP_IP]:9876
#endif

//////////////////////////////////////////////////////////////////////////




#ifdef PROTOCOL_TCP
#include <WiFiClient.h>
WiFiServer server(port);
WiFiClient client;
#endif

#ifdef PROTOCOL_UDP
#include <WiFiUdp.h>
WiFiUDP udp;
IPAddress remoteIp;
#endif


uint8_t buf1[bufferSize];
uint8_t i1=0;

uint8_t buf2[bufferSize];
uint8_t i2=0;



void setup() {

  delay(500);
  
  Serial.begin(UART_BAUD);

  #ifdef MODE_AP 
  //AP mode (phone connects directly to ESP) (no router)
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, ip, netmask); // configure ip address for softAP 
  WiFi.softAP(ssid, pw); // configure ssid and password for softAP
  #endif

  
  #ifdef MODE_STA
  // STATION mode (ESP connects to router and gets an IP)
  // Assuming phone is also connected to that router
  // from RoboRemo you must connect to the IP of the ESP
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pw);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  #endif

  #ifdef PROTOCOL_TCP
  Serial.println("Starting TCP Server");
  server.begin(); // start TCP server 
  #endif

  #ifdef PROTOCOL_UDP
  Serial.println("Starting UDP Server");
  udp.begin(port); // start UDP server 
  #endif
}


void loop() {

  #ifdef PROTOCOL_TCP
  if(!client.connected()) { // if client not connected
    client = server.available(); // wait for it to connect
    return;
  }

  // here we have a connected client

  if(client.available()) {
    while(client.available()) {
      buf1[i1] = (uint8_t)client.read(); // read char from client (RoboRemo app)
      if(i1<bufferSize-1) i1++;
    }
    // now send to UART:
    Serial.write(buf1, i1);
    i1 = 0;
  }

  if(Serial.available()) {

    // read the data until pause:
    
    while(1) {
      if(Serial.available()) {
        buf2[i2] = (char)Serial.read(); // read char from UART
        if(i2<bufferSize-1) i2++;
      } else {
        //delayMicroseconds(packTimeoutMicros);
        delay(packTimeout);
        if(!Serial.available()) {
          break;
        }
      }
    }
    
    // now send to WiFi:
    client.write((char*)buf2, i2);
    i2 = 0;
  }
  #endif



  #ifdef PROTOCOL_UDP
  // if there’s data available, read a packet
  int packetSize = udp.parsePacket();
  if(packetSize>0) {
    remoteIp = udp.remoteIP(); // store the ip of the remote device
    udp.read(buf1, bufferSize);
    // now send to UART:
    Serial.write(buf1, packetSize);
  }

  if(Serial.available()) {

    // read the data until pause:
    //Serial.println("sa");
    
    while(1) {
      if(Serial.available()) {
        buf2[i2] = (char)Serial.read(); // read char from UART
        if(i2<bufferSize-1) {
          i2++;
        }
      } else {
        //delayMicroseconds(packTimeoutMicros);
        //Serial.println("dl");
        delay(packTimeout);
        if(!Serial.available()) {
          //Serial.println("bk");
          break;
        }
      }
    }

    // now send to WiFi:  
    udp.beginPacket(remoteIp, port); // remote IP and port
    udp.write(buf2, i2);
    udp.endPacket();
    i2 = 0;
  }
    
  #endif
  
  
}
