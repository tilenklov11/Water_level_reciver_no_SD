#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <SSD1306Wire.h>

#include <WiFi.h>

#define a_SSID "ASUS_64"
#define a_PASSWORD "tilen999"
WiFiServer server(80);

#define a_DOMAIN (String) "hribrezervoar.com"
#define a_HOST (String) "@"
#define a_DNS_PASSWORD (String) "8585c1a622db48a3a63752e636729278"

// Pin definitions
#define SDA     21   // I2C data
#define SCL     22  // I2C clock
#define SS      18  // SPI slave select
#define RST     -1  // OLED reset
#define CONFIG_MOSI 27
#define CONFIG_MISO 19
#define CONFIG_CLK  5
#define CONFIG_NSS  18
#define CONFIG_RST  23
#define CONFIG_DIO0 26

// Images for water levels
#define WATER_0 ""
#define WATER_25 WATER_0
#define WATER_50 ""
#define WATER_75 WATER_50
#define WATER_100 ""

// Create SSD1306 object
SSD1306Wire display(0x3C, SDA, SCL);

char w_lvl;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Initialize LoRa module
  SPI.begin(CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);
  LoRa.setPins(CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa initialization failed. Check your connections!");
    while (1);
  }
   LoRa.setSpreadingFactor(12);
  // Initialize OLED display
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "Waiting for LoRa packets...");
  display.display();

  WiFi.begin(a_SSID, a_PASSWORD);
  server.begin();
}

bool old_wifi_state = false;
String OldData;

void loop() {
  // Check if a packet is available
  if (LoRa.parsePacket()) {
    Serial.println("Receaving packet...");
    String NewData;
    NewData = LoRa_parser();
    
    if (NewData != ""){
      // Display water level and voltage on the OLED display
      OldData = NewData;
      display.clear();
      display.drawString(0, 0, NewData);
      display.display();
    }
  }

  //Test for connection to the great all mighty wifi
  if ((WiFi.status() == WL_CONNECTED) != old_wifi_state){
    old_wifi_state = (WiFi.status() == WL_CONNECTED);
    Serial.println("Server:" + (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "Offline");
    display.drawString(0, 40, "Server:" + (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "Offline");
    display.display();
    changeIP();
  }

  Web_Server();
   display.drawString(0, 40, "Server:" + (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "Offline");
    display.display();
}


inline void Web_Server(){
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            String body;
            switch (w_lvl){
              case 1:
                body = WATER_25;
                break;
              case 2:
                body = WATER_50;
                break;
              case 3:
                body = WATER_75;
                break;
              case 4:
                body = WATER_100;
                break;
              default:
                body = WATER_0;
            }
            client.print("<body style=\"background-image: url(" + body + ");display: grid;\">");
            client.print("<div style=\"grid\">");
            String tempstr = OldData;
            tempstr.replace("\n","</text><text style=\"background-color:#2196F3;padding: 6px 8px 6px 10px\">");
            client.print("<text style=\"background-color:#2196F3;padding: 6px 8px 6px 10px\">" + tempstr + "</text>");
            client.print("</div>");
            client.print("</body>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}






String LoRa_parser(){
  String packet = "";
    
    // Read the packet into a string
    while (LoRa.available()) {
      packet += (char)LoRa.read();
    }

    Serial.println(packet);
    // Process the packet
    if (packet.length() > 0) {
      // Extract water level and voltage information
      String waterLevel = packet.substring(0,4);
      w_lvl = 0;
      for(int i = 0; i<4; i++){
        w_lvl += (int) waterLevel[i] - 48;
      }
      Serial.println(w_lvl,HEX);
      String voltage = packet.substring(5);

    

    return "Water level: " + getWaterLevelString(w_lvl) + "\nVoltage: " + voltage;
    }
    else{
      return "";
    }
}

inline String getWaterLevelString(char level) {
  switch (level) {
    case 1:
      return "25%";
    case 2:
      return "50%";
    case 3:
      return "75%";
    case 4:
      return "100%";
    default:
      return "0%";
  }
  
}


void changeIP(){
  WiFiClient client;
  String IP;
  if(!(WiFi.status() == WL_CONNECTED)){return;};
  IP = getIP(client);

  //Turn IP string into IP
  String UpdateIP = "/update?host=" + a_HOST + "&domain=" + a_DOMAIN + "&password=" + a_DNS_PASSWORD + "&ip=" + IP;
  Serial.println("Sending this thingy: " + UpdateIP);

  int escape_int = 0;
  if(client.connect("dynamicdns.park-your-domain.com",80) && escape_int < 1000){
    delay(1);
    escape_int++;
  

  client.println("GET " + UpdateIP + " HTTP/1.1");
  client.println("Host: dynamicdns.park-your-domain.com");
  client.println("Connection: close");
  client.println();
  }
  else{
    Serial.println("Failed to update IP");
  }

  client.stop();
  }


String getIP(WiFiClient client){
  int escape_int = 0;
  
  Serial.println("Attempting to get ip...");

  if (client.connect("api.ipify.org", 80)) {
    Serial.println("connected");
    client.println("GET / HTTP/1.0");
    client.println("Host: api.ipify.org");
    client.println();
  } else {
    Serial.println("connection failed");
    return "-------";
  }
  
  Serial.println(client.connected());
  escape_int = 0;
  String IP;
   while (!client.available()&& escape_int < 500){
    delay(1);
     escape_int ++;
    };

    
    escape_int = 0;
  while (client.available() && escape_int < 1000){
    //Serial.write(client.read());
    char c = client.read();
    if(IP.endsWith("gin")){
      IP = "";
    }
    //Serial.write(c);
    IP += c;

    //Serial.println("symbol: " + IP);
    escape_int ++;
  }

  IP.replace("\n","");

  
  Serial.println("Packet = " + IP);
  client.stop();
  return IP;
}
