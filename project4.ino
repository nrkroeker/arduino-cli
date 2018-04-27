#include <WiFi.h>
#include <WiFiUdp.h>
#include "EEPROM.h"
#include "time.h"

// const char* ssid = "Verizon-SM-G950U-339D"; -- SET IN CLI
// const char* password = "utkn871/"; -- SET IN CLI

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5*3600;
const int daylightOffset_sec = 3600;

volatile int interruptCounter = 0;
int numberOfInterrupts = 0;

WiFiUDP udp;
IPAddress udpIP(192,168,43,26); // Only way to set a default IP
// int udpPort = 9930; -- SET IN CLI

#define trigPin0 3
#define echoPin0 34
#define trigPin1 21
#define echoPin1 35
#define trigPin2 19
#define echoPin2 33
#define trigPin3 18
#define echoPin3 25

#define k 16//maximum distance timestamp pairs in packet

float distance0,distance1,distance2,distance3;
long duration0,duration1,duration2,duration3;
long startTime0,startTime1,startTime2,startTime3;
bool sensorReady0, sensorReady1, sensorReady2, sensorReady3; //bool sensor
long endTime0,endTime1,endTime2,endTime3;

// Definitions, function declarations, and variables for CLI
#define CLI_BUF_SIZE 128
#define ARG_BUF_SIZE 128
#define ARG_MAX_NUM 3
#define ARG_MAX_SIZE 84
#define EEPROM_SIZE 64
#define CONFIG_ADDR 0

int cmd_help();
int cmd_show();
int cmd_set();
int cmd_run();
void run_cli();

// Function pointers corresponding to each command
int (*commands_func[4])() = {
    &cmd_help,
    &cmd_show,
    &cmd_set,
    &cmd_run
};

char cli_line[CLI_BUF_SIZE];
char args[ARG_MAX_NUM][ARG_BUF_SIZE];

// Variables shared between CLI and other stuff
struct Config {
    char ssid[ARG_MAX_SIZE] = "Verizon-SM-G950U-339D";
    char password[ARG_MAX_SIZE] = "utkn871/";
    IPAddress ipaddress = udpIP; // pass into udpIP
    unsigned int port = 9930;
    unsigned int period = 250; // call when delaying sense distance task
    unsigned int buffer = 1024; 
} configVars; // set defaults to configVars

// Character arrays of each set of CLI commands
const char *commands_str[] = {
    "HELP",
    "SHOW",
    "SET",
    "RUN"
};
int num_commands = sizeof(commands_str) / sizeof(char *);

const char *param_args[] = {
    "--ssid",
    "--password",
    "--ipaddress",
    "--port",
    "--period",
    "--buffer"
};
int num_params = sizeof(param_args) / sizeof(char *);

// Function definitions for reading time/distance
void getTime(void *pvParameters);
void readDistance0(void *pvParameters);
void readDistance1(void *pvParameters);
void readDistance2(void *pvParameters);
void readDistance3(void *pvParameters);
void producer(void *pvParameters);
void sendPacket(void *pvParameters);

portMUX_TYPE consumerProducerMux = portMUX_INITIALIZER_UNLOCKED;

void startTimer0();
void endTimer0();
void startTimer1();
void endTimer1();
void startTimer2();
void endTimer2();
void startTimer3();
void endTimer3();
void writePulseToTriggerPin(const int trigPin);

unsigned long millisEpoch;
int sendInitialEpoch=1;
struct tm timeinfo;
time_t epoch=0;

//shared string buffer and buffer size
char consumerProducerBuffer[96][32];
char commonProducerBuffer[16][5];
int BUFFER_SIZE = 10;

int counter = 0;
int in=0;
int out=0;

