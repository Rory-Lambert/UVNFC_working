
/*********************************************************
** Working version of the arduino code, can read and write
** to/from NFC. Everything else to be implemented.
** Rory Lambert + Jamie Logan, based upon code by Ten Wong7
** Now includes mime type comparison and Timer Interrupts
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
int timer_f;
int count=0;
/*164 seems to be the max length it will work.
**the BB's are just to show a default message*/
byte payload[160]={0xBB}; 
byte msg_setup[] = MSG_SETUP;  //31b
byte mime_type[] = MIME_TYPE;  //27b
byte aar[] = AAR; //33b
int NFCount;

boolean WRITTEN = false;    //flag to see if a write occured 
void setup(void) 
{

   
    //reset RF430
    nfc.begin();
    delay(1000);
    
    
        /*********TIMER INTERRUPTS*********/
    
    cli();                                   //stop interrupts         
  
  
    TCCR1A = 0;                              //set entire TCCR1A register to 0
    TCCR1B = 0;                              //same for TCCR1B
    TCNT1  = 0;                              //initialize counter value to 0
    OCR1A = 46874;                           // = (8*10^6) / (1024*(1/6s) - 1 (must be <65536)
    TCCR1B |= (1 << WGM12);                  //turn on CTC mode
    TCCR1B |= (1 << CS12) | (1 << CS10);     //Set CS10 and CS12 bits for 1024 prescaler
    TIMSK1 |= (1 << OCIE1A);                 //enable timer compare interrupt
    sei();                                   //allow interrupts
  
    
}

ISR(TIMER1_COMPA_vect){
    
    count++;
            
     if (count == (Interval*10)){
         count = 0;
         
         if(timer_f == 0){          //if the flag isnt set...
             timer_f = 1;            //...set it.
         }
     }
}



