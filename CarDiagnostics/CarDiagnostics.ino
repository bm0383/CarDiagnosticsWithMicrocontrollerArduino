#include <SoftwareSerial.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>

#define param_size 11

File myFile;
#define I2C_ADDR 0x27
#define BACKLIGHT_PIN 3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

#define DS3231_I2C_ADDRESS 0x68
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}
void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); 
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

char recvChar;
int value = 0;
String year_name = "";
String command = "";
String inputString = ""; 
String s = "";
int index = -1;
int a = 0;
int b = 0;
String sub = "";
int temp = 0;

int speed_test = 0;
int maf_test = 0;
double poraba = 0;
double sum_poraba = 0;
int num_poraba = 0;
double avg_poraba = 0;
int revs = 0;
double poraba_save = 0;

//SETUP FUNCTION
void setup() { 
  Wire.begin();
  Serial.begin(9600);
   while (!Serial) {
    ;
  }
  //SD
  SD.begin(53);
  Wire.begin();
  //LCD 
  lcd.begin (16,2); 
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home (); 
  print_lcd_text("INIT 1/4");
  //BUTTON
  pinMode(8,INPUT);
  pinMode(9, INPUT);
  pinMode(10, OUTPUT);
  //BLUETOOTH SERIAL
  Serial3.begin(38400); 
  init_obd2();
}
String displayTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  String minut = String(minute,DEC);
  String sece = String(second,DEC);
  if(minute < 10){
    minut = "0"+String(minute,DEC);
  }
  if(second < 10){
    sece = "0"+String(second,DEC);
  }
  year_name = String(dayOfMonth,DEC)+"_"+String(month,DEC)+"_"+String(year,DEC)+".csv";
  return String(hour,DEC)+":"+minut+":"+ sece;
}

//INITIALIZE CONNECTION WITH OBD2 
void init_obd2(){
  send_command("ATZ\r");
  delay(1000);
  print_lcd_text("INIT 2/4");
  send_command("ATSPO\r");
  delay(1000);
  print_lcd_text("INIT 3/4");
  send_command("0100\r");
  delay(1000);
  print_lcd_text("INIT DONE");
}

//SEND COMMAND AND WAIT FOR RESPONSE > 
void send_command(String command){
  Serial3.print(command);
  while(Serial3.available() <= 0); 
  while (Serial3.available()>0){
     recvChar = Serial3.read();  
     if(recvChar == 62){
        delay(1000);
        break;        
     }
  }
}

//FUNCTON FOR READING PARAMETERS
void read_parameter(String parameter,int write_out){
   command = parameter + "1\r";
   inputString = "";       
   Serial3.print(command);
   while(Serial3.available()){ 
      recvChar = (char)Serial3.read();
      if(recvChar != ' '){
        inputString += recvChar;
      }
      if (recvChar == '>') {
        s = inputString;
        break;
      }
   }
   index = find_text("4"+parameter.substring(1,4),s);
   if(index != -1){
      index = index+4;
      a = 0;
      b = 0;
      sub = s.substring(index,index+4);
      temp = 0;
        for(int i = 4; i > 2;i--){
          if(sub[i]>'9'){
            sub[i] = sub[i] - 55;
          }
          else{
            sub[i] = sub[i] - 48;
          }
          b+=(sub[i]*pow(16,temp));
          temp++;
        }
      temp = 0;
  
      for(int i = 1; i >= 0;i--){
        if(sub[i]>'9'){
          sub[i] = sub[i] - 55;
        }
        else{
          sub[i] = sub[i] - 48;
        }
        a+=(sub[i]*pow(16,temp));
        temp++;
      }
      if(parameter == "010C"){
        value = (a*256+b)/4;
        if(write_out == 1){
          print_lcd(value, "rpm","Revs per minute");
        }
        else
          revs = value;
      }
      else if(parameter == "010D"){
        value = a;
        if(write_out==1){
          print_lcd(value, "Km\h","Speed");
        }
        speed_test = value;
      }
      else if(parameter == "0104"){
        value = (100/255)*a;
        print_lcd(value, "%","Engine load");
      }
      else if(parameter == "0105"){
        value = a-40;
        print_lcd(value, "C","Engine temp");
      }
      else if(parameter == "010B"){
        value = a;
        print_lcd(value, " kpa","intake press");
      } 
      else if(parameter == "010F"){
        value = a-40;
        print_lcd(value, "C","Intake temp");
      }
      else if(parameter == "0110"){
        value = (256*a+b)/100;
        if(write_out==1){
          print_lcd(value, " g/s", "MAF");
        }
        maf_test = value;
      }
      else if(parameter == "0111"){
        value = (100/255)*a;
        print_lcd(value, " %","Throttle");
      } 
      else if(parameter == "0121"){
        value = 256*a+b;
        print_lcd(value, " km","KM with MIL");
      }
     
      } inputString = ""; 
}

