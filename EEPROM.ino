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


