/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);  // Init Uart
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Hello World!\r\n");   //print "Hello World!"
  delay(1000);
}
