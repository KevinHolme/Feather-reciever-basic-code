/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * This is the code I am using for a one transmitter with multiple recievers controller based on adafruit feathers. Specifically Adafruit Feather M0 RFM69HCW Packet Radio 
 * It is a work in progress based on John Parks Remote effects Trigger Box here https://learn.adafruit.com/remote-effects-trigger/programing-the-remote
 * This is an example of a basic radio reciever setup.  I use this to control 3 IA lifters with the input pins. 
 * I have cut out a lot of content, it is easier to get what I am doing this way.  Basically the radios are listening all of the time, and the transmitter is sending all of the time. in the loop the reciever checks tyhe radio 
 * every 250 miliseconds compares it to its last stored command and if it is the same, does nothing. If the incoming message is a new one, it is changed into something called "Current order" and then the history is updated  and then waits .25 seconds and checke\s again. 
 * I am using this to do 2 types of commands, one thing at a time and changes each time a new thing is sent, this is done with a loop called "Apply Incoming".  This is a Switch case list and if you notice, it has no default command. so as it runs down its list, as it does every time the radio  
 * command changes, it will do nothing unless it is one of the few numbers on its list.  So in this example, If I put the perscope up, it will stay up until I give a command to either put it to random or down. 
 * Now the last two items in that list are really the heart of the way I control my droid. Shows. I left 2 as examples  so if the comand from the radio is 71, the show timer is triggered, and the number is transfered to the "show " command The Show is almost always 
 * a group of commands set to music.  All of the feathers recieve the show trigger withind a quarter of a second and set the show timer to 0. So now we can have them all work together.  This one simply puts up a speaker I have on a lifter mounted in the center of the dome, 
 * But the possibilities are endless. Again, the show once triggered will continue untill a new show is called. so I always have a show programmed in the transmitter that stops everything. But it is no longer here. 
*/
#include <SPI.h>
#include <RH_RF69.h>
#include <Wire.h>
#include <RHReliableDatagram.h>


 #define LED      13

//----------------------------------------Time--------------
unsigned long CurrentMillis = 0;
unsigned long PreviousDisplayMillis = 0;
unsigned long PreviousRadioMillis = 0;
unsigned long PreviousLoopMillis = 0;
unsigned long ShowTime = 0;
const long DisplayInterval = 1000;
const long RadioInterval = 250; 
const long LoopInterval = 100;

/****************************************************** Radio Setup ***************/


  #define RF69_FREQ 919.0
  #define RFM69_CS      8
  #define RFM69_INT     3
  #define RFM69_RST     4
  

#define MY_ADDRESS  8  // each reciever is assigned a different number

//  The data structure must be the same as the one defined in the transmitter. 
struct dataStruct{
byte n1;
byte n2;
byte n3;
}RadioPacket;

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);




//-----------------------------Constant (Never Change)---------------------------
//  these are the pin assignments for this feather,  Each lifter has a pin to trigger up, random and down  with this feather doing nothing but these lifters, I could use 9 pins to trigger them and not have to figure out the I2c address issue
//so I took the easy way.  The last 4 is a 4 spot relay board. I cut power to the lifters when not in use 
  #define PeriDown      5
  #define PeriMid       6
  #define PeriUp        9
  #define SpkrDown      10
  #define SpkrMid       11
  #define SpkrUp        12
  #define SaberDown      16
  #define SaberMid       17
  #define SaberUp        18
  #define PeriPow       19
  #define SpkrPow       14
  #define SaberPow       15
  #define Relay4        1


//============================== variable (will Change)==============
int CurrentOrder = 0;
int IncomingHist = 0; 
int Show = 0;
String Recieved;
String Incoming1;
String Incoming2;
String Incoming3;
//String Incoming4;
//String Incoming5;
String IncomingMsg;
int IncomingInt;
//String SendPack;
//String Pack1;
//String Pack2;
//String Pack3;
//String Pack4;
//String Pack5;
int RecInt;
int GroupInt;
int SendPackInt;