void setup() {  
    Serial.begin(115200);
    while (!Serial) {};
    
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("Couldn't initialize EEPROM");
        delay(1000000);
    }
    EEPROM.get(CONFIG_ADDR, configVars);
    run_cli();
    pinMode(trigPin0, OUTPUT);
    pinMode(echoPin0, INPUT);
    pinMode(trigPin1, OUTPUT);
    pinMode(echoPin1, INPUT);
    pinMode(trigPin2, OUTPUT);
    pinMode(echoPin2, INPUT);
    pinMode(trigPin3, OUTPUT);
    pinMode(echoPin3, INPUT);

    Serial.printf("Connecting to %s", configVars.ssid);
    WiFi.begin(configVars.ssid,configVars.password);
    while(WiFi.status()!=WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }
    Serial.println("CONNECTED");

    attachInterrupt(digitalPinToInterrupt(echoPin0),startTimer0,RISING);
    attachInterrupt(digitalPinToInterrupt(echoPin1),startTimer1,RISING);
    attachInterrupt(digitalPinToInterrupt(echoPin2),startTimer2,RISING);
    attachInterrupt(digitalPinToInterrupt(echoPin3),startTimer3,RISING);

    sensorReady0=false;
    sensorReady1=false;
    sensorReady2=false;
    sensorReady3=false;
    
    xTaskCreate(readDistance0, "readDistance0", 4000, NULL, 2, NULL);
    xTaskCreate(readDistance1, "readDistance1", 4000, NULL, 2, NULL);
    xTaskCreate(readDistance2, "readDistance2", 4000, NULL, 2, NULL);
    xTaskCreate(readDistance3, "readDistance3", 4000, NULL, 2, NULL);
    xTaskCreate(getTime, "getTIme", 4000, NULL, 1, NULL); 
    xTaskCreate(producer, "producer", 4000, NULL, 3, NULL); 
    xTaskCreate(sendPacket,"sendPacket", 4000, NULL, 3, NULL);
}

void loop() {
 // Don't loop, we have tasks running
}

void run_cli() {
    String line_string;
    // Continuously search for serial input
    int returnVar = 0;

    while(returnVar == 0) {
        while (!Serial.available());
        if (Serial.available()) {
            line_string = Serial.readStringUntil('\n');
            // When received input, parse and execute command
            if (line_string.length() < CLI_BUF_SIZE) {
                line_string.toCharArray(cli_line, CLI_BUF_SIZE);
                Serial.println(line_string);
                returnVar = parseLine();
            } else {
                Serial.println("Command entered is too long.");
            }
        }
        // Reset variables for next reading of serial input
        memset(cli_line, 0, CLI_BUF_SIZE);
        memset(args, 0, sizeof(args[ARG_MAX_NUM][ARG_BUF_SIZE]));
    }
    Serial.println("Running sensor program...");
}

int parseLine() {
    char *argument;
    int counter = 0;
    argument = strtok(cli_line, " ");

    // Loop through arguments separated by space
    while ((argument != NULL)) {
        if (counter < ARG_MAX_NUM) {
            if (strlen(argument) < ARG_BUF_SIZE) {
                // Add argument to array
                strcpy(args[counter], argument);
                argument = strtok(NULL, " ");
                counter++;
            } else {
                Serial.println("Command entered is too long.");
                break;
            }
        } else {
            break;
        }
    }
    // Find base command function matching input
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(args[0], commands_str[i]) == 0) {
            return (*commands_func[i])();
        }
    }
    Serial.println("Invalid command. Type HELP for more information.");
    return 0;
}

int cmd_run() {
    Serial.println("[RUN] Now switching from CLI mode to run mode.");
    return 1;
}

// HELP functions to provide clarification about commands
void help_help() {
    Serial.println("[HELP] Command: HELP\n[HELP] Description: displays the different CLI options.");
    Serial.println("[HELP] Commands: ");
    for (int i = 0; i < num_commands; i++) {
        Serial.print(" - ");
        Serial.println(commands_str[i]);
    }
    Serial.println("[HELP] Usage: Type HELP <command> for details on that specific option (ex. \"HELP SHOW\").");
}

