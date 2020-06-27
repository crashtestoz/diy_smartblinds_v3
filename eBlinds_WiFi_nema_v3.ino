/*******************
 Created 1 May. 2020
  by Peter Chodyra
  www.candco.com.au
 *******************/

#include <ArduinoJson.h>

#include <ESP_EEPROM.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

//#define DEBUG

//IMPORTANT Make sure the D pins are correctly connected
#define dirPIN D5 
#define stepPIN D6
#define sleepPIN D7
#define hotPIN D8

// ------------------------------------------------------------------------
// ############################# Global Definitions #########################
// ------------------------------------------------------------------------

int MAXSteps = 0; //Global variable fo rthe maximum steps to open blinds = 100% open

//const int moveSteps = 200; //Steps to advance the motor at setup
const int stepsPerRevolution = 200; //this has been changed to 200 for the Nema14 with A4988 driver board
int movesLeft = 0; //This holds the current remaining percent of move to make, this will be used in main loop to prevent blocking behaviour of teh stepper function 
int target = 0; //Target percentage as received in url
int targetPercentage = 0;

//int advanceAmount = 1; //for non blocking stepper set the advance amount each itteration
int turningDirection = 0;
int stepSpeed = 1000;


bool moveDone = false;

//Stepper setup
const int clockwise = 1;
const int counterclockwise = 0;

// ------------------------------------------------------------------------
// ############################# EEPROM Definitions #########################
// ------------------------------------------------------------------------
int addr = 0; //EEPROM Address memory space for the current_position of the blinds
struct EEPROMStruct {
  int MAXsteps;
  int CURsteps;
  int CURpercent;
  int STEPSpercent;
  int Orientation;
} eepromVar;

// ------------------------------------------------------------------------
// ############################# HTML Definitions #########################
// ------------------------------------------------------------------------

