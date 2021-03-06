/*****************
 * 
 * - kasper
 */


#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EEPROM.h>      // For storing IP numbers

// Include ATEM library and make an instance:
#include <ATEM.h>
ATEM AtemSwitcher;

//#include <MemoryFree.h>

// Configure the IP addresses and MAC address with the sketch "ConfigEthernetAddresses":
uint8_t ip[4];        // Will hold the Arduino IP address
uint8_t atem_ip[4];  // Will hold the ATEM IP address
uint8_t mac[6];    // Will hold the Arduino Ethernet shield/board MAC address (loaded from EEPROM memory, set with ConfigEthernetAddresses example sketch)



// No-cost stream operator as described at 
// http://arduiniana.org/libraries/streaming/
template<class T>
inline Print &operator <<(Print &obj, T arg)
{  
  obj.print(arg); 
  return obj; 
}




// Include the SkaarhojUtils library. 
#include "SkaarhojUtils.h"
SkaarhojUtils utils;
int touchCal[8];
uint8_t checksumByte;

void setup() { 

  // Start the Ethernet, Serial (debugging) and UDP:
  Serial.begin(9600);
  Serial << F("\n- - - - - - - -\nSerial Started\n");  


  // Setting the Arduino IP address:
  ip[0] = EEPROM.read(0+2);
  ip[1] = EEPROM.read(1+2);
  ip[2] = EEPROM.read(2+2);
  ip[3] = EEPROM.read(3+2);

  // Setting the ATEM IP address:
  atem_ip[0] = EEPROM.read(0+2+4);
  atem_ip[1] = EEPROM.read(1+2+4);
  atem_ip[2] = EEPROM.read(2+2+4);
  atem_ip[3] = EEPROM.read(3+2+4);
  
  Serial << F("SKAARHOJ Device IP Address: ") << ip[0] << "." << ip[1] << "." << ip[2] << "." << ip[3] << "\n";
  Serial << F("ATEM Switcher IP Address: ") << atem_ip[0] << "." << atem_ip[1] << "." << atem_ip[2] << "." << atem_ip[3] << "\n";
  
  // Setting MAC address:
  mac[0] = EEPROM.read(10);
  mac[1] = EEPROM.read(11);
  mac[2] = EEPROM.read(12);
  mac[3] = EEPROM.read(13);
  mac[4] = EEPROM.read(14);
  mac[5] = EEPROM.read(15);
  char buffer[18];
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial << F("SKAARHOJ Device MAC address: ") << buffer << F(" - Checksum: ")
        << ((mac[0]+mac[1]+mac[2]+mac[3]+mac[4]+mac[5]) & 0xFF) << "\n";
  if ((uint8_t)EEPROM.read(16)!=((mac[0]+mac[1]+mac[2]+mac[3]+mac[4]+mac[5]) & 0xFF))  {
    Serial << F("MAC address not found in EEPROM memory!\n") <<
      F("Please load example sketch ConfigEthernetAddresses to set it.\n") <<
      F("The MAC address is found on the backside of your Ethernet Shield/Board\n (STOP)");
    while(true);
  }

  Ethernet.begin(mac, ip);

    // Initialize touch library
  utils.touch_init();


  // Touch Screen calibration:
  touchCal[0] = (EEPROM.read(50) << 8) | EEPROM.read(51);
  touchCal[1] = (EEPROM.read(52) << 8) | EEPROM.read(53);
  touchCal[2] = (EEPROM.read(54) << 8) | EEPROM.read(55);
  touchCal[3] = (EEPROM.read(56) << 8) | EEPROM.read(57);
  touchCal[4] = (EEPROM.read(58) << 8) | EEPROM.read(59);
  touchCal[5] = (EEPROM.read(60) << 8) | EEPROM.read(61);
  touchCal[6] = (EEPROM.read(62) << 8) | EEPROM.read(63);
  touchCal[7] = (EEPROM.read(64) << 8) | EEPROM.read(65);
  checksumByte = ((int)(touchCal[0]
    + touchCal[1]
    + touchCal[2]
    + touchCal[3]
    + touchCal[4]
    + touchCal[5]
    + touchCal[6]
    + touchCal[7]))  & 0xFF;  // checksum
  if (checksumByte == EEPROM.read(66))  {
    Serial << F("TouchScreen Calibration Data: \n    (") 
      << touchCal[0] << "," 
      << touchCal[1] << "," 
      << touchCal[2] << "," 
      << touchCal[3] << "," 
      << touchCal[4] << "," 
      << touchCal[5] << "," 
      << touchCal[6] << "," 
      << touchCal[7] << ")" 
      << " - Checksum: " << checksumByte << " => OK\n";
    utils.touch_calibrationPointRawCoordinates(touchCal[0],touchCal[1],touchCal[2],touchCal[3],touchCal[4], touchCal[5],touchCal[6],touchCal[7]);
  } 
  else {
    Serial << F("Calibration not found in memory!\n") << 
      F("Please run the Touchscreen Calibration sketch first\n") <<
      F("Now, using default calibration"); 

    // The line below is calibration numbers for a specific monitor. 
    // Substitute this with calibration for YOUR monitor (see example "Touch_Calibrate")
    utils.touch_calibrationPointRawCoordinates(330,711,763,716,763,363,326,360);
  }
    

  
  // Sets the Bi-color LED to off = "no connection"
  digitalWrite(2,false);
  digitalWrite(3,false);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);


  // Initialize a connection to the switcher:
  AtemSwitcher.begin(IPAddress(atem_ip[0],atem_ip[1],atem_ip[2],atem_ip[3]), 56417);
  AtemSwitcher.serialOutput(true);  // Remove or comment out this line for production code. Serial output may decrease performance!
  AtemSwitcher.connect();
  
    // Set Bi-color LED orange - indicates "connecting...":
  digitalWrite(3,true);
  digitalWrite(2,true);
}