//-------------------------------------------------Setup-------------
void setup() {
   
  delay(500);
  Serial.begin(115200);
   
// set all the pins as output
pinMode( PeriPow, OUTPUT);
pinMode( SpkrPow, OUTPUT );
pinMode( SaberPow, OUTPUT );
pinMode( Relay4, OUTPUT );
pinMode( PeriDown, OUTPUT );
pinMode( PeriMid, OUTPUT );
pinMode( PeriUp, OUTPUT );
pinMode( SpkrDown, OUTPUT );
pinMode( SpkrMid, OUTPUT );
pinMode( SpkrUp, OUTPUT );
pinMode( SaberDown, OUTPUT );
pinMode( SaberMid, OUTPUT );
pinMode( SaberUp, OUTPUT );
// the lifters as well as the relay board is ground the pins to trigger, so all of the pins need to be set high
digitalWrite(PeriPow,HIGH);
digitalWrite(SpkrPow,HIGH);
digitalWrite(SaberPow,HIGH);
digitalWrite(Relay4,HIGH);
digitalWrite(PeriDown,HIGH);
digitalWrite(PeriMid,HIGH);
digitalWrite(PeriUp,HIGH);
digitalWrite(SpkrDown,HIGH);
digitalWrite(SpkrMid,HIGH);
digitalWrite(SpkrUp,HIGH);
digitalWrite(SaberDown,HIGH);
digitalWrite(SaberMid,HIGH);
digitalWrite(SaberUp,HIGH);


// ------------------------------------------Radio-----------------
     
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather RFM69 RX/TX Test!");

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  
  if (!rf69.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  
 
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
  
//  pinMode(LED, OUTPUT);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");


RadioPacket.n1 = 0;
RadioPacket.n2 = 0;
RadioPacket.n3 = 0;

  delay(500);
 

}

//=====================================================Rocket Man===========================
// this is a show that is set to Elton Johns Rocket man,  the song plays, and the booster rockets deplay whenever he sings rocket man.  this unit however just has one job. 
void RocketMan(){
if (millis() - ShowTime <= 85000) {// when the show was triggered, the show timer was reset to 0 . This line is saying, If the time is less than 85000 milliseconds, Put up the speaker lifter.  also notice I repeat that command, With shows 
 Serial.print(" Speaker Up ");
      digitalWrite(SpkrDown,HIGH);
      digitalWrite(SpkrMid,HIGH);
      digitalWrite(SpkrPow, LOW);
      delay (100);
      digitalWrite(SpkrUp, LOW);
      delay (100);
      digitalWrite(SpkrUp, LOW);
  
}     
if (millis() - ShowTime >= 85000) {  // once the time gets past 85000 milliseconds  it is time to put down the speaker.

      
       digitalWrite(SpkrMid,HIGH);
       digitalWrite(SpkrUp,HIGH);
       digitalWrite(SpkrPow, LOW);
       digitalWrite(SpkrDown,LOW);
       delay (100);
       digitalWrite(SpkrDown, LOW);
}      
if (millis() - ShowTime >= 90000) {  // after another 5 seconds, power it all down
      digitalWrite(SpkrDown, HIGH); 
      digitalWrite(PeriDown, HIGH); 
      digitalWrite(PeriPow, HIGH);
      digitalWrite(SpkrPow, HIGH);
}
if (millis() - ShowTime >= 95000) { // and after another 5 seconds  change the show to 0
Show = 0;
}  
      
}
//=====================================================Fav Things===========================
void FavThings(){
if (millis() - ShowTime <= 35000) {
 Serial.print(" Speaker Up ");
      digitalWrite(SpkrDown,HIGH);
      digitalWrite(SpkrMid,HIGH);
      digitalWrite(SpkrUp,HIGH);
      digitalWrite(SpkrPow, LOW);
      delay (100);
      digitalWrite(SpkrMid, LOW);
      delay (100);
      digitalWrite(SpkrMid, LOW);
  
}     
if (millis() - ShowTime >= 35000) {
 
       
       digitalWrite(SpkrMid,HIGH);
       digitalWrite(SpkrUp,HIGH);
       digitalWrite(SpkrPow, LOW);
       digitalWrite(SpkrDown,LOW);
       delay (100);
       digitalWrite(SpkrDown, LOW);
}
if (millis() - ShowTime >= 40000) {
      digitalWrite(SpkrDown, HIGH); 
      digitalWrite(PeriDown, HIGH); 
      digitalWrite(PeriPow, HIGH);
      digitalWrite(SpkrPow, HIGH);
}
if (millis() - ShowTime >= 41000) {
Show = 0;
}  
      
}
//===================================================Run Shows===========================
// This extra step with the default of 0. assures that the show will not play over and over.
void RunShows(){
//Serial.print (" Run Shows" );
switch (Show){
 case 71:
 RocketMan();
 break;
  case 75:
 FavThings();
 break;
   default:
  case 0:// No Show Now
          Show = 0; 
}
}

//=================================================================Apply Incoming============
void ApplyIncoming(){
CurrentOrder = IncomingInt;
  
switch(CurrentOrder){

 case 61:
   Serial.print(" Periscope Up ");
      digitalWrite(PeriDown,HIGH);
      digitalWrite(PeriMid,HIGH);
      digitalWrite(PeriPow, LOW);
      digitalWrite(PeriUp, LOW);
   break;
  case 65:
       Serial.print(" Periscope Down ");
       digitalWrite(PeriMid,HIGH);
       digitalWrite(PeriUp,HIGH);
       digitalWrite(PeriPow, LOW);
       digitalWrite(PeriDown, LOW);
      break;
  case 62:
  Serial.print(" Periscope Random ");
      digitalWrite(PeriDown,HIGH);
      digitalWrite(PeriUp,HIGH);
      digitalWrite(PeriPow, LOW);
      digitalWrite(PeriMid, LOW);
  break;
 case 63:
  Serial.print(" Speaker Up ");
      digitalWrite(SpkrDown,HIGH);
      digitalWrite(SpkrMid,HIGH);
      digitalWrite(SpkrPow, LOW);
      digitalWrite(SpkrUp, LOW);
  break;
 case 67:
 Serial.print(" Speaker Down ");
       digitalWrite(SpkrMid,HIGH);
       digitalWrite(SpkrUp,HIGH);
       digitalWrite(SpkrPow, LOW);
      digitalWrite(SpkrDown, LOW);
     break;
 case 64:
   Serial.print(" Saber Up ");
      digitalWrite(SaberDown,HIGH);
      digitalWrite(SaberMid,HIGH);
      digitalWrite(SaberPow, LOW);
     digitalWrite(SaberUp, LOW);
     break;
 case 68:
Serial.print(" Saber Down ");
       digitalWrite(SaberMid,HIGH);
       digitalWrite(SaberUp,HIGH);
       digitalWrite(SaberPow, LOW);
       digitalWrite(SaberDown, LOW);
  break;

    case 71:
 ShowTime = millis();
 Show = 71;
  break;
     case 75:
 ShowTime = millis();
 Show = 75;
  break; 
}

}


//-------------------------------------------------Radio-------------------------------
void radio (){
  
  uint8_t buf[sizeof(RadioPacket)];
  uint8_t from;
  uint8_t len = sizeof(buf);


 if (rf69_manager.available()){
      
    // Wait for a message addressed to us from the client
    if (rf69_manager.recvfrom(buf, &len, &from))
    {
    memcpy(&RadioPacket, buf, sizeof(RadioPacket));
      if (!len) return;
      buf[len] = 0;
Incoming1 = String(RadioPacket.n1);
Incoming2 = String(RadioPacket.n2);
Incoming3 = String(RadioPacket.n3);


IncomingMsg = Incoming1 + Incoming2 + Incoming3;
   
IncomingInt = IncomingMsg.toInt();
      
    }
  }

}

  

//---------------------------------------------------Debug Radio
void DebugRadio(){
     Serial.print("RSSI: ");
      Serial.println(rf69.lastRssi(), DEC);

      
      
      Serial.print("Got message from unit: ");
         
      Serial.print("Switch Code");
       Serial.print(RadioPacket.n1);
        Serial.print(RadioPacket.n2);
         Serial.print(RadioPacket.n3);
         
                  Serial.print("Incoming Message   ");
      Serial.print(IncomingMsg);  
      Serial.print("Incoming Int   ");
      Serial.println(IncomingInt);
       
}


//--------------------------------------------Loop--------------------
void loop(){
 
  CurrentMillis = millis();   // capture the latest value of millis()
  
  if(IncomingInt != IncomingHist){
   CurrentOrder = IncomingInt;
    ApplyIncoming();
    IncomingHist = IncomingInt;
   }
  
  if (millis() - PreviousDisplayMillis >= DisplayInterval){
    PreviousDisplayMillis = CurrentMillis;
   DebugRadio();
    }
  
     if (millis() - PreviousRadioMillis >= RadioInterval){
    PreviousRadioMillis = CurrentMillis;
  radio();
   
    }


   if (millis() - PreviousLoopMillis >= LoopInterval){
    PreviousLoopMillis = CurrentMillis;
    RunShows();
      }

}
