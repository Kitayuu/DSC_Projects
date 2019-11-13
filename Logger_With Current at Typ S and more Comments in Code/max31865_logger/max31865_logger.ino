// This is the program for a tempertaure data logger for the induction furnace.
// This project reads three temperature from the attached thermocouples and records data on an SD-Card attached to an Arduino Mega board.
// There is a LED light which displays the status of the data logger (whether ON/OFF) and a toggle switch that is used to start
// and stop the logging. Another feature of this data logger is that it creates a new file name to sava data each time the logger starts, and increments the file name dynamically. There is also an LCD attached, which displays the internal and the external temperature recorded by the three thermocouple that is attached to the microcontroller.
// Note: Before the program is loaded, check 1) Which I2C address the LCD is connected to and update the program accordingly. 2) Set the desired date, day and time for the clock.

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// The section below includes all the libraries that is required to run the various modules that are used in the integrated circuit of the data logger.
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <Wire.h> //This library allows to communicate with I2C 
#include <DS3231.h> //This library implements the feature of reading/writing the current time, day, date
#include <LiquidCrystal_I2C.h> //Library for the LiquidCrystal LCD display connected to the Arduino board
#include <SPI.h> //This library allows you to communicate with SPI devices
#include <SD.h> //The SD library allows for reading from and writing to SD cards
#include <Adafruit_MAX31865.h>

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// The section below describes all the variables and initialises them with the respective pin numbers on the Ard ino Mega board or/and assign them with initial values.
// The following are defined
// 1. The temperatures variables assigned to the three thermocouples and one variable each assigned to the timer, toggle switch(pin)status and the counter.
// 2. Specific pins on the Arduino Mega will be assigned to the LED, the toggle switch, thermocouple SPI pins and the output pin of the SD card.
// The function of the counter is to count the number of temperatures recorded when the logger in ON. In our case, it is twice every second.
// The funtion of the timer is to keep a track of the time starting from 0.00 sec, with a continuous increment of 0.50 sec.
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#define RREF      430.0
#define RNOMINAL  100.0

const int pinCS = 53; // Chip Select pin of SD card reader, which renders the output assigned to pin number 53 on the Arduino mega board

float temperature1 = 0; // Datatype for floating-point numbers (in this case the temperature), a number that has a decimal point
float temperature2 = 0;
float temperature3 = 0;

const int relay = 40; // output pin for relay control signal
const int inPin = 46; // the input pin (for the toggle switch)
const int relaySW = 48; // the switch input for relay control
const int processLed = 42; // LED for relay indicator
const int ledPin = 44; // the pin for the log LED

int val = 0; // variable for reading the pin status and initial value assignment
int counter = 0; // variable to store initial value of the counter
float timer = 0;// variable to store initial value of the timer, starting from the time logger is switched ON
double avgT = 0.0; // variable to store average value between T1 and T2
double diff = 0.0; // variable to store difference value of T1 and T2 - averageT
bool cast = false; // variable to indicate if there is any serial value input
double temp = 0.0; // variable to store serial input temperature needed
bool reached = false; // variable for checking if temperature reached value needed
bool sw = false;
int p = 0;

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// This section includes the initialisations of the DS3231 (clock module), LCD display and the thermocouple. This is required for the modules to communicate with the Arduino Board.
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DS3231  rtc(SDA, SCL); // Initialise the DS3231 using the hardware interface
LiquidCrystal_I2C lcd(0x3F, 20, 4); //Find from the i2c_scanner program which address is the I2C connected to.
//set the LCD address to 20, 4 for a 20 chars and 4 line display

// Use software SPI: CS, DI, SDO, CLK
Adafruit_MAX31865 thermocouple1(2, 3, 4, 5);
Adafruit_MAX31865 thermocouple2(6, 7, 8, 9);
Adafruit_MAX31865 thermocouple3(10, 11, 12, 13);

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// This section includes all the variable and functions required for the dynamic file name creation and incrementation.
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

long fileNum = 10000;  // maximum 99999
String fileName; // variable to store file name
char name[13]; //to store character value of the file name. Characters are stored as numbers, wherein each character has an ASCII value attached to it. [13] allocates space
// for the string
File dataFile; // variable to save the data from SD card

void incFileNum() { // generate next file name:
  String s = "Log" + String(++fileNum) + ".txt"; //saves in .txt format
  s.toCharArray(name, 13); //Copies the string's characters to the supplied buffer (to copy the characters into the char[])
}

