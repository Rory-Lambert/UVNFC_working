
/*********************************************************
** Working version of the arduino code, can read and write
** to/from NFC. Everything else to be implemented.
** Rory Lambert + Jamie Logan, based upon code by Ten Wong
***********************************************************/
#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif
#include <Wire.h>
#include <RF430CL330H_Shield.h>
#include "Header.h"

#define IRQ   (3)
#define RESET (4)  

RF430CL330H_Shield nfc(IRQ, RESET);

unsigned char NDEF_Application_Data[] = RF430_DEFAULT_DATA;
volatile byte into_fired = 0;
uint16_t flags = 0;
byte ndefdata[97];


boolean WRITTEN = false;    //flag to see if a write occured 
void setup(void) 
{
    Serial.begin(115200);    
    Serial.println("Hello!");
   
    //reset RF430
    nfc.begin();
    delay(1000);
}

void loop(void) 
{
    while(!(nfc.Read_Register(STATUS_REG) & READY)); //wait until READY bit has been set
    Serial.print("Fireware Version:"); Serial.println(nfc.Read_Register(VERSION_REG), HEX);    

    //write NDEF memory with Capability Container + NDEF message
    nfc.Write_Continuous(0, NDEF_Application_Data, sizeof(NDEF_Application_Data));

    //Enable interrupts for End of Read and End of Write
    nfc.Write_Register(INT_ENABLE_REG, EOW_INT_ENABLE + EOR_INT_ENABLE);

    //Configure INTO pin for active low and enable RF
    nfc.Write_Register(CONTROL_REG, INT_ENABLE + INTO_DRIVE + RF_ENABLE );

    //enable interrupt 1
    attachInterrupt(1, RF430_Interrupt, FALLING);
    
    Serial.println("Wait for read or write...");
    while(1)
    {    
        
        if (WRITTEN==true){
          showarray(ndefdata, sizeof(ndefdata));
          WRITTEN=false;
          change_write();
        }
        
        if(into_fired)
        {
            //clear control reg to disable RF
            nfc.Write_Register(CONTROL_REG, INT_ENABLE + INTO_DRIVE); 
            delay(750);
            
            //read the flag register to check if a read or write occurred
            flags = nfc.Read_Register(INT_FLAG_REG); 
            Serial.print("INT_FLAG_REG = 0x");Serial.println(flags, HEX);
            
            //ACK the flags to clear
            nfc.Write_Register(INT_FLAG_REG, EOW_INT_FLAG + EOR_INT_FLAG); 
            
            if(flags & EOW_INT_FLAG)      //check if the tag was written
            {
                Serial.println("The tag was writted!");
                getNDEFdata(ndefdata);
               
                WRITTEN=true;
            }
            else if(flags & EOR_INT_FLAG) //check if the tag was read
            {
                Serial.println("The tag was readed!");
           
            }
            flags = 0;
            into_fired = 0; //we have serviced INT1

            //Enable interrupts for End of Read and End of Write
            nfc.Write_Register(INT_ENABLE_REG, EOW_INT_ENABLE + EOR_INT_ENABLE);

            //Configure INTO pin for active low and re-enable RF
            nfc.Write_Register(CONTROL_REG, INT_ENABLE + INTO_DRIVE + RF_ENABLE);

            //re-enable INTO
            attachInterrupt(1, RF430_Interrupt, FALLING);
        }
    }
}

/**
**  @brief  interrupt service
**/
void RF430_Interrupt()            
{
    into_fired = 1;
    detachInterrupt(1);//cancel interrupt
}


/**
**  @brief get the writted NDEF data
**/
boolean getNDEFdata(uint8_t* ndefdata)
{
    byte buffer[99];
    uint16_t NDEFMessageLength = 99;
    
  
    nfc.Read_Continuous(0, buffer, 99);   
    for (uint8_t k=0; k < 99; k++)
    {
        ndefdata[k] = buffer[k];
        //Serial.print("ndefdata[0x");Serial.print(k, HEX);Serial.print("]=");Serial.println(buffer[k], HEX); 
    }
    
    
    
    return true;
}


void change_write(){
    
     Serial.println("into change write"); 
    int q;
    for (q=0x3A; q<0x41; q++){
      ndefdata[q]= (ndefdata[q] + 16);
    }
    Serial.print("altered array:");
    showarray(ndefdata,sizeof(ndefdata));
    while(!(nfc.Read_Register(STATUS_REG) & READY)); //wait until READY bit has been set
       

    //write NDEF memory with Capability Container + NDEF message
    nfc.Write_Continuous(0, ndefdata, sizeof(ndefdata));

    //Enable interrupts for End of Read and End of Write
    nfc.Write_Register(INT_ENABLE_REG, EOW_INT_ENABLE + EOR_INT_ENABLE);

    //Configure INTO pin for active low and enable RF
    nfc.Write_Register(CONTROL_REG, INT_ENABLE + INTO_DRIVE + RF_ENABLE );

    //enable interrupt 1
    attachInterrupt(1, RF430_Interrupt, FALLING);
    
    Serial.println("end of change write");
  
}