const String HTMLheader = "<!DOCTYPE html><html><head>"\
"<link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.3.1/css/all.css\" integrity=\"sha384-mzrmE5qonljUremFsqc01SB46JvROS7bZs3IO2EmfFsd15uHvIt+Y8vEf7N7fWAU\" crossorigin=\"anonymous\">"\
"</head>"\
"<style>"\
" .koobee_wrap{   "\
"    width: 100%;"\
"    margin: 0 auto;"\
"    background-color: grey;}"\
" .koobee {"\
"padding:10px; margin:5px;}"\
"  .blue_top{"\
"    background: #4096ee;"\ 
"background: -moz-linear-gradient(top, #4096ee 0%, #60abf8 44%, #7abcff 100%); "\
"background: -webkit-linear-gradient(top, #4096ee 0%,#60abf8 44%,#7abcff 100%); "\
"background: linear-gradient(to bottom, #4096ee 0%,#60abf8 44%,#7abcff 100%);}"\
"  .blue {"\
"    background: #7abcff; "\
"background: -moz-linear-gradient(top, #7abcff 0%, #60abf8 44%, #4096ee 100%); "\
"background: -webkit-linear-gradient(top, #7abcff 0%,#60abf8 44%,#4096ee 100%); "\
"background: linear-gradient(to bottom, #7abcff 0%,#60abf8 44%,#4096ee 100%); }"\
"  .green {"\
"    background: #f8ffe8; "\
"background: -moz-linear-gradient(top, #f8ffe8 0%, #e3f5ab 33%, #b7df2d 100%); "\
"background: -webkit-linear-gradient(top, #f8ffe8 0%,#e3f5ab 33%,#b7df2d 100%); "\
"background: linear-gradient(to bottom, #f8ffe8 0%,#e3f5ab 33%,#b7df2d 100%); }"\
"  .lavender {"\
"    background: #c3d9ff; "\
"background: -moz-linear-gradient(top, #c3d9ff 0%, #b1c8ef 41%, #98b0d9 100%);"\ 
"background: -webkit-linear-gradient(top, #c3d9ff 0%,#b1c8ef 41%,#98b0d9 100%); "\
"background: linear-gradient(to bottom, #c3d9ff 0%,#b1c8ef 41%,#98b0d9 100%); }"\
"  .orange{"\
"    background: #ffa84c; "\
"background: -moz-linear-gradient(top, #ffa84c 0%, #ff7b0d 100%); "\
"background: -webkit-linear-gradient(top, #ffa84c 0%,#ff7b0d 100%); "\
"background: linear-gradient(to bottom, #ffa84c 0%,#ff7b0d 100%); }"\
" .red{"\
"background: #ff776b;"\
"background: -moz-linear-gradient(top, #ff776b 0%, #f25252 100%); "\
"background: -webkit-linear-gradient(top, #ff776b 0%,#f25252 100%);"\
"background: linear-gradient(to bottom, #ff776b 0%,#f25252 100%);};"\
"  .koobee h1 {font-family: Gotham, \"Helvetica Neue\", Helvetica, Arial, \"sans-serif\";}"\
"  .koobee p {font-family: Gotham, \"Helvetica Neue\", Helvetica, Arial, \"sans-serif\";font-size: 28px;}"\
"  .btnrow1{text-align: center;}"\
"  .btns{"\
"    text-align: center;"\
"    width: 100%}"\
"  .menu {padding: 5px;}"\
"  .menu a {color: black;"\
"  text-decoration: none;}"\
".koobee button2 {"\
"     width: 105px;"\
"    display: inline-block;"\
"    border: none;"\
"    padding: 1rem 2rem;"\
"    margin: 0;"\
"    text-decoration: none;"\
"    background: #0069ed;"\
"    color: #ffffff;"\
"    font-family: sans-serif;"\
"    font-size: 1rem;"\
"    line-height: 1;"\
"    cursor: pointer;"\
"    text-align: center;"\
"    transition: background 250ms ease-in-out, transform 150ms ease;"\
"    -webkit-appearance: none;"\
"    -moz-appearance: none;"\
"}"\
".koobee button2:hover,button:focus {background: #0053ba;}"\
".koobee button2:focus {outline: 1px solid #fff;outline-offset: -4px;}"\
".koobee button2:active {transform: scale(0.99);}"\
".koobee button {"\
"     width: 115px;"\
"-moz-box-shadow: 0px 10px 14px -7px #3e7327;"\
" -webkit-box-shadow: 0px 10px 14px -7px #3e7327;"\
"  box-shadow: 0px 10px 14px -7px #3e7327;"\
"  background-color:#77b55a;"\
"  -moz-border-radius:4px;"\
"  -webkit-border-radius:4px;"\
"  border-radius:4px;"\
"  border:1px solid #4b8f29;"\
"  display:inline-block;"\
"  cursor:pointer;"\
"  color:#ffffff;"\
"  font-family:Arial;"\
"  font-size:2em;"\
"  font-weight:bold;"\
"  padding:6px 12px;"\
"  margin-bottom: 40px;"\
"  text-decoration:none;"\
"  text-shadow:0px 1px 0px #5b8a3c;}"\
".koobee button:hover { background-color:#72b352;}"\
".koobee button:active {position:relative;top:1px;}"\
"  </style>"\
"<body><div class=\"koobee_wrap\">";

const String HTMLfooter = "</div></body></html>";

const String HTMLheading =" <div class=\"koobee blue_top\">"\
"      <h1 style=\"text-align: center;\">DIY SmartBlinds vNema<br>Control Panel</h1>"\
"  </div>";

