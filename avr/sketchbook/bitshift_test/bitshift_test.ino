int i[3] ;
byte b[3];
char c[3];
int res;
int ii1,ii2,ii3;
byte bb1,bb2,bb3;
char cc1,cc2,cc3;


void setup() {
  Serial.begin(115200);

}

void loop() {
if (Serial.available()) {
  Serial.println();
  Serial.println();
  Serial.print("Start.. avail:");
  Serial.println(Serial.available());
  Serial.print("up: ");
  Serial.println(millis()/1000);
  Serial.read();
  Serial.println("BB zahl 1,zahl2");
  bb1 = Serial.read();
  bb2 = Serial.read();
  
  Serial.println("got: ");
  Serial.println(bb1);
  Serial.println(bb2);
  res = (bb1 << 8) | bb2;

  Serial.println("gives");
  Serial.println(bb1);
  Serial.println(bb2);
  Serial.println(res);
  Serial.println("end");
  Serial.println();

}
}
