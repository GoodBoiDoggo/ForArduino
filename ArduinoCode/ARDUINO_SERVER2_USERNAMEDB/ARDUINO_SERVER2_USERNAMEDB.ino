#include "Arduino.h"
#include <EDB.h>

// Use the external SPI SD card as storage
#include <SPI.h>
#include <SD.h>

#define SD_PIN 10  // SD Card CS pin
#define TABLE_SIZE 300000 // 8192
#define LED_PIN 7

// The number of demo records that should be created.  This should be less
// than (TABLE_SIZE - sizeof(EDB_Header)) / sizeof(LogEvent).  If it is higher,
// operations will return EDB_OUT_OF_RANGE for all records outside the usable range.
#define RECORDS_TO_CREATE 10

//db name
char db_name[] = "/db/edb_epon.db";
File dbFile;
//
int correctId = 0;
int temperatureId = 0;
// Arbitrary record definition for this table.
// This should be modified to reflect your record needs.
struct UserHeader {
    int id;
    String username;
    String password;
    String fullname;
    String role;
    String boxid;
}
userHeader;

struct TempReadingTable {
  int recordNumber;
  float temperature;
}
tempReadingTable;

// The read and write handlers for using the SD Library
// Also blinks the led while writing/reading
inline void writer (unsigned long address, const byte* data, unsigned int recsize) {
    digitalWrite(LED_PIN, HIGH); // arduino led
    dbFile.seek(address);
    dbFile.write(data,recsize);
    dbFile.flush();
    digitalWrite(LED_PIN, LOW); // arduino led
}

inline void reader (unsigned long address, byte* data, unsigned int recsize) {
    digitalWrite(LED_PIN, HIGH); // arduino led
    dbFile.seek(address);
    dbFile.read(data,recsize);
    digitalWrite(LED_PIN, LOW); // arduino led
}

// Create an EDB object with the appropriate write and read handlers
EDB db(&writer, &reader);




#define DEBUG true
#define LED 2
void setup()
{
  Serial.begin(115200);    ///////For Serial monitor
  Serial1.begin(115200); ///////ESP Baud rate
  pinMode(11, OUTPUT);   /////used if connecting a LED to pin 11
  digitalWrite(11, LOW);

  sendSerial1Cmdln("AT+RST\r\n", 2000); // reset module
  sendSerial1Cmdln("AT+CWMODE=2\r\n", 1000); // configure as access point
  sendSerial1Cmdln("AT+CIFSR\r\n", 1000); // get ip address
  sendSerial1Cmdln("AT+CIPMUX=1\r\n", 1000); // configure for multiple connections
  sendSerial1Cmdln("AT+CIPSERVER=1,80\r\n", 1000); // turn on server on port 80
  sendSerial1Cmdln("AT+CSYSWDTENABLE\r\n", 1000);
  Serial1.setTimeout(2000);



  pinMode(LED_PIN, OUTPUT); // arduino led
    digitalWrite(LED_PIN, LOW); // arduino led

    Serial.begin(9600);
    Serial.println(" Extended Database Library + External SD CARD storage demo");
    Serial.println();

    randomSeed(analogRead(0));

    if (!SD.begin(SD_PIN)) {
        Serial.println("No SD-card.");
        return;
    }

    // Check dir for db files
    if (!SD.exists("/db")) {
        Serial.println("Dir for Db files does not exist, creating...");
        SD.mkdir("/db");
    }

    if (SD.exists(db_name)) {

        dbFile = SD.open(db_name, FILE_WRITE);

        // Sometimes it wont open at first attempt, espessialy after cold start
        // Let's try one more time
        if (!dbFile) {
            dbFile = SD.open(db_name, FILE_WRITE);
        }

        if (dbFile) {
            Serial.print("Openning current table... ");
            EDB_Status result = db.open(0);
            EDB_Status result_2 = db.open(1);
            if (result == EDB_OK && result_2 == EDB_OK) {
                Serial.println("DONE");
            } else {
                Serial.println("ERROR");
                Serial.println("Did not find database in the file " + String(db_name));
                Serial.print("Creating new table... ");
                db.create(0, TABLE_SIZE, (unsigned int)sizeof(userHeader));
                db.create(1, TABLE_SIZE, (unsigned int)sizeof(tempReadingTable));
                Serial.println("DONE");
                return;
            }
        } else {
            Serial.println("Could not open file " + String(db_name));
            return;
        }
    } else {
        Serial.print("Creating table... ");
        // create table at with starting address 0
        dbFile = SD.open(db_name, FILE_WRITE);
        db.create(0, TABLE_SIZE, (unsigned int)sizeof(userHeader));
        db.create(1, TABLE_SIZE, (unsigned int)sizeof(tempReadingTable));
        Serial.println("DONE");
    }
    deleteAll();
    recordLimit();
    //createRecordUserHeader(int num_recs, String username, String password, String fullname, String role, String boxid)
//    createRecordUserHeader(1, "brixs", "password1234", "Nicholson Secretary", "admin", "7456789");
    //createRecordUserHeader(1, "mar", "ponsie", "Mar Roxas Ponsie", "user", "7543456");
    //createRecordUserHeader(1, "user123", "password4321", "Lebron Pascual", "user", "6572123");
    selectAll();
    /*recordLimit();
    countRecords();
    createRecords(RECORDS_TO_CREATE);
    countRecords();
    selectAll();
    deleteOneRecord(RECORDS_TO_CREATE / 2);
    countRecords();
    selectAll();
    appendOneRecord(RECORDS_TO_CREATE + 1);
    countRecords();
    selectAll();
    insertOneRecord(RECORDS_TO_CREATE / 2);
    countRecords();
    selectAll();
    updateOneRecord(RECORDS_TO_CREATE);
    selectAll();
    countRecords();
    deleteAll();
    Serial.println("Use insertRec() and deleteRec() carefully, they can be slow");
    countRecords();
    for (int i = 1; i <= 20; i++) insertOneRecord(1);  // inserting from the beginning gets slower and slower
    countRecords();
    for (int i = 1; i <= 20; i++) deleteOneRecord(1);  // deleting records from the beginning is slower than from the end
    countRecords(); */
    dbFile.close();
}


