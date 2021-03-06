// =============================================================================
//                         BLUETOOTH EXAMPLE FOR DEVDUINO
// =============================================================================
// Description:
// Code example to communicate with the DevDuino through a Bluetooth Module.
// =============================================================================
// Date: 14/05/2018
// Author: Alexandre PAILHOUX
// License: MIT
// =============================================================================
/*=============================================================================
                                   INCLUDES
==============================================================================*/
//#include <Wire.h>
#include <SoftwareSerial.h>
#include <devduino.h>
#include "devduinoLogo.h"
/*=============================================================================
                                  DEFINITIONS
==============================================================================*/
//#define RED_LED     13
/*=============================================================================
                                GLOBAL VARIABLES
==============================================================================*/
String messageFromUART= "";
float analogRef = 4.9;
/*=============================================================================
                                   INSTANCES
==============================================================================*/
SoftwareSerial UART(10,12);

// =============================================================================
//                                INITIALIZATION
// =============================================================================
void setup()
{
  /*============================================================================
                                  DEVDUINO INIT.
  =============================================================================*/
  devduino.begin();
  devduino.attachToIntButton(buttonPushed);
  display.drawSprite(devduinoLogo, 37, 0);
  display.flush();
  delay(1000);
  display.clear();

  UART.begin(9600);
  /*============================================================================
                                   DIGITAL OUT
  =============================================================================*/     
  for(int i=0;i<=1;i++)
  {
  pinMode(i, OUTPUT);
    digitalWrite(i, LOW); 
  }  
  for(int i=4;i<=6;i++)
  {
  pinMode(i, OUTPUT);
    digitalWrite(i, LOW); 
  }
  for(int i=8;i<=9;i++)
  {
  pinMode(i, OUTPUT);
  digitalWrite(i, LOW); 
  }
  
  pinMode(11, OUTPUT);
  digitalWrite(11, LOW);

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}
// =============================================================================
//                               INFINITE LOOP      
//                          -> -> -> -> -> -> -> ->
//                          |                     |              
//                          <- <- <- <- <- <- <- <-
// =============================================================================
void loop()
{
  processUARTInformation();
}

// =============================================================================
//                                INTERRUPTION                        
//                                     ||          
//                                     ||          
//                               O-----------O    
//                          ------O         0-------                       
// =============================================================================
void buttonPushed()
{
  /*============================================================================
                                DEBOUNCING FILTER
  =============================================================================*/
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200)
  {
    // ---------------------------------------------------------------------------
    // DO YOUR INTERRUPTION HERE...   
    // ---------------------------------------------------------------------------
  }   
  last_interrupt_time = interrupt_time;
}
// =============================================================================
//                     PROCESS THE INFORMATION FROM THE UART    
// =============================================================================
void processUARTInformation()
{
   while(UART.available()) 
   {
    delay(3);
    char c = UART.read();
    messageFromUART+= c;
   }
   
   if (messageFromUART.length() >0) 
   {  
    console.println(messageFromUART);
  /*============================================================================
                                  DIGITAL 0  
    =============================================================================*/
    if ((messageFromUART.indexOf("D00") != -1)) 
    {
     processDigital(0,messageFromUART);     
    }
  /*============================================================================
                                  DIGITAL 1 
    =============================================================================*/
    else if ((messageFromUART.indexOf("D01") != -1)) 
    {
     processDigital(1,messageFromUART);     
    }  
  /*============================================================================
                                  DIGITAL 4 
    =============================================================================*/
    else if ((messageFromUART.indexOf("D04") != -1)) 
    {
     processDigital(4,messageFromUART);     
    }
  /*============================================================================
                                  DIGITAL 5*
    =============================================================================*/
    else if ((messageFromUART.indexOf("D05") != -1)) 
    {
     processPWM(5,messageFromUART); 
    }
  /*============================================================================
                                  DIGITAL 6*
    =============================================================================*/
    else if ((messageFromUART.indexOf("D06") != -1)) 
    {
     processPWM(6,messageFromUART);  
    }
  /*============================================================================
                                  DIGITAL 8 
    =============================================================================*/
    else if ((messageFromUART.indexOf("D08") != -1)) 
    {
     processDigital(8,messageFromUART); 
    }
  /*============================================================================
                                  DIGITAL 9* 
    =============================================================================*/
    else if ((messageFromUART.indexOf("D09") != -1)) 
    {
     processPWM(9,messageFromUART); 
    }
  /*============================================================================
                                  DIGITAL 11*  
    =============================================================================*/
    else if ((messageFromUART.indexOf("D11") != -1)) 
    {
     processPWM(11,messageFromUART);
    }  
  /*============================================================================
                                  DIGITAL 13*  
    =============================================================================*/
    else if ((messageFromUART.indexOf("D13") != -1)) 
    {
      processPWM(13,messageFromUART);    
    }
  /*============================================================================
                                  ANALOG 0 
    =============================================================================*/
    else if ((messageFromUART.indexOf("A0") != -1)) 
    {
     processAnalog(0);
    }
  /*============================================================================
                                  ANALOG 1  
    =============================================================================*/
    else if ((messageFromUART.indexOf("A1") != -1)) 
    {
     processAnalog(1);
    }
  /*============================================================================
                                  ANALOG 2 
    =============================================================================*/
    else if ((messageFromUART.indexOf("A2") != -1)) 
    {
     processAnalog(2);
    }
  /*============================================================================
                                  ANALOG 3 
    =============================================================================*/
    else if ((messageFromUART.indexOf("A3") != -1)) 
    {
     processAnalog(3);
    }
  /*============================================================================
                                  ANALOG 4 
    =============================================================================*/
    else if ((messageFromUART.indexOf("A4") != -1)) 
    {
     processAnalog(4);
    }
  /*============================================================================
                                  ANALOG 5  
    =============================================================================*/
    else if ((messageFromUART.indexOf("A5") != -1)) 
    {
     processAnalog(5);
    }
 }
 messageFromUART="";
 
}
/*============================================================================
                                PROCESS DIGITAL 
  =============================================================================*/
void processDigital(unsigned int pin, String message)
{
  if ((message.indexOf("ON") != -1)) 
  {
    digitalWrite(pin,HIGH);
    }
  else if((message.indexOf("OFF") != -1)) 
  {
    digitalWrite(pin,LOW);
    }    
  
}
/*============================================================================
                                PROCESS PWM 
  =============================================================================*/
void processPWM(unsigned int pin, String message)
{
  if ((message.indexOf("ON") != -1)) 
   {
    digitalWrite(pin,HIGH);
    }
   else if((message.indexOf("OFF") != -1)) 
   {
    digitalWrite(pin,LOW);
    }   
   else
   {   
     message = message.substring(4,7);
     analogWrite(pin, message.toInt());
   }  
}
/*============================================================================
                                PROCESS ANALOG 
  =============================================================================*/
void processAnalog(unsigned int pin)
{
  float analogValue = analogRead(pin);   
  analogValue = analogValue*analogRef/1000;
  console.println("A" + String(pin) +"=" + String(analogValue));
  UART.print(String(analogValue));
}