const String HTMLsetup = "<div class=\"koobee red\">"\
"  <h1 style=\"text-align: center;\">Setup</h1>"\
"  <p style=\"text-align:justify\">Use the MORE and LESS buttons to adjust the OPEN limit of the blinds. Click SAVE to save your settings. RESET will reset the blinds to a closed position. Clicking MORE or LESS will move the motor by 100 steps. To increase the number of steps and advance faster, change the query string in the browser URL to e.g.<u>\\api\\position?move=1000</u>.</p>"\
"    <table class=\"btns\">"\
"    <tr>"\
"      <td style=\"text-align: center;\"><a href=\"\\api\\position?move=100\"><button>More</button></a></td>"\
"      <td></td>"\
"      <td style=\"text-align: center;\"> <a href=\"\\api\\position?move=-100\"><button>Less</button></a></td>"\
"    </tr>"\
"    <tr>"\
"      <td colspan=\"3\" style=\"text-align: center;\">"\
"         <a href=\"\\api\\save\"><button>Save</button></a>"\
"      </td>"\
"    </tr>"\
"    <tr>"\
"      <td colspan=\"3\" style=\"text-align: center;\">"\
"         <a href=\"\\api\\reset\"><button>Reset</button></a>"\
"      </td>"\
"    </tr>"\
"    <tr>"\
"      <td style=\"text-align: center;\">"\
"         <a href=\"\\api\\orientation\"><button>Left Orientation</button></a>"\
"      </td>"\
"      <td></td>"\
"      <td style=\"text-align: center;\">"\
"         <a href=\"\\api\\orientation\"><button>Right Orientation</button></a>"\
"      </td>"\
"    </tr>"\
"      </table>"\
"    </div>";

const String HTMLhelp = "  <div class=\"koobee green\">"\
"      <h1 style=\"text-align: center;\">Information</h1>"\
"      <p style=\"text-align:justify\">To control the motor via WiFi and the builtin API, use the following GET and PUT API calls.</p>"\
"        <p>"\
"        <ul style=\"font-size: 1.5em\">"\
"          <li><b>/api/status</b> - GET : returns the motor position in % {\"position\":20}</li>"\
"          <li><b>/api/blinds?open=[x]</b> - PUT : moves the motor to position x%</li>"\
"        </ul>"\
"      </p>"\
"  </div>";

const String HTMLsavedone = "<div class=\"koobee green\" style=\"text-align: center;\"><p><h1>Setup Complete!</h1></p></div>";
const String HTMLresetdone = "<div class=\"koobee green\" style=\"text-align: center;\"><p><h1>Reset Complete!</h1></p></div>";

const String HTMLmove = "<div class=\"koobee blue\">"\
  "<h1 style=\"text-align: center;\">Operation</h1>"\
  "<p>Click on the % buttons to move the blinds to the desired open state. Ensure you have gone through the setup process first.</p>"\
    "<table class=\"btns\">"\
    "<tr>"\
      "<td><a href=\"\\api\\blinds?open=0\"><button>0%</button></a></td>"\
      "<td></td>"\
      "<td> <a href=\"\\api\\blinds?open=100\"><button>100%</button></a></td>"\
    "</tr>"\
    "<tr>"\
      "<td><a href=\"\\api\\blinds?open=10\"><button>10%</button></a></td>"\
      "<td><a href=\"\\api\\blinds?open=20\"><button>20%</button></a></td>"\
      "<td><a href=\"\\api\\blinds?open=30\"><button>30%</button></a></td>"\
    "</tr>"\
    "<tr>"\
      "<td><a href=\"\\api\\blinds?open=40\"><button>40%</button></a></td>"\
      "<td><a href=\"\\api\\blinds?open=50\"><button>50%</button></a></td>"\
      "<td><a href=\"\\api\\blinds?open=60\"><button>60%</button></a></td>"\
    "</tr>"\
    "<tr>"\
      "<td><a href=\"\\api\\blinds?open=70\"><button>70%</button></a></td>"\
      "<td><a href=\"\\api\\blinds?open=80\"><button>80%</button></a></td>"\
      "<td><a href=\"\\api\\blinds?open=90\"><button>90%</button></a></td>"\
    "</tr>"\
    "</table>"\
  "</div>";

const String HTMLmenu = "<div class=\"koobee orange\">"\
 "<div class=\"menu\">"\
  "<table style=\"width: 100%;\"><tr style=\"text-align: center\">"\
    "<td id=\"home\"><a href=\"\\\"><i class=\"fas fa-home fa-4x\"></i></a></td>"\
    "<td id=\"operate\"><a href=\"\\api\\blinds\"><i class=\"fas fa-arrows-alt-h fa-4x\"></i></a></td>"
    "<td id=\"setup\"><a href=\"\\api\\setup\"><i class=\"fas fa-cog fa-4x\"></i></a></td>"\
    "<td id=\"info\"><a href=\"\\api\\help\"><i class=\"fas fa-info-circle fa-4x\"></i></a></td>"\
  "</tr></table></div></div>";