float sensetemp() ///////function to sense temperature.
{
  int val = analogRead(A0);
  float mv = ( val / 1024.0) * 5000;
  float celcius = mv / 10;
  return (celcius);
}

int connectionId;
String boi;
void loop() {

  if (Serial1.available()) // check if the esp is sending a message
  {
    Serial.println("Something received");

    if (Serial1.find("+IPD,"))
    {
      delay(500);
      String action;

      Serial.println("+IPD, found");
      int connectionId = Serial1.read() - 48;
      Serial.println("connectionId: " + String(connectionId));

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
      if (Serial1.find("/login?username=")) {
        String uname = Serial1.readStringUntil(' ');
//        action+="{\"userinfo\":";
//        action+="{\"username\":\""+ uname +"\"";
//        delay(200);
//        action += ",";
//        action+="\"password\":\"SECRET\"}";
//        action+="}";
        delay(500);
        Serial.println(action);
        Serial.println("YOU WENT IN THE BILAT");
        createRecordUserHeader(1, uname, "password1234", "Nicholson Secretary", "admin", "7456789");
        sendHTTPResponse(connectionId, action, "application/json");
      }


      //Close TCP/UDP
      //String cmdCIPCLOSE = "AT+CIPCLOSE=";
      //cmdCIPCLOSE += connectionId;
      //sendSerial1Cmdln(cmdCIPCLOSE, 1000);
    }

  }

}

