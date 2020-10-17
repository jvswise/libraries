// =============================================================================
//                    DEVDUINO MAX30102 HEARTRATE/OXYMETER
// =============================================================================
// Description:
/* 
 *  
 */
// =============================================================================
// Date: 23/08/2018
// Author: Alexandre Pailhoux
// License: MIT
// =============================================================================
/*=============================================================================
                                   INCLUDES
==============================================================================*/
#include <devduino.h>
#include "devduinoLogo.h"
#include "devduino_MAX30102.h"
#include "heartSprite.h"
#include <avr/wdt.h>
/*=============================================================================
                                  DEFINITIONS
==============================================================================*/
#define DIN_INT_MAX30102        1
#define DOUT_RED_LED           13
#define BUFFER_SIZE            62
#define PLOT_HEIGHT            34
#define THRESHOLD             110000
/*=============================================================================
                                GLOBAL VARIABLES
==============================================================================*/
uint32_t ir_buffer[BUFFER_SIZE], red_buffer[BUFFER_SIZE];
uint8_t  ir_buffer_scale[BUFFER_SIZE], red_buffer_scale[BUFFER_SIZE];

uint32_t beat_time = 0, previous_beat_time = 0;
/*=============================================================================
                                   INSTANCES
==============================================================================*/
DevDuino_MAX30102 MAX30102 = DevDuino_MAX30102();

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

  pinMode(DIN_INT_MAX30102, INPUT); 
  pinMode(DOUT_RED_LED, OUTPUT);
  
  MAX30102.ready();

  setHMI();
}
// =============================================================================
//                               INFINITE LOOP      
//                          -> -> -> -> -> -> -> ->
//                          |                     |              
//                          <- <- <- <- <- <- <- <-
// =============================================================================
void loop()
{
  
  for(uint8_t i=0;i<BUFFER_SIZE;i++)
    {
      wdt_enable(WDTO_60MS);
      //while(digitalRead(DIN_INT_MAX30102)==1);  
      
      MAX30102.read_fifo((red_buffer+i), (ir_buffer+i));  

      if (ir_buffer[i]<THRESHOLD)
      {
        display.clear();
        display.print("Place your finger", 15, 35, &defaultFont);  
        display.print("  on the sensor.", 15, 25, &defaultFont); 
        display.flush();
        display.clear();
        setHMI();        
      }
      else
      {
        /********************************************************************/
        // Find Min and Max
        /********************************************************************/  
        uint32_t ir_buffer_Max, red_buffer_Max, ir_buffer_Min, red_buffer_Min;
        
        ir_buffer_Max = 0;
        red_buffer_Max = 0;
        ir_buffer_Min = 0xFFFFFFFF;
        red_buffer_Min = 0xFFFFFFFF;    
         
        for(uint8_t j=0;j<BUFFER_SIZE;j++)
        {
          if(ir_buffer[j]>ir_buffer_Max)
          {
            ir_buffer_Max=ir_buffer[j];
          }
          else if(ir_buffer[j]<ir_buffer_Min)
          {
            ir_buffer_Min=ir_buffer[j];
          }  
        }  
        for(uint8_t j=0;j<BUFFER_SIZE;j++)
        {
          if(red_buffer[j]>red_buffer_Max)
          {
            red_buffer_Max=red_buffer[j];
          }
          else if(red_buffer[j]<red_buffer_Min)
          {
            red_buffer_Min=red_buffer[j];
          }  
        } 
  
        /********************************************************************/
        // SPO2 Calculation
        /********************************************************************/
        int16_t spo2 = spo2Calculation(ir_buffer_Max, ir_buffer_Min, red_buffer_Max,red_buffer_Min);
  
        /********************************************************************/
        // Display Results
        /********************************************************************/    
        display.clearArea(23, PLOT_HEIGHT+3, 35, 12);      
        display.print(String(ir_buffer[i], DEC), 23, PLOT_HEIGHT+4, &defaultFont);
        display.clearArea(89, PLOT_HEIGHT+3, 35, 12);
        display.print(String(red_buffer[i], DEC), 89, PLOT_HEIGHT+4, &defaultFont);   
  
        if(spo2<=100 && spo2>=90)
        { 
          display.clearArea(29, 50, 20, 11);       
          display.print(String(spo2, DEC), 32, 52, &defaultFont);    
        }
        else
        { 
          display.clearArea(29, 50, 20, 11);       
          display.print("..", 32, 52, &defaultFont);    
        }
  
        display.clearArea(117, 53, 7, 7);
        digitalWrite(DOUT_RED_LED , LOW);
        detectBeat(i); 
        display.clearArea(89, 50, 20, 11);         
        if(beat_time>0 && beat_time < 3000)
        {
          display.print(String(60000/beat_time), 90, 52, &defaultFont);     
        } 
        
        /********************************************************************/
        // Display IR Plot
        /********************************************************************/      
        display.clearArea(i, 0, 1, PLOT_HEIGHT+2);    
        ir_buffer_scale[i] = map(ir_buffer[i], ir_buffer_Min-10, ir_buffer_Max+10, 1, PLOT_HEIGHT);              
        if (i==0 || ir_buffer[i-1]<THRESHOLD)
        {
          display.drawPixel(i, ir_buffer_scale[i] );   
        } 
        else
        {
          display.drawLine(i-1, ir_buffer_scale[i-1], i, ir_buffer_scale[i]);         
        } 
        /********************************************************************/
        // Display RED Plot
        /********************************************************************/
        display.clearArea(i+65, 0, 1, PLOT_HEIGHT+2);    
        red_buffer_scale[i] = map(red_buffer[i], red_buffer_Min-10, red_buffer_Max+10, 1, PLOT_HEIGHT);                    
        if (i==0 || red_buffer[i-1]<THRESHOLD)
        {
          display.drawPixel(i+65, red_buffer_scale[i] );   
        } 
        else
        {
          display.drawLine(i+65-1, red_buffer_scale[i-1], i+65, red_buffer_scale[i]);         
        }        
        display.flush(); 
      }
    wdt_reset();  
    }    
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
//                               PEAK DETECTED
// =============================================================================
void beatDetected()
{
  display.drawSprite(heartSprite, 117, 51);
  digitalWrite(DOUT_RED_LED , HIGH);  
  
  if((millis()- previous_beat_time)>350 && (millis()- previous_beat_time)< 1500)
  {
    beat_time=millis();
    beat_time = beat_time - previous_beat_time; 
  }
  previous_beat_time=millis();  
}
// =============================================================================
//                          SET HUMAN MACHINE INTERFACE
// =============================================================================
void setHMI()
{  
  display.drawLine(0, PLOT_HEIGHT+2, 127, PLOT_HEIGHT+2); 
  display.drawLine(0, (63-(PLOT_HEIGHT+2))/2+(PLOT_HEIGHT+2), 127, (63-(PLOT_HEIGHT+2))/2+(PLOT_HEIGHT+2));   
  display.drawLine(0, 63, 127, 63); 
  display.drawLine(127, PLOT_HEIGHT+2, 127, 63);       
  display.drawLine(0, PLOT_HEIGHT+2, 127, PLOT_HEIGHT+2);  
  display.drawCircle(120, 56, 5); 

  display.drawLine(0, 53, 0, 63); 
  display.print("SPO2", 3, 52, &defaultFont); 
  display.print("%", 52, 52, &defaultFont);

  display.drawLine(28, (63-(PLOT_HEIGHT+2))/2+(PLOT_HEIGHT+2), 28, 63);

  display.drawLine(0, PLOT_HEIGHT+2, 0, 63); 
  display.print("IR", 3, PLOT_HEIGHT+4, &defaultFont); 
  display.drawLine(20, PLOT_HEIGHT+2, 20, (63-(PLOT_HEIGHT+2))/2+(PLOT_HEIGHT+2)); 

  display.drawLine(63, 0, 63, 63);
  display.print("BPM",66, 52, &defaultFont); 
  display.print("RED",66, PLOT_HEIGHT+4, &defaultFont); 
  display.drawLine(86, PLOT_HEIGHT+2, 86, 63);
  
  display.drawLine(113, (63-(PLOT_HEIGHT+2))/2+(PLOT_HEIGHT+2), 113, 63);  
}
// =============================================================================
//                          PEAK DETECTION
// =============================================================================
void detectBeat(uint8_t i)
{
  if((millis()- previous_beat_time)>350)
      {        
            if(i>=6)
            {
                if(ir_buffer[i]<ir_buffer[i-6] && ir_buffer[i-6]<ir_buffer[i-4] && ir_buffer[i-4]<ir_buffer[i-2])
                {
                  beatDetected();
                }     
            } 
            else if (i==0)            
            {
              if(ir_buffer[i]<ir_buffer[BUFFER_SIZE-6+i] && ir_buffer[BUFFER_SIZE-6+i]<ir_buffer[BUFFER_SIZE-4+i] && ir_buffer[BUFFER_SIZE-4+i]<ir_buffer[BUFFER_SIZE-2+i])
                 {
                  beatDetected();                
                 }              
            }
            else if (i==1)            
            {
              if(ir_buffer[i]<ir_buffer[BUFFER_SIZE-6+i] && ir_buffer[BUFFER_SIZE-6+i]<ir_buffer[BUFFER_SIZE-4+i] && ir_buffer[BUFFER_SIZE-4+i]<ir_buffer[BUFFER_SIZE-2+i])
                 {
                  beatDetected();                
                 }              
            }
            else if (i==2)            
            {
              if(ir_buffer[i]<ir_buffer[BUFFER_SIZE-6+i] && ir_buffer[BUFFER_SIZE-6+i]<ir_buffer[BUFFER_SIZE-4+i] && ir_buffer[BUFFER_SIZE-4+i]<ir_buffer[i-2])
                 {
                  beatDetected();                 
                 }              
            }
            else if (i==3)            
            {
              if(ir_buffer[i]<ir_buffer[BUFFER_SIZE-6+i] && ir_buffer[BUFFER_SIZE-6+i]<ir_buffer[BUFFER_SIZE-4+i] && ir_buffer[BUFFER_SIZE-4+i]<ir_buffer[i-2])
                 {
                  beatDetected();                  
                 }              
            }
            else if (i==4)            
            {
              if(ir_buffer[i]<ir_buffer[BUFFER_SIZE-6+i] && ir_buffer[BUFFER_SIZE-6+i]<ir_buffer[i-4] && ir_buffer[i-4]<ir_buffer[i-2])
                 {
                  beatDetected();               
                 }              
            }
            else if (i==5)            
            {
              if(ir_buffer[i]<ir_buffer[BUFFER_SIZE-6+i] && ir_buffer[BUFFER_SIZE-6+i]<ir_buffer[i-4] && ir_buffer[i-4]<ir_buffer[i-2])
                 {
                  beatDetected();                
                 }              
            } 
      }   
}

int spo2Calculation(uint16_t ir_buffer_Max, uint16_t ir_buffer_Min, uint16_t red_buffer_Max, uint16_t red_buffer_Min)
{
      /********************************************************************/
      // DC Calculation
      /********************************************************************/
      float ir_DC = 0.0; 
      float red_DC = 0.0;
      for (uint8_t k=0; k<BUFFER_SIZE; ++k) 
      {
        ir_DC += ir_buffer[k];
        red_DC += red_buffer[k];
      }
      ir_DC = ir_DC/BUFFER_SIZE ;
      red_DC = red_DC/BUFFER_SIZE ; 
     /********************************************************************/
      // Ratio Calculation
      /********************************************************************/  
      float ratio = ((red_buffer_Max-red_buffer_Min)*ir_DC)/((ir_buffer_Max-ir_buffer_Min)*red_DC);
      
      /********************************************************************/
      // SPO2 Calculation
      /********************************************************************/      
      int spo2 = (-45.060*ratio + 30.354)*ratio + 94.845;
      //int spo2 = 110 - 25*ratio;      
      
      return spo2;
}