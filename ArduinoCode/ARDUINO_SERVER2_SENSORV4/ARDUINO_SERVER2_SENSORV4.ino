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
  emon1.calcVI(20, 2000);
  digitalWrite(LED, !digitalRead(LED));

  if (Serial1.available()) // check if the esp is sending a message
  {


    if (Serial1.find("+IPD,"))
    {

      String action;
      
      int connectionId = Serial1.read() - 48;
      URL = Serial1.readString();
      delay(500);
      Serial.println("Something received");
      Serial.println(URL);
      delay(300);
      Serial.println("connectionId: " + String(connectionId));
      if (URL.indexOf("GET ")) {


        URL = URL.substring(URL.indexOf("GET ") + 4);
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
          action += "\"password\":\"SECRET\"}";
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
        else {
          sendHTTPResponse(connectionId, "INVALID LINK", "text/json");
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
  sendSerial1Cmdln(cmd, 1000);

  Serial.println("--- data ---");
  sendSerial1Data(response, 1000);
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