//saving the data each time in a new file, when one is created
void save(String s) {
  dataFile = SD.open(name, FILE_WRITE); // both read and write to the file
  if (dataFile) {
    dataFile.println(s);
    dataFile.close(); // close the file, physically save to sd card
    Serial.println(s);
  }
  else Serial.println("error opening " + String(name));
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//The setup() function is called when a sketch starts. The setup function will only run once, after each powerup or reset of the Arduino board.
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  //------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  //This section is used to initialise the serial communication, clock module, the LCD and SD card module.
  //------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  Serial.begin(9600); // Start serial communication at baud rate of 9600

  rtc.begin(); // Initialize the clock module
  rtc.setDOW(FRIDAY);     // Set Day-of-Week to, for eg.: SUNDAY
  rtc.setTime(00, 00, 00);   // Set the time to, for eg.: 12:00:00 (24hr format)
  rtc.setDate(28, 11, 2018);  // Set the date

  pinMode(pinCS, OUTPUT); // declare Chip select pin of the SD card as output
  pinMode(ledPin, OUTPUT); // declare LED as output
  pinMode(inPin, INPUT);  // declare recording switch as input
  pinMode(relaySW, INPUT); // declare relay switch as input
  pinMode(processLed, OUTPUT); // declare recording LED as output
  pinMode(relay, OUTPUT); // declare relay LED as output

  // LCD start
  lcd.begin(20, 4); // always in setup
  lcd.init();      // initialize the lcd
  lcd.backlight(); // initialize the lcd backlight

  // initialize thermocouple
  thermocouple1.begin(MAX31865_2WIRE); // begin first max31865 module and assign as thermocouple1
  thermocouple2.begin(MAX31865_2WIRE); // begin second max31865 module and assign as thermocouple2
  thermocouple3.begin(MAX31865_2WIRE); // begin third max31865 module and assign as thermocouple3
  
  // SD Card Initialization
  if (SD.begin())
  {
    Serial.println();
    Serial.println("SD card is ready to use.");
    lcd.setCursor(0, 0); // start using LCD top-left
    lcd.print("SD Card is Ready");

  } else
  {
    Serial.println();
    Serial.println("No SD Card");
    lcd.setCursor(0, 0);
    lcd.print("No SD Card");
    return;
  }

  // recieve Temp. value through serial port
  Serial.print("Temperature Needed : ");

}


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// After creating a setup() function, which initializes and sets the initial values, the loop() function does precisely what its name suggests, and loops consecutively,
// allowing the program to change and respond. This will be used to actively control the Arduino Mega board.
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void loop() {

  if (Serial.available()) {
    temp = Serial.parseFloat();
    Serial.print("Temperature Needed : ");
    Serial.println(temp);
    Serial.read();
    cast = true;
  }
  Serial.read();

  //------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  // This section reads the switch status and executes the following steps:
  // 1. Types the heading on the SD card (columnwise).
  // 2. Requests the temperature from the thermocouple breakout board.
  // 3. Prints the temperature on the LCD display after the very first time the logger is switched ON and for the rest of the time until the microcontroller board is powered.
  //------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  val = digitalRead(inPin);  // read input value
  p = digitalRead(relaySW); // read relay switch input and assign to variable 'p'
  //This section is for recording data into the SD Card
  dataFile = SD.open(name, FILE_WRITE); // Create/Open file
  // if the file opened okay, write to it:
  if (dataFile) {
    dataFile.close(); // close the file, physically save to sd card
  }

  // Sensor reading starts - This is outside the while loop, so that the LCD displays the temperatures even when the logger is stopped and it is not logging data to the SD Card
  // Requesting temperatures from thermocouple 1
  double c1 = thermocouple1.temperature(RNOMINAL, RREF); // 'double' is used to define "c1", the outside temperature recorded by the thermocouple with double precision floating point number
  // Alternatively, this could be defined at the start of the program with the other variables

  //Requesting temperatures from thermocouple 2
  double c2 = thermocouple2.temperature(RNOMINAL, RREF);

  //Requesting temperatures from thermocouple 3
  double c3 = thermocouple3.temperature(RNOMINAL, RREF);

  avgT = (c1 + c2) / 2; // average value between Thermocouple 1 and 2
  diff = (c1 - avgT) - (c2 - avgT);

  // This section will print the requested temperatures on the LCD display.
  //Thermocouple 1
  lcd.setCursor(0, 1);
  lcd.print("T1:");
  lcd.print(c1);
  lcd.print(" REF:");
  lcd.print(avgT);

  //Thermocouple 2
  lcd.setCursor(0, 2);
  lcd.print("T2:");
  lcd.print(c2);
  lcd.print(" T1-T2:");
  lcd.print(diff, 2);

  //Thermocouple 3
  lcd.setCursor(0, 3);
  lcd.print("T3:");
  lcd.print(c3);
  lcd.print(" Temp:");
  lcd.print(temp);

  //------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  //The section below executes new file name creation upon switching the logger on. Once the switch is ON, the LED lights up and the name of the file is incremented to the next
  //number. Also, the LCD at the same time displays the name of the file, in which the data will be logged. If the toggle switch is switched OFF, then the LED will be OFF,
  //but the LCD will still display the temperature as defined in the previous section.
  //------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  if (p == 1 && cast == false) { // if relay switch is on and no serial input

    digitalWrite(relay, HIGH);
    digitalWrite(processLed, HIGH);

  } if (p == 0 && cast == false) { //if relay switch is off and no serial input

    digitalWrite(relay, LOW);
    digitalWrite(processLed, LOW);

  } else if (p == 0 && cast == true) { //if relay switch is off and there's serial input

    if (avgT < (temp - 1.0) && reached == false && cast == true) {
      reached = false;
      digitalWrite(relay, HIGH);
      digitalWrite(processLed, HIGH);
    } else if (avgT >= (temp - 0.5) && cast == true) {
      reached = true;
      digitalWrite(relay, LOW);
      digitalWrite(processLed, LOW);
    }

    if (avgT < (temp - 1.0) && reached == true) {
      digitalWrite(relay, HIGH);
      digitalWrite(processLed, HIGH);

      if (c3 >= temp + 23) {
        digitalWrite(relay, LOW);
        digitalWrite(processLed, LOW);
      } else if (c3 < temp + 10) {
        digitalWrite(relay, HIGH);
        digitalWrite(processLed, HIGH);
      }
    } else if (avgT >= (temp - 0.3) && reached == true) {
      digitalWrite(relay, LOW);
      digitalWrite(processLed, LOW);
    }

    if (c3 >= (temp + 30)) {
      digitalWrite(relay, LOW);
      digitalWrite(processLed, LOW);
    }
  }

  if (val == HIGH) { //if toggle switch is ON

    digitalWrite(ledPin, HIGH); // turn LED ON
    //increment the file name

    if (SD.begin()) {
      incFileNum(); // set it to dat10000.txt
      while (SD.exists(name)) incFileNum(); //call the function incFileNum
      Serial.println("new file name: " + String(name)); // Print the new file name
      save(""); // call save function and print the label inside the quotes
      timer = 0.0; // set it to this value in order for the timer to increment to 0.0 secs when the push button is pressed
      lcd.setCursor(0, 0); //First line of the LCD
      lcd.print("LOGGING:" + String(name)); // Displaying the log file where the data is going to be saved
    } else {
      Serial.println("No SD card"); // Print the new file name
      lcd.setCursor(0, 0); //First line of the LCD
      lcd.print("   INSERT SD-CARD   "); // Displaying the log file where the data is going to be saved
    }

  } else {
    digitalWrite(ledPin, LOW);  // turn LED OFF
    lcd.setCursor(0, 0); //First line of the LCD
    lcd.print("        READY       ");
  }

  //------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  //This section uses the while loop. The while loop will loop continuously, and infinitely, until the expression inside the parenthesis() becomes false. Hence it is important to
  //keep reading and updating the value of the switch inside the while loop, or else the while loop will never exit. In regards to this specific program, inside the while loop
  //the following tasks are executed:
  //1. The timer and counter are incremented twice in every second while the switch remains ON.
  //2. The values corresponding to the timer, counter, clock and the thermocouples are printed in the serial moniter.
  //3. These values are additionally, displayed in the LCD display attached to the logger.
  //4. Values are recorded in the SD Card and into the respective file name.
  //5. The current status of the toggle switch is checked once in the whole cycle to decide whether or not to stay inside the while loop.
  //------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  while (val == HIGH) { //when toggle switch is ON

    val = digitalRead(inPin); // read current status of the toggle switch, this check is required so the program knows when to exit the while loop
    p = digitalRead(relaySW);

    //Sensor reading starts - This is for reading the temperature inside the while loop when the logger is logging data
    //Requesting temperatures from thermocouple 1, 2 and 3
    double tc1 = thermocouple1.temperature(RNOMINAL, RREF);
    double tc2 = thermocouple2.temperature(RNOMINAL, RREF);
    double tc3 = thermocouple3.temperature(RNOMINAL, RREF);

    avgT = (tc1 + tc2) / 2;
    diff = (tc1 - avgT) - (tc2 - avgT);

    if (Serial.available()) {
      temp = Serial.parseFloat();
      Serial.read();
      Serial.print("Temperature Needed : ");
      Serial.println(temp);
      cast = true;
    }
    Serial.read();

    // This section will print the requested temperatures on the LCD display.
    lcd.setCursor(0, 1);
    lcd.print("T1:");
    lcd.print(tc1);
    lcd.print(" REF:");
    lcd.print(avgT);

    lcd.setCursor(0, 2);
    lcd.print("T2:");
    lcd.print(tc2);
    lcd.print(" T1-T2:");
    lcd.print(diff, 2);

    //Thermocouple 3
    lcd.setCursor(0, 3);
    lcd.print("T3:");
    lcd.print(tc3);
    lcd.print(" Temp:");
    lcd.print(temp);

    if (p == 1 && cast == false) {
      digitalWrite(relay, HIGH);
      digitalWrite(processLed, HIGH);

    } else if (p == 0 && cast == false) {
      digitalWrite(relay, LOW);
      digitalWrite(processLed, LOW);

    } else if (p == 0 && cast == true) {
      if (avgT <= (temp - 1.0) && reached == false) {
        reached = false;
        digitalWrite(relay, HIGH);
        digitalWrite(processLed, HIGH);
      } else if (avgT > (temp - 0.5) && reached == false) {
        reached = true;
        digitalWrite(relay, LOW);
        digitalWrite(processLed, LOW);
      }


      if (avgT <= (temp - 1.0) && reached == true) {
        digitalWrite(relay, HIGH);
        digitalWrite(processLed, HIGH);

        if (tc3 >= temp + 23) {
          digitalWrite(relay, LOW);
          digitalWrite(processLed, LOW);
        } else if (tc3 < temp + 10) {
          digitalWrite(relay, HIGH);
          digitalWrite(processLed, HIGH);
        }

      } else if (avgT > (temp - 0.3) && reached == true) {
        digitalWrite(relay, LOW);
        digitalWrite(processLed, LOW);
      }

      if (tc3 >= (temp + 30)) {
        digitalWrite(relay, LOW);
        digitalWrite(processLed, LOW);
      }
    }

    // This section is used for printing values into the serial monitor
    counter = counter + 1; // increment counter to first reading and so on
    Serial.print(counter); // serial print data
    Serial.print("\t"); // prints Tab

    timer = timer + 1; // increment timer
    Serial.print(timer); // serial print data
    Serial.print("\t"); // prints Tab

    // This section is used for printing the temperatures into the serial monitor
    if (isnan(tc1)) { // if the reading is not a number
      Serial.println("Something wrong with thermocouple!");
    } else {
      Serial.print(tc1);
      Serial.print("\t");
    }
    if (isnan(tc2)) {
      Serial.println("Something wrong with thermocouple!");
    } else {
      Serial.print(tc2);
      Serial.print("\t");
    }
    if (isnan(tc3)) {
      Serial.println("Something wrong with thermocouple!");
    } else {
      Serial.print(tc3);
      Serial.print("\t");
    }

    Serial.print("Ref:");
    Serial.print(avgT, 2);
    Serial.print("\t");
    Serial.print("T1-T1:");
    Serial.print(diff);
    Serial.println();

    //This section is for recording data into the SD Card
    dataFile = SD.open(name, FILE_WRITE);// both read and write to the file
    // if the file opened okay, write to it:
    if (dataFile) {

      dataFile.print(counter);
      dataFile.print("\t");
      dataFile.print(timer);
      dataFile.print("\t");

      if (isnan(tc1)) {
        dataFile.println("Something wrong with thermocouple1!");
        dataFile.print("\t");
      }
      else {
        dataFile.print(tc1);
        dataFile.print("\t");
      }

      if (isnan(tc2)) {
        dataFile.println("Something wrong with thermocouple2!");
        dataFile.print("\t");
      }
      else {
        dataFile.print(tc2);
        dataFile.print("\t");
      }

      if (isnan(tc3)) {
        dataFile.println("Something wrong with thermocouple3!");
        dataFile.print("\t");
      }
      else {
        dataFile.print(tc3);
        dataFile.print("\t");
      }

      dataFile.print(avgT, 2);
      dataFile.print("\t");
      dataFile.print(diff);
      dataFile.println(); // Print new line
      dataFile.close(); // close the file, physically save to sd card
    }
    delay(1000);
    //    avgT = (tc1 + tc2) / 2;
    val = digitalRead(inPin); // reads the status of the recording switch
    p = digitalRead(relaySW); // reads the status of relay switch before out finish while loop
    
  }
}
