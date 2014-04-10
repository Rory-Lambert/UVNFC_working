
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


volatile byte into_fired = 0;
uint16_t flags = 0;
byte msg_from_phone[97];
int PAY_LEN;

/*164 seems to be the max length it will work.
**the BB's are just to show a default message*/
byte payload[164]={0xBB,0xBB,0xBB,0xBB,0xBB}; 
byte msg_setup[] = MSG_SETUP;  //31b
byte mime_type[] = MIME_TYPE;  //27b
byte aar[] = AAR; //33b


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
    
  
  PAY_LEN=sizeof(payload);                    //find the length of the payload
   
  /*sets the length of the NDEF message, depending upon the payload size*/
  byte NDEF_MSG[PAY_LEN + PRE_PAY_LEN];       //***removed -1!, appeared to fix 
  int NDEF_LEN = sizeof(NDEF_MSG);            //store its length in an int
      
  //Function call prepares the full NDEF message
  NDEF_prep(NDEF_MSG, PAY_LEN);    
    
    delay(500);
    Serial.println("out of NDEFPREP");


    //write NDEF memory with Capability Container + NDEF message
    nfc.Write_Continuous(0, NDEF_MSG, sizeof(NDEF_MSG));
    Serial.println("Writecont");
    //Enable interrupts for End of Read and End of Write
    nfc.Write_Register(INT_ENABLE_REG, EOW_INT_ENABLE + EOR_INT_ENABLE);
  Serial.println("writeREG");
    //Configure INTO pin for active low and enable RF
    nfc.Write_Register(CONTROL_REG, INT_ENABLE + INTO_DRIVE + RF_ENABLE );
    Serial.println("enableRF");
    //enable interrupt 1
    attachInterrupt(1, RF430_Interrupt, FALLING);
    
    Serial.println("\nWait for read or write...");
    while(1)
    {    
        
        if (WRITTEN==true){
          showarray(msg_from_phone, sizeof(msg_from_phone));
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
                getNDEFdata(msg_from_phone);
               
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
void getNDEFdata(uint8_t* msg_from_phone)
{
    byte buffer[99];
    uint16_t NDEFMessageLength = 99;
    
  
    nfc.Read_Continuous(0, buffer, 99);   
    for (uint8_t k=0; k < 99; k++)
    {
        msg_from_phone[k] = buffer[k];
        //Serial.print("msg_from_phone[0x");Serial.print(k, HEX);Serial.print("]=");Serial.println(buffer[k], HEX); 
    }
    
    
    
    
}


void change_write(){
    
    Serial.println("into change write"); 
    int q;
    byte mime_Rx[27];
    boolean corr_app;
    
    /*Get the mime type from the received message*/
    for (q=31; q<58; q++){
      mime_Rx[q-31]=msg_from_phone[q];
    }
    
    /*Call the message comparison function */
    corr_app=array_cmp(mime_Rx, mime_type, 26);
    
    /****Serial stuff, for testing****/
    Serial.print("MIME RX:\n");
    showASCII(mime_Rx, 26);
    
    Serial.print("\nMIME GLOBAL:\n");
    showASCII(mime_type, 26);
    Serial.print(corr_app);Serial.println("");
    /**************************************/
    
    
    if (corr_app == true){
      Serial.println("correct app");
      Device_ID =  msg_from_phone[0x3A]; 
      Year      =  msg_from_phone[0x3B];
      Day_MSB   =  msg_from_phone[0x3C];
      Day_LSB   =  msg_from_phone[0x3D];
      Time_Hr   =  msg_from_phone[0x3E];       
      Time_Min  =  msg_from_phone[0x3F];        
      Interval  =  msg_from_phone[0x40];
    }
    
    Serial.println(Device_ID);
    Serial.println(Year);
    Serial.println(Day_MSB);
    Serial.println(Day_LSB);
    Serial.println(Time_Hr);
    Serial.println(Time_Min);
    Serial.println(Interval);
    
    
      /*get the data we are interested in*/
      for (q=0x3A; q<0x41; q++){
        msg_from_phone[q]= (msg_from_phone[q] + 16);
      }
    Serial.print("altered array:");
    showarray(msg_from_phone,sizeof(msg_from_phone));
    while(!(nfc.Read_Register(STATUS_REG) & READY)); //wait until READY bit has been set
       

    //write NDEF memory with Capability Container + NDEF message
    nfc.Write_Continuous(0, msg_from_phone, sizeof(msg_from_phone));

    //Enable interrupts for End of Read and End of Write
    nfc.Write_Register(INT_ENABLE_REG, EOW_INT_ENABLE + EOR_INT_ENABLE);

    //Configure INTO pin for active low and enable RF
    nfc.Write_Register(CONTROL_REG, INT_ENABLE + INTO_DRIVE + RF_ENABLE );

    //enable interrupt 1
    attachInterrupt(1, RF430_Interrupt, FALLING);
    
    Serial.println("end of change write");
  
}


