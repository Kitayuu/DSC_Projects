// This is the program for a step-up module output voltage controller.
// This project reads voltage and current from step-up module output then sent signals to control motorized potentiometer to stabilize voltage needed.

#define enA 11 //define pin #11 for Enable signal to L298N module
#define relay 10 //define pin #10 for Relay control

float VoltageInput = 0.0; // pin number for voltage input
float vout = 0.0; // voltage output
float vin = 0.0; // voltage input
float R1 = 200000.0; // resistance of R1 (200K)
float R2 = 10000.0; // resistance of R2 (10K)
float value = 0.0; // final voltage output
bool max = false, min = false, greaterThan = false;

float crntInput = 1.0; // pin number for hall effect sensor
float mVperAmp = 100; // use 100 for 20A Module and 66 for 30A Module
float RawValue = 0; // raw value of hall effect bofore the formula
float ACSoffset = 2500.0; // offset from hall effect sensor. 2500 comes from 5v in milli-volt which is 5000 divided by 2
float Voltage = 0.0; // voltage from hall effect sensor
float Amps = 0.0; // current output
float ma = 0.0; // current output in milli-amp

float numReadings = 1024.0; // number of readings for averaging
float total = 0.0; // total A/D readings
float Recieve = 0.0; // variable for store voltage needed
bool newData = 0.0; // boolean variable for new voltage needed checking
String rec = "";
char Character;

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// function to turn on the relay
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void relayOn() {
  digitalWrite(relay, HIGH);
  Serial.println("Relay ON !!");
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// function to off on the relay
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void relayOff() {
  digitalWrite(relay, LOW);
  Serial.println("Relay Off !!");
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// function that reads voltage values 128 times and take average out of them then return voltage value
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
float readVolt() {
  total = 0.0;
  for (int i = 0 ; i < 128 ; i++) {
    total = total + analogRead(VoltageInput); 
  }
  value = (total / 128);
  vout = (value * 5.0) / 1024.0;
  vin = vout / (R2 / (R1 + R2));

  return vin;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// function that reads current values 1024 times from hall effect and average them then return current value
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
float readCurrent() {
  RawValue = 0.0;
  for (int i = 0; i < numReadings; i++) {    
    RawValue += analogRead(A1);    
  }  
  RawValue = RawValue / numReadings;
  Voltage = (RawValue / numReadings) * 5000; // Gets you mV
  Amps = ((Voltage - ACSoffset) / mVperAmp);
  
  if (Amps < 0.01) {    
    Amps = 0;    
  }
  
  return Amps;

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// function that call 2 function which is readVolt and readCurrent then print out value of both 
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void showVal() {

  Serial.print("INPUT V = ");
  Serial.print(readVolt(), 2);
  Serial.print(" V : ");
  Serial.print("Current = ");
  
  if (readCurrent() < 1.0) {    
    ma = readCurrent() * 1000;
    Serial.print(ma);
    Serial.println(" mA");    
  } else {    
    Serial.print(readCurrent(), 3);
    Serial.println(" A");    
  }

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// function that will adjust the value of potentiometer to maximum
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void turnToMax() {

  digitalWrite(12, LOW);
  digitalWrite(13, HIGH);
  digitalWrite(enA, HIGH);
  delay(7000);
  digitalWrite(enA, LOW);
  min = false;
  max = true;

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// function that will adjust the value of potentiometer to minimum
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void turnToMin() {

  digitalWrite(12, HIGH);
  digitalWrite(13, LOW);
  digitalWrite(enA, HIGH);
  delay(7000);
  digitalWrite(enA, LOW);
  max = false;
  min = true;

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// function that will decrease potentiometer value for 8ms once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void stepDown() {

  digitalWrite(enA, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, LOW);
  delay(8);
  digitalWrite(enA, LOW);

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// function that take 1 parameter as a time needed(ms) and use that value to decrease potentiometer value once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void stepDown(float mil) {

  digitalWrite(enA, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, LOW);
  delay(mil);
  digitalWrite(enA, LOW);

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// function that will increase potentiometer value for 8ms once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void stepUp() {

  digitalWrite(enA, HIGH);
  digitalWrite(12, LOW);
  digitalWrite(13, HIGH);
  delay(8);
  digitalWrite(enA, LOW);

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// function that take 1 parameter as a time needed(ms) and use that value to increase potentiometer value once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void stepUp(float mil) {

  digitalWrite(enA, HIGH);
  digitalWrite(12, LOW);
  digitalWrite(13, HIGH);
  delay(mil);
  digitalWrite(enA, LOW);

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// voltage stabilizer function recieve 1 parameter which is voltage needed then try to adjust the potentiometer using stepUp and stepDown function. 
// after output voltage reached the point program will try to stabilize output voltage
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void voltageStabilizer(float voltage) {

  if (readVolt() >= voltage && greaterThan != true) {
    greaterThan = true;
    stepDown(50);
    delay(500);
    showVal();
  }
  if (readVolt() < voltage && greaterThan != true && readVolt() > (voltage - voltage * 0.2)) {
    stepUp(80);
    delay(1000);
    showVal();
  }
  if (readVolt() < voltage && greaterThan != true) {
    stepUp(50);
    delay(100);
    showVal();
  }
  if (readVolt() > voltage && greaterThan == true) {
    if (readVolt() <= (voltage + voltage * 0.01)) {
      showVal();
      digitalWrite(enA, LOW);
      digitalWrite(12, LOW);
      digitalWrite(13, LOW);
      delay(2000);
    } else if (readVolt() >= voltage + 3) {
      stepDown(100);
      //delay(500);
      showVal();
    } else {
      stepDown(50);
      delay(2000);
      showVal();
    }
  }
  if (readVolt() < voltage && greaterThan == true) {
    if (readVolt() >= (voltage - voltage * 0.01)) {
      showVal();
      digitalWrite(enA, LOW);
      digitalWrite(12, LOW);
      digitalWrite(13, LOW);
      delay(2000);
    } else {
      if (voltage < 18) {
        stepUp(30);
        delay(2000);
        showVal();
      } if (readVolt() < voltage - 1.5) {
        stepUp(30);
        delay(500);
        showVal();
      } else {
        stepUp(50);
        delay(500);
        showVal();
      }
    }
  }
}


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//The setup() function is called when a sketch starts. The setup function will only run once, after each powerup or reset of the Arduino board.
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup() {

  pinMode(VoltageInput, INPUT); // set pin A0 as input pin
  pinMode(crntInput, INPUT);// set pin A1 as input pin
  pinMode(relay, OUTPUT);// set pin 10 as output pin
  pinMode(enA, OUTPUT);// set pin 111 as output pin
  pinMode(12, OUTPUT);// set pin 12 as output pin
  pinMode(13, OUTPUT);// set pin 13 as output pin

  Serial.begin(9600);// start serial monitor protocol using bit buad 9600

// Adjust the value of potentiometer to minimum then calibrate the hall effect sensor 
  Serial.println("Adjusting and Calibrating ... please wait");
  turnToMin(); 
  RawValue = 0.0; // 
  for (int i = 0; i < numReadings; i++) {
    RawValue += analogRead(A1);
  }
  RawValue = RawValue / numReadings;
  Voltage = (RawValue / numReadings) * 5000; // Gets you mV
  ACSoffset = Voltage;
  Serial.print("Current Sensor offset : ");
  Serial.println(ACSoffset);

// the program pausing and waiting for voltage needed.
// User must give the voltage needed !! to continue the program
  Serial.print("Voltage Needed : ");
  while (!Serial.available()) {
  }
  
  Recieve = Serial.parseFloat();
  Serial.println(Recieve);
  Serial.read();

// turn off relay (just in case) and start printing voltage and current on serial monitor
  relayOff();
  showVal();
  
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// After creating a setup() function, which initializes and sets the initial values, the loop() function does precisely what its name suggests, and loops consecutively,
// allowing the program to change and respond. This will be used to actively control the Arduino board.
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void loop() {

  // 
  if (Serial.available()) { // check if there's any input available

    rec = Serial.readStringUntil('\n'); // read string from serial port
    
    if (rec == "on" || rec == "ON" || rec == "On" || rec == "oN" ) { // if readed value is on

      Serial.read(); // clear serial
      relayOn(); // turn on the relay

    } else if (rec == "off" || rec == "OFF") { // if readed value is off

      Serial.read(); // clear serial
      relayOff(); // turn off the relay

    } else { // if string isn't on or off

      Recieve = rec.toFloat(); // cast readed string to number
      Serial.read(); // clear serial
      if (Recieve < 12) { // check if readed value is less than 12 (minimum output voltage is 12)

        Recieve = 12;
        Serial.println("Adjusting to 12v");
        relayOff(); // turn off relay
        voltageStabilizer(Recieve); // try to stabilize output voltage at 12v.

      } else if (Recieve > 80) { // check if readed value is greater than 80 (maximum output voltage is 80)

        Recieve = 80;
        Serial.println("Adjusting to maximum voltage 80v");
        relayOff(); // turn off relay
        voltageStabilizer(Recieve); // try to stabilize output voltage at 80v.

      } else { // if readed string not less than 12 and not greater than 80

        Serial.print("New Voltage Value Recieved : ");
        Serial.println(Recieve);
        voltageStabilizer(Recieve); // try to stabilize output voltage at the voltage needed.

      }

    }
  } else {

    voltageStabilizer(Recieve); // try to stabilize output voltage at the voltage needed.

  }

}