void sendHTTPResponse(int id, String content, String contenttype)
{
  String response;
  response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: "+contenttype+"; charset=UTF-8\r\n";
  response += "Content-Length: ";
  response += content.length();
  response += "\r\n";
  response += "Connection: close\r\n\r\n";
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
void recordLimit()
{
    Serial.print("Record Limit: ");
    Serial.println(db.limit());
}

void deleteOneRecord(int recno)
{
    Serial.print("Deleting recno: ");
    Serial.println(recno);
    db.deleteRec(recno);
}

void deleteAll()
{
    Serial.print("Truncating table... ");
    db.clear();
    Serial.println("DONE");
}

void countRecords()
{
    Serial.print("Record Count: ");
    Serial.println(db.count());
}

void createRecordUserHeader(int num_recs, String username, String password, String fullname, String role, String boxid) //confusing pa ni diri dapit
//void createRecordUserHeader(int num_recs, String username, char *password, char *fullname, char *role, char *boxid)
{
    Serial.println("Creating User Header Record... ");
    correctId++;
    Serial.print("Correct id: ");
    Serial.println(correctId);
    Serial.print("username: ");
    Serial.println(username);
    Serial.print("Fullname: ");
    Serial.println(fullname);
    Serial.print("Password: ");
    Serial.println(password);
    userHeader.id = correctId;
    userHeader.username = username;
    userHeader.password = password;
    userHeader.fullname = fullname;
    userHeader.role = role;
    userHeader.boxid = boxid;
    
    EDB_Status result = db.appendRec(EDB_REC userHeader);

    if(result != EDB_OK) printError(result);
    
    Serial.println("DONE");
}

void createRecordReading(float temperature)
{
  temperatureId++;
  tempReadingTable.recordNumber = temperatureId;
  tempReadingTable.temperature = temperature;

  EDB_Status result = db.appendRec(EDB_REC tempReadingTable);

  if(result != EDB_OK) printError(result);
}

void selectAll()
{
    Serial.println("DBCount: ");
    for (int recno = 1; recno <= db.count(); recno++)
    {
        int totalRecno = db.count();
        Serial.print(totalRecno);
        EDB_Status result = db.readRec(recno, EDB_REC userHeader);
        if (result == EDB_OK)
        {
            Serial.print("Recno: ");
            Serial.print(recno);
            Serial.print(" ID: ");
            Serial.print(userHeader.id);
            Serial.print(" Name: ");
            Serial.print(userHeader.fullname);
            Serial.print(" Boxid: ");
            Serial.print(userHeader.boxid);
            Serial.print(" Role: ");
            Serial.print(userHeader.role);
            Serial.print(" username: ");
            Serial.print(userHeader.username);
            Serial.print(" password: ");
            Serial.println(userHeader.password);
        }
        else printError(result);
    }
}

void selectLastRecord()
{
  int lastRecNum = db.count();
  EDB_Status result = db.readRec(lastRecNum, EDB_REC tempReadingTable);
  if(result == EDB_OK)
  {
     Serial.print(" ID: ");
     Serial.print(tempReadingTable.recordNumber);
     Serial.print(" Temperature: ");
     Serial.println(tempReadingTable.temperature);
  }
   else printError(result);
}

void updateOneRecord(int recno)
{
    /*Serial.print("Updating record at recno: ");
    Serial.print(recno);
    Serial.print("... ");
    logEvent.id = 1234567;
    logEvent.temperature = 4321123;
    EDB_Status result = db.updateRec(recno, EDB_REC logEvent);
    if (result != EDB_OK) printError(result);
    Serial.println("DONE");*/
}

void insertOneRecord(int recno)
{
    /*Serial.print("Inserting record at recno: ");
    Serial.print(recno);
    Serial.print("... ");
    logEvent.id = recno;
    logEvent.temperature = random(1, 125);
    EDB_Status result = db.insertRec(recno, EDB_REC logEvent);
    if (result != EDB_OK) printError(result);
    Serial.println("DONE");*/
}

void appendOneRecord(int id)
{
    /*Serial.print("Appending record... ");
    logEvent.id = id;
    logEvent.temperature = random(1, 125);
    EDB_Status result = db.appendRec(EDB_REC logEvent);
    if (result != EDB_OK) printError(result);
    Serial.println("DONE");*/
}

void printError(EDB_Status err)
{
    Serial.print("ERROR: ");
    switch (err)
    {
        case EDB_OUT_OF_RANGE:
            Serial.println("Recno out of range");
            break;
        case EDB_TABLE_FULL:
            Serial.println("Table full");
            break;
        case EDB_OK:
        default:
            Serial.println("OK");
            break;
    }
}