bool AtemOnline = false;

void loop() {

  // Check for packets, respond to them etc. Keeping the connection alive!
  AtemSwitcher.runLoop();

  // If connection is gone, try to reconnect:
  if (AtemSwitcher.isConnectionTimedOut())  {
    if (AtemOnline)  {
      AtemOnline = false;
    }

    Serial.println("Connection to ATEM Switcher has timed out - reconnecting!");
    AtemSwitcher.connect();
  }

  // If the switcher has been initialized, check for button presses as reflect status of switcher in button lights:
  if (AtemSwitcher.hasInitialized())  {
    if (!AtemOnline)  {
      AtemOnline = true;
      newRandomButton();
    }

//    AtemSwitcher.delay(15);	// This determines how smooth you will see the graphic move.
    manageTouch();
  }
}




  int xSize;
  int ySize;
  int xOrigin;
  int yOrigin;

  int topMask;
  int bottomMask;
  int leftMask;
  int rightMask;
 
  int lightness = 250;

void manageTouch()  {

  uint8_t tState = utils.touch_state();  // See what the state of touch is:
  if (tState !=0)  {  // Different from zero? Then a touch event has happened:
    Serial << "New touch event " << tState;
    if (tState==1 || tState==2)  {  // Single tap somewhere:
      uint16_t xC = utils.touch_getEndedXCoord();
      uint16_t yC = utils.touch_getEndedYCoord();
      Serial << "New touch in: " << xC << "," << yC   
            << " (Must be: " << xOrigin << "-" << (xOrigin+xSize) << " , " 
            << (720-yOrigin-ySize) << "-" << (720-yOrigin) << ") ";
      
      if (xC>=xOrigin && xC<=(xOrigin+xSize) && yC>=(720-yOrigin-ySize) && yC<=(720-yOrigin))  {
        Serial << " => OK! Changing color.\n";
        lightness = 1000-lightness;
        AtemSwitcher.changeColorValue(1,random(0,37)*100,random(0,3)*500,lightness);
      } else {
        Serial << " => Outside bounding box\n";
      }
    } else if(tState==9)  {
      newRandomButton();
      Serial << "New button...\n";
    }
  }

}



void newRandomButton()  {
  xSize = random(3,51)*10;
  ySize = random(3,51)*10;
  xOrigin = random(0,1280-xSize);
  yOrigin = random(0,720-ySize);

  Serial << "New box, upper left: " << xOrigin << "," << yOrigin << " - size: " << xSize << "," << ySize << "\n";
  
  leftMask = (xOrigin-640)*25;         //(25 = 16000/640);
  rightMask = (xOrigin-640+xSize)*25;  //(25 = 16000/640);
  topMask = (360-yOrigin)*25;         //(25 = 9000/360);
  bottomMask = (360-yOrigin-ySize)*25;  //(25 = 9000/360);
  
  AtemSwitcher.changeKeyerMask(topMask,bottomMask,leftMask,rightMask);
}
