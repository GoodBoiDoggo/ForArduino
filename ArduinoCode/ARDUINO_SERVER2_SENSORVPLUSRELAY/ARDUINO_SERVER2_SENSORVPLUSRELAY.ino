#define CURRENT_SENSOR A2  // Define Analog input pin that sensor is attached

float amplitude_current;      // Float amplitude current
float effective_value;       // Float effective current

#define RELAY1  7

#define DEBUG true
#define LED 13
#include "EmonLib.h"             // Include Emon Library
EnergyMonitor emon1;             // Create an instance
void setup()
{
  Serial.begin(115200);    ///////For Serial monitor
  Serial1.begin(115200); ///////ESP Baud rate
  //  pinMode(11, OUTPUT);   /////used if connecting a LED to pin 11
  digitalWrite(13, LOW);
  emon1.voltage(1, 234.26, 1.7);  // Voltage: input pin, calibration, phase_shift
  emon1.current(0, 30);       // Current: input pin, calibration.
  sendSerial1Cmdln("AT+RST\r\n", 2000); // reset module
  sendSerial1Cmdln("AT+CWMODE=2\r\n", 1000); // configure as access point
  sendSerial1Cmdln("AT+CIFSR\r\n", 1000); // get ip address
  sendSerial1Cmdln("AT+CIPMUX=1\r\n", 1000); // configure for multiple connections
  sendSerial1Cmdln("AT+CIPSERVER=1,80\r\n", 1000); // turn on server on port 80
  sendSerial1Cmdln("AT+CSYSWDTENABLE\r\n", 1000);
  //Serial1.setTimeout(2000);

  pins_init();

  pinMode(RELAY1, OUTPUT);
  digitalWrite(RELAY1, 1);
}


//float sensetemp() ///////function to sense temperature.
//{
//  int val = analogRead(A0);
//  float mv = ( val / 1024.0) * 5000;
//  float celcius = mv / 10;
//  return (celcius);
//}

int connectionId;
String boi;
String URL;
void loop() {
  int sensor_max;
  sensor_max = getMaxValue();
  Serial.print("sensor_max = ");
  Serial.println(sensor_max);

  //the VCC on the Arduino interface of the sensor is 5v

  amplitude_current = (float)(sensor_max - 512) / 1024 * 5 / 185 * 1000000; // for 5A mode,you need to modify this with 20 A and 30A mode;
  effective_value = amplitude_current / 1.414;

  //for minimum current=1/1024*5/185*1000000/1.414=18.7(mA)
  //Only sinusoidal alternating current

  //  Serial.println("The amplitude of the current is(in mA)");
  //  Serial.println(amplitude_current, 1);
  //
  //  //Only one number after the decimal point
  //
  //  Serial.println("The effective value of the current is(in mA)");
  //  Serial.println(effective_value, 1);
  //=============================================================

  emon1.calcVI(20, 2000);
  digitalWrite(LED, !digitalRead(LED));

  if (Serial1.available()) // check if the esp is sending a message
  {
    Serial.println("Something received");

    if (Serial1.find("+IPD,"))
    {
      delay(500);


      String action;


      int connectionId = Serial1.read() - 48;
      Serial.println("connectionId: " + String(connectionId));
      if (Serial1.find("GET ")) {


        URL = Serial1.readString();
        URL.remove(URL.indexOf(" "));
        Serial.println("+IPD, found");
        Serial.println("URL: " + URL);

        //      if (Serial1.find("led=")) {
        //        char s = Serial1.read();
        //        if (s == '0') {
        //          action = "led=0";
        //          digitalWrite(LED, LOW);
        //        } else if (s == '1') {
        //          action = "led=1";
        //          digitalWrite(LED, HIGH);
        //        } else {
        //          action = "led=?";
        //        }
        //        Serial.println(action);
        //        sendHTTPResponse(connectionId, action, "text/plain");
        //      }

        if (URL.indexOf("/getsensordata") > -1) {

          emon1.serialprint();
          String sensordata = (String)emon1.realPower;

          action += "{\"data\":";
          action += "{\"watts\":" + sensordata + "";
          action += ",";
          action += "\"solar\":" + (String)effective_value + "}";
          action += "}";
          Serial.println("SENSOR REQUEST");
          Serial.println(action.length());
          sendHTTPResponse(connectionId, action, "text/json");
        }
        else if (URL.indexOf("/login?username=") > -1) {
          action += "{\"userinfo\":";
          action += "{\"username\":\"" + URL.substring(URL.indexOf("username=") + 9) + "\"";
          action += ",";
          action += "\"password\":\"SECRET\"}";
          action += "}";
          delay(500);
          Serial.println(action);
          Serial.println("USER REQUEST");

          sendHTTPResponse(connectionId, action, "text/json");
        }
        else if (URL.indexOf("/fav") > -1) {
          Serial.print("BLOCKED FAVICON");
        }
        else if (URL.indexOf("/getsolarread") > -1) {

          action += "{\"data\":";
          action += "{\"name\":\"solar\"";
          action += ",";
          action += "\"watts\":" + (String)effective_value + "}";
          action += "}";

          //          action += (String)effective_value;
          sendHTTPResponse(connectionId, action, "text/plain");
        }
        else if (URL.indexOf("/relayswitch") > -1) {
          digitalWrite(RELAY1, !digitalRead(RELAY1));
          sendHTTPResponse(connectionId, "DONE", "text/plain");
        }
        else {
          sendHTTPResponse(connectionId, "INVALID LINK", "text/plain");
        }
        //Close TCP/UDP
        String cmdCIPCLOSE = "AT+CIPCLOSE=";
        cmdCIPCLOSE += connectionId;
        sendSerial1Cmdln(cmdCIPCLOSE, 1000);
      }
    }
  }

}