String HTMLstatus ="";



// initialize the stepper library on pins 1 through 4:        
//Stepper myStepper(stepsPerRevolution, PIN1,PIN2,PIN3,PIN4);   //note the modified sequence D1, D5, D2, D6 

//Web server setup
ESP8266WebServer server(80);   //Web server object. Will be listening in port 80 (default for HTTP)

// ------------------------------------------------------------------------
// ############################# getHTMLstatus() #############################
// ------------------------------------------------------------------------
void getHTMLstatus(){
  //eepromVar.LUXstate = getLux(photocellPin);
  HTMLstatus ="<div class=\"koobee lavender\"><p style=\"text-align:center;\">"\
  "Open LIMIT is set to: "+String(eepromVar.MAXsteps)+"<br>"\
  "Current position is: "+String(eepromVar.CURsteps)+"<br>"\
  "Open position is: "+String(eepromVar.CURpercent)+"%<br>"\
  "Orentation is: "+String(eepromVar.Orientation)+"<br>";
  //"Light level is: "+String(eepromVar.LUXstate)+"</p></div>";
}
// ------------------------------------------------------------------------
// ############################# handleRoot() #############################
// ------------------------------------------------------------------------
void handleRoot() {
  server.send(200, "text/plain", HTMLheader+HTMLhelp+HTMLsetup+HTMLmove+HTMLfooter);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

// ------------------------------------------------------------------------
// ############################# getBlindsPosition() #############################
// ------------------------------------------------------------------------

int getBlindsPosition(){
  return eepromVar.CURpercent;
}

// ------------------------------------------------------------------------
// ############################# jsonOutput() #############################
// ------------------------------------------------------------------------

String jsonOutput(String jName,int jValue){  
  String output;
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root[jName] = jValue;
  root.printTo(output);
  return output;
}

// ------------------------------------------------------------------------
// ############################# saveBlindsPosition() #############################
// ------------------------------------------------------------------------
void saveBlindsPosition(){  

    eepromVar.CURpercent=100; //At the completion of the setup the current position is at 100% open
    eepromVar.CURsteps = eepromVar.MAXsteps;
    eepromVar.STEPSpercent=eepromVar.MAXsteps/100;
    EEPROM.put(addr, eepromVar);
    // write the data to EEPROM
    boolean ok2 = EEPROM.commit();
#ifdef DEBUG     
    Serial.println((ok2) ? "Commit OK" : "Commit failed");
#endif     


}

// ------------------------------------------------------------------------
// ############################# saveBlindsOrientation() #############################
// ------------------------------------------------------------------------
void saveBlindsOrientation(int orient){  

    //Reset the orientation of the blinds to -1, i.e. chain pulley facing away from the wall
    //When chain pulley is facing the wall then Orientation = 1 
    eepromVar.Orientation=orient;
    EEPROM.put(addr, eepromVar);
    // write the data to EEPROM
    boolean ok2 = EEPROM.commit();
#ifdef DEBUG     
    Serial.println((ok2) ? "Commit OK" : "Commit failed");
#endif     


}

// ------------------------------------------------------------------------
// ############################# resetBlindsPosition() #############################
// ------------------------------------------------------------------------
void resetBlindsPosition(){  

    //Reset the position of the blinds to 0  
    eepromVar.MAXsteps=0;
    eepromVar.CURsteps=0;
    eepromVar.CURpercent=0;
    eepromVar.STEPSpercent=0;
    eepromVar.Orientation=-1;
    EEPROM.put(addr, eepromVar);
    // write the data to EEPROM
    boolean ok2 = EEPROM.commit();
#ifdef DEBUG     
    Serial.println((ok2) ? "Commit OK" : "Commit failed");
#endif     


}

// ------------------------------------------------------------------------
// ############################# handleOpenArgs() #############################
// ------------------------------------------------------------------------
void handleOpenArgs(){
  //Handle normal operation e.g Open to 20% or Open to 100%
  
  String message = "";

  if (server.arg("open")== ""){     //Parameter not found
    message = "Open Argument not found";
  }else{     //Parameter found
    message = "Manual input trigered Open Argument = ";
    message += server.arg("open");     //Gets the value of the query parameter
    
    //Move the blinds
    openBlinds(atoi(server.arg("open").c_str()));
    
    target=atoi(server.arg("open").c_str()); //target % to open to
    targetPercentage = target;
    movesLeft=abs(eepromVar.CURpercent-targetPercentage); //change the global variale to percent to move the blinds
    
    //if (movesLeft == 0) {movesLeft = -1;} // make sure the blinds don't move
    
    if (targetPercentage-eepromVar.CURpercent > 0) {turningDirection = clockwise;}
    if (targetPercentage-eepromVar.CURpercent < 0) {turningDirection = counterclockwise;}
    if (targetPercentage-eepromVar.CURpercent == 0) {turningDirection = -1;}
    
    //reset the completed move flag
    moveDone = false;
    
#ifdef DEBUG     
    Serial.print("handleOpenArgs(): Turning Direction : ");
    Serial.println(turningDirection);
#endif  
  }

 #ifdef DEBUG  
  //server.send(200, "text/plain", message);          //Returns the HTTP response
 #endif   

}
// ------------------------------------------------------------------------
// ############################# handleMoveArgs() #############################
// ------------------------------------------------------------------------
void handleMoveArgs(){
  //Handle setup opperation e.g. increse steps by 100 or decrease steaps by -100 to adjust the MAX opn position
  String message = "";
  int moveSteps = 0; //Ths value can either be positive -> clockwise rotation or negative -> counetrclockwise rotation
  
  if (server.arg("move")== ""){     //Parameter not found
    message = "Move Argument not found";
  }else{     //Parameter found
    message = "Manual input trigered Open Argument = ";
    message += server.arg("move");     //Gets the value of the query parameter
    //Move the blinds
    moveSteps = atoi(server.arg("move").c_str());
 #ifdef DEBUG  
  Serial.print("Adjusting steps to: ");
  Serial.println(moveSteps);
 #endif      
    //Determine which driection to spin the motor
    if (moveSteps > 0){turningDirection = clockwise;}
    if (moveSteps < 0){turningDirection = counterclockwise;}
    if (moveSteps == 0){turningDirection = -1;}

    //Check and make sure we are not ging past 0
    if (eepromVar.CURsteps + moveSteps >= 0) {  
      
      //Move the motor
      moveMotor(abs(moveSteps), turningDirection);
      moveDone=true;
      
      eepromVar.MAXsteps = eepromVar.MAXsteps + moveSteps;
      eepromVar.CURsteps = eepromVar.CURsteps + moveSteps;
      eepromVar.CURpercent=100; //During setup always assume the current position is at 100% open
      
      //EEPROM.put(addr, eepromVar);
      // write the data to EEPROM
      //boolean ok2 = EEPROM.commit();
#ifdef DEBUG     
    //Serial.println((ok2) ? "Commit OK" : "Commit failed");
#endif
    } else {
      //can not go past 0 steps, not moving motor  
    }
  }
}

// ------------------------------------------------------------------------
// ############################# openBlinds() #############################
// ------------------------------------------------------------------------
void openBlinds(int targetPercent){

  //map percentage to number of steps where 4000 is fully open or 100%
  int newSteps = map(targetPercent, 0,100,0,eepromVar.MAXsteps);

  if (targetPercent-eepromVar.CURpercent > 0) {turningDirection = clockwise;}
  if (targetPercent-eepromVar.CURpercent < 0) {turningDirection = counterclockwise;}
  if (targetPercent-eepromVar.CURpercent == 0) {turningDirection = -1;} //do need to turn the motor

#ifdef DEBUG 
    Serial.print("Moving to Percent : ");
    Serial.print(targetPercent);   
    Serial.print(" Moving number of Steps : ");
    Serial.print(abs(newSteps-eepromVar.CURsteps));
    Serial.print(" Current Steps number is: ");
    Serial.print(eepromVar.CURsteps);
    Serial.print(" New Number of Steps will be: ");
    Serial.print(newSteps);
    Serial.print(" MAXSteps is: ");
    Serial.println(eepromVar.MAXsteps);
#endif  

  //Only move the motor if there is a need
  if (turningDirection != -1) {
    
    //MOVE COMMAND
    moveMotor(abs(newSteps - eepromVar.CURsteps), turningDirection);
    moveDone=true;
    
    eepromVar.CURsteps = newSteps;
    eepromVar.CURpercent = targetPercent;
    
    EEPROM.put(addr, eepromVar);
    // write the data to EEPROM
    boolean ok2 = EEPROM.commit();
#ifdef DEBUG     
    Serial.println((ok2) ? "Commit OK" : "Commit failed");
#endif     
  }
  else {
    //do nothing newSteps = currentSteps
  }

#ifdef DEBUG 
  Serial.print("Blinds are at ");
  Serial.print(targetPercent);   
  Serial.println("%");
#endif 

}

// ------------------------------------------------------------------------
// ############################# advanceBlinds() #############################
// ------------------------------------------------------------------------

void advanceBlinds() {
  //Setup Function used for settign up the MAX OPEN position and for moving the blinds x number of steps
  
  //int advanceSteps = eepromVar.STEPSpercent; //map(advanceAmount,0,100,0,eepromVar.MAXsteps);
  int targetSteps = target*eepromVar.STEPSpercent; //map(target,0,100,0,eepromVar.MAXsteps);
  //int targetSteps = map(target,0,100,0,eepromVar.MAXsteps); //Calculate hoe many steps to advance based on target %
 
  
#ifdef DEBUG 
    Serial.print("advanceBlinds() : ");
    Serial.print("Target %: ");
    Serial.print(target);   
    Serial.print(" Target Steps : ");
    Serial.print(targetSteps);
    Serial.print(" STEPSpercent is: ");
    Serial.print(eepromVar.STEPSpercent);
    Serial.print(" CURsteps is: ");
    Serial.print(eepromVar.CURsteps);
    Serial.print(" CURpercent % is: ");
    Serial.print(eepromVar.CURpercent);
    Serial.print(" movesLeft is: ");
    Serial.println(movesLeft);
#endif 
  if (turningDirection = 0){  //determine the direction of the turn - clockwise
    moveMotor(1,turningDirection);
    eepromVar.CURpercent=eepromVar.CURpercent+1;
    eepromVar.CURsteps=eepromVar.CURsteps+eepromVar.STEPSpercent;
  }
  if (turningDirection = 1) { //determine the direction of the turn - counterclockwise
    //move the motor counterclockwise
    moveMotor(1,turningDirection);  
    eepromVar.CURpercent=eepromVar.CURpercent-1;
    eepromVar.CURsteps=eepromVar.CURsteps-eepromVar.STEPSpercent;
  } 
  if (turningDirection == -1) {
    //is -1 and no need to move the blinds
  }
  movesLeft = movesLeft-1;
  if (movesLeft==0) {
      moveDone=true;
  }
  EEPROM.put(addr, eepromVar);
  // write the data to EEPROM
  boolean ok2 = EEPROM.commit();
#ifdef DEBUG     
    Serial.println((ok2) ? "Commit OK" : "Commit failed");
#endif
}

// ------------------------------------------------------------------------
// ############################# moveMotor() #############################
// ------------------------------------------------------------------------

void moveMotor(int moveSteps, int dir){

  digitalWrite(sleepPIN,HIGH); //Wake teh A4988 from sleep
  
  if (dir == clockwise){
    digitalWrite(dirPIN,HIGH); //Changes the rotations direction to CW
  } else if (dir == counterclockwise){
    digitalWrite(dirPIN,LOW); //Changes the rotations direction to CCW
  }
  
#ifdef DEBUG 
    Serial.print("Direction: ");
    Serial.print(dir);
    Serial.print(" Steps: ");
    Serial.println(moveSteps);
#endif

  //Move the motor number of steps, for normal opperation steps = 1 during setup steps = 100
  for(int x = 0; x < moveSteps; x++) {
    digitalWrite(stepPIN, HIGH); 
    delayMicroseconds(5000); 
    digitalWrite(stepPIN, LOW); 
    delayMicroseconds(5000);
    yield(); //important to allow the ESP8266 watchdog inturupt 
  }
  digitalWrite(sleepPIN,LOW); //put the A4988 to sleep
}

// ------------------------------------------------------------------------
// ############################# moveOneStep() #############################
// ------------------------------------------------------------------------

void moveOneStep() {
    digitalWrite(stepPIN, HIGH);
    delayMicroseconds(5000);
    digitalWrite(stepPIN, LOW);
    delayMicroseconds(5000);
    yield(); //important to allow the ESP8266 watchdog inturupt
}

// ------------------------------------------------------------------------
// ############################# setup() #############################
// ------------------------------------------------------------------------

void setup() {
  
  pinMode(stepPIN,OUTPUT); 
  pinMode(dirPIN,OUTPUT);
  pinMode(sleepPIN,OUTPUT);
  
  //digitalWrite(hotPIN, LOW);
  digitalWrite(sleepPIN, LOW); //Put the motor to sleep when not in opperation
  
  // initialize the serial port:
  Serial.begin(115200);
  Serial.println();

  //Setup WiFiManager
  WiFiManager wfManager;
  //Switchoff debug mode
  wfManager.setDebugOutput(false);

  //exit after config instead of connecting
  wfManager.setBreakAfterConfig(true);
  
  //tries to connect to last known settings
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP" with password "password"
  //and goes into a blocking loop awaiting configuration
  
  if (!wfManager.autoConnect("DIY-SmartBlinds")) {
    Serial.println("failed to connect, resetting and attempting to reconnect");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected... ");

  //server.reset(new ESP8266WebServer(WiFi.localIP(), 80));
  //String HTMLstatus ="<p style=\"text-align:center;\">LIMIT is set to: "+String(eepromVar.MAXsteps)+"<br>"+"Current position is: "+String(eepromVar.CURsteps)+"</p>";
  
  server.on("/", [](){
    //String HTMLstatus ="<p style=\"text-align:center;\">LIMIT is set to: "+String(eepromVar.MAXsteps)+"<br>"+"Current position is: "+String(eepromVar.CURsteps)+"</p>";
    getHTMLstatus();
    server.send(200, "text/html", HTMLheader+HTMLheading+HTMLstatus+HTMLmenu+HTMLfooter);
  });

  server.on("/api/help", []() {
    //String HTMLstatus ="<p style=\"text-align:center;\">LIMIT is set to: "+String(eepromVar.MAXsteps)+"<br>"+"Current position is: "+String(eepromVar.CURsteps)+"</p>";
    getHTMLstatus();
    server.send(200, "text/html", HTMLheader+HTMLheading+HTMLhelp+HTMLstatus+HTMLmenu+HTMLfooter);
  });

  //hande querystring /blinds?open=20
  server.on("/api/blinds",[](){
    handleOpenArgs();
    //String HTMLstatus ="<p style=\"text-align:center;\">LIMIT is set to: "+String(eepromVar.MAXsteps)+"<br>"+"Current position is: "+String(eepromVar.CURsteps)+"</p>";
    getHTMLstatus();
    server.send(200, "text/html", HTMLheader+HTMLheading+HTMLmove+HTMLstatus+HTMLmenu+HTMLfooter);
  });
  
  server.on("/api/setup", [](){
    //String HTMLstatus ="<p style=\"text-align:center;\">LIMIT is set to: "+String(eepromVar.MAXsteps)+"<br>"+"Current position is: "+String(eepromVar.CURsteps)+"</p>";
    getHTMLstatus();
    server.send(200, "text/html", HTMLheader+HTMLheading+HTMLsetup+HTMLstatus+HTMLmenu+HTMLfooter);
  });
  
  server.on("/api/position", [](){
    //When ?move=100 is passed move the blinds 100 steps
    handleMoveArgs();
    //String HTMLstatus ="<p style=\"text-align:center;\">LIMIT is set to: "+String(eepromVar.MAXsteps)+"<br>"+"Current position is: "+String(eepromVar.CURsteps)+"</p>";
    getHTMLstatus();
    server.send(200, "text/html", HTMLheader+HTMLheading+HTMLsetup+HTMLstatus+HTMLmenu+HTMLfooter);
  });

  server.on("/api/save", [](){
    saveBlindsPosition();
    //String HTMLstatus ="<p style=\"text-align:center;\">LIMIT is set to: "+String(eepromVar.MAXsteps)+"<br>"+"Current position is: "+String(eepromVar.CURsteps)+"</p>";
    getHTMLstatus();
    server.send(200, "text/html", HTMLheader+HTMLheading+HTMLsavedone+HTMLstatus+HTMLmenu+HTMLfooter);
  });
  
    server.on("/api/reset", [](){
    resetBlindsPosition();
    //String HTMLstatus ="<p style=\"text-align:center;\">LIMIT is set to: "+String(eepromVar.MAXsteps)+"<br>"+"Current position is: "+String(eepromVar.CURsteps)+"</p>";
    getHTMLstatus();
    server.send(200, "text/html", HTMLheader+HTMLheading+HTMLresetdone+HTMLstatus+HTMLmenu+HTMLfooter);
  });
  
    server.on("/api/orientation", [](){
    //resetBlindsPosition();
    //String HTMLstatus ="<p style=\"text-align:center;\">LIMIT is set to: "+String(eepromVar.MAXsteps)+"<br>"+"Current position is: "+String(eepromVar.CURsteps)+"Current orientation is: "+String(eepromVar.Orientation)+"</p>";
    getHTMLstatus();
    server.send(200, "text/html", HTMLheader+HTMLheading+HTMLresetdone+HTMLstatus+HTMLmenu+HTMLfooter);
  });
  server.on("/api/status", [](){
    server.send(200, "application/json", jsonOutput("position",getBlindsPosition()));
  });
  
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("local ip: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  eepromVar.MAXsteps = -1;
  eepromVar.CURsteps = -1;
  eepromVar.CURpercent = -1;
  eepromVar.STEPSpercent = -1;
  eepromVar.Orientation = -1;
  
  EEPROM.begin(sizeof(EEPROMStruct));

  EEPROM.get(addr, eepromVar); //read the eeprom structure

  if (eepromVar.MAXsteps < 0 || eepromVar.CURsteps < 0 || eepromVar.CURpercent < 0 || eepromVar.STEPSpercent < 0) {
    //eeprom has been corrupted or not set
    eepromVar.MAXsteps = 0;
    eepromVar.CURsteps = 0;
    eepromVar.CURpercent = 0;
    //moveBlinds(0); //rest blind position to closed on startup
    EEPROM.put(addr, eepromVar);
   }

#ifdef DEBUG    
    Serial.print(" currentSteps is: ");
    Serial.print(eepromVar.CURsteps);
    Serial.print(" MAXSteps is: ");
    Serial.println(eepromVar.MAXsteps);
#endif  

}

// ------------------------------------------------------------------------
// ############################# loop() #############################
// ------------------------------------------------------------------------
void loop() {

  //if(movesLeft != 0) {
    //move the steper motor
  //   advanceBlinds();
  //} else {
  //    if (moveDone){
        //Make sure motor is Powered off and wait for a server responce
  //      digitalWrite(sleepPIN,LOW); //put the A4988 to sleep
  //      moveDone = false;
  //  }
 // }
  
  //Handle server requests
  server.handleClient();
    
}
