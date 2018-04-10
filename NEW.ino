
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// If using software SPI (the default case):
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    LOW
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define XPOS 0
#define YPOS 1
#define DELTAY 2
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif
char* messageString[]={"ENTER #", "CALL:", "CALL IN PROGRESS", "CALL ENDED", "INCOMING CALL", "BAD NUMBER", "NO ACTIVE CALLS", "THEY HUNG UP!"};
const byte ROWS = 4; //FIVE  rows
const byte COLS = 4; //three columns
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'},
};
byte rowPins[ROWS] = {2, 3, 4, 5 };   //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 12};    //connect to the column pinouts of the keypad
int debounce[4][4]; 
int key = 0;              //What key is pressed
byte input[127];            //The input buffer
byte buffPos = 0;           //Where we are in the buffer
byte numberCall[11] = {'_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',};
byte cursor = 0;
unsigned long timer = 0;
int battery = 1;            //Battery 0-9 powerz. Set to 1 to use as initialization flag
int signal = 0;             //Signal zero to 4 bars can you hear me now?
int message = 0;            //What message should be onscreen
int activeCall = 0;           //If a call is active or not
unsigned long ringingTimer =0;
int theyHungUp = 0;
unsigned long sleepTimer = millis();
int screenStatus = 1;
int incomingCall = 0;

void setup()   {                
  Serial.begin(115200);
  //Define column pins for keyboard matrix
  pinMode(colPins[0], 1);
  pinMode(colPins[1], 1);
  pinMode(colPins[2], 1);
  pinMode(colPins[3], 1);
  pinMode(A0, 0); //ringing
  digitalWrite(A0, 1);
  pinMode(A2, OUTPUT);//ringing indicator
  digitalWrite(A2, LOW);
  pinMode(A1, OUTPUT); //A6 module sleep pin , 1 == wake ; 0 == sleep
  digitalWrite(A1, 1);
  //Define row pins for keyboard matrix
  for (int x = 0 ;  x < 4 ; x++) {
    pinMode(rowPins[x], 0);        //Set as input
    digitalWrite(rowPins[x], 1);   //Set internal pullup
  }
  //display setup
  display.begin(SSD1306_SWITCHCAPVCC);
  display.setTextColor(WHITE);
  display.display();
  delay(2000);
  display.clearDisplay(); 
  while(0){
  //while(battery) {            //Wait for the phone module to boot 

    Serial.println("AT");         //Try and get a response
    if (Serial.available()) {       //A response?
      battery = 0;            //Continue!
    }
    display.clearDisplay();
    display.setTextSize(1);   
    display.setCursor(0, 0);  
    display.print("WAITING FOR SIM");
    display.setCursor(0, 12);
    display.print(millis()/1000);
    display.display();  

    delay(1000);    
  }
    battery = 8;              //Used as flag, so reset it now
  drawDisplay();              //Show display
  Serial.println("AT+CRSL=15");         //Ringer SoundLevel
  Serial.println("AT+CLVL=8");     //Loudspeaker volume level
  Serial.println("AT+VGR=8");        //Receive gain selection
  Serial.println("ATE0");
  
}

void loop() {
  if(millis() - sleepTimer > 1000 * 20){ //turn off the oled screen after 20 seconds without any input
    if(activeCall == 0){
      sleepDisplay();
    }
  }
  if (digitalRead(A0) == 0 and incomingCall == 1) {
    wakeDisplay();
    message = 4;
    activeCall = 2;
    drawDisplay();
    digitalWrite(A2, HIGH);
    ringingTimer = millis();
  }
  if(digitalRead(A0) == 1 and activeCall == 2){   //we didn't answer and they hung up
    if(millis()-ringingTimer > 4000){
      wakeDisplay();
      message = 3;
      activeCall = 0;
      incomingCall = 0;
      digitalWrite(A2, LOW);
      drawDisplay();  
    }
  }
  if(activeCall == 1 and theyHungUp == 1){ //if they hung up
      wakeDisplay();
      activeCall = 0;
      message = 3;
      theyHungUp = 0;
      incomingCall = 0;
      drawDisplay();
      
  }

  getKeys();
  if (key) {
    if(screenStatus == 1){
    wakeDisplay();
    if (key > 64) {             //A B or C key?
      if (key == 'A') {
      if (activeCall) {
        message = 3;   
        Serial.println("ATH");   //End the call
        activeCall = 0;
        digitalWrite(A2, LOW);
      }
      else {
        message = 6;        //No current call
      }             
      }
      if (key == 'C') {
      if (activeCall == 2) {      //Someone calling us?
        activeCall = 1;
        Serial.println("ATA");    //Answer it
        message = 2;
        incomingCall = 0;
        digitalWrite(A2, LOW);
      }
      if (activeCall == 0) {      //Call me maybe?
        if (cursor < 10) {     
        message = 5;      
        }
        else {
        makeCall();       
        }   
      }       
      }   
      if (key == 'B') {
      if (cursor == 10) {        //On last character?
        if (numberCall[cursor] == '_') {
        cursor -= 1;
        numberCall[cursor] = '_';
        }
        else {
        numberCall[cursor] = '_'; 
        }
      }
      else {
        if (cursor > 0 and cursor < 10) {
        cursor -= 1;        //Decrement cursor
        numberCall[cursor] = '_'; //and erase whatever was there        
        if (cursor == 0) {
          message = 0;
        }         
        }           
      }           
      }
      if(key == 'D'){
      sleepDisplay();
      }       
    }
    else {
      numberCall[cursor] = key;
      if (cursor < 10) {     
      cursor += 1;      
      }
      message = 1;
    }
    
    }else if(key == 'D'){
      wakeDisplay();
    }
  drawDisplay();
    key = 0;
  }
  timer += 1;
  if (timer == 4000) {
    Serial.println("AT+CSQ");   //Get signal status
    timer = 0;            //Reset timer
  }
  
  if (Serial.available()) {      //Something available?
    
    input[buffPos] = Serial.read(); //Read it into buffer

    if (buffPos > 2 and input[buffPos - 3] == 13 and input[buffPos - 2] == 10 and input[buffPos - 1] == 13 and input[buffPos] == 10) {    
       interpretBuffer();
    } 
    else if(buffPos > 2 and input[buffPos - 3] == 78 and input[buffPos - 2] == 71 and input[buffPos - 1] == 13 and input[buffPos] == 10){ 
        incomingCall = 1;
        Serial.print("getting ring");
    }
    else {
      buffPos += 1;
      if (buffPos > 125) {
        clearBuffer();
      }
    }
  }
}

