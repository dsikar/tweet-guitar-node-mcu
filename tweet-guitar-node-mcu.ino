/**
 * Authorization.ino
 *
 *  Created on: 09.12.2015
 *
 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;

/**********************
START GUITAR INIT VALS
**********************/

// Global variables to compute delays
int iMoveX;
int iMoveY;

// To accept commands via serial monitor
String txtMsg = ""; 

// Use these pins on Nano to match NodeMCU
int iXStepPin = 16;
int iXDirPin = 5;
int iYStepPin = 4;

// Changed to pin 2 to avoid potential Serial RX TX pin conflicts
// int iYDirPin = 0;
int iYDirPin = 2;

// Direction generics
int iFwd = 1;
int iBwd = 0;

// Exception case flag for homefwdx
bool bSafe;

// End Stop expected threshold AD Conversion
int iThreshold = 185;

// Serial debug. Set to 1 to debug.
#define SERIAL_DEBUG 0

// Current X and Y position markers
int iXPos;
int iYPos;
int iXInitPos = 25;
int iYInitPos = 36;

// Relative origin
int iYRelPos;
int iXRelPos;

// String position mapping
int iOneStringStep = 50;


/**********************
END GUITAR INIT VALS
**********************/

void setup() {

    // pinMode(15, OUTPUT);
    
    // init pins
    initGuitar();
  
    USE_SERIAL.begin(115200);
   // USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    WiFiMulti.addAP("Makerversity_2G", "mak3rv3rs1ty");

}

void initGuitar() {
  pinMode(iXDirPin, OUTPUT);
  pinMode(iXStepPin, OUTPUT);  
  pinMode(iYDirPin, OUTPUT);
  pinMode(iYStepPin, OUTPUT);
}

void loop() {
    // wait for WiFi connection
    if((WiFiMulti.run() == WL_CONNECTED)) {

        HTTPClient http;

        USE_SERIAL.print("[HTTP] begin...\n");
        // configure traged server and url
        
        // String httpReq = "http://52.213.86.82/date.php?analogRead=";
        String httpReq = "http://sikarsystems.com/jukebox/play.php";
        // httpReq += analogRead(iMQ_4_pin);
        
        USE_SERIAL.println(httpReq);
                
        // http.begin("http://52.209.97.70/date.php");
        http.begin(httpReq);
        
        /*
          // or
          http.begin("http://192.168.1.12/test.html");
          http.setAuthorization("user", "password");

          // or
          http.begin("http://192.168.1.12/test.html");
          http.setAuthorization("dXNlcjpwYXN3b3Jk");
         */


        USE_SERIAL.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                USE_SERIAL.println(payload);
                parse(payload);
                // For future reference
                // USE_SERIAL.println(analogRead(0));
            }
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
    // USE_SERIAL.println(analogRead(0));
    delay(10000);

}


/********************
START MOVE FUNCTIONS
********************/

void movx(int iDir, int iSteps, int iDelay) {
  
  digitalWrite(iXDirPin, iDir); 
  
  for(int i = 0; i < iSteps; i++) {
    digitalWrite(iXStepPin, LOW);
    delay(iDelay);
    digitalWrite(iXStepPin, HIGH);
    if(bEnd() && bSafe) {
      break; 
    }
  }
}

void movy(int iDir, int iSteps, int iDelay) {

  digitalWrite(iYDirPin, iDir);

  for(int i = 0; i < iSteps; i++) {
    digitalWrite(iYStepPin, LOW);
    delay(iDelay);
    digitalWrite(iYStepPin, HIGH);
    if(bEnd() && bSafe) {
      break; 
    }
  }
}

bool bEnd() {
  if(analogRead(0) > iThreshold) {
   return true;
  }
  return false; 
}

void homeX() {
  while(!(bEnd())) {
    movx(iBwd, 1, 1);  
  }
  bSafe = false;
  movx(iFwd, iXInitPos, 1);
  iXPos = iXInitPos;
  iXRelPos = 0;
  bSafe = true;  
  iMoveX = 0;
}

