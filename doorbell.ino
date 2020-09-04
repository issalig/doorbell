/*
  Name:		    doorbell.ino
  Created:	  18/11/2019 Initial version
              10/02/2019 NTP added
              27/08/2020 OTA and updated telegram SHA key
              04/09/2020 web with AJAX and DST functions
  Author:	    issalig
  Description: sends a message to telegram when doorbell rings and also on web

              HW
              If connecting to a 220v doorbell just use optocoupler module like this https://es.aliexpress.com/item/32809745991.htm to detect ringing without frying anything.
              Connect signal from optocoupler to D6 (or other pin you like), and 3V3 and GND (Used Wemos D1 mini, others should work too)

              SW
              Upload data directory with ESP8266SketchDataUpload and flash it with USB for the first time. Then, when installed you can use OTA updates.                           
              
  Todos:      add wifi scan and/or ble

  References: https://tttapa.github.io/ESP8266
              https://github.com/luisllamasbinaburo/ESP8266-Examples
              
*/

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#include <NTPClient.h> //install from arduino or get it from https://github.com/arduino-libraries/NTPClient
#include <WiFiUdp.h>

#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);       // create a web server on port 80
File fsUploadFile;                                    // a File variable to temporarily store the received file

const char* mdnsName = "doorbell"; // Domain name for the mDNS responder
const char *OTAName = "doorbell";           // A name and a password for the OTA service
const char *OTAPassword = "esp8266";

IPAddress ip(192, 168, 2, 206);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dnsAddr(8, 8, 8, 8);  //google DNS, if you do not have a DNS, use 149.154.167.220 instead api.telegram.org

String ssid  = "x"    ; // REPLACE ssid WITH YOUR WIFI SSID
String pass  = "x"; // REPLACE pass YOUR WIFI PASSWORD, IF ANY

String token = "x"   ; // REPLACE token WITH YOUR TELEGRAM BOT TOKEN
int32_t chat_id = 0; // REPLACE chat_id WITH YOUR CHAT ID

int doorbell = 0;
int ring_idx = 0;

#define MAX_RINGS 16
String rings[MAX_RINGS];


//https://www.esp8266.com/viewtopic.php?p=54376
int bell_pin = D6; //D2, D6 and D7 available. Avoid the following pins: D8 is pulled up at boot, D4 wired to led and D0 for sleep mode

//fingerprint is got from Firefox browser. if it changes you must reflash
// SHA1 Fingerprint for api.telegram.org, expires on 23/5/2022, needs to be updated well before this date
const uint8_t fingerprint[20] = {0xF2, 0xAD, 0x29, 0x9C, 0x34, 0x48, 0xDD, 0x8D, 0xF4, 0xCF, 0x52, 0x32, 0xF6, 0x57, 0x33, 0x68, 0x2E, 0x81, 0xC1, 0x90};
//const char fingerprint[] = "F2:AD:29:9C:34:48:DD:8D:F4:CF:52:32:F6:57:33:68:2E:81:C1:90";

// Define NTP Client to get time
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 3600; //1 hour for Europe/Brussels
int timeOffset = 0;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0); //do not apply utc it here

//interrupt handle
//it is a good practice to keep interupt handlers small and do the processing in the main loop
void ICACHE_RAM_ATTR int_doorbell() {
  doorbell = !digitalRead(bell_pin);  //read pin another time to avoid glitches 1 means NO_RING   0 mean RING
  //doorbell = 1;
}

void setup() {
  // initialize serial port
  Serial.begin(115200);
  Serial.flush();
  delay(50);
  Serial.printf("\r\nStarting Doorbell on pin %d\n", bell_pin);

  startWiFi();
  startOTA();
  startSPIFFS();
  startMDNS();
  startServer();

  doorbell = 0;
  pinMode(bell_pin, INPUT_PULLUP); //if on air pin it will report HIGH
  attachInterrupt(digitalPinToInterrupt(bell_pin), int_doorbell, FALLING);  //1 means NO_RING   0 mean RING

  initRings();
  timeClient.begin(); //get NTP time

}

void loop() {
  handleDoorbell();
  timeClient.update();                        // update time
  server.handleClient();                      // run the server
  ArduinoOTA.handle();                        // listen for OTA events
}


/*__________________________________________________________DOORBELL_FUNCTIONS__________________________________________________________*/


void handleDoorbell() {
  // if there is a ring...
  if (doorbell) { //interrupt occurred
    //int val = digitalRead(bell_pin);  //1 means NO_RING   0 mean RING
    //if (1) { //(val == LOW) { //if still at LOW level assumes it was not a glitch and is ringing

    timeClient.setTimeOffset(timeOffset); //change it only for text
    String ring_time = timeClient.getFormattedTime();
    timeClient.setTimeOffset(0);
    String url = "https://api.telegram.org/bot" + token + "/sendMessage?chat_id=" + chat_id + "&text=TIMBRE🔔" + ring_time;
    //Serial.println(url);

    HTTPClient http;    //direct connection is faster than CTBot library
    http.begin(url, fingerprint);     //Specify request destination
    http.addHeader("Content-Type", "text/plain");  //Specify content-type header
    int httpCode = http.GET();  //send request
    String payload = http.getString(); //get response payload
    Serial.printf("Return code: %d\n", httpCode);  //Print HTTP return code
    Serial.printf("Payload: %s\n", payload.c_str());    //Print request response payload
    http.end();  //Close connection

    rings[ring_idx] = timeClient.getEpochTime();
    ring_idx = (ring_idx + 1) % MAX_RINGS;
    //}
    // wait for debouncing
    delay(500);
    doorbell = 0; //clear doorbell status
  }
}


/*__________________________________________________________SETUP_FUNCTIONS__________________________________________________________*/

