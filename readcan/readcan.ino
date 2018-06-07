// demo: CAN-BUS Shield, receive data with check mode
// send data coming to fast, such as less than 10ms, you can use this way
// loovee, 2014-6-13

//realtime clock 
#include <Wire.h>
#include <DS3231.h>

#include <SPI.h>
#include "mcp_can.h"

#include <Arduino.h>
#include <U8g2lib.h>

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
const char menu[][6] = {"MAP", "Volts", "AFR", "Clock"};
const int menuItems = 4;
int menuSelected = 3;
boolean atMenu = false;

//clock setup
DS3231 clock;
RTCDateTime dt;

// Interrupt routine runs if CLK goes from HIGH to LOW
void isr ()  {

   static unsigned long last_interrupt_time = 0;
   unsigned long interrupt_time = millis();
   if (interrupt_time - last_interrupt_time > 200) 
   {
      if (digitalRead(PinCLK)){
        rotationdirection= digitalRead(PinDT);
      }
      else {
        rotationdirection= !digitalRead(PinDT);
      }

      if(atMenu == true){
        if(rotationdirection == true && menuSelected < menuItems-1){
          menuSelected++;
        } else if( rotationdirection == false && menuSelected > 0)
        { menuSelected--; }  
      }      
      TurnDetected = true;  
   }
   last_interrupt_time = interrupt_time;
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

    //setup clock
    Serial.println("Initialize DS3231");;
    clock.begin();
    clock.setDateTime(__DATE__, __TIME__);

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

    static unsigned long last_message_time = 0;
    unsigned long message_time = millis();
   
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
          case 3: 
            displayClock();
            break;
        }
        last_message_time = message_time;    
    } else if(menuSelected == 3){
      displayClock();
    } else if(message_time - last_message_time > 2000){
      char na[] = "N/A";
      writeToScreen(na);
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

void displayClock()
{
   dt = clock.getDateTime();
  writeToScreenSmall(clock.dateFormat("h:i", dt));
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

void writeToScreenSmall(char text[])
{
    u8g2.setFont(u8g2_font_logisoso42_tr);
    u8g2.firstPage();
    do {
      u8g2.setCursor(0,42);
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
        delay(200);
    }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
