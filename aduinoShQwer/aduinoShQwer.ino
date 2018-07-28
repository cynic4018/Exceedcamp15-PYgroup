#include <SoftwareSerial.h>
#include "DHT.h"
#define LED 2
#define LEDwalk 8
#define LEDg 4
#define LEDsentLIntens 7
#define ldr 17
#define SWITCH_LOCK 5
#define SWITCH_FOOT 6
#define DHTPIN 3
#define DHTTYPE DHT11
DHT dht (DHTPIN, DHTTYPE);
SoftwareSerial se_read(12, 13); // write only
SoftwareSerial se_write(10, 11); // read only
int MANcount;
struct ProjectData {
  int32_t local_switch;//light booking in auduino
  int32_t web_switch;//light booking from  web
  int32_t Walk_in;
  int32_t Walk_out;
  float Temperature;//Temperature dht
  int32_t Humidity;//Humidity dht
  int32_t turn_off_light;
} project_data = { 1, 1, 0, 0, 25.20, 30, 0}; //your value

struct ServerData {
  int32_t local_switch;//light booking in auduino
  int32_t web_switch;//light booking from  web
  int32_t Walk_in;
  int32_t Walk_out;
  float Temperature;//Temperature dht
  int32_t Huminity;//Humidity dht
  int32_t turn_off_light;
} server_data = { 1, 1, 0, 0, 25.20, 30, 0 }; // your value

const char GET_SERVER_DATA = 1;
const char GET_SERVER_DATA_RESULT = 2;
const char UPDATE_PROJECT_DATA = 3;

void send_to_nodemcu(char code, void *data, char data_size) {
  char *b = (char*)data;
  char sent_size = 0;
  while (se_write.write(code) == 0) {
    delay(1);
  }
  while (sent_size < data_size) {
    sent_size += se_write.write(b, data_size);
    delay(1);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  se_read.begin(38400);
  se_write.begin(38400);
  pinMode(LED, OUTPUT);
  pinMode(SWITCH_LOCK, INPUT);
  pinMode(SWITCH_FOOT, INPUT);
  pinMode(LEDg, OUTPUT);
  pinMode(ldr, INPUT);
  pinMode(LEDsentLIntens, OUTPUT);
  digitalWrite(LEDsentLIntens, HIGH);
  while (!se_read.isListening()) {
    se_read.listen();
  }

  Serial.println((int)sizeof(ServerData));
  Serial.println("ARDUINO READY!");
}

uint32_t last_sent_time = 0;
boolean is_data_header = false;
char expected_data_size = 0;
char cur_data_header = 0;
char buffer[256];
int8_t cur_buffer_length = -1;



void loop() {
  uint32_t cur_time = millis();
  //send to nodemcu
  if (cur_time - last_sent_time > 500) {//always update
    send_to_nodemcu(UPDATE_PROJECT_DATA, &project_data, sizeof(ProjectData));
    send_to_nodemcu(GET_SERVER_DATA, &server_data, sizeof(ServerData));
    last_sent_time = cur_time;
  }

  //read from sensor....
  //send to nodemcu

  //read data from server pass by nodemcu
  while (se_read.available()) {
    char ch = se_read.read();
    //Serial.print("RECV: ");
    //Serial.println((byte)ch);
    if (cur_buffer_length == -1) {
      cur_data_header = ch;
      switch (cur_data_header) {
        case GET_SERVER_DATA_RESULT:
          //unknown header
          expected_data_size = sizeof(ServerData);
          cur_buffer_length = 0;
          break;
      }
    } else if (cur_buffer_length < expected_data_size) {
      buffer[cur_buffer_length++] = ch;
      if (cur_buffer_length == expected_data_size) {
        switch (cur_data_header) {
          case GET_SERVER_DATA_RESULT: {
              ServerData *data = (ServerData*)buffer;
              //use data to control sensor
              server_data.local_switch = data->local_switch;
              server_data.web_switch = data->web_switch;
              server_data.turn_off_light = data->turn_off_light;
            }
            cur_buffer_length = -1;
        }
      }
    }
  }
  /////////////////////////////// Read device ////////////////////
  project_data.Temperature = dht.readTemperature();
  project_data.Humidity = dht.readHumidity();
  //    project_data.Light_intensity = analogRead(ldr);
  project_data.local_switch = digitalRead(SWITCH_LOCK);
  ///////////////////////////////////////////////////////////////
  //Lock switch turn led lock
  Serial.print("webswitch from server :");
  Serial.println(server_data.web_switch);

  Serial.print("localswitch from local :");
  Serial.println(project_data.local_switch); 
  /*if (project_data.local_switch == 0 )
  {
    digitalWrite(LEDwalk, HIGH);

    if (project_data.local_switch == 1)
    {
      digitalWrite(LEDwalk, LOW);
    }
  }
  else if (server_data.web_switch == 0)
  {
    digitalWrite(LED, HIGH);
    if (project_data.local_switch == 0)
    {
      digitalWrite(LED, LOW);
      digitalWrite(LEDwalk, HIGH);
      if(project_data.local_switch == 1)
      {
        digitalWrite(LEDwalk, LOW);
      }
    }
  }*/
  if(project_data.local_switch == 0){
    digitalWrite(LEDwalk,HIGH);
  }
  else if(project_data.local_switch == 1){
    digitalWrite(LEDwalk,LOW);
  }
  if(server_data.web_switch == 0){
    digitalWrite(LED,HIGH);
  }
  else if(server_data.web_switch == 1){
    digitalWrite(LED,LOW);
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////////
  int a = 0;
  int ldr_state = 0;
  int switch_state = 0;

  Serial.println(project_data.Walk_in);
  Serial.println(project_data.Walk_out);
  if (analogRead(ldr) <= 150 && switch_state == 0 && ldr_state == 0)
  {
    int ldr_state = 1;
    while (digitalRead(SWITCH_FOOT) == 1 && a <= 15)
    {
      delay(200);
      a++;
      Serial.print("a :");
      Serial.println(a);
      if (a >= 15)
      {
        ldr_state = 0;
        break;
      }
    }
    if (digitalRead(SWITCH_FOOT) == 0 && ldr_state == 1)
    {
      project_data.Walk_in += 1;
    }
    delay(250);
  }

  else if (digitalRead(SWITCH_FOOT) == 0 && ldr_state == 0 && switch_state == 0)
  {
    switch_state = 1;
    while (analogRead(ldr) >= 300 && a <= 15)
    {
      delay(200);
      a++;
      Serial.print("a :");
          Serial.println(a);
      if (a == 15)
      {
        switch_state = 0;
        break;
      }
    }
    if (digitalRead(analogRead(ldr)) <= 300 && switch_state == 1)
    {
      project_data.Walk_out += 1;
    }
    delay(500);
  }

  //delay(500);
  
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  MANcount = project_data.Walk_in-project_data.Walk_out;
  if(MANcount>0)
    {
      digitalWrite(LEDg, HIGH);
    }
  else
  {
    digitalWrite(LEDg, LOW);
    }
    delay(500);
}