void help_show() {
    Serial.println("[HELP] Command: SHOW");
    Serial.println("[HELP] Description: Displays the data values currently set for a given parameter.");
    print_params();
    Serial.println("[HELP] Usage: Type SHOW <param> to see that variable's value (ex. \"SHOW  --ssid\").");
}

void help_set() {
    Serial.println("[HELP] Command: SET");
    Serial.println("[HELP] Description: Allows you to alter a variable in the program.");
    print_params();
    Serial.println("[HELP] Usage: Type SET <param> <value> to change that variable (ex. \"SET --ssid wifi-network-name\").");
}

void help_run() {
    Serial.println("[HELP] Command: RUN");
    Serial.println("[HELP] Description: Switches from CLI mode to run mode.");
}

int cmd_help() {
    if (args[1] == NULL) {
        Serial.println("Please enter a valid command. See HELP options for more information.");
        help_help();
    } else if (strcmp(args[1], commands_str[0]) == 0) {
        help_help();
    } else if (strcmp(args[1], commands_str[1]) == 0) {
        help_show();
    } else if (strcmp(args[1], commands_str[2]) == 0) {
        help_set();
    } else if (strcmp(args[1], commands_str[3]) == 0) {
        help_run();
    } else {
        Serial.println("Please enter a valid command. See HELP options for more information.");
        help_help();
    }
    return 0;
}

void print_params() {
    Serial.println("[HELP] Parameters:");
    for (int i = 0; i < num_params; i++) {
        Serial.print("  ");
        Serial.println(param_args[i]);
    }
}

// SHOW function to display currently set values
int cmd_show() {
    if (args[1] == NULL) {
        Serial.print("SSID: ");
        Serial.println(configVars.ssid);
        Serial.print("Password: ");
        Serial.println(configVars.password);
        Serial.print("IP Address: ");
        Serial.println(configVars.ipaddress);
        Serial.print("Port: ");
        Serial.println(configVars.port);
        Serial.print("Period: ");
        Serial.println(configVars.period);
        Serial.print("Buffer: ");
        Serial.println(configVars.buffer);
    } else if (strcmp(args[1], param_args[0]) == 0) {
        Serial.print("SSID: ");
        Serial.println(configVars.ssid);
    } else if (strcmp(args[1], param_args[1]) == 0) {
        Serial.print("Password: ");
        Serial.println(configVars.password);
    } else if (strcmp(args[1], param_args[2]) == 0) {
        Serial.print("IP Address: ");
        Serial.println(configVars.ipaddress);
    } else if (strcmp(args[1], param_args[3]) == 0) {
        Serial.print("Port: ");
        Serial.println(configVars.port);
    } else if (strcmp(args[1], param_args[4]) == 0) {
        Serial.print("Period: ");
        Serial.println(configVars.period);
     } else if (strcmp(args[1], param_args[5]) == 0) {
        Serial.print("Buffer: ");
        Serial.println(configVars.buffer);
    } else {
        Serial.println("[SHOW] You must specify a valid parameter or none. Type HELP SHOW for more information.");
    }
    return 0;
}