//currently ntpclient has no year, months, day, only time
//using code from https://github.com/arduino-libraries/NTPClient/issues/113

#define DAYSOF4YEARS 1461UL    // 4*year plus leap day
#define DAYSOFAYEAR  365UL
#define SEC20200101  1577750400UL // 2020/01/01 - 1 day (86400s) because the result of the integer division is rounded off

void NTPymd(unsigned long mytime, uint16_t *_date, uint16_t *_month, uint16_t *_year){
  *_date  = 0;
  *_month = 0;
  *_year  = 0;
  bool        _isLeapYear  = false;

  uint32_t    _days2k = (mytime - SEC20200101)/86400; // distance to today in days
  if (_days2k > DAYSOF4YEARS) {         // how many four-year packages + remaining days, from 2050 faster than subtraction on an esp8266 ;)
    *_year = _days2k / DAYSOF4YEARS;
    _days2k = _days2k - *_year * DAYSOF4YEARS;
    *_year *= 4;
  }
  
  if (_days2k > DAYSOFAYEAR + 1) {        //2020 was a leap year, how many years + remaining days
    _days2k--;
    while (_days2k > DAYSOFAYEAR) {       //three times at most
      _days2k -= DAYSOFAYEAR;
      *_year++;
    }
  } else _isLeapYear = true;
  
  *_year += 2020;              // + 2020
  
  if (_isLeapYear && _days2k > 60) _days2k--;     // subtract the possible leap day when February passed
  
  if  (_days2k > 334) { *_month = 12;  *_date = _days2k - 334;  }    //boring but quick
  else if (_days2k > 304) { *_month = 11;  *_date = _days2k - 304;  }
  else if (_days2k > 273) { *_month = 10;  *_date = _days2k - 273;  }
  else if (_days2k > 243) { *_month = 9; *_date = _days2k - 243;  }
  else if (_days2k > 212) { *_month = 8; *_date = _days2k - 212;  }
  else if (_days2k > 181) { *_month = 7; *_date = _days2k - 181;  }
  else if (_days2k > 151) { *_month = 6; *_date = _days2k - 151;  }
  else if (_days2k > 120) { *_month = 5; *_date = _days2k - 120;  }
  else if (_days2k > 90)  { *_month = 4; *_date = _days2k - 90; }
  else if (_days2k > 59)  { *_month = 3; *_date = _days2k - 59; }
  else if (_days2k > 31)  { *_month = 2; *_date = _days2k - 31; }
  else      { *_month = 1; *_date = _days2k;  }
}

//from adafruit rtclib
bool isDSTeu(uint8_t tzHours, int y, int m, int d, int h) {
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameter: tzHours (0=UTC, 1=Europe/Brussels)
// return value: returns true during Daylight Saving Time, false otherwise
  if (m<3 || m>10) return false; // no summertime in Jan, Feb, Nov, Dec
  if (m>3 && m<10) return true; // summertime in Apr, May, Jun, Jul, Aug, Sep
  return (((m==3)  && (h + 24 * d)>=(1 + tzHours + 24*(31 - (5 * (y) /4 + 4) % 7))) || //spring
          ((m==10) && (h + 24 * d)< (1 + tzHours + 24*(31 - (5 * (y) /4 + 1) % 7)))); //autumn
}


void startWiFi() { //fixed IP
  //WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet, dnsAddr);
  WiFi.begin(ssid, pass);
  Serial.print("Connected to:\t");
  Serial.println(ssid);

  // wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    Serial.print('.');
  }

  // show IP
  Serial.println("Connection stablished.");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}


bool isInternalIP(ESP8266WebServer *myserver) {
  IPAddress clientIP = myserver->client().remoteIP();
  IPAddress serverIP = WiFi.localIP();

  Serial.println(myserver->client().remoteIP());
  Serial.println(WiFi.localIP());

  Serial.println(myserver->client().remoteIP()[0]);
  Serial.println(WiFi.localIP()[0]);

  Serial.println(myserver->client().remoteIP()[1]);
  Serial.println(WiFi.localIP()[1]);
  Serial.println(clientIP[0] == serverIP[0] && clientIP[1] == serverIP[1]);

  return (clientIP[0] == serverIP[0] && clientIP[1] == serverIP[1]);
}

void initRings() {
  String msg;
  int i;

  for (i = 0; i < MAX_RINGS; i++) {
    rings[i] = "-";
  }
}

String getRings() {
  String msg;
  int i;

  uint16_t day, month, year;
  NTPymd(timeClient.getEpochTime(),&day, &month, &year);
  bool dst = isDSTeu(1, year, month, day, timeClient.getHours());
  timeOffset = dst*3600+utcOffsetInSeconds; //add 1h if dst to the UTC offset
  
  //handmade json, if it gets more complex, use ArduinoJson library
  msg = "{\"id\":\"rings\",\"time\":\"" + String(timeClient.getEpochTime())+"\",\"offset\":"+timeOffset;
  msg += ", \"vals\":[\"" + rings[0] + "\"" ;
  for (i = 1; i < MAX_RINGS; i++) {
    msg += ",\"" + rings[i] + "\"" ;
  }
  msg += "]}";

  Serial.print(msg);

  return msg;
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", "");
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists

  server.on("/getRings", HTTP_GET, []() {
    server.send(200, "text/plain", getRings());
  });

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}


void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

/*__________________________________________________________SERVER_HANDLERS__________________________________________________________*/

//https://randomnerdtutorials.com/esp8266-web-server-spiffs-nodemcu/

void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/")) path = "/" + path;
    if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file
      String pathWithGz = path + ".gz";                  // So if an uploaded file is not compressed, the existing compressed
      if (SPIFFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
        SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}



/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