//functions
void makeCall() {
  message = 2;
  activeCall = 1; 
  Serial.print("ATD");        //Prefix
  for (int x = 0 ; x < 11 ; x++) {  //Stream the phone #  
    Serial.write(numberCall[x]);  
  }
  Serial.write( '\r' ); //  Carriage Return
  Serial.write( '\n' ); //  Line feed
}

void getKeys() {
  for (int column = 0 ; column < 4 ; column++ ) {
    digitalWrite(colPins[0], 1);      //Set all high
    digitalWrite(colPins[1], 1);
    digitalWrite(colPins[2], 1);
    digitalWrite(colPins[3], 1);
    digitalWrite(colPins[column], 0);   //Then set active column LOW
    for (int row = 0 ; row < 4 ; row++) {
      if (digitalRead(rowPins[row]) == 0) { //Is that switch pressed to active low?
        if (debounce[column][row] == 0) { //Debounce cleared?
          key = hexaKeys[row][column];
          debounce[column][row] = 2000;
        }
        else {
          debounce[column][row] -= 1;   //If button pressed, count down debounce for repeat
        }
      }
      else {
        debounce[column][row] = 0;      //If button released, zero out debounce
      }   
    }
  }
}

void drawDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  //Draw the current message
  display.setTextSize(0); 
  display.setCursor(0, 2);
  //display.print(battery); 
  //display.print(" ");
  //display.print(signal);  
  display.print(messageString[message]);
  //Draw the signal bars
  int horizontal = 96;
  if (signal) {
    for (int x = 1 ; x < (signal + 1) ; x++) {
      display.drawLine(horizontal + x, 8, horizontal + x, 8 - (x * 2), WHITE);
      display.drawLine(horizontal + 1 + x, 8, horizontal + 1 + x, 8 - (x * 2), WHITE);
      horizontal += 3;
    } 
  }
  else {
    display.setTextSize(0); 
    display.setCursor(96, 1);
    display.print("X");   
  }
  //Draw the battery
  display.drawLine(114, 0, 126, 0, WHITE);
  display.drawLine(114, 0, 114, 8, WHITE);
  display.drawLine(114, 8, 126, 8, WHITE);  
  display.drawLine(126, 8, 126, 0, WHITE);
  display.drawLine(127, 2, 127, 6, WHITE);  
  for (int x = 0 ; x < (battery + 1) ; x++) {
    display.drawLine(115 + x, 1, 115 + x, 7, WHITE);
  }
  display.drawLine(0, 15, 127, 15, WHITE);
  display.setTextSize(2); 
  display.setCursor(0, 22);//first 3 numbers for the first row
  for (int x = 0 ; x < 11 ; x++) {
    if(x > 2 ){ //rest of the numbers
      int pos = (x-3)*12.4;
      display.setCursor(pos, 42);
    }
    display.write(numberCall[x]);
  }
  display.display();  
}

void interpretBuffer() {
  int flag = 1;
  while(flag) {
    buffPos -= 1;
    if (input[buffPos] == ':') {  //Go backwards and find the colon
      flag = 0;
    }
    if (buffPos == 0 and flag == 1) {
      flag = 0;         //Didn't find it? Abort
      clearBuffer();
      return;
    }
  }
  flag = buffPos - 3;         //Look for the code CDC or CSQ
  if (input[flag] == 'C' and input[flag + 1] == 'S' and input[flag + 2] == 'Q') { //Signal report?
    if (input[flag + 6] == ',') { //Single digit? Charge that sucker man!
      signal = (input[flag + 5] - 48);      
    }
    else {
      signal = ((input[flag + 5] - 48) * 10) + (input[flag + 6] - 48);    
    }
    signal /= 6; 
    if (signal > 4) {
      signal = 4;
    }
  }
  if(input[flag] == 'I' and input[flag + 1] == 'E' and input[flag + 2] == 'V' and input[flag + 12] == '0'){ //did they hung up on us?
    if(activeCall == 1){
      theyHungUp = 1;  
    }
  }
  clearBuffer();
  drawDisplay();
}

void clearBuffer() {
  for (int x = 0 ; x < 127 ; x++) {
    input[x] = 32;
  }
  buffPos = 0;
}

void sleepDisplay() {
  screenStatus = 0;
  digitalWrite(A1, 0); // a6 module sleep
  display.ssd1306_command(SSD1306_DISPLAYOFF);
}

void wakeDisplay() {
  sleepTimer = millis();
  digitalWrite(A1, 1);//wake up the a6 module
  screenStatus = 1;
  display.ssd1306_command(SSD1306_DISPLAYON);
}