//PRINTING VALUE AND TEXT TO LCD 
void print_lcd(int value,String text,String name_param){
    lcd.setCursor(0,0);
    lcd.print("                ");
    lcd.setCursor(0,1);
    lcd.print("                ");
    lcd.setCursor(0,0);
    lcd.print(name_param);
    lcd.setCursor(0,1);
    lcd.print(value);
    lcd.print(text);
}

void print_lcd_double(double value,String text,String name_param){
    lcd.setCursor(0,0);
    lcd.print("                ");
    lcd.setCursor(0,1);
    lcd.print("                ");
    lcd.setCursor(0,0);
    lcd.print(name_param);
    lcd.setCursor(0,1);
    lcd.print (int(value));  //prints the int part
    lcd.print("."); // print the decimal point
    unsigned int frac;
    if(value >= 0)
       frac = (value - int(value)) * 100;
    else
       frac = (int(value)- value ) * 100;
   lcd.print(frac);
   lcd.print("  l/100km");
}

//PRINTING TEXT TO LCD
void print_lcd_text(String text){
    lcd.setCursor(0,0);
    lcd.print("                ");
    lcd.setCursor(0,0);
    lcd.print(text);
}

void calc_maf(int write_out){
  read_parameter("0110",0);
  delay(500);
  read_parameter("0110",0);
  delay(500);
  read_parameter("010D",0);
  delay(500);
  read_parameter("010D",0);
  delay(500);
  if(write_out == 1){
     print_lcd_double((maf_test/1000.0*3600.0/14.5*0.83*100.0/speed_test)," l/100km","Consumption");
  }
  poraba = double(maf_test/1000.0*3600.0/14.5*0.83*100.0/speed_test);
  poraba_save = 0;
  if (!isinf(poraba)) {
   sum_poraba += poraba;
   poraba_save = poraba;
   num_poraba = num_poraba + 1;
 }
}

//FUNCTION THAT RECEIVES TWO STRING AND RETURN INDEX OF SUBSTRING IN FIRST STRING
int find_text(String needle, String haystack) {
  int foundpos = -1;
  for (int i = 0; i <= haystack.length() - needle.length(); i++) {
    if (haystack.substring(i,needle.length()+i) == needle) {
      foundpos = i;
    }
  }
  return foundpos;
}
 
const long interval = 500; 
const long interval_param_revs = 5000;
const long intervalSave = 5000;

unsigned long previousMillis = 0;  
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;
unsigned long previousMillis4 = 0;

int buttonPushCounter = 0;   
int lastButtonState = 0;    
int lastButtonState2 = 0; 
int times = 0;
//LOOP FUNCTION
void loop() {
  //TABLE OF PARAMETERS
  char* param[]={"9999","8888","010C", "010D", "0104","0105","010B","010F","0110","0111","0121"};
  
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis2 >= interval_param_revs) {
    previousMillis2 = currentMillis;
    read_parameter("010C",0);
    delay(500);
    read_parameter("010C",0);
    delay(500);
    read_parameter("010D",0);
    delay(500);
    read_parameter("010D",0);
    delay(500);
    Serial.println(speed_test);
    Serial.println(revs);
    String times = displayTime();
    myFile = SD.open(year_name, FILE_WRITE);
    Serial.println(year_name);
    if (myFile) {
        myFile.println(times+ "," + String(speed_test) + "," + String(revs));
    }
    else{
      Serial.println("ERROR");
    }
    myFile.close();
  }
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    //Serial.println(buttonPushCounter%(param_size));
    if(buttonPushCounter%(param_size)==0){
      calc_maf(1);
    }
    else if(buttonPushCounter%(param_size)==1){
      avg_poraba = sum_poraba/num_poraba;
      print_lcd_double(avg_poraba," l/100km","AVG consumption");
    }
    else{
      read_parameter(param[buttonPushCounter%(param_size)],1);
    }
  } 
 
  //BUTTON NEXT
  int buttonState = digitalRead(9);
  if (buttonState != lastButtonState) {
    if (buttonState == HIGH) {
      buttonPushCounter++;
    }     
    delay(50); 
  }
  lastButtonState = buttonState;

  //BUTTON BACK
  int buttonState2 = digitalRead(8);
  if (buttonState2 != lastButtonState2) {
    if (buttonState2 == HIGH) {
      buttonPushCounter--;
    }     
    delay(50); 
  }
  lastButtonState2 = buttonState2;
  
  if(buttonPushCounter < 0){
    buttonPushCounter = param_size-1;
  }
}
