#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <EspSoftwareSerial.h>
#include <math.h>

SoftwareSerial se_read(D5, D6); // write only
SoftwareSerial se_write(D0, D1); // read only
String const url = "http://ecourse.cpe.ku.ac.th/exceed/api/";

struct ProjectData {
  int32_t local_switch;//light booking in auduino
  int32_t web_switch;//light booking from  web
  int32_t Walk_in;
  int32_t Walk_out;
  float Temperature;//Temperature dht
  int32_t Huminity;//Humidity dht
  int32_t turn_off_light;
  int32_t turn_on_light;
} cur_project_data;

struct ServerData {
  int32_t local_switch;//light booking in auduino
  int32_t web_switch;//light booking from  web
  int32_t Walk_in;
  int32_t Walk_out;
  float Temperature;//Temperature dht
  int32_t Huminity;//Humidity dht
  int32_t turn_off_light;
  int32_t turn_on_light;
} server_data;

const char GET_SERVER_DATA = 1;
const char GET_SERVER_DATA_RESULT = 2;
const char UPDATE_PROJECT_DATA = 3;

// wifi configuration
const char SSID[] = "EXCEED_RIGHT_1_2.4GHz";
const char PASSWORD[] = "1234567890";

// for nodemcu communication
uint32_t last_sent_time = 0;
char expected_data_size = 0;
char cur_data_header = 0;
char buffer[256];
int8_t cur_buffer_length = -1;

void send_to_arduino(char code, void *data, char data_size) {
  char *b = (char*)data;
  int sent_size = 0;
  while (se_write.write(code) == 0) {
    delay(1);
  }
  while (sent_size < data_size) {
    sent_size += se_write.write(b, data_size);
    delay(1);
  }
}

void wifi_initialization() {
  Serial.println("WIFI INITIALIZING.");

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    yield();
    delay(10);
  }

  Serial.println("WIFI INITIALIZED.");
}

void serial_initialization() {
  Serial.begin(115200);
  se_read.begin(38400);
  se_write.begin(38400);

  while (!se_read.isListening()) {
    se_read.listen();
  }

  Serial.println();
  Serial.println("SERIAL INITIALIZED.");
}

String set_builder(const char *key, int32_t value) {
  String str = url;
  str = str + key;
  str = str + "/set?value=";
  str = str + value;
  Serial.println(str);
  return str;
}

String get_builder(const char *key) {
  String str = url;
  str = str + key;
  str = str + "/view/";
  return str;
}

void update_data_to_server_callback(String const &str) {
  Serial.println("update_data_to_server_callback FINISHED!");
}

bool GET(const char *url, void (*callback)(String const &str, int32_t &value), int32_t &value) {
  HTTPClient main_client;
  main_client.begin(url);
  if (main_client.GET() == HTTP_CODE_OK) {
    Serial.println("GET REQUEST RESPONSE BEGIN");
    if (callback != 0) {
      callback(main_client.getString(), value);
    }
    delay(200);
    return true;
  }
  Serial.println("GET REQUEST RESPONSE BEGIN");
  return false;
}
bool GET(const char *url, void (*callback)(String const &str, float &value), float &value) {
  HTTPClient main_client;
  main_client.begin(url);
  if (main_client.GET() == HTTP_CODE_OK) {
    Serial.println("GET REQUEST RESPONSE BEGIN");
    if (callback != 0) {
      callback(main_client.getString(), value);
    }
    delay(200);
    return true;
  }
  Serial.println("GET REQUEST RESPONSE BEGIN");
  return false;
}

bool POST(const char *url, void (*callback)(String const &str)) {
  HTTPClient main_client;
  main_client.begin(url);
  if (main_client.GET() == HTTP_CODE_OK) {
    Serial.println("POST REQUEST RESPONSE BEGIN");
    if (callback != 0) {
      callback(main_client.getString());
    }
    delay(200);
    return true;
  }
  Serial.println("POST REQUEST RESPONSE BEGIN");
  return false;
}

int get_request_int(String const &str) {
  int32_t tmp = str.toInt();
  return tmp;
}

float get_request_float(String const &str) {
  float tmp = str.toFloat();
  return tmp;
}
void get_request(String const &str, int32_t &value) {
  value = get_request_int(str);
}
void get_request(String const &str, float &value) {
  value = get_request_float(str);
}

void get_request_raw_callback(String const &str) {
  Serial.println("GET REQUEST RESPONSE BEGIN");
  Serial.println("========================");
  Serial.println(str.c_str());
  Serial.println("========================");
  Serial.println("GET REQUEST RESPONSE END");
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  serial_initialization();
  wifi_initialization();

  Serial.print("sizeof(ServerData): ");
  Serial.println((int)sizeof(ServerData));
  Serial.print("ESP READY!");
}
void loop() {
  /* 1) Server polling data from server every 1500 ms
     2) Arduino always get local data
  */

  uint32_t cur_time = millis();
  if (cur_time - last_sent_time > 1000) {
    GET(get_builder("palmyut-webswitch").c_str(), get_request, server_data.web_switch);
    Serial.print("webswitch : ");
    Serial.println(server_data.web_switch);
    last_sent_time = cur_time;
  }

  while (se_read.available()) {
    char ch = se_read.read();
    //Serial.print("RECV: ");0
    //Serial.println((byte)ch);
    if (cur_buffer_length == -1) {
      cur_data_header = ch;
      switch (cur_data_header) {
        case UPDATE_PROJECT_DATA:
          expected_data_size = sizeof(ProjectData);
          cur_buffer_length = 0;
          break;
        case GET_SERVER_DATA:
          expected_data_size = sizeof(ServerData);
          cur_buffer_length = 0;
          break;
      }
    } else if (cur_buffer_length < expected_data_size) {
      buffer[cur_buffer_length++] = ch;
      if (cur_buffer_length == expected_data_size) {
        switch (cur_data_header) {
          case UPDATE_PROJECT_DATA: {
              ProjectData *project_data = (ProjectData*)buffer;
              int32_t local_switch = project_data->local_switch;
              int32_t Walk_in = project_data->Walk_in;
              int32_t Walk_out = project_data->Walk_out;
              float Temperature = project_data->Temperature;
              int32_t Huminity = project_data->Huminity;
              POST(set_builder("palmyut-localswitch", local_switch).c_str(), update_data_to_server_callback);
              POST(set_builder("palmyut-walkin", Walk_in).c_str(), update_data_to_server_callback);
              POST(set_builder("palmyut-walkout", Walk_out).c_str(), update_data_to_server_callback);
              POST(set_builder("palmyut-Temperature", Temperature).c_str(), update_data_to_server_callback);
              POST(set_builder("palmyut-Huminity", Huminity).c_str(), update_data_to_server_callback);
              
            }
            break;
          case GET_SERVER_DATA:
            Serial.println("Send data to arduino");
            send_to_arduino(GET_SERVER_DATA_RESULT, &server_data, sizeof(ServerData));
            break;
        }
        cur_buffer_length = -1;
      }
    }
  }
  delay(500);
}
