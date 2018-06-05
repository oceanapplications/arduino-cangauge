// demo: CAN-BUS Shield, receive data with check mode
// send data coming to fast, such as less than 10ms, you can use this way
// loovee, 2014-6-13


#include <SPI.h>
#include "mcp_can.h"

#include <Arduino.h>
#include <U8g2lib.h>

//screen setup
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//Screen setup end

//rotary encoder
volatile boolean TurnDetected;  // need volatile for Interrupts
volatile boolean rotationdirection;  // CW or CCW rotation

const int PinCLK=3;   // Generating interrupts using CLK signal
const int PinDT=4;    // Reading DT signal
const int PinSW=5;    // Reading Push Button switch
//rotary encoder end

//CAN Setup
const int SPI_CS_PIN = 9;
MCP_CAN CAN(SPI_CS_PIN);

//menu setup
const char menu[][6] = {"MAP", "Volts", "AFR"};
const int menuItems = 3;
int menuSelected = 0;
boolean atMenu = false;


// Interrupt routine runs if CLK goes from HIGH to LOW
void isr ()  {
  if (digitalRead(PinCLK)){
    rotationdirection= digitalRead(PinDT);
  }
  else {
    rotationdirection= !digitalRead(PinDT);
  }
  //Serial.println(rotationdirection);
  if(atMenu == true){
    if(rotationdirection == true && menuSelected < menuItems-1){
      menuSelected++;
    } else if( rotationdirection == false && menuSelected > 0)
    { menuSelected--; }  
  }
  
  TurnDetected = true;
  delay(200);  // delay for Debouncing
}

void setup()
{
    Serial.begin(115200);
    //screen setup
    u8g2.begin();
    //setup encoder
    pinMode(PinCLK,INPUT);
    pinMode(PinDT,INPUT);  
    pinMode(PinSW,INPUT);
    digitalWrite(PinSW, HIGH); // Pull-Up resistor for switch
    attachInterrupt (1,isr,FALLING); // interrupt 0 always connected to pin 2 on Arduino UNO

    //setup canbus
    while (CAN_OK != CAN.begin(CAN_1000KBPS))
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println(" Init CAN BUS Shield again");
        delay(100);
    }
    Serial.println("CAN BUS Shield init ok!");
}


void loop()
{
   if(atMenu){
       showMenu();
    } else {
      readCan();
    }
    checkEncoder();
}

void readCan()
{
    unsigned char len = 0;
    unsigned char buf[8];

    char text[8] = "12.5";
    /*switch (menuSelected)
        {
          case 0: 
             strncpy( text, "1.A", sizeof(text) );
             text[sizeof(text)-1] = 0;
            break;  
          case 1: 
             strncpy( text, "2.3", sizeof(text) );
             text[sizeof(text)-1] = 0;
            break;
          case 2: 
             strncpy( text, "3.3", sizeof(text) );
             text[sizeof(text)-1] = 0;
            break;    
        }*/
   
    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf
        unsigned int canId = CAN.getCanId();

        switch (menuSelected)
        {
          case 0: 
            displayMAP(canId, buf);
            break;  
          case 1: 
            displayVoltage(canId, buf);
            break;  
          case 2: 
            displayAFR(canId, buf);
            break;
        }
        
    }
     
}

void displayMAP(unsigned int canId, unsigned char buf[8]){
        
   if(canId == 0x125){
        Serial.println("-----------------------------");
        Serial.print("Get data from ID: ");
        Serial.println(canId, HEX);
        unsigned int data = buf[6] * 256 + buf[7];
        Serial.print(data);
        Serial.print("\t");
        Serial.println();

        float fData = data/100.0;
        
        writeToScreen(fData);
    }
}

void displayVoltage(unsigned int canId, unsigned char buf[8]){
        
   if(canId == 0x118){
        Serial.println("-----------------------------");
        Serial.print("Get data from ID: ");
        Serial.println(canId, HEX);
        unsigned int data = buf[2] * 256 + buf[3];
        Serial.print(data);
        Serial.print("\t");
        Serial.println();

        float fData = data/100.0;
        
        writeToScreen(fData);
    }
}

void displayAFR(unsigned int canId, unsigned char buf[8]){
        
   if(canId == 0x125){
        Serial.println("-----------------------------");
        Serial.print("Get data from ID: ");
        Serial.println(canId, HEX);
        unsigned int data = buf[2] * 256 + buf[3];
        Serial.print(data);
        Serial.print("\t");
        Serial.println();

        float fData = data/100.0;
        
        writeToScreen(fData);
    }
}

void writeToScreen(char text[])
{
    u8g2.setFont(u8g2_font_logisoso58_tr);
    u8g2.firstPage();
    do {
      u8g2.setCursor(0,62);
      u8g2.drawStr(0, 62, text); 
    } while ( u8g2.nextPage() );
}

void writeToScreen(unsigned int text)
{
    u8g2.setFont(u8g2_font_logisoso58_tr);
    u8g2.firstPage();
    do {
      u8g2.setCursor(0,62);
      u8g2.print(text);
    } while ( u8g2.nextPage() );
}

void writeToScreen(float text)
{
    u8g2.setFont(u8g2_font_logisoso58_tr);
    u8g2.firstPage();
    do {
      u8g2.setCursor(0,62);
      u8g2.print(text);
    } while ( u8g2.nextPage() );
}

void showMenu()
{
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
    u8g2.firstPage();
    do {
        int position = 0;
        for(int i=0; i<menuItems; i++)
        {
            position += 15;
            if(i==menuSelected){
              u8g2.drawStr(10, position, menu[i]); // write something to the internal memory  
            } else {
              u8g2.drawStr(0, position, menu[i]); // write something to the internal memory  
            }
        }
    } while ( u8g2.nextPage() );
    
}

void checkEncoder(){
  if (!(digitalRead(PinSW))) {   // check if button is pressed
        atMenu = !atMenu;
        if(atMenu == false){
          char text[] = "";
          writeToScreen(text);
        }
        Serial.println("Clicked");
        delay(200);
    }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
