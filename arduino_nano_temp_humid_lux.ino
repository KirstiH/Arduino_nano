/*
  7.11.2023
  Kirsti Härö

  This will create a web-based application which will display temperature, humidity and luminosity
  Change ssid and password before using.
  This uses Arduino Nano 33 IoT, SHTC sensor and TSL2591 sensor
 */

#include <SPI.h>
#include <WiFiNINA.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_SHTC3.h"
#include "Adafruit_TSL2591.h"

Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = "xxxxx";        // your network SSID (name)
char pass[] = "123456789";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key index number (needed only for WEP)
sensors_event_t humidity, temp; // saving values from sensors
uint16_t ir, full; // for sensor value
WiFiClient client; //for using WiFiClient

int status = WL_IDLE_STATUS;
WiFiServer server(80);

//configuring TSL2591 sensor
void configureSensor(void)
{
  tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  Serial.println(F("------------------------------------"));
  Serial.print  (F("Gain:         "));
  tsl2591Gain_t gain = tsl.getGain();
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //making sure SHTC3 works
  Serial.println("SHTC3 test");
  if (! shtc3.begin()) {
    Serial.println("Couldn't find SHTC3");
    while (1) delay(1);
  }
  Serial.println("Found SHTC3 sensor");

  Serial.println("Access Point Web Server");

  Serial.println(F("Starting Adafruit TSL2591 Test!"));
  
  // starting tsl2591 sensor
  if (tsl.begin(0x29)) 
  {
    Serial.println(F("Found a TSL2591 sensor"));
  } 
  else 
  {
    Serial.println(F("No sensor found ... check your wiring?"));
    while (1);
  }
  
  /* Configure the sensor */
  configureSensor();


  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();

}


void loop() {

  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }

  client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      delayMicroseconds(10);                // This is required for the Arduino Nano RP2040 Connect - otherwise it will loop so fast that SPI will never be served.
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out to the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          // continue to subprogram sendData()
          if (currentLine.length() == 0) {
            sendData();
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
        // Check to see if the client request was "GET /U"
        if (currentLine.endsWith("GET /U")) {
          !client;               // connect the client again
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}


// measuring temperature and humidity
void mittaaSHT(){
  shtc3.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
}

// measuring luminosity
void mittaaTSL(){
  uint32_t lum = tsl.getFullLuminosity();
  ir = lum >> 16;
  full = lum & 0xFFFF;
}

// forming the website and dispalying the data
void sendData(){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  // Display the HTML web page and set the styles
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" charset=\"UTF-8\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  client.println("<style>.center {margin-left: auto; margin-right: auto;}</style>");
  client.println("<style>.button {border: 12px solid #000000 text-align: center; background-color: white; font-size: 20px; border-radius: 12px; padding: 8px 50px; margin: 4px 6px;}</style>");
  client.println("<style>.center2 {display: flex; justify-content: center; align-items: center; height: 50px;</style>");

  // the content of the HTTP response follows the header:
  client.println("</head><body><h1 style=\"text-align: center\">Mittaukset</h1>");
  mittaaSHT(); //calling subprogram to measure temperature and humidity
  mittaaTSL(); //calling subprogram to measure luminosity
  //printing the values in formatted table
  client.print("<table class=\"center\"><tbody><tr><td style=\"width:72%\">Lämpötila</td><td>");
  client.print(temp.temperature, 1); 
  client.println(" °C</td></tr>");
  client.print("<tr><td style=\"padding\">Kosteus</td><td>");
  client.print(humidity.relative_humidity, 0);
  client.println(" %RH</td></tr>");
  client.print("<tr><td style=\"padding\">Valoisuus</td><td>");
  client.print(tsl.calculateLux(full, ir), 0);
  client.print(" LUX</td></tr>");
  client.println("</tbody></table><br>");     
  //adding the button to update the website       
  client.print("<div class=\"center2\"><form action=\"/U\"><button class=\"button\" type=\"submit\"><b>UPDATE</b></button></form></div><br>");
  client.println("</body></html>");

  // The HTTP response ends with another blank line:
  client.println();
}

// checking WiFiStatus and printing the information
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}

