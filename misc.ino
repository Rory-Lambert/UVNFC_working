void showarray (byte arr[], int length){
  int x;
  for (x=0; x<=length; x++){
    Serial.print(arr[x],HEX);Serial.print(",");
    //if x is divisible by ten print a new line
    if ((x+1)%10==0){
      Serial.println("");
    }
    //delay(100);
  }
  Serial.println("\nEND\n");
  //delay(3000);
}

void showASCII (byte arr[], int length){
  int y;
  char z;
  for (y=0; y<=length; y++){
    if (arr[y] <=64){
      z=46;
    }
    else{
      
    z=arr[y];
    }
    Serial.print(z);
  }
}


//function which checks if 2 arrays are equal to each other
boolean array_cmp(byte a[], byte b[], int len){
  int n;
  for (n=0;n<len;n++) if (a[n]!=b[n]) return false; // test each element to be the same. if not, return false
  return true; //ok, if we have not returned yet, they are equal
}
