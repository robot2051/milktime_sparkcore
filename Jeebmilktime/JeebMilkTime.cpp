/*
 DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. You just DO WHAT THE FUCK YOU WANT TO.
*/
#include <string>
#include <sstream>
#include "application.h"
#include "clickButton.h"
#include "spark_protocol.h"
#include "tropicssl/rsa.h"
#include "tropicssl/aes.h"
#include "spark_utilities.h"
#include "lcd.h"

#define SERVER "10.1.1.12"
#define PORT 9000
#define MAIN_SCREEN 0
#define MILK_TIME_SCREEN 1
#define MILK_AMOUNT_SCREEN 2
#define MILK_SUBMIT_SCREEN 3

#define EXPRESS_TIME_SCREEN 4
#define EXPRESS_AMOUNT_SCREEN 5
#define EXPRESS_SUBMIT_SCREEN 6

#define NAPPIE_TIME_SCREEN  7
#define NAPPIE_STATE_SCREEN 8
#define NAPPIE_SUBMIT_SCREEN  9

#define MAIN_STATE 0;
#define MILK_STATE 1;
#define EXPRESS_STATE 2;
#define NAPPIE_STATE 3;

#define BUTTON_1 0
#define BUTTON_2 1
#define BUTTON_3 2
//NAPPIE STATE
#define WET 0
#define SOILED 1
#define MILK_DEFAULT 100
#define EXPRESS_DEFAULT 250
// [MAIN_SCREEN,MILK_TIME_SCREEN,...NAPPIE_SUBMIT_SCREEN]

int screenmap[3][10] = {
  {MILK_TIME_SCREEN,MILK_TIME_SCREEN,MILK_AMOUNT_SCREEN,0,EXPRESS_TIME_SCREEN,EXPRESS_AMOUNT_SCREEN,0,NAPPIE_TIME_SCREEN,NAPPIE_STATE_SCREEN,0},
{EXPRESS_TIME_SCREEN,MILK_TIME_SCREEN,MILK_AMOUNT_SCREEN,0,EXPRESS_TIME_SCREEN,EXPRESS_AMOUNT_SCREEN,0,NAPPIE_TIME_SCREEN,NAPPIE_STATE_SCREEN,0},
{NAPPIE_TIME_SCREEN,MILK_AMOUNT_SCREEN,MILK_SUBMIT_SCREEN,100,EXPRESS_AMOUNT_SCREEN,EXPRESS_SUBMIT_SCREEN,100,NAPPIE_STATE_SCREEN,NAPPIE_SUBMIT_SCREEN,100}
};


// If you ever need to encrypt stuffs, look here: https://gist.github.com/towynlin/fb1f56bdd0a77b46cf09#file-spark-aes-encryption-demo-ino
//unsigned char key[16] = {0x40,0x76,0x65,0x72,0x79,0x73,0x33,0x63,0x75,0x72,0x33,0x4B,0x45,0x59,0x21,0x32};
//unsigned char iv[16] = {0x61,0x31,0x62,0x32,0x63,0x33,0x64,0x34,0x65,0x35,0x66,0x36,0x67,0x37,0x68,0x38};
//Declare functions

void ipArrayFromString(byte ipArray[], String ipString);
int connectToMyServer(String ip);
void testconnectToMyServer();
void reportTCP(String text);
void screenSet(int button_pressed, int clicks);
void drawScreen(int next_screen, int changes);

//---------------------

// Variables

TCPClient client;

ClickButton button1(BUTTON_1, LOW, CLICKBTN_PULLUP);
ClickButton button2(BUTTON_2, LOW, CLICKBTN_PULLUP);
ClickButton button3(BUTTON_3, LOW, CLICKBTN_PULLUP);
int function = 0;
int screen_state=MAIN_SCREEN;