void homeY() {
  while(!(bEnd())) {
    movy(iBwd, 1, 1);  
  } 
  bSafe = false;
  movy(iFwd, iYInitPos , 1);
  iYPos = iYInitPos; 
  iYRelPos = 0;
  bSafe = true;
  iMoveY = 0;  
}

void homeXY() {
  homeY();
  homeX();
}

void pluckString(int iIndex) {
   // iIndex
   // 1-E, 2-A, 3-D, 4-G, 5-B, 6-E
   
   // Safety net
   if(iIndex > 6) {
     return;
   }
   
   // int iMove;
   
   // Downwards movement
   if(iYRelPos < (iOneStringStep * iIndex)) {
      iMoveY = (iOneStringStep * iIndex) - iYRelPos;
      if(SERIAL_DEBUG) {
        USE_SERIAL.print("Total Y move = ");
        USE_SERIAL.print(iMoveY);
      }      
      movy(iFwd, iMoveY, 1);
      iYRelPos = iYRelPos + iMoveY;
      if(SERIAL_DEBUG) {
        USE_SERIAL.print("Current relative position = ");
        USE_SERIAL.print(iYRelPos);
      }
      return;     
   }
   
   // Upwards movement
   if(iYRelPos >= (iOneStringStep * iIndex)) {
      iMoveY = iYRelPos - (iOneStringStep * (iIndex - 1));
      if(SERIAL_DEBUG) {
        USE_SERIAL.print("Total Y move = ");
        USE_SERIAL.print(iMoveY);
      }   
      movy(iBwd, iMoveY, 1);
      iYRelPos = iYRelPos - iMoveY;
      if(SERIAL_DEBUG) {
        USE_SERIAL.print("Current relative position = ");
        USE_SERIAL.print(iYRelPos);
      }
      return;     
   }   
}

int getXPos(int iIndex) {
  // dummy return value
  // if(iIndex == 1
  // return (iIndex) * 50;
  // create lookup table from calibration 
  
  switch (iIndex) {
    case 1:
      return 20;
    case 2:
      return 185;
    case 3:
      return 350;
    case 4:
      return 505;
    case 5:
      return 655;      
    case 6:
      return 790;      
    case 7:
      return 925;      
    case 8:
      return 1050;
    case 9:
      return 1140;
    case 10:
      return 1260;      
    case 11:
      return 1360;
    case 12:
      return 1465; 
    default: 
      return 20;
    break;
  }
  
}

void slideFret(int iIndex) {
   // iIndex
   // 1-F#, 2-G, 3-G#, 4-A, etc
   
      // Safety net
   if(iIndex > 16) {
     return;
   }
   
   // int iMove;
   int iMoveXPos = getXPos(iIndex);
   if(SERIAL_DEBUG) {
     USE_SERIAL.print("iMoveXPos = ");
     USE_SERIAL.print(iMoveXPos);
     USE_SERIAL.print("iXRelPos = ");
     USE_SERIAL.print(iXRelPos);
   }   
   if(iXRelPos < iMoveXPos) {
     iMoveX = iMoveXPos - iXRelPos;
      if(SERIAL_DEBUG) {
        USE_SERIAL.print("Total X move = ");
        USE_SERIAL.print(iMoveX);
      }   
     movx(iFwd, iMoveX, 1);
      iXRelPos = iXRelPos + iMoveX;
      if(SERIAL_DEBUG) {
        USE_SERIAL.print("iXRelPos = ");
        USE_SERIAL.print(iXRelPos); 
      }
      return;
   }
   
   if(iXRelPos > iMoveXPos) {
     iMoveX = iXRelPos - iMoveXPos;
     if(SERIAL_DEBUG) {
        USE_SERIAL.print("Total X move = ");
        USE_SERIAL.print(iMoveX);
      }       
     movx(iBwd, iMoveX, 1);
      iXRelPos = iXRelPos - iMoveX;
      return;
   }
   
   if(iXRelPos == iMoveXPos) {
     iMoveX = 0;
     if(SERIAL_DEBUG) {
       USE_SERIAL.print("Total X move = ");
       USE_SERIAL.print(iMoveX);
     }        
     return;
   }
}

/********************
END MOVE FUNCTIONS
********************/