// SET function to alter the specified parameter
int cmd_set() {
    bool error = false;
    if (args[1] == NULL) {
        Serial.println("[SET] You must specify a valid parameter. Type HELP SET for more information");
    } else if (strcmp(args[1], param_args[0]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            strcpy(configVars.ssid, args[2]);
        }
    } else if (strcmp(args[1], param_args[1]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            strcpy(configVars.password, args[2]);
        }
    } else if (strcmp(args[1], param_args[2]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            int counter = 0;
            char* ipVal = strtok(args[2], ".");
            while (ipVal != NULL) {
                if (counter < 5) {
                    configVars.ipaddress[counter] = atoi(ipVal);
                    ipVal = strtok(NULL, ".");
                    counter++;
                } else {
                    Serial.println("IP Address is invalid, too many characters.");
                    break;
                }
            }
        }
    } else if (strcmp(args[1], param_args[3]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            configVars.port = atoi(strcat(args[2],"\n"));
        }
    } else if (strcmp(args[1], param_args[4]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            configVars.period = atoi(strcat(args[2],"\n"));
        }
    } else if (strcmp(args[1], param_args[5]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            configVars.buffer = atoi(strcat(args[2],"\n"));
        }
    }
    else {
        Serial.println("[SET] You must specify a valid parameter. Type HELP SET for more information");
    }
    if (error == true) {
        Serial.println("[SET] You must specify a value. Type HELP SET for more information.");
    } else {
        Serial.print("Successfully set ");
        Serial.println(args[2]);
        EEPROM.put(0, configVars);
    }
    return 0;
}

void sendPacket(void *pvParameters){
  (void)pvParameters;
  int out=0;
//   char sendingBuffer[1024]=""; This is the size it needs to be set to in the CLI
    char sendingBuffer[configVars.buffer]="";

  for(;;){
    epoch=time(NULL);
    getLocalTime(&timeinfo);

    //waits for clock to be updated and then sends epoch
    if(sendInitialEpoch==1&&!timeinfo.tm_year<(1970-1900)) {
      sendInitialEpoch=0;
      delay(500);
      char stringEpoch[32];
      epoch=time(NULL);
      millisEpoch=millis();
      
      double doubleEpoch=epoch;
      sprintf(stringEpoch, "Epoch Time = %.2f", doubleEpoch);
      
      Serial.println(stringEpoch);
      
      uint8_t  *uintEpoch=(uint8_t*)(stringEpoch);
      udp.beginPacket(configVars.ipaddress, configVars.port);
      udp.write(uintEpoch,32);
      udp.endPacket();

      delay(1000);
    } else {
      portENTER_CRITICAL(&consumerProducerMux);

      if(counter!=0){

        int tempCounter=counter;
        int savedCounter=counter;
        if(tempCounter>k) {
          tempCounter=k;
        }

        for(;tempCounter>0;tempCounter--){
          strcat(sendingBuffer,consumerProducerBuffer[out]);
          strcpy(consumerProducerBuffer[out],"");
          out=(out+1)%BUFFER_SIZE;
        }

        Serial.println(sendingBuffer);
        Serial.println(counter);
        
        counter=counter-savedCounter;
       
        portEXIT_CRITICAL(&consumerProducerMux);

        uint8_t  *packet=(uint8_t*)(sendingBuffer);

        udp.beginPacket(configVars.ipaddress, configVars.port);
        udp.write(packet,1024);
        udp.endPacket();
        
        strcpy(sendingBuffer,"");
      } else {
        portEXIT_CRITICAL(&consumerProducerMux);
      }
    } 
      delay(1000);

  }
}