void sendHTTPResponse(int id, String content, String contenttype)
{
  String response;
  response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: " + contenttype + "; charset=UTF-8\r\n";
  response += "Content-Length: ";
  response += (String)content.length();
  response += "\r\n";
  response += "Connection: Close\r\n\r\n";
  response += content;
  String cmd = "AT+CIPSEND=";
  cmd += id;
  cmd += ",";
  cmd += response.length();

  Serial.println("--- AT+CIPSEND ---");
  sendSerial1Cmdln(cmd, 1250);

  Serial.println("--- data ---");
  sendSerial1Data(response, 1250);
}

boolean waitOKfromSerial1(int timeout)
{
  do {
    Serial.println("wait OK...");
    delay(1000);
    if (Serial1.find("OK"))
    {
      return true;
    }

  } while ((timeout--) > 0);
  return false;
}

//Send command to Serial1, assume OK, no error check
//wait some time and display respond
void sendSerial1Cmdln(String cmd, int waitTime)
{
  Serial1.println(cmd);
  delay(waitTime);
  clearSerial1SerialBuffer();
}

//Basically same as sendSerial1Cmdln()
//But call Serial1.print() instead of call Serial1.println()
void sendSerial1Data(String data, int waitTime)
{
  Serial1.print(data);
  delay(waitTime);
  clearSerial1SerialBuffer();
}

//Clear and display Serial Buffer for Serial1
void clearSerial1SerialBuffer()
{
  Serial.println("= clearSerial1SerialBuffer() =");
  while (Serial1.available() > 0) {
    char a = Serial1.read();
    Serial.write(a);

  }
  Serial.println("==============================");
}

void pins_init()
{
  pinMode(CURRENT_SENSOR, INPUT);
}

int getMaxValue()
{
  int sensorValue;    //value read from the sensor
  int sensorMax = 0;
  uint32_t start_time = millis();
  while ((millis() - start_time) < 1000) //sample for 1000ms
  {
    sensorValue = analogRead(CURRENT_SENSOR);
    if (sensorValue > sensorMax)
    {
      /*record the maximum sensor value*/

      sensorMax = sensorValue;
    }
  }
  return sensorMax;
}