int nappie = SOILED;
int milk = MILK_DEFAULT;
int express = EXPRESS_DEFAULT;
int etime;
String toptext = "    HARRO!!!    ";
String toptextSim = "NA";
String toptextKim = "NA";
//currentSetting : MILK - 0, EXPRESS - 1, NAPPIE - 2
int currentSetting = 0;
int connection_status = 1;

void setup()
{
  RGB.control(true);
  RGB.brightness(0);
  Time.zone(+11);
  Spark.syncTime();
  etime = Time.now();
  pinMode(D4, INPUT_PULLUP);
  Serial1.begin(9600);
  // Setup button timers (all in milliseconds / ms)
  // (These are default if not set, but changeable for convenience)
  button1.debounceTime   = 20;   // Debounce timer in ms
  button1.multiclickTime = 250;  // Time limit for multi clicks
  button1.longClickTime  = 1000; // time until "held-down clicks" register
  //printLCD("Hello World!");
  testconnectToMyServer();
}


void loop()
{
  // Update button state
  button1.Update();
  button2.Update();
  button3.Update();
  if(button3.clicks == -1){
     client.stop();
     testconnectToMyServer();
     delay(1000);
  }
  if (connection_status) {
    if (button1.clicks < 0){
      drawScreen(MAIN_SCREEN,0);
    }
        // Save click codes in LEDfunction, as click codes are reset at next Update()
    if (button1.clicks != 0) {
      screenSet(BUTTON_1,button1.clicks);
    }
    if (button2.clicks != 0) {
        screenSet(BUTTON_2,abs(button2.clicks));
    }
    if (button3.clicks != 0) {
        screenSet(BUTTON_3,button3.clicks);
    }
      delay(2);
  }
}

void screenSet(int button_pressed, int clicks){
  int nextscreen = screenmap[button_pressed][screen_state];

      int changes = 0;
      if (nextscreen == screen_state){
          if(button_pressed == BUTTON_1){
            changes = -clicks;
          } else if(button_pressed == BUTTON_2){
            changes = clicks;
          }
      }
      if (nextscreen < 100){
        drawScreen(nextscreen,changes);
      }
      else {
        //SUBMIT TCP HERE!
        switch(screen_state){
          case MILK_SUBMIT_SCREEN :
              currentSetting = 0;
              toptextSim = String(Time.hour(etime))+":"+String(Time.minute(etime));
              toptext = "S-"+toptextSim+"  "+"K-"+toptextKim;
              reportTCP("MILK,"+String(etime)+","+String(milk));
              break;
          case EXPRESS_SUBMIT_SCREEN :
              currentSetting = 1;
              toptextKim = String(Time.hour(etime))+":"+String(Time.minute(etime));
              toptext = "S-"+toptextSim+"  "+"K-"+toptextKim;
              reportTCP("EXPRESS,"+String(etime)+","+String(express));
              break;
          case NAPPIE_SUBMIT_SCREEN :
              currentSetting = 2;
              if(nappie == WET) reportTCP("NAPPIE,"+String(etime)+",WET");
              if(nappie == SOILED) reportTCP("NAPPIE,"+String(etime)+",SOILED");
              break;
          default:
              printLCD("Something went wrong");
              delay(1000);
              drawScreen(MAIN_SCREEN,0);
              break;
        }
      }

}


