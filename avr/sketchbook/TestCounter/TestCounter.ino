/* Test counter */

// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led = 13;
int out = 3;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);     
  pinMode(out, OUTPUT);     
  digitalWrite(out, LOW);
  for (int i = 0; i < 1000; i++)
    {
      digitalWrite(out,HIGH);
      delay(40);
      digitalWrite(out,LOW);
      delay(40);
    }
}

// the loop routine runs over and over again forever:
void loop() {
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);               // wait for a second
}