void loop(void) 
{
    while(!(nfc.Read_Register(STATUS_REG) & READY)); //wait until READY bit has been set
    
    
  
  PAY_LEN=sizeof(payload);                    //find the length of the payload
   
  /*sets the length of the NDEF message, depending upon the payload size*/
  byte NDEF_MSG[PAY_LEN + PRE_PAY_LEN];       //***removed -1!, appeared to fix 
  int NDEF_LEN = sizeof(NDEF_MSG);            //store its length in an int
      
  //Function call prepares the full NDEF message
  NDEF_prep(NDEF_MSG, PAY_LEN);    
    
    delay(500);
    ////Serial////Serial.println("out of NDEFPREP");


    //write NDEF memory with Capability Container + NDEF message
    nfc.Write_Continuous(0, NDEF_MSG, sizeof(NDEF_MSG));
    ////Serial.println("Writecont");
    //Enable interrupts for End of Read and End of Write
    nfc.Write_Register(INT_ENABLE_REG, EOW_INT_ENABLE + EOR_INT_ENABLE);
  ////Serial.println("writeREG");
    //Configure INTO pin for active low and enable RF
    nfc.Write_Register(CONTROL_REG, INT_ENABLE + INTO_DRIVE + RF_ENABLE );
    ////Serial.println("enableRF");
    //enable interrupt 1
    attachInterrupt(1, RF430_Interrupt, FALLING);
    
    ////Serial.println("\nWait for read or write...");
    while(1)
    {    
        /*Routine for if a write occured*/
        if (WRITTEN==true){
          //showarray(msg_from_phone, sizeof(msg_from_phone));
          WRITTEN=false;
          change_write();
        }
        
        /*Routine for if a timer interrupt occured*/
        if (timer_f==1){
          timer_f=0;
          NFCount++;
          write_different();
          
          //Serial.print("\nNFCount:");//Serial.println(NFCount);
          //Serial.print("\nInterval:");//Serial.println(Interval);
  
        }  
        
        if(into_fired)
        {
            //clear control reg to disable RF
            nfc.Write_Register(CONTROL_REG, INT_ENABLE + INTO_DRIVE); 
            delay(750);
            
            //read the flag register to check if a read or write occurred
            flags = nfc.Read_Register(INT_FLAG_REG); 
            //Serial.print("INT_FLAG_REG = 0x");//Serial.println(flags, HEX);
            
            //ACK the flags to clear
            nfc.Write_Register(INT_FLAG_REG, EOW_INT_FLAG + EOR_INT_FLAG); 
            
            if(flags & EOW_INT_FLAG)      //check if the tag was written
            {
                //Serial.println("The tag was writted!");
                getNDEFdata(msg_from_phone);
               
                WRITTEN=true;
            }
            else if(flags & EOR_INT_FLAG) //check if the tag was read
            {
                //Serial.println("The tag was readed!");
           
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
        ////Serial.print("msg_from_phone[0x");//Serial.print(k, HEX);//Serial.print("]=");//Serial.println(buffer[k], HEX); 
    }
    
    
    
    
}


void change_write(){
    
    //Serial.println("into change write"); 
    int q;
    byte mime_Rx[27];
    boolean corr_app;
    
    /*Get the mime type from the received message*/
    for (q=31; q<58; q++){
      mime_Rx[q-31]=msg_from_phone[q];
    }
    
    /*Call the message comparison function */
    corr_app=array_cmp(mime_Rx, mime_type, 26);
    
    /****///Serial stuff, for testing****/
    //Serial.print("MIME RX:\n");
    //showASCII(mime_Rx, 26);
    
    //Serial.print("\nMIME GLOBAL:\n");
    //showASCII(mime_type, 26);
    //Serial.print(corr_app);//Serial.println("");
    /**************************************/
    
    
    if (corr_app == true){
      //Serial.println("correct app");
      Device_ID =  msg_from_phone[0x3A]; 
      Year      =  msg_from_phone[0x3B];
      Day_MSB   =  msg_from_phone[0x3C];
      Day_LSB   =  msg_from_phone[0x3D];
      Time_Hr   =  msg_from_phone[0x3E];       
      Time_Min  =  msg_from_phone[0x3F];        
      Interval  =  msg_from_phone[0x40];
      /*the header is getting re-written, so we need to reset the interrupt count
      **to be in time with the timestamp*/
      count=0; 
      
      //Put this data in EEPROM
      EepromWrite(0x00, Device_ID);
      EepromWrite(0x01, Year);
      EepromWrite(0x02, Day_MSB);
      EepromWrite(0x03, Day_LSB);
      EepromWrite(0x04, Time_Hr);
      EepromWrite(0x05, Time_Min);
      EepromWrite(0x06, Interval);
      EepromWrite(0x07, Device_ID);
      
      
 
      
    }
   
    //for testing, puts the things from EEPROM into bytes 20+ of the payload
    for (int s = 20; s<27; s++){
      payload[s]=EepromRead(s-20);
    }
    
    byte NDEF_MSG[PAY_LEN + PRE_PAY_LEN];
    NDEF_prep(NDEF_MSG, PAY_LEN); 
    
    while(!(nfc.Read_Register(STATUS_REG) & READY)); //wait until READY bit has been set
       

    //write NDEF memory with Capability Container + NDEF message
    nfc.Write_Continuous(0, NDEF_MSG, sizeof(NDEF_MSG));

    //Enable interrupts for End of Read and End of Write
    nfc.Write_Register(INT_ENABLE_REG, EOW_INT_ENABLE + EOR_INT_ENABLE);

    //Configure INTO pin for active low and enable RF
    nfc.Write_Register(CONTROL_REG, INT_ENABLE + INTO_DRIVE + RF_ENABLE );

    //enable interrupt 1
    attachInterrupt(1, RF430_Interrupt, FALLING);
    
    //Serial.println("end of change write");
  
}


/*Writes a new NDEF message after the timer interrupt and puts it on the RF430*/
void write_different(){
    //Serial.println("into write different");
    int t;
    for (t=90; t<100; t++){
      payload[t]=NFCount;
    }
  
    byte NDEF_MSG[PAY_LEN + PRE_PAY_LEN];
    NDEF_prep(NDEF_MSG, PAY_LEN); 
    
    while(!(nfc.Read_Register(STATUS_REG) & READY)); //wait until READY bit has been set
       

    //write NDEF memory with Capability Container + NDEF message
    nfc.Write_Continuous(0, NDEF_MSG, sizeof(NDEF_MSG));

    //Enable interrupts for End of Read and End of Write
    nfc.Write_Register(INT_ENABLE_REG, EOW_INT_ENABLE + EOR_INT_ENABLE);

    //Configure INTO pin for active low and enable RF
    nfc.Write_Register(CONTROL_REG, INT_ENABLE + INTO_DRIVE + RF_ENABLE );

    //enable interrupt 1
    attachInterrupt(1, RF430_Interrupt, FALLING);
    
    //Serial.println("end write different");
  
}