void drawScreen(int next_screen, int changes) {
  if (screen_state == MAIN_SCREEN){
    etime = 600 * (Time.now()/600); //Get epoch time and round it up to the 10th minutes
  }

  switch(next_screen){
        case MAIN_SCREEN :
          //RESET ALL VARIABLE TO DEFAULT AND SET MAIN_SCREEN.
            nappie = WET;
            milk = MILK_DEFAULT;
            express = EXPRESS_DEFAULT;
            printLCD(toptext);
            selectLineTwo();
            Serial1.write("MILK  EXPR  NAPP");
            button1.Update();
            button2.Update();
            button3.Update();
            break;
        case MILK_TIME_SCREEN :
            etime += 600 * changes;
            printLCD("MILK TIME:      " + String(Time.hour(etime))+":"+String(Time.minute(etime)));
            break;
        case MILK_AMOUNT_SCREEN :
            milk += 10 * changes;
            printLCD("MILK_AMOUNT_SCREEN");
            printLCD("MILK AMOUNT:    " + String(milk)+" ML");
            break;
        case MILK_SUBMIT_SCREEN :
            printLCD("Submit info:    "+String(Time.hour(etime))+":"+String(Time.minute(etime))+" - "+String(milk)+" ML");
            break;
        case EXPRESS_TIME_SCREEN :
            etime += 600 * changes;
            printLCD("EXPRESS TIME:   " + String(Time.hour(etime))+":"+String(Time.minute(etime)));
            break;
        case EXPRESS_AMOUNT_SCREEN :
            express += 10 * changes;
            printLCD("EXPRESS_AMOUNT_SCREEN");
            printLCD("MILK AMOUNT:    " + String(express)+" ML");
            break;
        case EXPRESS_SUBMIT_SCREEN :
            printLCD("Submit info:    "+String(Time.hour(etime))+":"+String(Time.minute(etime))+" - "+String(express)+" ML");
            break;
        case NAPPIE_TIME_SCREEN  :
            etime += 600 * changes;
            printLCD("NAPPIE TIME:    " + String(Time.hour(etime))+":"+String(Time.minute(etime)));
            break;
        case NAPPIE_STATE_SCREEN :
            if(nappie == WET) {
              nappie = SOILED;
              printLCD("NAPPIE STATUS:   SOILED");
            } else {
              nappie = WET;
              printLCD("NAPPIE STATUS:   WET");
            }
            break;
        case NAPPIE_SUBMIT_SCREEN  :
            if(nappie == WET) printLCD("Submit info:    "+String(Time.hour(etime))+":"+String(Time.minute(etime))+" - WET");
            if(nappie == SOILED) printLCD("Submit info:    "+String(Time.hour(etime))+":"+String(Time.minute(etime))+" - SOILED");
            break;
        default:
            printLCD("Something went wrong");
            delay(1000);
            drawScreen(MAIN_SCREEN,0);
            break;
   }
   screen_state=next_screen;
}


//Expand functions

void ipArrayFromString(byte ipArray[], String ipString) {
  int dot1 = ipString.indexOf('.');
  ipArray[0] = ipString.substring(0, dot1).toInt();
  int dot2 = ipString.indexOf('.', dot1 + 1);
  ipArray[1] = ipString.substring(dot1 + 1, dot2).toInt();
  dot1 = ipString.indexOf('.', dot2 + 1);
  ipArray[2] = ipString.substring(dot2 + 1, dot1).toInt();
  ipArray[3] = ipString.substring(dot1 + 1).toInt();
}

int connectToMyServer(String ip) {
  byte serverAddress[4];
  ipArrayFromString(serverAddress, ip);
  if (client.connect(serverAddress, PORT)) {
    connection_status = 1;
    return 1; // successfully connected
  } else {
    connection_status = 0;
    client.flush();
    client.stop();
    printLCD("Server dropped! Hold button 3!");
    return -1; // failed to connect
  }
}
void testconnectToMyServer(){
  if(connectToMyServer(SERVER)){
    if(client.connected()){
      printLCD("Server is online!");
      delay(1000);
      drawScreen(MAIN_SCREEN,0);
      client.flush();
      client.stop();
    } else {
      printLCD("Server dropped! Hold button 3!");
    }
  }
}

void reportTCP(String text){
    if(connectToMyServer(SERVER)){
      if(client.connected()){
        client.print(text.c_str());
        client.flush();
        client.stop();
        printLCD("Sent data!");
        delay(1000);
        drawScreen(MAIN_SCREEN,0);
      } else {
        printLCD("Server dropped! Hold button 3!");
      }
    }
    Spark.syncTime();
}
