/*****************************WRITE************************************/

//writes one byte of data to stated address in memory
void EepromWrite(int address, byte data){
  
  //split new 16 bit address into high and low bytes
  byte add_lo = address;
  byte add_hi = (address >> 8);
  
  //start transmission & send control byte
  Wire.beginTransmission(eeprom_cntrl);
  //send address high byte
  Wire.write(add_hi);
  //send address low byte
  Wire.write(add_lo);
  //send data byte
  Wire.write(data);
  //end transmission
  Wire.endTransmission();
  delay(5);
  
} 

/*********************************READ***********************************/

//reads one byte of data from stated address in memory
byte EepromRead(int address){
  
  //split 16 bit address into high and low bytes
  byte add_lo = address;
  byte add_hi = (address >> 8);
    
  
  //start transmission & send control byte
  Wire.beginTransmission(eeprom_cntrl);
  //send address high byte
  Wire.write(add_hi);
  //send address low byte
  Wire.write(add_lo);
  //end write
  Wire.endTransmission();
  delay(5);
  //read one byte from eeprom
  Wire.requestFrom(eeprom_cntrl, 1);
  //wait for receipt
  while(Wire.available() == 0);
  //assign value
  byte receivedValue = Wire.read();
  
  return receivedValue;
}

/*************************************STOREDATA**********************************/

/**USE THIS FUNCTION TO WRITE DATA TO EEPROM**/
void StoreData(int address, int data){

  //scale value from adc from 10 bits to 8 bits
  unsigned long int scaledData = (data);    //***removed scaling factor
  byte newdata = (byte)scaledData;
  
  EepromWrite(address, newdata);    //Tx data_hi byte
  ee_address++;                        //increment eeprom address
  storedcount++;                    //increment global storedcount

  UpdateCounter();  
}


/***************************UPDATE COUNTER**************************************/

void UpdateCounter(){
  /* function for keeping track of number of 
  *  values in eeprom and writing that value into 
  *  the appropriate locations in eeprom        */
  
  //split storedcount int into two 8 bit values
  byte count_lo = storedcount;
  byte count_hi = (storedcount >> 8);
  
  //Update Counter Value in eeprom addresses 0x00 and 0x01 //now 8 and 9!!!!!
  EepromWrite(0x08, count_hi);    //tx count_hi byte
  EepromWrite(0x09, count_lo);    //tx count_lo byte

}

/****************************READALLDATA******************************************/

void ReadAllData(){
  /* function for reading all of eeprom */
  byte receivedValue = 0;
  
  //get count bytes from eeprom and convert to integer
  byte count_hi = EepromRead(0x08);
  byte count_lo = EepromRead(0x09);
  int countEE = count_lo | (count_hi << 8);
  
  /*READ storedcount NUMBER OF VALUES FROM EEPROM
    AND STORE SEQUENTIALLY IN A BUFFER*/
  
  //put the data header into the payload
  for (int y=0; y<10; y++){    
    payload[y]=EepromRead(y);
    }
  
  int address = 0x0A;        //start at first address
  int y = 0;
  for (y=0; y<(countEE); y++){
    
    receivedValue = EepromRead(address);    //read hi data value
    address++;                                 //increment eeprom address
    
    //put the data into the payload
    payload[y+10] = receivedValue;
    
  }
  
}
