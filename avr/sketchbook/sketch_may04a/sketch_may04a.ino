
void setup() {
  Serial.begin(115200);
  uint8_t test[] = { 0x69, 0x69, 0xA8, 0xC4 };
  uint32_t long1;
  long1 = ((unsigned int)(test[0] << 8)) | test[1];
  Serial.print(long1,HEX); //gives 0x6969
  long1 = long1 << 16;
  Serial.print(long1,HEX); //gives 0x6968A8C4
  long1 += ((unsigned int)(test[2] << 8)) | test[3];
  Serial.println();
  Serial.print(long1,HEX); //gives 0x6968A8C4
  Serial.println("#2");

  long1 = test[0] << 8) | test[1];
  Serial.print(long1,HEX); //gives 0x6969
  long1 = long1 << 16;
  Serial.print(long1,HEX); //gives 0x6968A8C4
  long1 += ((unsigned int)(test[2] << 8)) | test[3];
  Serial.println();
  Serial.print(long1,HEX); //gives 0x6968A8C4
  Serial.println();


/*
  long1 = (unsigned long) test[0] << 24 | (unsigned long) test[1] << 16 | (unsigned long) test[2] << 8 | (unsigned long) test[3];
  Serial.println();
  Serial.print(long1,HEX); //gives 0x6968A8C4
  Serial.println();
  long1 = (unsigned long) test[0] << 24 | (unsigned long) test[1] << 16 | (unsigned int) test[2] << 8 | test[3];
  Serial.println();
  Serial.print(long1,HEX); //gives 0x6968A8C4
  Serial.println();
*/
}

void loop() {}

