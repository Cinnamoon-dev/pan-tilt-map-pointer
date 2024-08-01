// you can redefine the LED_BUILTIN variable
// if the compiler does not find a definition
// but you'll have to specify the pin number
#define LED_BUILTIN 2

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