void producer(void *pvParameters){
  (void)pvParameters;  

  unsigned long millisStamp;

  char sensorBuffer0[16];
  char sensorBuffer1[16];
  char sensorBuffer2[16];
  char sensorBuffer3[16];
  char timestampBuffer[16];
  
  for(;;){
    if(sensorReady0&&sensorReady1&&sensorReady2&&sensorReady3) {
        distance0=(duration0/2)*0.0343;
        distance1=(duration1/2)*0.0343;
        distance2=(duration2/2)*0.0343;
        distance3=(duration3/2)*0.0343;

        sprintf(sensorBuffer0, "%.2f",distance0);
        sprintf(sensorBuffer1, "%.2f",distance1);
        sprintf(sensorBuffer2, "%.2f",distance2);
        sprintf(sensorBuffer3, "%.2f",distance3);

        millisStamp=millis();
        millisStamp=millisStamp-millisEpoch;

        sprintf(timestampBuffer, "%lu",millisStamp);    
        portENTER_CRITICAL(&consumerProducerMux);

        if(counter!=BUFFER_SIZE){
            strcpy(consumerProducerBuffer[in], "");
            strcat(consumerProducerBuffer[in], sensorBuffer0);
            strcat(consumerProducerBuffer[in], ",");
            strcat(consumerProducerBuffer[in], sensorBuffer1);
            strcat(consumerProducerBuffer[in], ",");
            strcat(consumerProducerBuffer[in], sensorBuffer2);
            strcat(consumerProducerBuffer[in], ",");
            strcat(consumerProducerBuffer[in], sensorBuffer3);
            strcat(consumerProducerBuffer[in], ",");
            strcat(consumerProducerBuffer[in], timestampBuffer);
            strcat(consumerProducerBuffer[in], ";");
            Serial.println(consumerProducerBuffer[in]);

            in=(in+1)%configVars.buffer;
            counter++;      
            portEXIT_CRITICAL(&consumerProducerMux);
        }      
        sensorReady0=false;
        sensorReady1=false;
        sensorReady2=false;
        sensorReady3=false;
    }
    // Set period to 250 in CLI
    delay(configVars.period);    

  }
}

void readDistance0(void *pvParameters){
  (void)pvParameters;
  for(;;){
      if(!sensorReady0){
        writePulseToTriggerPin(trigPin0);
      }
      delay(configVars.period);
  }
}

void readDistance1(void *pvParameters){
  (void)pvParameters;
  for(;;){
    if(!sensorReady1){
      writePulseToTriggerPin(trigPin1);
    }  
      delay(configVars.period);
  }
}

void readDistance2(void *pvParameters){
  (void)pvParameters;
  for(;;){
    if(!sensorReady2){
        writePulseToTriggerPin(trigPin2);
    }
      delay(configVars.period);
  }   
}

void readDistance3(void *pvParameters){
  (void)pvParameters;
  for(;;){
     if(!sensorReady0){
        writePulseToTriggerPin(trigPin3); 
     }       
    delay(configVars.period);
  }
}

// Retrieve distance from sensor pin
void writePulseToTriggerPin(const int trigPin){
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
}

// Start/end timer methods
void startTimer0(){
  startTime0 = micros();
  attachInterrupt(digitalPinToInterrupt(echoPin0), endTimer0, FALLING);
}

void endTimer0(){
  endTime0 = micros();
  duration0 = endTime0 - startTime0;
  sensorReady0 = true;
  attachInterrupt(digitalPinToInterrupt(echoPin0), startTimer0, RISING);
}

void startTimer1(){
  startTime1 = micros();
  attachInterrupt(digitalPinToInterrupt(echoPin1), endTimer1, FALLING);
}

void endTimer1(){
  endTime1 = micros();
  duration1 = endTime1 - startTime1;
  sensorReady1 = true;
  attachInterrupt(digitalPinToInterrupt(echoPin1), startTimer1, RISING);
}

void startTimer2(){
  startTime2 = micros();
  attachInterrupt(digitalPinToInterrupt(echoPin2), endTimer2, FALLING);
}

void endTimer2(){  
  endTime2 = micros();
  duration2 = endTime2 - startTime2;
  sensorReady2 = true;
  attachInterrupt(digitalPinToInterrupt(echoPin2), startTimer2, RISING);
}

void startTimer3(){
  startTime3 = micros();
  attachInterrupt(digitalPinToInterrupt(echoPin3), endTimer3, FALLING);
}

void endTimer3(){
  endTime3 = micros();
  duration3 = endTime3 - startTime3;
  sensorReady3 = true;
  attachInterrupt(digitalPinToInterrupt(echoPin3), startTimer3, RISING);
}

// Update time from server
void getTime(void *pvParameters){
  (void)pvParameters;  

  for(;;){
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    sendInitialEpoch=1;
    
    delay(48*1000*3600);
  }
}
