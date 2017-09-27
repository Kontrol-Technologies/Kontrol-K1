/*****************
 * Basis control for the SKAARHOJ C20x series devices
 * This example is programmed for ATEM 1M/E versions
 *
 * This example also uses several custom libraries which you must install first.
 * Search for "#include" in this file to find the libraries. Then download the libraries from http://skaarhoj.com/wiki/index.php/Libraries_for_Arduino
 *
 * Works with ethernet-enabled arduino devices (Arduino Ethernet or a model with Ethernet shield)
 * Run the example sketch "ConfigEthernetAddresses" to set up Ethernet before installing this sketch.
 *
 * - kasper
 */







// Including libraries:
#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <utility/w5100.h>
#include "WebServer.h"  // For web interface
#include <Streaming.h>
#include <EEPROM.h>      // For storing IP numbers
#include <MenuBackend.h>  // Library for menu navigation. Must (for some reason) be included early! Otherwise compilation chokes.
#include <SkaarhojTools.h>
#include <SkaarhojPgmspace.h>
#include <SkaarhojBufferTools.h>
#include <SkaarhojASCIIClient.h>
#include <SkaarhojTCPClient.h>
#include <ClientBMDHyperdeckStudio.h>
//#include <ClientBMDVideohubTCP.h>

SkaarhojTools sTools(1);    // 0=No runtime serial logging, 1=Moderate runtime serial logging, 2=more verbose... etc.

static uint8_t default_ip[] = {     // IP for Configuration Mode (192.168.10.99)
  192, 168, 10, 99
};
uint8_t ip[4];        // Will hold the C200 IP address
uint8_t atem_ip[4];  // Will hold the ATEM IP address
//uint8_t videohub_ip[4]; // Will hold the Videohub IP address
uint8_t hyperdeck_ip[4];  // Will hold the HyperDeck IP address
uint8_t mac[6];    // Will hold the Arduino Ethernet shield/board MAC address (loaded from EEPROM memory, set with ConfigEthernetAddresses example sketch)


/* MEMORY USAGE:
 * This can be used to track free memory.
 * Include "MemoryFree.h" and use the following line
 *     Serial << F("freeMemory()=") << freeMemory() << "\n";
 * in your code to see available memory.
 */
#include <MemoryFree.h>


// Include ATEM library and make an instance:
#include <ATEMbase.h>
#include <ATEMext.h>
ATEMext AtemSwitcher;

// HyperDeck Studio (Socket 2)
ClientBMDHyperdeckStudio hyperDeck;

// Videohub (Socket 3):
//ClientBMDVideohubTCP Videohub;

// All related to library "SkaarhojBI8", which controls the buttons:
#include "Wire.h"
#include "MCP23017.h"
#include "PCA9685.h"
#include "SkaarhojBI8.h"
SkaarhojBI8 previewSelect;
SkaarhojBI8 programSelect;
SkaarhojBI8 cmdSelect;
SkaarhojBI8 extraButtons;


// All related to library "SkaarhojUtils". Used for rotary encoder and "T-bar" potentiometer:
#include "SkaarhojUtils.h"
SkaarhojUtils utils;

#include "SkaarhojEncoders.h"
SkaarhojEncoders encoders;

#include "SkaarhojEADOGMDisplay.h"
SkaarhojEADOGMDisplay Disp163;

#include <Adafruit_GFX.h>
#include <SkaarhojSmartSwitch2.h>
SkaarhojSmartSwitch2 SSWboard;

int greenLED = 22;
int redLED = 23;





uint8_t MEselectForInputbuttons = 0;  // 0=ME1, 1=ME2
uint8_t DVEpositionX;
uint8_t DVEpositionY;
uint8_t DVEsizeX;
uint8_t DVEsizeY;
uint8_t DVEborderSize;



//
//MCP23017 GPIOchipArray[] = {
//  MCP23017(), MCP23017(), MCP23017(), MCP23017(), MCP23017(), MCP23017(), MCP23017(), MCP23017()
//};
//word MCP23017_states[8];
//
//PCA9685 ledDriver;
//bool PCA9685_states[64];

uint16_t buttonColors[5][2] = {
  {100, 80},   // Full amber
  {100, 0}, // Full red
  {0, 100}, // Full green
  {100, 80},   // Amber
  {10, 8}      // Dimmed Amber
};
uint8_t panelIntensityLevel = 10;



// width x height = 64,32
/**static const uint8_t SKAA[] PROGMEM = {
        B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B10011111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B10011111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B10011111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B10011111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B10011111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B10011111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B10011111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B10011111, B11111111, B11111111, B11100011, B11111111, B11111111, B11111000, B01111111, 
	B10011111, B11110001, B11111111, B00000011, B11111111, B11111001, B11000000, B00001111, 
	B10011111, B11100011, B11111100, B00110011, B11111111, B11111001, B10011111, B11000111, 
	B10011111, B11001111, B11111000, B11110011, B11100011, B11111000, B00111111, B11100011, 
	B10011111, B00011111, B11110001, B11110011, B11110011, B11111000, B01111111, B11110001, 
	B10011110, B00111111, B11110011, B11110011, B11111001, B11111000, B11111111, B11111001, 
	B10011100, B11111111, B11100111, B11110011, B11111000, B11111000, B11111111, B11111001, 
	B10011001, B11111111, B11100111, B11110011, B11111100, B11111000, B11111111, B11111001, 
	B10000000, B11111111, B11100110, B00000011, B11111100, B11111000, B11111111, B11111001, 
	B10001110, B01111111, B11100110, B00000011, B11111100, B11111000, B11111111, B11111001, 
	B10011111, B00111111, B11100111, B11110011, B11111000, B11111000, B11111111, B11111001, 
	B10011111, B10001111, B11100011, B11110011, B11111001, B11111000, B11111111, B11111001, 
	B10011111, B11000111, B11110011, B11110011, B11110001, B11111000, B11111111, B11111001, 
	B10011111, B11100011, B11111000, B11110011, B11100011, B11111000, B11111111, B11111001, 
	B10011111, B11110001, B11111100, B11110011, B11000111, B11111000, B11111111, B11111001, 
	B10011111, B11111000, B11111111, B11110000, B00001111, B11111000, B11111111, B11111001, 
	B10011111, B11111100, B01111111, B11110000, B00111111, B11111101, B11111111, B11111001, 
	B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 
	B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, 

};

// width x height = 64,32

*/










/*************************************************************


                       Webserver


 **********************************************************/




#define PREFIX ""
WebServer webserver(PREFIX, 80);

void logoCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  /* this data was taken from a PNG file that was converted to a C data structure
     by running it through the directfb-csource application.
     (Alternatively by PHPSH script "HeaderGraphicsWebInterfaceInUnitsPNG8.phpsh")
  */
  P(logoData) = {
   //INSERT IMAGE HERE
   
   //
  };

  if (type == WebServer::POST)
  {
    // ignore POST data
    server.httpFail();
    return;
  }

  /* for a GET or HEAD, send the standard "it's all OK headers" but identify our data as a PNG file */
  server.httpSuccess("image/png");

  /* we don't output the body for a HEAD request */
  if (type == WebServer::GET)
  {
    server.writeP(logoData, sizeof(logoData));
  }
}

// commands are functions that get called by the webserver framework
// they can read any posted data from client, and they output to server
void webDefaultView(WebServer &server, WebServer::ConnectionType type)
{
  P(htmlHead) =
    "<html>"
    "<head>"
    "<title>KONTROL Device Configuration</title>"
    "<style type=\"text/css\">"
    "BODY { font-family: ARIAL }"
    "H1 { font-size: 14pt; }"
    "H2 { font-size: 16pt; }"
    "P  { font-size: 10pt; }"
    "</style>"
    "</head>"
    "<body>";
    

  int i;
  int j;
  server.httpSuccess();
  server.printP(htmlHead);

  server << F("<div style='width:660px; margin-left:10px;'><form action='") << PREFIX << F("/form' method='post'>");
  // HEADER:
 server << F("<h2>KONTROL DEVICE CONFIGURATION</h2><p>");
 server << F("<i>SERIAL# A3BIY1BA02</i><p>");
 server << F("<i>Registered to:</i><p>");
 server << F("<i>Virtual Productions</i><p>");
 server << F("<i>012 991-5924</i><p>");
 server << F("<hr/><br>");
  // Panel IP:
  server << F("<h1>KONTROL Device IP Address:</h1><p>");
  for (i = 0; i <= 3; ++i)
  {
    server << F("<input type='text' name='IP") << i << F("' value='") << EEPROM.read(i + 2) << F("' id='IP") << i << F("' size='4'>");
    if (i < 3)  server << '.';
  }
  server << F("<hr/>");

  // ATEM Switcher Panel IP:
  server << F("<h1>ATEM Switcher IP Address:</h1><p>");
  for (i = 0; i <= 3; ++i)
  {
    server << F("<input type='text' name='ATEM_IP") << i << F("' value='") << EEPROM.read(i + 2 + 4) << F("' id='ATEM_IP") << i << F("' size='4'>");
    if (i < 3)  server << '.';
  }
  //server << F("<hr/>");

  // VideoHub IP:
 /** server << F("<h1>VideoHub IP Address:</h1><p>");
  for (i = 0; i <= 3; ++i)
  {
    server << F("<input type='text' name='VHUB_IP") << i << F("' value='") << EEPROM.read(i + 50) << F("' id='VHUB_IP") << i << F("' size='4'>");
    if (i < 3)  server << F(".");
  }*/
  server << F("<hr/>");

  // HyperDeck IP:
  server << F("<h1>HyperDeck IP Address:</h1><p>");
  for (i = 0; i <= 3; ++i)
  {
    server << F("<input type='text' name='HDCK_IP") << i << F("' value='") << EEPROM.read(i + 55) << F("' id='HDCK_IP") << i << F("' size='4'>");
    if (i < 3)  server << F(".");
  }
  server << F("<hr/>");

  // Set routing for USER buttons
  server << F("<h1>USER DEFINED KEYS:</h1><table border=0 cellspacing=3><tr>");

  // Set routing for USER buttons
  for (i = 1; i <= 4; ++i)
  {
    server << F("<td><p>Button ") << i << F(":</p>");
    server << F("<select name='Buttons2F1") << i << F(":'div style='width:78px;>");
    server << F("<option value='0'></option>");

    server << F("<option value='") << 0 << F("'") << (EEPROM.read(450 + i) == 0 ? F(" selected='selected'") : F("")) << F("> \t") << F("</option>");

    // USK 1-4 on/off
    server << F("<option value='") << 1 << F("'") << (EEPROM.read(450 + i) == 1 ? F(" selected='selected'") : F("")) << F(">USK1") << F("</option>");
    server << F("<option value='") << 2 << F("'") << (EEPROM.read(450 + i) == 2 ? F(" selected='selected'") : F("")) << F(">USK1On") << F("</option>");
    server << F("<option value='") << 3 << F("'") << (EEPROM.read(450 + i) == 3 ? F(" selected='selected'") : F("")) << F(">USK1Off") << F("</option>");
    server << F("<option value='") << 4 << F("'") << (EEPROM.read(450 + i) == 4 ? F(" selected='selected'") : F("")) << F(">USK2") << F("</option>");
    server << F("<option value='") << 5 << F("'") << (EEPROM.read(450 + i) == 5 ? F(" selected='selected'") : F("")) << F(">USK2On") << F("</option>");
    server << F("<option value='") << 6 << F("'") << (EEPROM.read(450 + i) == 6 ? F(" selected='selected'") : F("")) << F(">USK2Off") << F("</option>");
    server << F("<option value='") << 7 << F("'") << (EEPROM.read(450 + i) == 7 ? F(" selected='selected'") : F("")) << F(">USK3") << F("</option>");
    server << F("<option value='") << 8 << F("'") << (EEPROM.read(450 + i) == 8 ? F(" selected='selected'") : F("")) << F(">USK3On") << F("</option>");
    server << F("<option value='") << 9 << F("'") << (EEPROM.read(450 + i) == 9 ? F(" selected='selected'") : F("")) << F(">USK3Off") << F("</option>");
    server << F("<option value='") << 10 << F("'") << (EEPROM.read(450 + i) == 10 ? F(" selected='selected'") : F("")) << F(">USK4") << F("</option>");
    server << F("<option value='") << 11 << F("'") << (EEPROM.read(450 + i) == 11 ? F(" selected='selected'") : F("")) << F(">USK4On") << F("</option>");
    server << F("<option value='") << 12 << F("'") << (EEPROM.read(450 + i) == 12 ? F(" selected='selected'") : F("")) << F(">USK4Off") << F("</option>");

    // DSK 1-2 on/off
    server << F("<option value='") << 13 << F("'") << (EEPROM.read(450 + i) == 13 ? F(" selected='selected'") : F("")) << F(">DSK1") << F("</option>");
    server << F("<option value='") << 14 << F("'") << (EEPROM.read(450 + i) == 14 ? F(" selected='selected'") : F("")) << F(">DSK1On") << F("</option>");
    server << F("<option value='") << 15 << F("'") << (EEPROM.read(450 + i) == 15 ? F(" selected='selected'") : F("")) << F(">DSK1Off") << F("</option>");
    server << F("<option value='") << 16 << F("'") << (EEPROM.read(450 + i) == 16 ? F(" selected='selected'") : F("")) << F(">DSK1Auto") << F("</option>");
    server << F("<option value='") << 17 << F("'") << (EEPROM.read(450 + i) == 17 ? F(" selected='selected'") : F("")) << F(">DSK2") << F("</option>");
    server << F("<option value='") << 18 << F("'") << (EEPROM.read(450 + i) == 18 ? F(" selected='selected'") : F("")) << F(">DSK2On") << F("</option>");
    server << F("<option value='") << 19 << F("'") << (EEPROM.read(450 + i) == 19 ? F(" selected='selected'") : F("")) << F(">DSK2Off") << F("</option>");
    server << F("<option value='") << 20 << F("'") << (EEPROM.read(450 + i) == 20 ? F(" selected='selected'") : F("")) << F(">DSK2Auto") << F("</option>");

    // Others
    server << F("<option value='") << 21 << F("'") << (EEPROM.read(450 + i) == 21 ? F(" selected='selected'") : F("")) << F(">Keys Off") << F("</option>");
    server << F("<option value='") << 22 << F("'") << (EEPROM.read(450 + i) == 22 ? F(" selected='selected'") : F("")) << F(">Cut") << F("</option>");
    server << F("<option value='") << 23 << F("'") << (EEPROM.read(450 + i) == 23 ? F(" selected='selected'") : F("")) << F(">Auto") << F("</option>");
    server << F("<option value='") << 24 << F("'") << (EEPROM.read(450 + i) == 24 ? F(" selected='selected'") : F("")) << F(">FTB") << F("</option>");


    server << F("<option value='") << 25 << F("'") << (EEPROM.read(450 + i) == 25 ? F(" selected='selected'") : F("")) << F(">PiP") << F("</option>");
    server << F("<option value='") << 26 << F("'") << (EEPROM.read(450 + i) == 26 ? F(" selected='selected'") : F("")) << F(">VGA+PiP") << F("</option>");

    server << F("</select></td>");
  }
  server << F("</tr></table><hr/>");
  ///////////////////////////////////////////////////

  // DVE Settings:
/**  server << F("<h1>PIP Parameters:</h1><table border=0 cellspacing=3><tr>");

  // Set routing for USER buttons
  for (i = 1; i <= 5; ++i)
  {
    if (i == 1) {
      server << F("<td><p>Position X:</p>");
    }
    if (i == 2) {
      server << F("<td><p>Position Y:</p>");
    }
    if (i == 3) {
      server << F("<td><p>Size X:</p>");
    }
    if (i == 4) {
      server << F("<td><p>Size Y:</p>");
    }
    if (i == 5) {
      server << F("<td><p>Border Size:</p>");
    }
    server << F("<select name='DVE") << i << F(":'div style='width:78px;>");
    server << F("<option value='0'></option>");

    for (j = 0; j <= 200; j++) {
      server << F("<option value='") << j << F("'") << (EEPROM.read(500 + i) == j ? F(" selected='selected'") : F("")) << F(">") << j << F("</option>");
    }

    server << F("</select></td>");
  }
  server << F("</tr></table>");
  server << F("<br><i>(Set all to 0 in order to disable)</i>");
  server << F("<hr/>"); 
  */

  ///////////////////////////////////////////////////

  ///////////////////////////////////////////////////

  // End form and page:
  server << F("<input type='submit' value='Save settings'/></form></div>");
  server << F("<br><i>(Please power cycle unit after submitting for the changes to take effect.)</i>");
  server << F("</body></html>");
}

void formCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  if (type == WebServer::POST)
  {
    bool repeat;
    char name[16], value[16];
    do
    {
      repeat = server.readPOSTparam(name, 16, value, 16);
      String Name = String(name);

      // C200 Panel IP:
      if (Name.startsWith("IP"))  {
        int addr = strtoul(name + 2, NULL, 10);
        int val = strtoul(value, NULL, 10);
        if (addr >= 0 && addr <= 3)  {
          EEPROM.write(addr + 2, val); // IP address stored in bytes 0-3
        }
      }

      // ATEM Switcher Panel IP:
      if (Name.startsWith("ATEM_IP"))  {
        int addr = strtoul(name + 7, NULL, 10);
        int val = strtoul(value, NULL, 10);
        if (addr >= 0 && addr <= 3)  {
          EEPROM.write(addr + 2 + 4, val); // IP address stored in bytes 4-7
        }
      }

      // VideoHub IP:
     /** 
     if (Name.startsWith("VHUB_IP"))  {
        int addr = strtoul(name + 7, NULL, 10);
        int val = strtoul(value, NULL, 10);
        if (addr >= 0 && addr <= 3)  {
          EEPROM.write(addr + 50, val);  // IP address stored in bytes 60-63
        }
      }
      */

      // HyperDeck IP:
      if (Name.startsWith("HDCK_IP"))  {
        int addr = strtoul(name + 7, NULL, 10);
        int val = strtoul(value, NULL, 10);
        if (addr >= 0 && addr <= 3)  {
          EEPROM.write(addr + 55, val); // IP address stored in bytes 70-73
        }
      }

      // routing
      if (Name.startsWith("Buttons2F1"))  {
        int inputNum = strtoul(name + 10, NULL, 10);
        int val = strtoul(value, NULL, 10);
        if (inputNum >= 1 && inputNum <= 4)  {
          EEPROM.write(450 + inputNum, val);
        }
      }

      if (Name.startsWith("DVE"))  {
        int inputNum = strtoul(name + 3, NULL, 10);
        int val = strtoul(value, NULL, 10);
        if (inputNum >= 1 && inputNum <= 5)  {
          EEPROM.write(500 + inputNum, val);
        }
      }

    }
    while (repeat);

    server.httpSeeOther(PREFIX "/form");
  }
  else
    webDefaultView(server, type);
}
void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  webDefaultView(server, type);
}





























/*************************************************************
 *
 *
 *                     MAIN PROGRAM CODE AHEAD
 *
 *
 **********************************************************/



bool isConfigMode;  // If set, the system will run the Web Configurator, not the normal program
uint16_t buttons2function[4];
bool setDisp = true;

//unsigned long previousMillis = 0;
//const long interval = 1000;

void setup() {
  Serial.begin(115200);
  Serial << F("\n- - - - - - - -\nSerial Started\n");


  // *********************************
  // Start up BI8 boards and I2C bus:
  // *********************************
  // Always initialize Wire before setting up the SkaarhojBI8 class!
  Wire.begin();

  // Set up the SkaarhojBI8 boards:
  previewSelect.begin(0,false,true);
  programSelect.begin(1,false,true);
  cmdSelect.begin(2,false,true);
  extraButtons.begin(3,false,true);
  if (EEPROM.read(399) <= 10)   {
    panelIntensityLevel = (3);
    setPanelIntensity();
  }


  // ********************
  // Start up LCD (has to be after Ethernet has been started!)
  // ********************
  Disp163.begin(5, 0, 3); // DOGM162

  Disp163.contrast(0x5);
  
  Disp163.cursor(false);
  Disp163 << F("     KONTROL    ");
  Disp163.gotoRowCol(1, 0);
  Disp163 << F("                ");
  Disp163 << F("      v1.1      ");
  cmdSelect.setButtonColor(1, 2);
  delay(2000);

  delay(1000);
cmdSelect.setButtonColor(1, 5);
  // Switches on address 4 + SDI on pins 48+49:
  SSWboard.begin(4);
  // Setting full brightness (range 0-7) of all buttons:
  SSWboard.setButtonBrightness(7, B11);


  // Setting white color for all buttons:
  SSWboard.setButtonColor(3, 3, 3, B11);

  // Init done
  SSWboard.clearDisplay();   // clears the screen and buffer
  SSWboard.display(B11);  // Write to all

  SSWboard.setRotation(0);

  initGraphics();

  // *********************************
  // Mode of Operation (Normal / Configuration)
  // *********************************
  // Determine web config mode
  // This is done by:
  // -  either flipping a switch connecting A1 to GND
  // -  Holding the CUT button during start up.
  pinMode(A1, INPUT_PULLUP);
  delay(100);
  isConfigMode = analogRead(A1) < 100 ? true : false;
 
  uint16_t cmdSelection = cmdSelect.buttonDownAll();
   if (cmdSelection & (B1 << 0))  {
   isConfigMode = (cmdSelection & (B1 << 0)) < 100 ? true : false;
   }


  // *********************************
  // INITIALIZE EEPROM memory:
  // *********************************
  // Check if EEPROM has ever been initialized, if not, install default IP
  if (EEPROM.read(0) != 12 ||  EEPROM.read(1) != 232)  {  // Just randomly selected values which should be unlikely to be in EEPROM by default.
    // Set these random values so this initialization is only run once!
    EEPROM.write(0, 12);
    EEPROM.write(1, 232);

    // Set default IP address for Arduino/C100 panel (192.168.10.99)
    EEPROM.write(2, 192);
    EEPROM.write(3, 168);
    EEPROM.write(4, 10);
    EEPROM.write(5, 99); // Just some value I chose, probably below DHCP range?

    // Set default IP address for ATEM Switcher (192.168.10.240):
    EEPROM.write(6, 192);
    EEPROM.write(7, 168);
    EEPROM.write(8, 10);
    EEPROM.write(9, 240);
  }


  // *********************************
  // Setting up IP addresses, starting Ethernet
  // *********************************
  if (isConfigMode)  {
    // Setting the default ip address for configuration mode:
    ip[0] = default_ip[0];
    ip[1] = default_ip[1];
    ip[2] = default_ip[2];
    ip[3] = default_ip[3];
  }
  else {
    ip[0] = EEPROM.read(0 + 2);
    ip[1] = EEPROM.read(1 + 2);
    ip[2] = EEPROM.read(2 + 2);
    ip[3] = EEPROM.read(3 + 2);
  }

  // Setting the ATEM IP address:
  atem_ip[0] = EEPROM.read(0 + 2 + 4);
  atem_ip[1] = EEPROM.read(1 + 2 + 4);
  atem_ip[2] = EEPROM.read(2 + 2 + 4);
  atem_ip[3] = EEPROM.read(3 + 2 + 4);

  // Setting the VideoHub IP address:
  /**
  videohub_ip[0] = EEPROM.read(0 + 50);
  videohub_ip[1] = EEPROM.read(1 + 50);
  videohub_ip[2] = EEPROM.read(2 + 50);
  videohub_ip[3] = EEPROM.read(3 + 50);
  */

  // Setting the HyperDeck IP address:
  hyperdeck_ip[0] = EEPROM.read(0 + 55);
  hyperdeck_ip[1] = EEPROM.read(1 + 55);
  hyperdeck_ip[2] = EEPROM.read(2 + 55);
  hyperdeck_ip[3] = EEPROM.read(3 + 55);

  Serial << F("SKAARHOJ Device IP Address: ") << ip[0] << "." << ip[1] << "." << ip[2] << "." << ip[3] << "\n";
  Serial << F("ATEM Switcher IP Address: ") << atem_ip[0] << "." << atem_ip[1] << "." << atem_ip[2] << "." << atem_ip[3] << "\n";
  //Serial << F("VideoHub IP Address: ") << videohub_ip[0] << "." << videohub_ip[1] << "." << videohub_ip[2] << "." << videohub_ip[3] << "\n";
  Serial << F("HyperDeck IP Address: ") << hyperdeck_ip[0] << "." << hyperdeck_ip[1] << "." << hyperdeck_ip[2] << "." << hyperdeck_ip[3] << "\n";

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
         << ((mac[0] + mac[1] + mac[2] + mac[3] + mac[4] + mac[5]) & 0xFF);
  if ((uint8_t)EEPROM.read(16) != ((mac[0] + mac[1] + mac[2] + mac[3] + mac[4] + mac[5]) & 0xFF))  {
    Serial << F("MAC address not found in EEPROM memory!\n") <<
           F("Please load example sketch ConfigEthernetAddresses to set it.\n") <<
           F("The MAC address is found on the backside of your Ethernet Shield/Board\n (STOP)");
    while (true);
  }

  Ethernet.begin(mac, ip);

  // Setting short timeout on IP connections:
  //W5100.setRetransmissionTime(0xD0);  // Milli seconds  // prev: 2000
  W5100.setRetransmissionTime(500);  // Milli seconds  // prev: 2000
  W5100.setRetransmissionCount(2);

  delay(1000);

  // Sets the Bi-color LED to off = "no connection"
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  digitalWrite(greenLED, false);
  digitalWrite(redLED, false);

  encoders.begin(6);
  encoders.serialOutput(false);


  menuSetup();

  // *********************************
  // Final Setup based on mode
  // *********************************
  if (isConfigMode)  {

    //    for (uint8_t i = 0; i <= 7; i++)  {
    //      GPIOchipArray[i].begin(i);
    //      GPIOchipArray[i].init();
    //      GPIOchipArray[i].internalPullupMask(65535);
    //      GPIOchipArray[i].inputOutputMask(65535);  // All inputs
    //    }

    // LCD IP info:
    Disp163.clearDisplay();
    Disp163 << F("     KONTROL    ");
    Disp163.gotoRowCol(1, 0);
    Disp163.print("CONFIG MODE, IP:");
    Disp163.gotoRowCol(2, 0);
    Disp163.print(ip[0]);
    Disp163.print('.');
    Disp163.print(ip[1]);
    Disp163.print('.');
    Disp163.print(ip[2]);
    Disp163.print('.');
    Disp163.print(ip[3]);
    delay(1000);

    SSWboard.setButtonColor(3, 3, 3, B11);

    // Red by default:
    previewSelect.setDefaultColor(2);
    programSelect.setDefaultColor(2);
    cmdSelect.setDefaultColor(2);
    extraButtons.setDefaultColor(2);

    previewSelect.setButtonColorsToDefault();
    programSelect.setButtonColorsToDefault();
    cmdSelect.setButtonColorsToDefault();
    extraButtons.setButtonColorsToDefault();
   

    webserver.begin();
    webserver.setDefaultCommand(&defaultCmd);
    webserver.addCommand("form", &formCmd);
    webserver.addCommand("logo.png", &logoCmd);

    Serial << F("freeMemory()=") << freeMemory() << "\n";

    while (true) {
      webserver.processConnection();
      digitalWrite(redLED, (((unsigned long)millis() >> 3) & B11000000) ? true : false);

      if (millis() > 600000) {
        // LCD IP info:
        if (setDisp) {
          Disp163.clearDisplay();
          Disp163 << F("     KONTROL    ");
          Disp163.gotoRowCol(1, 0);
          Disp163.print("TEST MODE       ");
          Disp163.gotoRowCol(2, 0);
          Disp163.print("Running...      ");
          delay(1000);
          setDisp = false;
        }
        runTest();
        static int j = 1;
        static long k = 0;
        j++;
        if (j > 1000) {
          j = 1;
          k++;
          Disp163.gotoRowCol(2, 0);
          Disp163.print("                ");
          Disp163.gotoRowCol(2, 0);
          Disp163 << F("Run: ") << k;
        }
      }
    }
  }
  else {

    Disp163.clearDisplay();
    Disp163 << F("     KONTROL    ");
    Disp163.gotoRowCol(1, 0);
    Disp163.print("IP Address:");
    Disp163.gotoRowCol(2, 0);
    Disp163.print(ip[0]);
    Disp163.print('.');
    Disp163.print(ip[1]);
    Disp163.print('.');
    Disp163.print(ip[2]);
    Disp163.print('.');
    Disp163.print(ip[3]);
    delay(1000);

    // Colors of buttons:
    previewSelect.setDefaultColor(0);  // Off by default
    programSelect.setDefaultColor(0);  // Off by default
    cmdSelect.setDefaultColor(0);  // Off by default
    extraButtons.setDefaultColor(0);  // Off by default

    previewSelect.testSequence(10);
    programSelect.testSequence(10);
    cmdSelect.testSequence(10);
    extraButtons.testSequence(10);

    // Initializing the slider:
    utils.uniDirectionalSlider_init();
    utils.uniDirectionalSlider_hasMoved();

    menuSetup();

    Disp163.clearDisplay();
    Disp163 << F("     KONTROL    ");
    Disp163.gotoRowCol(1, 0);
    Disp163.print("Connecting to:");
    Disp163.gotoRowCol(2, 0);
    Disp163.print(atem_ip[0]);
    Disp163.print('.');
    Disp163.print(atem_ip[1]);
    Disp163.print('.');
    Disp163.print(atem_ip[2]);
    Disp163.print('.');
    Disp163.print(atem_ip[3]);

    // read buttons functions
    for (uint16_t i = 1; i <= 4; i++) {
      buttons2function[i - 1] = (EEPROM.read(450 + i));
    }
    DVEpositionX = EEPROM.read(501);
    DVEpositionY = EEPROM.read(502);
    DVEsizeX = EEPROM.read(503);
    DVEsizeY = EEPROM.read(504);
    DVEborderSize = EEPROM.read(505);

    // Connect to an ATEM switcher on this address and using this local port:
    // The port number is chosen randomly among high numbers.

    // Start Videohub connection:
   
   /** Serial.println("Videohub connecting...");
    Videohub.begin(videohub_ip);  // <= SETUP (the IP address of the Videohub)
    Videohub.connect();
    Videohub.serialOutput(0);
    */

    // Start Hyperdeck connection:
    Serial.println("HyperDeck connecting...");
    hyperDeck.begin(hyperdeck_ip);   // <= SETUP (the IP address of the Hyperdeck Studio)
    hyperDeck.serialOutput(0);  // 1= normal, 2= medium verbose, 3=Super verbose
    hyperDeck.connect();  // For some reason the first connection attempt seems to fail, but in the runloop it will try to reconnect.

    AtemSwitcher.begin(IPAddress(atem_ip[0], atem_ip[1], atem_ip[2], atem_ip[3]), 56417);
    // AtemSwitcher.serialOutput(true);
    AtemSwitcher.connect();

    // Set Bi-color LED orange - indicates "connecting...":
    digitalWrite(redLED, true);
    digitalWrite(greenLED, true);

    Serial << F("freeMemory()=") << freeMemory() << "\n";
  }
}






/*************************************************************
 * Loop function (runtime loop)
 *************************************************************/

// These variables are used to track state, for instance when the VGA+PIP button has been pushed.
bool preVGA_active = false;
bool preVGA_UpstreamkeyerStatus = false;
int preVGA_programInput = 0;
static bool updateSSW[2] = {true};
bool dskTrans = false;
uint16_t autoDSK;
bool shift = false;
bool SSW2lock = false;
bool SSW1lock = false;
bool hyperdeckControl = false;
int playSpeed = 100;
uint8_t HDclipId;

// AtemOnline is true, if there is a connection to the ATEM switcher
bool AtemOnline = false;

static uint8_t devicesOnline = 0;
static uint8_t devicesOnline_prev = 0;
/**
uint8_t videoHubIn = 0;
uint8_t videoHubOut = 0;
*/
bool pipSet = true;
//bool firstTime = true;
//unsigned long counter = 0;
//bool deviation = false;
//int theSpeed = 10;

// The loop function:
void loop() {

  // Check for packets, respond to them etc. Keeping the connection alive!
  lDelay(0);
  menuNavigation();
  menuValues();
  autoDSK = EEPROM.read(90);
  dskTrans = autoDSK == 1 ? true : false;

  devicesOnline = (AtemSwitcher.hasInitialized() ? 1 : 0) /** | (hyperDeck.hasInitialized() ? 2 : 0) /**| (Videohub.hasInitialized() ? 4 : 0)*/;
  if (devicesOnline_prev != devicesOnline)  {
    devicesOnline_prev = devicesOnline;
    Serial << F("Device Connection status changed: ") << devicesOnline << F("\n");
    for (uint8_t i = 0; i < 2; i++)  updateSSW[i] = true;
  }

  if (devicesOnline) {
    // Set Bi-color LED to red or green depending on mode:
    digitalWrite(redLED, false);
    digitalWrite(greenLED, true);
  } else {
    // Set Bi-color LED off = "no connection"
    digitalWrite(redLED, true);
    digitalWrite(greenLED, false);
  }

  // If the switcher has been initialized, check for button presses as reflect status of switcher in button lights:
  if (AtemSwitcher.hasInitialized())  {
    if (!AtemOnline)  {
      AtemOnline = true;
      //        // Set Bi-color LED to red or green depending on mode:
      //        digitalWrite(redLED, false);
      //        digitalWrite(greenLED, true);

      Disp163.clearDisplay();
      Disp163.gotoRowCol(0, 0);
      Disp163 << F("Connected");
      Disp163.gotoRowCol(1, 0);
      Disp163.print(AtemSwitcher.getProductIdName());
      Disp163.gotoRowCol(1, 11);
      //      Disp163.print("v.");
      //      Disp163.gotoRowCol(1, 13);
      //      Disp163.print(AtemSwitcher.getProtocolVersionMajor());
      //      Disp163.gotoRowCol(1, 15);
      //      Disp163.print(AtemSwitcher.getProtocolVersionMinor());
      previewSelect.setDefaultColor(5);  // Dimmed by default
      programSelect.setDefaultColor(5);  // Dimmed by default
      cmdSelect.setDefaultColor(5);  // Dimmed by default
      extraButtons.setDefaultColor(5);  // Dimmed by default

      previewSelect.setButtonColorsToDefault();
      programSelect.setButtonColorsToDefault();
      cmdSelect.setButtonColorsToDefault();
      extraButtons.setButtonColorsToDefault();

      SSWboard.clearDisplay();
      updateSSW[0] = true;
      updateSSW[1] = true;

    /**  if (!pipSet) {
        if (DVEpositionX != 0 || DVEpositionY != 0 || DVEsizeX != 0 || DVEsizeY != 0 || DVEborderSize != 0) {
          AtemSwitcher.setKeyDVEBorderBevel(0, 0, 0);
          AtemSwitcher.setKeyDVERotation(0, 0, 0);
          if (DVEborderSize != 0) {
            AtemSwitcher.setKeyDVEBorderEnabled(0, 0, true);
            AtemSwitcher.setKeyDVEBorderOuterWidth(0, 0, 10 * DVEborderSize);
            AtemSwitcher.setKeyDVEBorderInnerWidth(0, 0, 0);
          } else {
            AtemSwitcher.setKeyDVEBorderEnabled(0, 0, false);
          }
          AtemSwitcher.setKeyerType(0, 0, 3);
          AtemSwitcher.setKeyDVEPositionX(0, 0, 100 * DVEpositionX);
          AtemSwitcher.setKeyDVEPositionY(0, 0, -100 * DVEpositionY);
          AtemSwitcher.setKeyDVESizeX(0, 0, 10 * DVEsizeX);
          AtemSwitcher.setKeyDVESizeY(0, 0, 10 * DVEsizeY);
        }
        pipSet = true;
      }
*/
      lDelay(1000);

      Disp163.clearDisplay();
      Disp163.print("     KONTROL    ");
      Disp163.gotoRowCol(1, 0);
      Disp163 << F("Connected");
      Disp163.gotoRowCol(2, 0);
      Disp163.print(AtemSwitcher.getProductIdName());
      //      Disp163.gotoRowCol(2, 11);
      //      Disp163.print("v.");
      //      Disp163.gotoRowCol(2, 13);
      //      Disp163.print(AtemSwitcher.getProtocolVersionMajor());
      //      Disp163.gotoRowCol(2, 15);
      //      Disp163.print(AtemSwitcher.getProtocolVersionMinor());
      Disp163.gotoRowCol(0, 0);
      Disp163.print("     KONTROL    ");
    }


    setButtonColors();
    lDelay(0);

    readingButtonsAndSendingCommands();
    lDelay(0);

    extraButtonsCommands();
    lDelay(0);

    encoders.runLoop();
    lDelay(0);

    menuNavigation();
    menuValues();
    lDelay(0);

    smartSwitches();
    lDelay(0);
  }

  // If connection is gone, try to reconnect:
  else  {
    if (AtemOnline)  {
      AtemOnline = false;

      //        // Set Bi-color LED off = "no connection"
      //        digitalWrite(redLED, true);
      //        digitalWrite(greenLED, false);

      Disp163.clearDisplay();
      Disp163 << F("     ERROR     ");
      Disp163.gotoRowCol(1, 0);
      Disp163 << F("Connection Lost!");
      Disp163.gotoRowCol(2, 0);
      Disp163 << F("Reconnecting... ");

      SSWboard.clearDisplay();
      updateSSW[0] = true;
      updateSSW[1] = true;

      previewSelect.setDefaultColor(0);  // Off by default
      programSelect.setDefaultColor(0);  // Off by default
      cmdSelect.setDefaultColor(0);  // Off by default
      extraButtons.setDefaultColor(0);  // Off by default

      previewSelect.setButtonColorsToDefault();
      programSelect.setButtonColorsToDefault();
      cmdSelect.setButtonColorsToDefault();
      extraButtons.setButtonColorsToDefault();

      smartSwitches();
    }
  }
}




/*************************************************************
 *
 *
 *                     MENU SYSTEM
 *
 *
 **********************************************************/


uint8_t userButtonMode = 0;  // 0-3
uint8_t setMenuValues = 0;  // The value of this variable determines what the function "menuValues()" prints to the displays second line.
uint8_t BUSselect = 0;  // Preview/Program by default
bool menuSetValueMode = false;
// Configuration of the menu items and hierarchi plus call-back functions:
MenuBackend menu = MenuBackend(menuUseEvent, menuChangeEvent);
// Beneath is list of menu items needed to build the menu

// First argument: the "menu" object (created above), second argument: The text string, third argument: Menu level
MenuItem menu_UP1       = MenuItem(menu, "< Exit", 1);
MenuItem menu_aux       = MenuItem(menu, "AUX", 1);// On Change: Use it (and se as default)! On Use: Exit
  MenuItem menu_auxl1      = MenuItem(menu, "AUX 1", 2);  // (As Media Bank 1)
    MenuItem menu_aux1      = MenuItem(menu, "AUX 1", 3);
  MenuItem menu_auxl2      = MenuItem(menu, "AUX 2", 2);    // (As Media Bank 1)
    MenuItem menu_aux2      = MenuItem(menu, "AUX 2", 3);
  MenuItem menu_auxl3      = MenuItem(menu, "AUX 3", 2);  // (As Media Bank 1)
    MenuItem menu_aux3      = MenuItem(menu, "AUX 3", 3); 
MenuItem menu_mediab   = MenuItem(menu, "Media Bank", 1);
  MenuItem menu_mediab1   = MenuItem(menu, "Media Bank 1", 2);    // On Change: Show selected item/Value (2nd encoder rotates). On Use: N/A
  MenuItem menu_mediab2   = MenuItem(menu, "Media Bank 2", 2);    // (As Media Bank 1)
MenuItem menu_userbut   = MenuItem(menu, "User Buttons", 1);    // On Change: N/A. On Use: Show active configuration
  MenuItem menu_usrcfg1 = MenuItem(menu, "DSK1        DSK2MEDIA1    MEDIA2", 2);  // On Change: Use it (and se as default)! On Use: Exit
  MenuItem menu_usrcfg2 = MenuItem(menu, "DSK1        DSK2AUTO         PIP", 2);  // (As above)
  MenuItem menu_usrcfg3 = MenuItem(menu, "KEY1        KEY2KEY3        KEY4", 2);  // (As above)
  MenuItem menu_usrcfg4 = MenuItem(menu, "COLOR1    COLOR2BLACK       BARS", 2);  // (As above)
  MenuItem menu_usrcfg5 = MenuItem(menu, "AUX1        AUX2AUX3     PROGRAM", 2);  // (As above)
  MenuItem menu_usrcfg6 = MenuItem(menu, "DSK1     VGA+PIPAUTO         PIP", 2);  // (As above)
  MenuItem menu_usrcfg7 = MenuItem(menu, "USER1      USER2USER3      USER4", 2);  // (As above)
MenuItem menu_trans     = MenuItem(menu, "Transitions", 1);    // (As User Buttons)
  MenuItem menu_trtype  = MenuItem(menu, "Type", 2);  // (As Media Bank 1)
  MenuItem menu_trtime  = MenuItem(menu, "Trans. Time", 2);  // (As Media Bank 1)
MenuItem menu_ftb       = MenuItem(menu, "Fade To Black", 1);
  MenuItem menu_ftbtime = MenuItem(menu, "FTB Time", 2);
  MenuItem menu_ftbexec = MenuItem(menu, "Do Fade to Black", 2);

MenuItem menu_meSelect      = MenuItem(menu, "ATEM", 1);
MenuItem menu_dsktrans      = MenuItem(menu, "DSK Transition", 1);
MenuItem menu_panelIntensity      = MenuItem(menu, "Panel Intensity", 1);
MenuItem menu_UP2       = MenuItem(menu, "< Exit", 1);



// This function builds the menu and connects the correct items together
void menuSetup()
{
  Serial << F("Setting up menu...");

  // Add first item to the menu root:
  menu.getRoot().add(menu_aux);

  // Setup the rest of menu items on level 1:
  menu_aux.addBefore(menu_UP1);
  menu_aux.addAfter(menu_mediab);
  menu_mediab.addAfter(menu_userbut);
  menu_userbut.addAfter(menu_trans);
  menu_trans.addAfter(menu_ftb);
  menu_ftb.addAfter(menu_meSelect);
  menu_meSelect.addAfter(menu_dsktrans);
  menu_dsktrans.addAfter(menu_panelIntensity);
  menu_panelIntensity.addAfter(menu_UP2);

  // Set up aux menu (level 2):
  menu_auxl1.addAfter(menu_auxl2);
  menu_auxl2.addAfter(menu_auxl3);
  menu_auxl2.addLeft(menu_aux);
  menu_auxl3.addLeft(menu_aux);
  menu_aux.addRight(menu_auxl1);
  
  
  menu_auxl1.addRight(menu_aux1);
 
  
 
   // Set up media bank menu (level 2):
  menu_mediab1.addAfter(menu_mediab2);
  menu_mediab2.addLeft(menu_mediab);
  menu_mediab.addRight(menu_mediab1);
  
  menu_usrcfg1.addAfter(menu_usrcfg2);  // Chain subitems...
  menu_usrcfg2.addAfter(menu_usrcfg3);
  menu_usrcfg3.addAfter(menu_usrcfg4);
  menu_usrcfg4.addAfter(menu_usrcfg5);
  menu_usrcfg5.addAfter(menu_usrcfg6);
  menu_usrcfg6.addAfter(menu_usrcfg7);
  menu_usrcfg2.addLeft(menu_userbut);  // Add parent item - starting with number 2...
  menu_usrcfg3.addLeft(menu_userbut);
  menu_usrcfg4.addLeft(menu_userbut);
  menu_usrcfg5.addLeft(menu_userbut);
  menu_usrcfg6.addLeft(menu_userbut);
  menu_usrcfg7.addLeft(menu_userbut);
  menu_userbut.addRight(menu_usrcfg1);     // Add the submenu to the parent - this will also see "left" for "menu_usercfg1"

  // Set up transition menu:
  menu_trtype.addAfter(menu_trtime);    // Chain subitems...
  menu_trtime.addLeft(menu_trans);      // Add parent item
  menu_trans.addRight(menu_trtype);     // Add the submenu to the parent - this will also see "left" for "menu_trtype"

  // Set up fade-to-black menu:
  menu_ftbtime.addAfter(menu_ftbexec);    // Chain subitems...
  menu_ftbexec.addLeft(menu_ftb);      // Add parent item
  menu_ftb.addRight(menu_ftbtime);     // Add the submenu to the parent
}


void menuOperationReset()  {
  setMenuValues = 0;
  menuSetValueMode = false;
}
/*
  Here all use events are handled. Mainly these are used to navigate in to and out of menu items with the encoder button.
 */
void menuUseEvent(MenuUseEvent used)
{
 

  if (used.item.getName() == "MenuRoot")  {
    menu.moveDown();
     menuOperationReset();
    return;
  }

  // Exit in upper level:
  if (used.item.isEqual(menu_UP1) || used.item.isEqual(menu_UP2))  {
    menu.toRoot();
     menuOperationReset();
    return;
  }

  // This will set the selected element as default when entering the menu again.
  // PS: I don't know why I needed to put the "*" before "used.item.getLeft()" It was a lucky guess, or...?


  // Using an element moves left or right depending on where there are elements.
  // This works fine for a two level menu like this one.
  if ((bool)used.item.getRight())  {
    menu.moveRight();
  }
  else {
     menuSetValueMode = true;
     menuValues();
    
  }
}

/*
  Here we get a notification whenever the user changes the menu
 That is, when the menu is navigated
 */
void menuChangeEvent(MenuChangeEvent changed) {
   menuOperationReset();

  if (changed.to.getName() == "MenuRoot")  {
    // Show default text.... status whatever....
    Disp163.clearDisplay();
    //Disp163.gotoRowCol(0, 0);
    //Disp163 << F("SKAARHOJ C201");
    Disp163.gotoRowCol(1, 0);
    Disp163 << F("Connected");
    Disp163.gotoRowCol(2, 0);
    Disp163.print(AtemSwitcher.getProductIdName());
    //    Disp163.gotoRowCol(2, 9);
    //    Disp163.print(" v .   ");
    //    Disp163.gotoRowCol(2, 11);
    //    Disp163.print(AtemSwitcher.getProtocolVersionMajor());
    //    Disp163.gotoRowCol(2, 13);
    //    Disp163.print(AtemSwitcher.getProtocolVersionMinor());
    Disp163.gotoRowCol(0, 0);
    Disp163.print("     KONTROL    ");
    //    Disp163.gotoRowCol(1, 0);
    //    Disp163 << F("      C201      ");
    setMenuValues = 0;
  }
  else {
    // Show the item name in upper line:
     Disp163.clearDisplay();
      Disp163.gotoRowCol(0, 0);
      Disp163 << F("      MENU      ");
      
    Disp163.gotoRowCol(1, 0);
    Disp163 << (changed.to.getName());
    for (int i = strlen(changed.to.getName()); i < 16; i++)  {
      Disp163 << F(" ");
    }
    if (strlen(changed.to.getName()) > 16)  {
      Disp163.gotoRowCol(2, 0);
      for (int i = 16; i < strlen(changed.to.getName()); i++)  {
        Disp163 << changed.to.getName()[i];
      }
    }

    // If there are no menu items to the right, we assume its a value change:
    if (!(bool)changed.to.getRight())  {
     
     
      // Make settings as a consequence of menu selection:
      if (changed.to.getName() == menu_usrcfg1.getName())  {
        userButtonMode = 0;
      }
      if (changed.to.getName() == menu_usrcfg2.getName())  {
        userButtonMode = 1;
      }
      if (changed.to.getName() == menu_usrcfg3.getName())  {
        userButtonMode = 2;
      }
      if (changed.to.getName() == menu_usrcfg4.getName())  {
        userButtonMode = 3;
      }
      if (changed.to.getName() == menu_usrcfg5.getName())  {
        userButtonMode = 4;
      }
      if (changed.to.getName() == menu_usrcfg6.getName())  {
        userButtonMode = 5;
      }
      if (changed.to.getName() == menu_usrcfg7.getName())  {
        userButtonMode = 6;
      }
     
      
      
      if (changed.to.getName() == menu_mediab1.getName())  {
        setMenuValues = 1;
      }
      if (changed.to.getName() == menu_mediab2.getName())  {
        setMenuValues = 2;
   
   }
      if (changed.to.getName() == menu_aux1.getName())  {
        setMenuValues = 3;
      }
      if (changed.to.getName() == menu_aux2.getName())  {
        setMenuValues = 4;
      }
      if (changed.to.getName() == menu_aux3.getName())  {
        setMenuValues = 5;
      }
      if (changed.to.getName() == menu_panelIntensity.getName())  {
        setMenuValues = 6;
      }
      if (changed.to.getName() == menu_meSelect.getName())  {
        setMenuValues = 7;
      }
      if (changed.to.getName() == menu_dsktrans.getName())  {
        setMenuValues = 8;
      }
      if (changed.to.getName() == menu_trtype.getName())  {
        setMenuValues = 10;
      }
      if (changed.to.getName() == menu_trtime.getName())  {
        setMenuValues = 11;
      }
      if (changed.to.getName() == menu_ftbtime.getName())  {
        setMenuValues = 20;
      }
      if (changed.to.getName() == menu_ftbexec.getName())  {
        setMenuValues = 21;
      }
      // TODO: I HAVE to find another way to match the items here because two items with the same name will choke!!
       menuValues();

    }
    else {  // Just clear the displays second line if there are items to the right in the menu:
      //Disp163.gotoRowCol(1, 15);
      //Disp163 << F(">");  // Arrow + Clear the line...
      Disp163.gotoRowCol(2, 0);
      Disp163 << F("                ");  // Arrow + Clear the line...
    }
  }
}


/**********************************
 * Navigation for the main menu:
 ***********************************/


static int lastsVal = 1;
static bool ftbState = false;

void menuNavigation() {
  // Read upper encoder value
 
  int encValue = encoders.state(1, 1000);
   int lastCount =  encoders.lastCount(1);
  
  // Decide the action to take (navigation wise):
  if (!menuSetValueMode)  {
  
  switch (encValue)  {
    case 1:
      menu.moveDown();
 
      break;
    case -1:
      menu.moveUp();
    
      break;
    default:
      if (encValue>2)  {
        if (encValue<500)  {
          menu.use();
         
        }
        else {
          menu.toRoot();
      
        }
      }
      break;
  }
}

 else {
    // VALUE CHANGE:
    switch(encValue)  {


    }
 }
}
void menuValues()  {
int sVal; 

  // Read lower encoder value
  int encValue = encoders.state(1);
  int lastCount =  encoders.lastCount(1);

  switch (setMenuValues)  {
     
    case 1:  // Media bank 1 selector:
    case 2:  // Media bank 2 selector:
      sVal = AtemSwitcher.getMediaPlayerSourceStillIndex(setMenuValues - 1);

      if (encValue == 1 || encValue == -1)  {
        sVal += lastCount;
        if (sVal > 31) sVal = 0;
        if (sVal < 0) sVal = 31;
        AtemSwitcher.setMediaPlayerSourceStillIndex(setMenuValues - 1, sVal) ;
      }
      if (lastsVal != sVal) {
        Disp163.gotoRowCol(2, 0);
        Disp163 << F("Still ");
        menuValues_printValue(sVal + 1, 6, 3);
       
      }
      break;
     
    case 3:  // AUX 1
    case 4:  // AUX 2
    case 5:  // AUX 3
      if (AtemSwitcher.getAuxSourceInput(setMenuValues - 3) == 1000) {
        sVal = 9;
      }
      else if (AtemSwitcher.getAuxSourceInput(setMenuValues - 3) == 2001) {
        sVal = 10;
      }
      else if (AtemSwitcher.getAuxSourceInput(setMenuValues - 3) == 2002) {
        sVal = 11;
      }
      else if (AtemSwitcher.getAuxSourceInput(setMenuValues - 3) == 3010) {
        sVal = 12;
      }
      else if (AtemSwitcher.getAuxSourceInput(setMenuValues - 3) == 3011) {
        sVal = 13;
      }
      else if (AtemSwitcher.getAuxSourceInput(setMenuValues - 3) == 3020) {
        sVal = 14;
      }
      else if (AtemSwitcher.getAuxSourceInput(setMenuValues - 3) == 3021) {
        sVal = 15;
      }
      else if (AtemSwitcher.getAuxSourceInput(setMenuValues - 3) == 10010) {
        sVal = 16;
      }
      else if (AtemSwitcher.getAuxSourceInput(setMenuValues - 3) == 10011) {
        sVal = 17;
      }
      else if (AtemSwitcher.getAuxSourceInput(setMenuValues - 3) == 7001) {
        sVal = 18;
      }
      else if (AtemSwitcher.getAuxSourceInput(setMenuValues - 3) == 7002) {
        sVal = 19;
      }
      else {
        sVal = AtemSwitcher.getAuxSourceInput(setMenuValues - 3);
      }
      if (encValue == 1 || encValue == -1)  {
        sVal += lastCount;
        if (sVal > 19) sVal = 0;
        if (sVal < 0) sVal = 19;
        if (sVal == 9) {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, 1000);
        }
        else if (sVal == 10) {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, 2001);
        }
        else if (sVal == 11) {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, 2002);
        }
        else if (sVal == 12) {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, 3010);
        }
        else if (sVal == 13) {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, 3011);
        }
        else if (sVal == 14) {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, 3020);
        }
        else if (sVal == 15) {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, 3021);
        }
        else if (sVal == 16) {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, 10010);
        }
        else if (sVal == 17) {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, 10011);
        }
        else if (sVal == 18) {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, 7001);
        }
        else if (sVal == 19) {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, 7002);
        }
        else {
          AtemSwitcher.setAuxSourceInput(setMenuValues - 3, sVal);
        }
        menuValues_clearValueLine();
      }
      if (lastsVal != sVal) {
        Disp163.gotoRowCol(2, 0);
        menuValues_printValueName(sVal);
        lastsVal = sVal;
      }
      break;
    case 6:
      if (encValue == 1 && panelIntensityLevel < 20)  {
        panelIntensityLevel++;
        EEPROM.write(399, panelIntensityLevel);
        setPanelIntensity();
      }
      if (encValue == -1 && panelIntensityLevel > 0)  {
        panelIntensityLevel--;
        EEPROM.write(399, panelIntensityLevel);
        setPanelIntensity();
      }
      if (lastsVal != panelIntensityLevel) {
        Disp163.gotoRowCol(2, 0);
        Disp163 << F("Level: ") << panelIntensityLevel << F(" ");
        lastsVal = panelIntensityLevel;
      }
      break;
    case 7:
      if (encValue == 1 && MEselectForInputbuttons < 1 && AtemSwitcher.getTopologyMEs() == 2)  {
        MEselectForInputbuttons = 1;
        BUSselect = 5;
        hyperdeckControl = false;
        updateSSW[1] = true;
      }
      if (encValue == -1 && MEselectForInputbuttons > 0)  {
        MEselectForInputbuttons = 0;
        BUSselect = 0;
        hyperdeckControl = false;
        updateSSW[1] = true;
      }
      if (lastsVal != MEselectForInputbuttons) {
        Disp163.gotoRowCol(2, 0);
        Disp163 << F("ME: ") << MEselectForInputbuttons + 1 << F("           ");
        lastsVal = MEselectForInputbuttons;
        updateSSW[1] = true;
      }
      break;
    case 8:
      if (encValue == 1 && autoDSK < 1)  {
        autoDSK++;
        EEPROM.write(90, autoDSK);
      }
      if (encValue == -1 && autoDSK > 0)  {
        autoDSK--;
        EEPROM.write(90, autoDSK);
      }
      if (lastsVal != autoDSK) {
        Disp163.gotoRowCol(2, 0);
        if (autoDSK == 1) {
          Disp163 << F("Auto            ");
        } else {
          Disp163 << F("On/Off          ");
        }
        lastsVal = autoDSK;
      }
      break;
    case 10:  // Transition: Type
      sVal = AtemSwitcher.getTransitionStyle(MEselectForInputbuttons);
      if (encValue == 1 || encValue == -1)  {
        sVal += encValue;
        if (sVal > 4) sVal = 0;
        if (sVal < 0) sVal = 4;
        AtemSwitcher.setTransitionStyle(MEselectForInputbuttons, sVal);
        menuValues_clearValueLine();
      }
      if (lastsVal != sVal) {
        Disp163.gotoRowCol(2, 0);
        menuValues_printTrType(sVal);
        lastsVal = sVal;
      }
      break;
    case 11:  // Transition: Time
      switch (AtemSwitcher.getTransitionStyle(MEselectForInputbuttons)) {
        case 0:
          sVal = AtemSwitcher.getTransitionMixRate(MEselectForInputbuttons);
          if (encValue == 1 || encValue == -1)  {
            sVal += (int)encValue + (abs(lastCount) - 1) * 5 * encValue;
            if (sVal > 0xFA) sVal = 1;
            if (sVal < 1) sVal = 0xFA;
            AtemSwitcher.setTransitionMixRate(MEselectForInputbuttons, sVal);
            menuValues_clearValueLine();
          }
          if (lastsVal != sVal) {
            Disp163.gotoRowCol(2, 0);
            Disp163 << F("Frames: ");
            menuValues_printValue(sVal, 8, 3);
            Disp163 << F("    ");
            lastsVal = sVal;
          }
          break;
        case 1:
          sVal = AtemSwitcher.getTransitionDipRate(MEselectForInputbuttons);
          if (encValue == 1 || encValue == -1)  {
            sVal += (int)encValue + (abs(lastCount) - 1) * 5 * encValue;
            if (sVal > 0xFA) sVal = 1;
            if (sVal < 1) sVal = 0xFA;
            AtemSwitcher.setTransitionDipRate(MEselectForInputbuttons, sVal);
            menuValues_clearValueLine();
          }
          if (lastsVal != sVal) {
            Disp163.gotoRowCol(2, 0);
            Disp163 << F("Frames: ");
            menuValues_printValue(sVal, 8, 3);
            Disp163 << F("    ");
            lastsVal = sVal;
          }
          break;
        case 2:
          sVal = AtemSwitcher.getTransitionWipeRate(MEselectForInputbuttons);
          if (encValue == 1 || encValue == -1)  {
            sVal += (int)encValue + (abs(lastCount) - 1) * 5 * encValue;
            if (sVal > 0xFA) sVal = 1;
            if (sVal < 1) sVal = 0xFA;
            AtemSwitcher.setTransitionWipeRate(MEselectForInputbuttons, sVal);
            menuValues_clearValueLine();
          }
          if (lastsVal != sVal) {
            Disp163.gotoRowCol(2, 0);
            Disp163 << F("Frames: ");
            menuValues_printValue(sVal, 8, 3);
            Disp163 << F("    ");
            lastsVal = sVal;
          }
          break;
        case 3:
          sVal = AtemSwitcher.getTransitionDVERate(MEselectForInputbuttons);
          if (encValue == 1 || encValue == -1)  {
            sVal += (int)encValue + (abs(lastCount) - 1) * 5 * encValue;
            if (sVal > 0xFA) sVal = 1;
            if (sVal < 1) sVal = 0xFA;
            AtemSwitcher.setTransitionDVERate(MEselectForInputbuttons, sVal);
            menuValues_clearValueLine();
          }
          if (lastsVal != sVal) {
            Disp163.gotoRowCol(2, 0);
            Disp163 << F("Frames: ");
            menuValues_printValue(sVal, 8, 3);
            Disp163 << F("    ");
            lastsVal = sVal;
          }
          break;
        case 4:
          sVal = AtemSwitcher.getTransitionStingerMixRate(MEselectForInputbuttons);
          if (encValue == 1 || encValue == -1)  {
            sVal += (int)encValue + (abs(lastCount) - 1) * 5 * encValue;
            if (sVal > 0xFA) sVal = 1;
            if (sVal < 1) sVal = 0xFA;
            AtemSwitcher.setTransitionStingerMixRate(MEselectForInputbuttons, sVal);
            menuValues_clearValueLine();
          }
          if (lastsVal != sVal) {
            Disp163.gotoRowCol(2, 0);
            Disp163 << F("Frames: ");
            menuValues_printValue(sVal, 8, 3);
            Disp163 << F("    ");
            lastsVal = sVal;
          }
          break;
      }
      break;
    case 20:  // Fade-to-black: Time
      sVal = AtemSwitcher.getFadeToBlackRate(MEselectForInputbuttons);
      if (encValue == 1 || encValue == -1)  {
        sVal += (int)encValue + (abs(lastCount) - 1) * 5 * encValue;
        if (sVal > 0xFA) sVal = 1;
        if (sVal < 1) sVal = 0xFA;
        AtemSwitcher.setFadeToBlackRate(MEselectForInputbuttons, sVal);
        menuValues_clearValueLine();
      }
      if (lastsVal != sVal) {
        Disp163.gotoRowCol(2, 0);
        Disp163 << F("Frames: ");
        menuValues_printValue(sVal, 8, 3);
        Disp163 << F("    ");
        lastsVal = sVal;
      }
      break;
    case 21:  // Fade-to-black: Execute
      if (!ftbState) {
        Disp163.gotoRowCol(2, 0);
        Disp163 << F("Press to execute");
        ftbState = true;
      }
      if (encValue > 2 && encValue < 1000)  {
        AtemSwitcher.performFadeToBlackME(MEselectForInputbuttons);
      }
      break;
    default:
      break;
  }
}

void menuValues_clearValueLine()  {
  Disp163.gotoRowCol(2, 0);
  Disp163 << F("                ");
}
void menuValues_printValue(int number, uint8_t pos, uint8_t padding)  {
  Disp163.gotoRowCol(2, pos);
  Disp163 << number;
  for (int i = String(number).length(); i < padding; i++)  {
    Disp163 << F(" ");
  }
}
void menuValues_printValueName(int sVal)  {
  switch (sVal)  {
    case 0:
      if (lastsVal != sVal) {
        Disp163 << F("Black           ");
        lastsVal = sVal;
      }
      break;
    case 1:
      if (lastsVal != sVal) {
        Disp163 << F("Camera 1        ");
        lastsVal = sVal;
      }
      break;
    case 2:
      if (lastsVal != sVal) {
        Disp163 << F("Camera 2        ");
        lastsVal = sVal;
      }
      break;
    case 3:
      if (lastsVal != sVal) {
        Disp163 << F("Camera 3        ");
        lastsVal = sVal;
      }
      break;
    case 4:
      if (lastsVal != sVal) {
        Disp163 << F("Camera 4        ");
        lastsVal = sVal;
      }
      break;
    case 5:
      if (lastsVal != sVal) {
        Disp163 << F("Camera 5        ");
        lastsVal = sVal;
      }
      break;
    case 6:
      if (lastsVal != sVal) {
        Disp163 << F("Camera 6        ");
        lastsVal = sVal;
      }
      break;
    case 7:
      if (lastsVal != sVal) {
        Disp163 << F("Camera 7        ");
        lastsVal = sVal;
      }
      break;
    case 8:
      if (lastsVal != sVal) {
        Disp163 << F("Camera 8        ");
        lastsVal = sVal;
      }
      break;
    case 9:
      if (lastsVal != sVal) {
        Disp163 << F("Color Bars      ");
        lastsVal = sVal;
      }
      break;
    case 10:
      if (lastsVal != sVal) {
        Disp163 << F("Color 1         ");
        lastsVal = sVal;
      }
      break;
    case 11:
      if (lastsVal != sVal) {
        Disp163 << F("Color 2         ");
        lastsVal = sVal;
      }
      break;
    case 12:
      if (lastsVal != sVal) {
        Disp163 << F("Media Player 1  ");
        lastsVal = sVal;
      }
      break;
    case 13:
      if (lastsVal != sVal) {
        Disp163 << F("Media Play.1 key");
        lastsVal = sVal;
      }
      break;
    case 14:
      if (lastsVal != sVal) {
        Disp163 << F("Media Player 2  ");
        lastsVal = sVal;
      }
      break;
    case 15:
      if (lastsVal != sVal) {
        Disp163 << F("Media Play.2 key");
        lastsVal = sVal;
      }
      break;
    case 16:
      if (lastsVal != sVal) {
        Disp163 << F("Program         ");
        lastsVal = sVal;
      }
      break;
    case 17:
      if (lastsVal != sVal) {
        Disp163 << F("Preview         ");
        lastsVal = sVal;
      }
      break;
    case 18:
      if (lastsVal != sVal) {
        Disp163 << F("Clean Feed 1    ");
        lastsVal = sVal;
      }
      break;
    case 19:
      if (lastsVal != sVal) {
        Disp163 << F("Clean Feed 2    ");
        lastsVal = sVal;
      }
      break;
    default:
      if (lastsVal != sVal) {
        Disp163 << F("N/A             ");
        lastsVal = sVal;
      }
      break;
  }
}
void menuValues_printTrType(int sVal)  {
  switch (sVal)  {
    case 0:
      if (lastsVal != sVal) {
        Disp163 << F("Mix             ");
        lastsVal = sVal;
      }
      break;
    case 1:
      if (lastsVal != sVal) {
        Disp163 << F("Dip             ");
        lastsVal = sVal;
      }
      break;
    case 2:
      if (lastsVal != sVal) {
        Disp163 << F("Wipe            ");
        lastsVal = sVal;
      }
      break;
    case 3:
      if (lastsVal != sVal) {
        Disp163 << F("DVE             ");
        lastsVal = sVal;
      }
      break;
    case 4:
      if (lastsVal != sVal) {
        Disp163 << F("Sting           ");
        lastsVal = sVal;
      }
      break;
    default:
      if (lastsVal != sVal) {
        Disp163 << F("N/A             ");
        lastsVal = sVal;
      }
      break;
  }
}







uint8_t audioExt[] = {2001, 2002, 1001, 1101, 1201, 0, 0, 0};

/**********************************
 * Button State Colors set:
 ***********************************/
void setButtonColors()  {

  // Setting colors of PREVIEW input select buttons:
  for (uint16_t i = 1; i <= 8; i++)  {
    switch (BUSselect) {
      case 1:
      case 2:
      case 3:
      case 4:
      case 8:
        if (shift) {
          previewSelect.setButtonColor(i, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == i + 8 ? 2 : AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == i + 8 ? 3 : 5);
        } else {
          previewSelect.setButtonColor(i, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == i ? 2 : AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == i ? 3 : 5);
        }
        break;
      case 0:
      case 5:
      case 11:
        if (shift) {
          previewSelect.setButtonColor(i, AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == i + 8 ? 3 : 5);
        } else {
          previewSelect.setButtonColor(i, AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == i ? 3 : 5);
        }
        break;
      case 6:
        if (shift) {
          previewSelect.setButtonColor(i, AtemSwitcher.getAudioMixerInputMixOption(i + 8) == 2 ? 3 : 5);
        } else {
          previewSelect.setButtonColor(i, AtemSwitcher.getAudioMixerInputMixOption(i) == 2 ? 3 : 5);
        }
        break;
      case 7:
        if (i < 3) {
          previewSelect.setButtonColor(i, AtemSwitcher.getAudioMixerInputMixOption(audioExt[i - 1]) == 2 ? 3 : 5);
        } else {
          previewSelect.setButtonColor(i, 0);
        }
        break;
     /** case 9:  // 9=VIDEOHUB:IN|OUT(s)
        if (Videohub.hasInitialized())  {
          Videohub.getRoute(i + (shift ? 1 : 0) * 8) == videoHubIn ? previewSelect.setButtonColor(i, 4) : previewSelect.setButtonColor(i, 5);
        } else {
          previewSelect.setButtonColor(i, 0);
        }
        break;
      case 10:  // 10=VIDEOHUB:OUT|IN
        if (Videohub.hasInitialized())  {
          Videohub.getRoute(videoHubOut) == i + (shift ? 1 : 0) * 8 ? previewSelect.setButtonColor(i, 4) : previewSelect.setButtonColor(i, 5);
        } else {
          previewSelect.setButtonColor(i, 0);
        }
        break;
      */  
    }
  }

  // Setting colors of PROGRAM input select buttons:
  for (uint16_t i = 1; i <= 10; i++)  {
    switch (BUSselect) {
      case 0:
      case 5:
      case 11:
        if (shift) {
          programSelect.setButtonColor(i, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == i + 10 ? 2 : 5);
        } else {
          programSelect.setButtonColor(i, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == i ? 2 : 5);
         
          programSelect.setButtonColor(11, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 3010 ? 2 : 5); // Media 1
          
          programSelect.setButtonColor(12, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 3020 ? 2 : 5); // Media 2 
        }
        break;
      case 1:
      case 2:
      case 3:
        if (shift) {
          programSelect.setButtonColor(i, AtemSwitcher.getAuxSourceInput(BUSselect - 1) == i + 10 ? 4 : 5);
        } else {
          programSelect.setButtonColor(i, AtemSwitcher.getAuxSourceInput(BUSselect - 1) == i ? 4 : 5);
        }
        break;
      case 6:
        if (shift) {
          programSelect.setButtonColor(i, AtemSwitcher.getAudioMixerInputMixOption(i + 10) == 1 ? 2 : 5);
        } else {
          programSelect.setButtonColor(i, AtemSwitcher.getAudioMixerInputMixOption(i) == 1 ? 2 : 5);
          
        }
        break;
      case 7:
        if (i > 2 && i < 6) {
          programSelect.setButtonColor(i, AtemSwitcher.getAudioMixerInputPlugtype(audioExt[i - 1]) == 0 ? 0 : AtemSwitcher.getAudioMixerInputMixOption(audioExt[i - 1]) == 1 ? 2 : 5);
        } else {
          programSelect.setButtonColor(i, 0);
        }
        break;
      case 8:
        if (shift) {
          programSelect.setButtonColor(i, AtemSwitcher.getKeyerFillSource(MEselectForInputbuttons, 0) == i + 10 ? 4 : 5);
        } else {
          programSelect.setButtonColor(i, AtemSwitcher.getKeyerFillSource(MEselectForInputbuttons, 0) == i ? 4 : 5);
        }
        break;
        /**
      case 9: // 9=VIDEOHUB:IN|OUT(s)
        if (Videohub.hasInitialized())  {
          videoHubIn == (i + (shift ? 1 : 0) * 8) ? programSelect.setButtonColor(i, 4) : programSelect.setButtonColor(i, 5);
        } else {
          programSelect.setButtonColor(i, 0);
        }
        break;
      case 10:  // 10=VIDEOHUB:OUT|IN
        if (Videohub.hasInitialized())  {
          videoHubOut == (i + (shift ? 1 : 0) * 8) ? programSelect.setButtonColor(i, 4) : programSelect.setButtonColor(i, 5);
        } else {
          programSelect.setButtonColor(i, 0);
        }
        break;
        */
           // case 10:
             
      // break;
        //      case 11:
        //        if (shift) {
        //          programSelect.setButtonColor(i, AtemSwitcher.getMediaPlayerSourceStillIndex(BUSselect - 10) == i + 8 - 1 ? 4 : 5);
        //        } else {
        //          programSelect.setButtonColor(i, AtemSwitcher.getMediaPlayerSourceStillIndex(BUSselect - 10) == i - 1 ? 4 : 5);
        //        }
        //        break;
    }
  }

  // The user button mode tells us, how the four user buttons should be programmed. This sets the colors to match the function:
  switch (userButtonMode)  {
    case 1:
      // Setting colors of the command buttons:
      cmdSelect.setButtonColor(5, AtemSwitcher.getDownstreamKeyerOnAir(MEselectForInputbuttons) ? 2 : 5);    // DSK1 button
      cmdSelect.setButtonColor(6, AtemSwitcher.getDownstreamKeyerOnAir(MEselectForInputbuttons) ? 2 : 5);    // DSK2 button
      cmdSelect.setButtonColor(3, AtemSwitcher.getTransitionPosition(MEselectForInputbuttons) > 0 ? 2 : 5);   // Auto button
      cmdSelect.setButtonColor(8, AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0) ? 4 : 5);     // PIP button
      break;
    case 2:
      // Setting colors of the command buttons:
      cmdSelect.setButtonColor(5, AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0) ? 4 : 5);     // Key1
      cmdSelect.setButtonColor(6, AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 1) ? 4 : 5);     // Key2
      cmdSelect.setButtonColor(3, AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 2) ? 4 : 5);     // Key3
      cmdSelect.setButtonColor(8, AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 3) ? 4 : 5);     // Key4
      break;
    case 3:
      // Setting colors of the command buttons:
      cmdSelect.setButtonColor(5, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 2001 ? 2 : (AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == 2001 ? 3 : 5)); // Color1
      cmdSelect.setButtonColor(6, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 2002 ? 2 : (AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == 2002 ? 3 : 5)); // Color2
      cmdSelect.setButtonColor(3, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 0 ? 2 : (AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == 0 ? 3 : 5)); // Black
      cmdSelect.setButtonColor(8, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 1000 ? 2 : (AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == 1000 ? 3 : 5)); // Bars
      break;
    case 4:
      // Setting colors of the command buttons:
      cmdSelect.setButtonColor(5, BUSselect == 1 ? 2 : 5);   // AUX1
      cmdSelect.setButtonColor(6, BUSselect == 2 ? 2 : 5);   // AUX2
      cmdSelect.setButtonColor(3, BUSselect == 3 ? 2 : 5);   // AUX3
      cmdSelect.setButtonColor(8, BUSselect == 0 ? 2 : 5);   // Program
      break;
    case 5:
      // Setting colors of the command buttons:
      cmdSelect.setButtonColor(5, AtemSwitcher.getDownstreamKeyerOnAir(MEselectForInputbuttons) ? 2 : 5);    // DSK1 button
      cmdSelect.setButtonColor(6, preVGA_active ? 4 : 5);     // VGA+PIP button
      cmdSelect.setButtonColor(3, AtemSwitcher.getTransitionPosition(MEselectForInputbuttons) > 0 ? 2 : 5);   // Auto button
      cmdSelect.setButtonColor(8, AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0) ? 4 : 5);     // PIP button
      break;
    case 6:
      cmdSelect.setButtonColor(5, setColor(1));    // USER1 button
      cmdSelect.setButtonColor(6, setColor(2));    // USER2 button
      cmdSelect.setButtonColor(3, setColor(3)); // USER3 button
      cmdSelect.setButtonColor(8, setColor(4)); // USER4 button
      break;
    default:
      cmdSelect.setButtonColor(5, AtemSwitcher.getDownstreamKeyerOnAir(0) ? 2 : 5);    // DSK1 button
      cmdSelect.setButtonColor(6, AtemSwitcher.getDownstreamKeyerOnAir(1) ? 2 : 5);    // DSK2 button
      cmdSelect.setButtonColor(3, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 3010 ? 2 : (AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == 3010 ? 3 : 5)); // Media 1
      cmdSelect.setButtonColor(8, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 3020 ? 2 : (AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == 3020 ? 3 : 5)); // Media 2
      break;
  }

  if (!cmdSelect.buttonIsPressed(1))  {
    cmdSelect.setButtonColor(1, 5);
 cmdSelect.setButtonColor(11, 5);
cmdSelect.setButtonColor(12, 5); // de-highlight CUT button
  }
  
  cmdSelect.setButtonColor(2, AtemSwitcher.getTransitionPosition(MEselectForInputbuttons) > 0 ? 2 : 5);   // Auto button
  if (AtemSwitcher.getFadeToBlackStateInTransition(MEselectForInputbuttons))  {  // Setting button color. This is a more complex example which includes blinking during execution:
    if (AtemSwitcher.getFadeToBlackStateFullyBlack(MEselectForInputbuttons) == 0 && (AtemSwitcher.getFadeToBlackStateFramesRemaining(MEselectForInputbuttons) != AtemSwitcher.getFadeToBlackRate(MEselectForInputbuttons)))  { // It's important to test if fadeToBlack time is more than zero because it's the kind of state from the ATEM which is usually not captured during initialization. Hopefull this will be solved in the future.
      // Blinking with AMBER color if Fade To Black is executing:
      cmdSelect.setButtonColor(7, 2);  // Sets color of button to RED (2) if Fade To Black is activated
    }
  }
  else if (AtemSwitcher.getFadeToBlackStateFullyBlack(MEselectForInputbuttons)) {
    if ((unsigned long)millis() & B10000000)  {
      cmdSelect.setButtonColor(7, 2);
    } else {
      cmdSelect.setButtonColor(7, 5);
    }
  } else {
    cmdSelect.setButtonColor(7, 5);  // Dimmed background if no fade to black
  }

  cmdSelect.setButtonColor(4, shift ? 4 : 5);     // SHIFT button
  //cmdSelect.setButtonColor(4, AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0) ? 4 : 5);     // PIP button
  //  cmdSelect.setButtonColor(7, AtemSwitcher.getDownstreamKeyerOnAir(0) ? 4 : 5);    // DSK1 button
  //  cmdSelect.setButtonColor(4, AtemSwitcher.getDownstreamKeyerOnAir(1) ? 4 : 5);    // DSK2 button
}









/**********************************
 * ATEM Commands
 ***********************************/
void readingButtonsAndSendingCommands() {

  // Sending commands for PREVIEW input selection:
  uint16_t busSelection = previewSelect.buttonDownAll();

  for (uint16_t i = 1; i <= 10; i++)  {
    if (previewSelect.isButtonIn(i, busSelection))  {
      switch (BUSselect) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 8:
        case 11:
          if (shift) {
            AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, i + 10);
          } else {
            AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, i);
          }
          break;
        case 6:
          if (shift) {
            AtemSwitcher.setAudioMixerInputMixOption((i + 10), AtemSwitcher.getAudioMixerInputMixOption(i + 10) == 2 ? 0 : 2);
          } else {
            AtemSwitcher.setAudioMixerInputMixOption((i), AtemSwitcher.getAudioMixerInputMixOption(i) == 2 ? 0 : 2);
          }
          break;
        case 7:
          if (i < 3) {
            AtemSwitcher.setAudioMixerInputMixOption((audioExt[i - 1]), AtemSwitcher.getAudioMixerInputMixOption(audioExt[i - 1]) == 2 ? 0 : 2);
          } else if (i > 2 && i < 6 && AtemSwitcher.getAudioMixerInputPlugtype(audioExt[i - 1]) != 0) {
            AtemSwitcher.setAudioMixerInputMixOption((audioExt[i - 1]), AtemSwitcher.getAudioMixerInputMixOption(audioExt[i - 1]) == 2 ? 0 : 2);
          }
          break;
          /**
        case 9:
          if (Videohub.hasInitialized())  {
            Videohub.routeInputToOutput(videoHubIn, i + (shift ? 1 : 0) * 8);
          }
          break;
        case 10:
          if (Videohub.hasInitialized())  {
            Videohub.routeInputToOutput(i + (shift ? 1 : 0) * 8, videoHubOut);
          }
          break;
          */
      }
    }
  }

  // Sending commands for PROGRAM input selection:
  busSelection = programSelect.buttonDownAll();
  switch (BUSselect) {
    case 0:
    case 5:
    case 11:
      for (uint16_t i = 1; i <= 10; i++)  {
        if (programSelect.isButtonIn(i, busSelection))  {
          if (shift) {
            AtemSwitcher.setProgramInputVideoSource(MEselectForInputbuttons, i + 10);
          } else {
            AtemSwitcher.setProgramInputVideoSource(MEselectForInputbuttons, i);
          }
        }
         
          if  (programSelect.isButtonIn(11, busSelection))  
        AtemSwitcher.setProgramInputVideoSource(MEselectForInputbuttons, 3010);
        } if  (programSelect.isButtonIn(12, busSelection))  
        AtemSwitcher.setProgramInputVideoSource(MEselectForInputbuttons, 3020);
      
        
          
        
      
      break;
    case 1:
    case 2:
    case 3:
      for (uint16_t i = 1; i <= 10; i++)  {
        if (programSelect.isButtonIn(i, busSelection))  {
          if (shift) {
            AtemSwitcher.setAuxSourceInput(BUSselect - 1, i + 10);
          } else {
            AtemSwitcher.setAuxSourceInput(BUSselect - 1, i);
          }
        }
      }
      break;
    case 4:  // 4=Macro:Play(Record/Delete on hold)|Prv/Prog
      if (AtemSwitcher.hasInitialized())  {
        for (uint16_t i = 0; i < 8; i++)  {
          uint16_t j = shift ? i + 8 : i;
          if (AtemSwitcher.getMacroPropertiesIsUsed(j) && !AtemSwitcher.getMacroRecordingStatusIsRecording())  {  // There is a macro and nothing is recording...
            if (AtemSwitcher.getMacroRunStatusState())  {  // Something is running...
              if (AtemSwitcher.getMacroRunStatusIndex() == j)  {
                if (AtemSwitcher.getMacroRunStatusState() & B10)  {  // Waiting...
                  if (programSelect.isButtonIn(i + 1, busSelection))  {
                    AtemSwitcher.setMacroAction(0xffff, 4);  // Continue
                  }
                  programSelect.setButtonColor(i + 1, millis() & 512 ? 5 : 4);
                } else {  // Just running:
                  if (programSelect.isButtonIn(i + 1, busSelection))  {
                    AtemSwitcher.setMacroAction(0xffff, 1);  // Stop macro...
                  }
                  programSelect.setButtonColor(i + 1, 2);
                }
              } else {
                programSelect.setButtonColor(i + 1, 0);
              }
            } else {  // All is stopped:
              if (programSelect.isButtonIn(i + 1, busSelection))  {
                AtemSwitcher.setMacroAction(j, 0); // Run macro...
              }
              programSelect.setButtonColor(i + 1, 4);
            }
          } else {
            programSelect.setButtonColor(i + 1, 0);
          }
        }
      } else for (uint16_t i = 0; i < 10; i++)  programSelect.setButtonColor(i + 1, 0);
      break;
    case 6:
      for (uint16_t i = 1; i <= 10; i++)  {
        if (programSelect.isButtonIn(i, busSelection))  {
          if (shift) {
            AtemSwitcher.setAudioMixerInputMixOption((i + 10), AtemSwitcher.getAudioMixerInputMixOption(i + 10) == 1 ? 0 : 1);
          } else {
            AtemSwitcher.setAudioMixerInputMixOption((i), AtemSwitcher.getAudioMixerInputMixOption(i) == 1 ? 0 : 1);
          }
        }
      }
      break;
    case 7:
      for (uint16_t i = 1; i <= 10; i++)  {
        if (programSelect.isButtonIn(i, busSelection))  {
          if (i < 3) {
            AtemSwitcher.setAudioMixerInputMixOption((audioExt[i - 1]), AtemSwitcher.getAudioMixerInputMixOption(audioExt[i - 1]) == 1 ? 0 : 1);
          } else if (i > 2 && i < 6 && AtemSwitcher.getAudioMixerInputPlugtype(audioExt[i - 1]) != 0) {
            AtemSwitcher.setAudioMixerInputMixOption((audioExt[i - 1]), AtemSwitcher.getAudioMixerInputMixOption(audioExt[i - 1]) == 1 ? 0 : 1);
          }
        }
      }
      break;
    case 8:
      for (uint16_t i = 1; i <= 10; i++)  {
        if (programSelect.isButtonIn(i, busSelection))  {
          if (shift) {
            AtemSwitcher.setKeyerFillSource(MEselectForInputbuttons, 0, (i + 10));
          } else {
            AtemSwitcher.setKeyerFillSource(MEselectForInputbuttons, 0, (i));
          }
        }
      }
      break;
      /*
    case 9:  // 9=VIDEOHUB:IN|OUT(s)
      if (Videohub.hasInitialized())  {
        for (uint16_t i = 1; i <= 8; i++)  {
          if (programSelect.isButtonIn(i, busSelection))  {
            videoHubIn = i + (shift ? 1 : 0) * 8;
          }
        }
      }
      break;
    case 10:  // 10=VIDEOHUB:OUT|IN
      if (Videohub.hasInitialized())  {
        for (uint16_t i = 1; i <= 8; i++)  {
          if (programSelect.isButtonIn(i, busSelection))  {
            videoHubOut = i + (shift ? 1 : 0) * 8;
          }
        }
      }
      break;
      */
         case 10:
         for (uint16_t i = 1; i = 11; i++)  {
           if (programSelect.isButtonIn(i, busSelection))  {
        AtemSwitcher.setProgramInputVideoSource(MEselectForInputbuttons, 3010);
      }
      for (uint16_t i = 1; i = 12; i++)  {
        if (programSelect.isButtonIn(i, busSelection))  {
        AtemSwitcher.setProgramInputVideoSource(MEselectForInputbuttons, 3020);
      }
      }
         }
      break;
      //    case 11:
      
  }

  // "T-bar" slider:
  // if (utils.uniDirectionalSlider_hasMoved())  {
  //  AtemSwitcher.setTransitionPosition(MEselectForInputbuttons, 10 * utils.uniDirectionalSlider_position());
  //  lDelay(20);
  //  if (utils.uniDirectionalSlider_isAtEnd())  {
  //    AtemSwitcher.setTransitionPosition(MEselectForInputbuttons, 0);
  //    lDelay(5);
  //  }
 // }

  // Cut button:
  uint16_t cmdSelection = cmdSelect.buttonDownAll();
  if (cmdSelection & (B1 << 0))  {
    cmdSelect.setButtonColor(1, 4);
 cmdSelect.setButtonColor(11, 4);
 cmdSelect.setButtonColor(12, 4); // Highlight CUT button
    AtemSwitcher.performCutME(MEselectForInputbuttons);
    preVGA_active = false;
  }
  
   if (cmdSelection & (B1 << 10))  {
    cmdSelect.setButtonColor(11, 4);
     cmdSelect.setButtonColor(1, 4);
     AtemSwitcher.performCutME(MEselectForInputbuttons);
    preVGA_active = false;
  }
  
  if (cmdSelection & (B1 << 11))  {
    cmdSelect.setButtonColor(12, 4);
     cmdSelect.setButtonColor(1, 4);
     AtemSwitcher.performCutME(MEselectForInputbuttons);
    preVGA_active = false;
  }

  // Auto button:
  if (cmdSelection & (B1 << 1))  {
    AtemSwitcher.performAutoME(MEselectForInputbuttons);
    preVGA_active = false;
  }
  
 

  // DSK1 button:
  if (cmdSelection & (B1 << 6))  {
    AtemSwitcher.performFadeToBlackME(MEselectForInputbuttons);
    //AtemSwitcher.setDownstreamKeyerOnAir(0, !AtemSwitcher.getDownstreamKeyerOnAir(0));
  }

  shift = cmdSelect.buttonIsPressed(4);
  // DSK2 button:
  //  if (cmdSelection & (B1 << 3))  {
  //    //cmd_pipToggle();
  //    //AtemSwitcher.setDownstreamKeyerOnAir(1, !AtemSwitcher.getDownstreamKeyerOnAir(1));
  //  }

  switch (userButtonMode)  {
    case 1:
      if (cmdSelection & (B1 << 4))  {
        AtemSwitcher.setDownstreamKeyerOnAir(0, !AtemSwitcher.getDownstreamKeyerOnAir(0));
      }  // DSK1
      if (cmdSelection & (B1 << 5))  {
        AtemSwitcher.setDownstreamKeyerOnAir(1, !AtemSwitcher.getDownstreamKeyerOnAir(1));
      }  // DSK1
      if (cmdSelection & (B1 << 2))  {
        AtemSwitcher.performAutoME(MEselectForInputbuttons);
        preVGA_active = false;
      }
      if (cmdSelection & (B1 << 7))  {
        cmd_pipToggle();
      }  // PIP
      break;
    case 2:
      if (cmdSelection & (B1 << 4))  {
        AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 0, !AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0));
      }  // Key1
      if (cmdSelection & (B1 << 5))  {
        AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 1, !AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 1));
      }  // Key2
      if (cmdSelection & (B1 << 2))  {
        AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 2, !AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 2));
      }  // Key3
      if (cmdSelection & (B1 << 7))  {
        AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 3, !AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 3));
      }  // Key4
      break;
    case 3:
      if (cmdSelection & (B1 << 4))  {
        AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, 2001);
      }  // Color1
      if (cmdSelection & (B1 << 5))  {
        AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, 2002);
      }  // Color2
      if (cmdSelection & (B1 << 2))  {
        AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, 0);
      }   // Black
      if (cmdSelection & (B1 << 7))  {
        AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, 1000);
      }  // Bars
      break;
    case 4:
      if (cmdSelection & (B1 << 4))  {
        BUSselect = BUSselect == 1 ? 0 : 1;
        updateSSW[1] = true;
      }
      if (cmdSelection & (B1 << 5))  {
        BUSselect = BUSselect == 2 ? 0 : 2;
        updateSSW[1] = true;
      }
      if (cmdSelection & (B1 << 2))  {
        BUSselect = BUSselect == 3 ? 0 : 3;
        updateSSW[1] = true;
      }
      if (cmdSelection & (B1 << 7))  {
        BUSselect = 0;
        MEselectForInputbuttons = 0;
        updateSSW[1] = true;
      }
      break;
    case 5:
      if (cmdSelection & (B1 << 4))  {
        AtemSwitcher.setDownstreamKeyerOnAir(0, !AtemSwitcher.getDownstreamKeyerOnAir(0));
      }  // DSK1
      if (cmdSelection & (B1 << 5))  {
        cmd_vgaToggle();
      }
      if (cmdSelection & (B1 << 2))  {
        AtemSwitcher.performAutoME(MEselectForInputbuttons);
        preVGA_active = false;
      }
      if (cmdSelection & (B1 << 7))  {
        cmd_pipToggle();
      }  // PIP
      break;
    case 6:
      if (cmdSelection & (B1 << 4))  {
        setAction(1);
      }
      if (cmdSelection & (B1 << 5))  {
        setAction(2);
      }
      if (cmdSelection & (B1 << 2))  {
        setAction(3);
      }
      if (cmdSelection & (B1 << 7))  {
        setAction(4);
      }
      break;
    default:
      if (cmdSelection & (B1 << 4))  {
        if (dskTrans) {
          AtemSwitcher.performDownstreamKeyerAutoKeyer(0);
        } else {
          AtemSwitcher.setDownstreamKeyerOnAir(0, !AtemSwitcher.getDownstreamKeyerOnAir(0));
        }
      }  // DSK1
      if (cmdSelection & (B1 << 5))  {
        if (dskTrans) {
          AtemSwitcher.performDownstreamKeyerAutoKeyer(1);
        } else {
          AtemSwitcher.setDownstreamKeyerOnAir(1, !AtemSwitcher.getDownstreamKeyerOnAir(1));
        }
      }
      if (cmdSelection & (B1 << 2))  {
        AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, 3010);
      }
      if (cmdSelection & (B1 << 7))  {
        AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, 3020);
      }
      break;
  }
}
void cmd_vgaToggle()  {
  if (!preVGA_active)  {
    preVGA_active = true;
    preVGA_UpstreamkeyerStatus = AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0);
    preVGA_programInput = AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons);

    AtemSwitcher.setProgramInputVideoSource(MEselectForInputbuttons, 8);
    AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 0, 1);
  }
  else {
    preVGA_active = false;
    AtemSwitcher.setProgramInputVideoSource(MEselectForInputbuttons, preVGA_programInput);
    AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 0, preVGA_UpstreamkeyerStatus);
  }
}
void cmd_pipToggle()  {
  // For Picture-in-picture, do an "auto" transition:
  unsigned long timeoutTime = millis() + 5000;

  // First, store original preview input:
  uint16_t tempPreviewInput = AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons);

  // Then, set preview=program (so auto doesn't change input)
  AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons));
  while (AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) != AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons))  {
    AtemSwitcher.runLoop();
    if (timeoutTime < millis()) {
      break;
    }
  }

  // Then set transition status:
  bool tempOnNextTransitionStatus = AtemSwitcher.getTransitionNextTransition(MEselectForInputbuttons);
  AtemSwitcher.setTransitionNextTransition(MEselectForInputbuttons, B00011);  // Set upstream key next transition
  while (!AtemSwitcher.getTransitionNextTransition(MEselectForInputbuttons) == 1)  {
    AtemSwitcher.runLoop();
    if (timeoutTime < millis()) {
      break;
    }
  }

  // Make Auto Transition:
  AtemSwitcher.performAutoME(MEselectForInputbuttons);
  while (AtemSwitcher.getTransitionPosition(MEselectForInputbuttons) == 0)  {
    AtemSwitcher.runLoop();
    if (timeoutTime < millis()) {
      break;
    }
  }
  while (AtemSwitcher.getTransitionPosition(MEselectForInputbuttons) > 0)  {
    AtemSwitcher.runLoop();
    if (timeoutTime < millis()) {
      break;
    }
  }

  // Then reset transition status:
  AtemSwitcher.setTransitionNextTransition(MEselectForInputbuttons, tempOnNextTransitionStatus);
  while (tempOnNextTransitionStatus != AtemSwitcher.getTransitionNextTransition(MEselectForInputbuttons))  {
    AtemSwitcher.runLoop();
    if (timeoutTime < millis()) {
      break;
    }
  }
  // Reset preview bus:
  AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, tempPreviewInput);
  while (tempPreviewInput != AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons))  {
    AtemSwitcher.runLoop();
    if (timeoutTime < millis()) {
      break;
    }
  }
  // Finally, tell us how we did:
  if (timeoutTime < millis()) {
    Serial.println("Timed out during operation!");
  }
  else {
    Serial.println("DONE!");
  }
}

void commandCut()  {
  // If VGA is the one source, make Auto instead!
  //if (AtemSwitcher.getProgramInput()==8 || AtemSwitcher.getPreviewInput()==8)  {
  //  AtemSwitcher.doAuto();
  //}
  //else {
  AtemSwitcher.performCutME(MEselectForInputbuttons);
  //}
  preVGA_active = false;
}

void commandAuto()  {
  AtemSwitcher.performAutoME(MEselectForInputbuttons);
  preVGA_active = false;
}

void extraButtonsCommands()  {

  // Check if buttons has been pressed down. This is a byte where each bit represents a button-down press:
  uint16_t buttonDownPress = extraButtons.buttonDownAll();
  if (hyperdeckControl) {
    if (hyperDeck.hasInitialized())  {
      if (extraButtons.isButtonIn(1, buttonDownPress))  { // Previous
        hyperDeck.gotoClipID(HDclipId - 1);
      }
      extraButtons.setButtonColor(1, extraButtons.buttonIsPressed(1) ? 4 : 5);
      if (extraButtons.isButtonIn(2, buttonDownPress))  { // Rewind
        if (playSpeed > 0) {
          playSpeed = -100;
        } else if (playSpeed > -1600) {
          playSpeed *= 2;
        }
        hyperDeck.playWithSpeed(playSpeed);
      }
      if (hyperDeck.getPlaySpeed() == -100) {
        extraButtons.setButtonColor(2, (((unsigned long)millis() >> 3) & B01000000) ? 4 : 5);
      } else if (hyperDeck.getPlaySpeed() == -200) {
        extraButtons.setButtonColor(2, (((unsigned long)millis() >> 3) & B00100000) ? 4 : 5);
      } else if (hyperDeck.getPlaySpeed() == -400) {
        extraButtons.setButtonColor(2, (((unsigned long)millis() >> 3) & B00010000) ? 4 : 5);
      } else if (hyperDeck.getPlaySpeed() == -800) {
        extraButtons.setButtonColor(2, (((unsigned long)millis() >> 3) & B00001000) ? 4 : 5);
      } else if (hyperDeck.getPlaySpeed() == -1600) {
        extraButtons.setButtonColor(2, (((unsigned long)millis() >> 3) & B00000100) ? 4 : 5);
      } else {
        extraButtons.setButtonColor(2, 5);
      }
      if (hyperDeck.isStopped()) {
        extraButtons.setButtonColor(3, 2);
      } else if (hyperDeck.isInPreview()) {
        extraButtons.setButtonColor(3, 3);
      } else {
        extraButtons.setButtonColor(3, 5);
      }
      if (extraButtons.isButtonIn(3, buttonDownPress))  { // Stop
        if (hyperDeck.isPlaying() || hyperDeck.isRecording()) {
          hyperDeck.stop();
        } else {
          hyperDeck.previewEnable(!hyperDeck.isInPreview() ? true : false);
        }
      }
     
      if (extraButtons.isButtonIn(4, buttonDownPress))  { // Play
        playSpeed = 100;
        hyperDeck.playWithSpeed(playSpeed);
      }
      extraButtons.setButtonColor(4, (hyperDeck.isPlaying() && playSpeed == 100) ? 3 : 5);
      if (extraButtons.isButtonIn(5, buttonDownPress))  { // Slow-mo
        playSpeed = 25;
        hyperDeck.playWithSpeed(playSpeed);
      }
      extraButtons.setButtonColor(5, hyperDeck.getPlaySpeed() == 25 ? 3 : 5);
      if (extraButtons.isButtonIn(6, buttonDownPress))  { // Record
        playSpeed = 100;
        hyperDeck.record();
      }
      extraButtons.setButtonColor(6, hyperDeck.isRecording() ? 2 : 5);
      if (extraButtons.isButtonIn(7, buttonDownPress))  { // F.Forward
        if (playSpeed < 200) {
          playSpeed = 200;
        } else if (playSpeed < 1600) {
          playSpeed *= 2;
        }
        hyperDeck.playWithSpeed(playSpeed);
      }
      if (hyperDeck.getPlaySpeed() == 200) {
        extraButtons.setButtonColor(7, (((unsigned long)millis() >> 3) & B00100000) ? 4 : 5);
      } else if (hyperDeck.getPlaySpeed() == 400) {
        extraButtons.setButtonColor(7, (((unsigned long)millis() >> 3) & B00010000) ? 4 : 5);
      } else if (hyperDeck.getPlaySpeed() == 800) {
        extraButtons.setButtonColor(7, (((unsigned long)millis() >> 3) & B00001000) ? 4 : 5);
      } else if (hyperDeck.getPlaySpeed() == 1600) {
        extraButtons.setButtonColor(7, (((unsigned long)millis() >> 3) & B00000100) ? 4 : 5);
      } else {
        extraButtons.setButtonColor(7, 5);
      }
      if (extraButtons.isButtonIn(8, buttonDownPress))  { // Next
        hyperDeck.gotoClipID(HDclipId + 1);
      }
      extraButtons.setButtonColor(8, extraButtons.buttonIsPressed(8) ? 4 : 5);
    } else {
      for (uint16_t i = 1; i <= 8; i++) {
        extraButtons.setButtonColor(i, 0);
      }
    }
  } else {
    // B8: "AUX1 BUS"
    if (extraButtons.isButtonIn(1, buttonDownPress))  {   // Executes button command if pressed:
      BUSselect = BUSselect == 1 ? 0 : 1;
      MEselectForInputbuttons = 0;
      updateSSW[1] = true;
    }
    extraButtons.setButtonColor(1, BUSselect == 1 ? 2 : 5); // Sets color of button to AMBER (2) if in AUX1 BUS mode. Otherwise color "5" which is dimmed yellow

    // B7: "AUX2 BUS"
    if (extraButtons.isButtonIn(2, buttonDownPress))  {   // Executes button command if pressed:
      BUSselect = BUSselect == 2 ? 0 : 2;
      MEselectForInputbuttons = 0;
      updateSSW[1] = true;
    }
    extraButtons.setButtonColor(2, BUSselect == 2 ? 2 : 5); // Sets color of button to AMBER (2) if in AUX1 BUS mode. Otherwise color "5" which is dimmed yellow

    // B6: "AUX3 BUS"
    if (extraButtons.isButtonIn(3, buttonDownPress))  {   // Executes button command if pressed:
      BUSselect = BUSselect == 3 ? 0 : 3;
      MEselectForInputbuttons = 0;
      updateSSW[1] = true;
    }
    extraButtons.setButtonColor(3, BUSselect == 3 ? 2 : 5); // Sets color of button to AMBER (2) if in AUX1 BUS mode. Otherwise color "5" which is dimmed yellow

    // B5: "Macros"
    if (extraButtons.isButtonIn(4, buttonDownPress))  {   // Executes button command if pressed:
      BUSselect = BUSselect == 4 ? 0 : 4;
      MEselectForInputbuttons = 0;
      updateSSW[1] = true;
    }
    extraButtons.setButtonColor(4, BUSselect == 4 ? 2 : 5); // Sets color of button to AMBER (2) if in AUX1 BUS mode. Otherwise color "5" which is dimmed yellow

    // B4: "Col 1"
    if (extraButtons.isButtonIn(5, buttonDownPress))  {   // Executes button command if pressed:
      AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, 2001);
    }
    extraButtons.setButtonColor(5, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 2001 ? 2 : AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == 2001 ? 3 : 5);

    // B3: "Col 2"
    if (extraButtons.isButtonIn(6, buttonDownPress))  {   // Executes button command if pressed:
      AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, 2002);
    }
    extraButtons.setButtonColor(6, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 2002 ? 2 : AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == 2002 ? 3 : 5);

    // B2: "Black"
    if (extraButtons.isButtonIn(7, buttonDownPress))  {   // Executes button command if pressed:
      AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, 0);
    }
    extraButtons.setButtonColor(7, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 0 ? 2 : AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == 0 ? 3 : 5);

    // B1: "Bars"
    if (extraButtons.isButtonIn(8, buttonDownPress))  {   // Executes button command if pressed:
      AtemSwitcher.setPreviewInputVideoSource(MEselectForInputbuttons, 1000);
    }
    extraButtons.setButtonColor(8, AtemSwitcher.getProgramInputVideoSource(MEselectForInputbuttons) == 1000 ? 2 : AtemSwitcher.getPreviewInputVideoSource(MEselectForInputbuttons) == 1000 ? 3 : 5);
  }
}

uint16_t setColor(uint16_t button) {
  switch (buttons2function[button - 1]) {
    case 0:
      return 0;
      break;
    case 1:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0) ? 4 : 5;
      break;
    case 2:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0) ? 4 : 5;
      break;
    case 3:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0) ? 4 : 5;
      break;
    case 4:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 1) ? 4 : 5;
      break;
    case 5:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 1) ? 4 : 5;
      break;
    case 6:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 1) ? 4 : 5;
      break;
    case 7:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 2) ? 4 : 5;
      break;
    case 8:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 2) ? 4 : 5;
      break;
    case 9:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 2) ? 4 : 5;
      break;
    case 10:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 3) ? 4 : 5;
      break;
    case 11:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 3) ? 4 : 5;
      break;
    case 12:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 3) ? 4 : 5;
      break;
    case 13:
      return AtemSwitcher.getDownstreamKeyerOnAir(0) ? 4 : 5;
      break;
    case 14:
      return AtemSwitcher.getDownstreamKeyerOnAir(0) ? 4 : 5;
      break;
    case 15:
      return AtemSwitcher.getDownstreamKeyerOnAir(0) ? 4 : 5;
      break;
    case 16:
      return AtemSwitcher.getDownstreamKeyerOnAir(0) ? 4 : 5;
      break;
    case 17:
      return AtemSwitcher.getDownstreamKeyerOnAir(1) ? 4 : 5;
      break;
    case 18:
      return AtemSwitcher.getDownstreamKeyerOnAir(1) ? 4 : 5;
      break;
    case 19:
      return AtemSwitcher.getDownstreamKeyerOnAir(1) ? 4 : 5;
      break;
    case 20:
      return AtemSwitcher.getDownstreamKeyerOnAir(1) ? 4 : 5;
      break;
    case 21:
      if (AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0) || AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 1) || AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 2) || AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 3) || AtemSwitcher.getDownstreamKeyerOnAir(0) || AtemSwitcher.getDownstreamKeyerOnAir(1)) {
        return 4;
      }
      else {
        return 5;
      }
      break;
    case 22:
      if ((button == 1 && !cmdSelect.buttonIsPressed(4)) || (button == 2 && !cmdSelect.buttonIsPressed(5)) || (button == 3 && !cmdSelect.buttonIsPressed(2)) || (button == 4 && !cmdSelect.buttonIsPressed(7)))  {
        return 5; // de-highlight button
      }
      break;
    case 23:
      return AtemSwitcher.getTransitionPosition(MEselectForInputbuttons) > 0 ? 2 : 5;
      break;
    case 24:
      if (AtemSwitcher.getFadeToBlackStateInTransition(MEselectForInputbuttons))  {  // Setting button color. This is a more complex example which includes blinking during execution:
        if (AtemSwitcher.getFadeToBlackStateFullyBlack(MEselectForInputbuttons) == 0 && (AtemSwitcher.getFadeToBlackStateFramesRemaining(MEselectForInputbuttons) != AtemSwitcher.getFadeToBlackRate(MEselectForInputbuttons)))  { // It's important to test if fadeToBlack time is more than zero because it's the kind of state from the ATEM which is usually not captured during initialization. Hopefull this will be solved in the future.

          return 2;  // Sets color of button to RED (2) if Fade To Black is executing
        }
      }
      else if (AtemSwitcher.getFadeToBlackStateFullyBlack(MEselectForInputbuttons)) {
        // Blinking with RED color if Fade To Black is activated:
        if ((unsigned long)millis() & B10000000)  {
          return 2;
        }
        else {
          return 5;
        }
      }
      else {
        return 5;  // Dimmed background if no fade to black
      }
      break;
    case 25:
      return AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0) ? 2 : 5;
      break;
    case 26:
      return preVGA_active ? 4 : 5;
      break;
    case 255:
      return 0;
      break;
  }
}

uint16_t setAction(uint16_t button) {
  switch (buttons2function[button - 1]) {
    case 1:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 0, !AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0));
      break;
    case 2:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 0, 1);
      break;
    case 3:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 0, 0);
      break;
    case 4:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 1, !AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 1));
      break;
    case 5:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 1, 1);
      break;
    case 6:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 1, 0);
      break;
    case 7:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 2, !AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 2));
      break;
    case 8:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 2, 1);
      break;
    case 9:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 2, 0);
      break;
    case 10:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 3, !AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 3));
      break;
    case 11:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 3, 1);
      break;
    case 12:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 3, 0);
      break;
    case 13:
      AtemSwitcher.setDownstreamKeyerOnAir(0, !AtemSwitcher.getDownstreamKeyerOnAir(0));
      break;
    case 14:
      AtemSwitcher.setDownstreamKeyerOnAir(0, 1);
      break;
    case 15:
      AtemSwitcher.setDownstreamKeyerOnAir(0, 0);
      break;
    case 16:
      AtemSwitcher.performDownstreamKeyerAutoKeyer(0);
      break;
    case 17:
      AtemSwitcher.setDownstreamKeyerOnAir(1, !AtemSwitcher.getDownstreamKeyerOnAir(1));
      break;
    case 18:
      AtemSwitcher.setDownstreamKeyerOnAir(1, 1);
      break;
    case 19:
      AtemSwitcher.setDownstreamKeyerOnAir(1, 0);
      break;
    case 20:
      AtemSwitcher.performDownstreamKeyerAutoKeyer(1);
      break;
    case 21:
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 0, 0);
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 1, 0);
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 2, 0);
      AtemSwitcher.setKeyerOnAirEnabled(MEselectForInputbuttons, 3, 0);
      AtemSwitcher.setDownstreamKeyerOnAir(0, 0);
      AtemSwitcher.setDownstreamKeyerOnAir(1, 0);
      break;
    case 22:
      if (button == 1) {
        cmdSelect.setButtonColor(5, 4);    // Highlight CUT button
      } else if (button == 2) {
        cmdSelect.setButtonColor(6, 4);    // Highlight CUT button
      } else if (button == 3) {
        cmdSelect.setButtonColor(3, 4);    // Highlight CUT button
      } else if (button == 4) {
        cmdSelect.setButtonColor(8, 4);    // Highlight CUT button
      }
      commandCut();
      break;
    case 23:
      commandAuto();
      break;
    case 24:
      AtemSwitcher.performFadeToBlackME(MEselectForInputbuttons);
      break;
    case 25:
      cmd_pipToggle();
      break;
    case 26:
      cmd_vgaToggle();
      break;
  }
}

void smartSwitches()  {
  
 word buttons = SSWboard.buttonUpAll(); 
    
  if (SSWboard.buttonIsHeldFor(2, 200)) {
      updateSSW[1] = true;
    Serial << F("SSW 1 held (") << F(")\n");
    Serial << F("ATEM ME: ") << AtemSwitcher.getTopologyMEs() << ("\n");
    SSW2lock = true;
    switch (BUSselect) {  
      case 0:
        BUSselect = 11;
        hyperdeckControl = true;
        break;
      case 5:
        BUSselect = 11;
        hyperdeckControl = true;
        break;
      case 6:
        BUSselect = 11;
        hyperdeckControl = true;
        break;
      case 7:
        BUSselect = 11;
        hyperdeckControl = true;
        break;
      case 8:
        BUSselect = 11;
        hyperdeckControl = true;
        break;
     
  
    }
  }
      
  

   
 if (SSWboard.isButtonIn(2, buttons))  {
    updateSSW[1] = true;
    Serial << F("SSW 1 pressed (") << F(")\n");
    Serial << F("ATEM ME: ") << AtemSwitcher.getTopologyMEs() << ("\n");
     if (SSW2lock) {
        SSW2lock = false;
      } else {
    switch (BUSselect) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
        //        if (AtemSwitcher.getTopologyMEs() == 2) {
        //          BUSselect = 5;
        //          MEselectForInputbuttons = 1;
        //        } else {
        BUSselect = 6;
        //}
        break;
      case 5:
        BUSselect = 6;
         hyperdeckControl = false;
        MEselectForInputbuttons = 0;
        break;
      case 6:
        BUSselect = 7;
         hyperdeckControl = false;
        //MEselectForInputbuttons = 0;
        break;
      case 7:
        BUSselect = 11;
         hyperdeckControl = true;
        //MEselectForInputbuttons = 0;
        break;
       
          case 8:
        BUSselect = 0;
         hyperdeckControl = false;
        //MEselectForInputbuttons = 0;
        break;
       
      case 9:
        BUSselect = 0;
        hyperdeckControl = false;
        //MEselectForInputbuttons = 0
        break;
        
      case 10:
        BUSselect = 11;
        hyperdeckControl = true;
        //MEselectForInputbuttons = 0;
        break;
        
      case 11:
        BUSselect = 0;
        hyperdeckControl = false;
        //MEselectForInputbuttons = 0;
        break;

  }
 
 }
 }
     
 /** static uint16_t PIPmode = 5;
  if (SSWboard.buttonIsHeldFor(2, 1000))  {
    Serial << F("SSW 2 held (") << F(")\n");
    if (AtemSwitcher.hasInitialized())  {
      PIPmode = (PIPmode + 1) % 4;
      updateSSW[1] = true;
      AtemSwitcher.commandBundleStart();
      AtemSwitcher.setKeyDVEPositionX(0, 0, abs(AtemSwitcher.getKeyDVEPositionX(0, 0)) * (PIPmode & B1 ? -1 : 1));
      AtemSwitcher.setKeyDVEPositionY(0, 0, abs(AtemSwitcher.getKeyDVEPositionY(0, 0)) * (PIPmode & B10 ? 1 : -1));
      AtemSwitcher.commandBundleEnd();
      SSW2lock = true;
    }
  }
  if (SSWboard.isButtonIn(2, buttons))  {
    Serial << F("SSW 2 pressed (") << F(")\n");
    if (AtemSwitcher.hasInitialized())  {
      if (SSW2lock) {
        SSW2lock = false;
      } else {
        SSWboard.setButtonColor(2, 2, 0, B1 << (1));
        cmd_pipToggle();
        updateSSW[1] = true;
      }
    }
  }
*/
  if (updateSSW[1])  {
    updateSSW[1] = false;
    Serial << F("Update graphics ") << 1 << F("\n");

    if (AtemOnline)  {
      SSWboard.clearDisplay();
      SSWboard.fillRoundRect(0, 0, 64, 9, 1, WHITE);
      SSWboard.setTextColor(BLACK, WHITE);
      SSWboard.setTextSize(1);

      SSWboard.drawFastHLine(2, 21, 4, WHITE);
      SSWboard.drawFastHLine(8 + 2, 21, 4, WHITE);
      SSWboard.drawFastHLine(16 + 2, 21, 4, WHITE);
      SSWboard.drawFastHLine(24 + 2, 21, 4, WHITE);
      SSWboard.drawFastHLine(32 + 2, 21, 4, WHITE);
      SSWboard.drawFastHLine(40 + 2, 21, 4, WHITE);
      SSWboard.drawFastHLine(48 + 2, 21, 4, WHITE);
      SSWboard.drawFastHLine(56 + 2, 21, 4, WHITE);

      switch (BUSselect) {
        case 0:
          SSWboard.setCursor(6, 1);
          if (MEselectForInputbuttons == 1) {
            SSWboard << F("ATEM ME1:");
          } else {
            SSWboard << F("ATEM ME2:");
          }

          SSWboard.setTextColor(WHITE, BLACK);
          SSWboard.setCursor(6, 12);
          SSWboard << F("Program");
          SSWboard.setCursor(6, 24);
          SSWboard << F("Preview");

          SSWboard.setButtonColor(1, 3, 1, B1 << (1)); // Light green
          break;
        case 1:
          SSWboard.setCursor(6, 1);
          if (MEselectForInputbuttons == 1) {
            SSWboard << F("ATEM ME1:");
          } else {
            SSWboard << F("ATEM ME2:");
          }

          SSWboard.setTextColor(WHITE, BLACK);
          SSWboard.setCursor(6, 12);
          SSWboard << F("AUX 1");
          SSWboard.setCursor(6, 24);
          SSWboard << F("Prv/Prog");

          SSWboard.setButtonColor(1, 3, 1, B1 << (1)); // Light green
          break;
        case 2:
          SSWboard.setCursor(6, 1);
          if (MEselectForInputbuttons == 1) {
            SSWboard << F("ATEM ME1:");
          } else {
            SSWboard << F("ATEM ME2:");
          }

          SSWboard.setTextColor(WHITE, BLACK);
          SSWboard.setCursor(6, 12);
          SSWboard << F("AUX 2");
          SSWboard.setCursor(6, 24);
          SSWboard << F("Prv/Prog");

          SSWboard.setButtonColor(1, 3, 1, B1 << (1)); // Light green
          break;
        case 3:
          SSWboard.setCursor(6, 1);
          if (MEselectForInputbuttons == 1) {
            SSWboard << F("ATEM ME1:");
          } else {
            SSWboard << F("ATEM ME2:");
          }

          SSWboard.setTextColor(WHITE, BLACK);
          SSWboard.setCursor(6, 12);
          SSWboard << F("AUX 3");
          SSWboard.setCursor(6, 24);
          SSWboard << F("Prv/Prog");

          SSWboard.setButtonColor(1, 3, 1, B1 << (1)); // Light green
          break;
        case 4:
          SSWboard.setCursor(6, 1);
          if (MEselectForInputbuttons == 1) {
            SSWboard << F("ATEM ME1:");
          } else {
            SSWboard << F("ATEM ME2:");
          }

          SSWboard.setTextColor(WHITE, BLACK);
          SSWboard.setCursor(6, 12);
          SSWboard << F("Macro");
          SSWboard.setCursor(6, 24);
          SSWboard << F("Prv/Prog");

          SSWboard.setButtonColor(3, 1, 3, B1 << (1)); // Light green

          break;
        case 5:
          SSWboard.setCursor(6, 1);
          SSWboard << F("ATEM ME2:");

          SSWboard.setTextColor(WHITE, BLACK);
          SSWboard.setCursor(6, 12);
          SSWboard << F("Program");
          SSWboard.setCursor(6, 24);
          SSWboard << F("Preview");

          SSWboard.setButtonColor(1, 1, 3, B1 << (1)); // Light blue
          break;
        case 6:
          SSWboard.setCursor(4, 1);
          SSWboard << F("Audio Int:");

          SSWboard.setTextColor(WHITE, BLACK);
          SSWboard.setCursor(20, 12);
          SSWboard << F("On");
          SSWboard.setCursor(20, 24);
          SSWboard << F("AFV");

          SSWboard.setButtonColor(1, 1, 3, B1 << (1)); // Purple
          break;
        case 7:
          SSWboard.setCursor(4, 1);
          SSWboard << F("Audio Ext:");

          SSWboard.setTextColor(WHITE, BLACK);
          SSWboard.setCursor(20, 12);
          SSWboard << F("On");
          SSWboard.setCursor(20, 24);
          SSWboard << F("AFV");

          SSWboard.setButtonColor(1, 1, 3, B1 << (1)); // Purple
          break;
        case 8:
          SSWboard.setCursor(2, 1);
          SSWboard << F("PIP Source");

          SSWboard.setTextColor(WHITE, BLACK);
          SSWboard.setCursor(6, 12);
          SSWboard << F("Source");
          SSWboard.setCursor(6, 24);
          SSWboard << F("Prv/Prog");

          SSWboard.setButtonColor(2, 2, 0, B1 << (1)); // Purple
          break;
        /** case 9:  // 9=VIDEOHUB:IN|OUT(s)
          if (Videohub.hasInitialized())  {
            SSWboard.setCursor(6, 1);
            SSWboard << F("VideoHub:");

            SSWboard.setTextColor(WHITE, BLACK);
            SSWboard.setCursor(6, 12);
            SSWboard << F("Input");
            SSWboard.setCursor(6, 24);
            SSWboard << F("Output(s)");

            SSWboard.setButtonColor(3, 1, 3, B1 << (0)); // Purple
          } else smartSwitches_VideohubNotConnected(0);
          break;
       case 10:  // 10=VIDEOHUB:OUT|IN
          if (Videohub.hasInitialized())  {
            SSWboard.setCursor(6, 1);
            SSWboard << F("VideoHub:");

            SSWboard.setTextColor(WHITE, BLACK);
            SSWboard.setCursor(6, 12);
            SSWboard << F("Output");
            SSWboard.setCursor(6, 24);
            SSWboard << F("Input");

            SSWboard.setButtonColor(3, 1, 3, B1 << (0)); // Purple
          } else smartSwitches_VideohubNotConnected(0);
          break;
          */
        case 11:
          if (hyperDeck.hasInitialized())  {
            SSWboard.setCursor(6, 1);
            if (MEselectForInputbuttons == 1) {
              SSWboard << F("HyperDeck");
            } else {
              SSWboard << F("HyperDeck");
            }

            SSWboard.setTextColor(WHITE, BLACK);
            SSWboard.setCursor(6, 12);
            SSWboard << F("Program");
            SSWboard.setCursor(6, 24);
            SSWboard << F("Preview");

            SSWboard.setButtonColor(2, 2, 3, B1 << (1)); // Light green
          } else smartSwitches_HyperdeckNotConnected(1);
          break;
      }
    
    } else {
      smartSwitches_AtemNotConnected((1));
    }
    SSWboard.display(B1 << 1);
  }

/**  if (updateSSW[1])  {
    updateSSW[1] = false;
    Serial << F("Update graphics ") << 2 << F("\n");

    SSWboard.clearDisplay();

    if (AtemOnline)  {
      if (AtemSwitcher.getKeyerOnAirEnabled(MEselectForInputbuttons, 0)) {
        SSWboard.setButtonColor(2, 2, 0, B1 << (1));
      } else {
        SSWboard.setButtonColor(0, 2, 3, B1 << (1));
      }

      SSWboard.fillRoundRect(0, 0, 64, 9, 1, WHITE);
      SSWboard.setTextColor(BLACK, WHITE);
      SSWboard.setTextSize(1);
      SSWboard.setCursor(23, 1);
      SSWboard << F("PIP:");

      switch (PIPmode)  {
        case 5:
          break;
        case 1:
          SSWboard.drawRect(0, 32 - 14, 28, 14, WHITE);
          break;
        case 2:
          SSWboard.drawRect(64 - 28, 10, 28, 14, WHITE);
          break;
        case 3:
          SSWboard.drawRect(0, 10, 28, 14, WHITE);
          break;
        default:
          SSWboard.drawRect(64 - 28, 32 - 14, 28, 14, WHITE);
          break;
      }
    } else {
      smartSwitches_AtemNotConnected((1));
    }
    SSWboard.display(B1 << (1));
  }
}
*/
}
void smartSwitches_AtemNotConnected(uint8_t n)  {
  SSWboard.clearDisplay();
  SSWboard.setButtonColor(1, 1, 1, B1 << n); // Setting color

  SSWboard.setTextColor(WHITE, BLACK);
  SSWboard.setCursor(18, 4);
  SSWboard << F("ATEM");
  SSWboard.setCursor(21, 12);
  SSWboard << F("not");
  SSWboard.setCursor(3, 20);
  SSWboard << F("connected");
}
/**
void smartSwitches_VideohubNotConnected(uint8_t n)  {
  SSWboard.clearDisplay();
  SSWboard.setButtonColor(1, 1, 1, B1 << n); // Setting color

  SSWboard.setTextColor(WHITE, BLACK);
  SSWboard.setCursor(7, 4);
  SSWboard << F("Videohub");
  SSWboard.setCursor(21, 12);
  SSWboard << F("not");
  SSWboard.setCursor(3, 20);
  SSWboard << F("connected");
}
*/
void smartSwitches_HyperdeckNotConnected(uint8_t n)  {
  SSWboard.clearDisplay();
  SSWboard.setButtonColor(1, 1, 1, B1 << n); // Setting color

  SSWboard.setTextColor(WHITE, BLACK);
  SSWboard.setCursor(3, 4);
  SSWboard << F("HyperDeck");
  SSWboard.setCursor(21, 12);
  SSWboard << F("not");
  SSWboard.setCursor(3, 20);
  SSWboard << F("connected");
}

void initGraphics() {
 /** SSWboard.clearDisplay();   // clears the screen and buffer
  SSWboard.setButtonColor(0, 2, 3, B11);
  SSWboard.clearDisplay();
  SSWboard.drawBitmap(0, 0,  SKAA, 64, 32, 1, true);
  SSWboard.display(B1);
*/
  SSWboard.clearDisplay();
  SSWboard.setButtonColor(1, 1, 1, B0); // Setting color
  SSWboard.setTextColor(WHITE, BLACK);
  SSWboard.setCursor(11, 4);
  SSWboard << F("SERIAL#");
  SSWboard.setCursor(24, 12);
  SSWboard << F("");
  SSWboard.setCursor(1, 20);
  SSWboard << F("A3BIY1BA02");
  SSWboard.display(B10);
}

void setPanelIntensity()    {
  for (uint8_t a = 1; a <= 5; a++) {
    if (panelIntensityLevel == 0) {
      if (a == 5)   {
        previewSelect.setColorBalance(a, 0, 0);
        programSelect.setColorBalance(a, 0, 0);
        extraButtons.setColorBalance(a, 0, 0);
        cmdSelect.setColorBalance(a, 0, 0);
        
      } else {
        previewSelect.setColorBalance(a, ceil((float)buttonColors[a - 1][0] / 10), ceil((float)buttonColors[a - 1][1] / 10));
        programSelect.setColorBalance(a, ceil((float)buttonColors[a - 1][0] / 10), ceil((float)buttonColors[a - 1][1] / 10));
        cmdSelect.setColorBalance(a, ceil((float)buttonColors[a - 1][0] / 10), ceil((float)buttonColors[a - 1][1] / 10));
        extraButtons.setColorBalance(a, ceil((float)buttonColors[a - 1][0] / 10), ceil((float)buttonColors[a - 1][1] / 10));
      }
    } else {
      previewSelect.setColorBalance(a, ceil((float)buttonColors[a - 1][0] / 10 * panelIntensityLevel), ceil((float)buttonColors[a - 1][1] / 10 * panelIntensityLevel));
      programSelect.setColorBalance(a, ceil((float)buttonColors[a - 1][0] / 10 * panelIntensityLevel), ceil((float)buttonColors[a - 1][1] / 10 * panelIntensityLevel));
      cmdSelect.setColorBalance(a, ceil((float)buttonColors[a - 1][0] / 10 * panelIntensityLevel), ceil((float)buttonColors[a - 1][1] / 10 * panelIntensityLevel));
      extraButtons.setColorBalance(a, ceil((float)buttonColors[a - 1][0] / 10 * panelIntensityLevel), ceil((float)buttonColors[a - 1][1] / 10 * panelIntensityLevel));
    }

    previewSelect.clearButtonColorCache();
    programSelect.clearButtonColorCache();
    extraButtons.clearButtonColorCache();
    cmdSelect.clearButtonColorCache();
  }
}

void runTest() {
  previewSelect.testProgramme(B11111111);
  programSelect.testProgramme(B11111111);
  extraButtons.testProgramme(B11111111);
  cmdSelect.testProgramme(B11111111);
  SSWboard.testProgramme(B11);
  encoders.testProgramme(B11);
}

//void chipScan() {
//  if (firstTime)  Serial << "----\nMCP23017::\n";
//  for (uint8_t i = 0; i <= 7; i++)  {
//    word buttonStatus = GPIOchipArray[i].digitalWordRead();
//    if (firstTime)  {
//      Serial << "Board #" << i << ": ";
//      Serial.println(buttonStatus, BIN);
//      MCP23017_states[i] = buttonStatus;
//    }
//    else {
//      if (MCP23017_states[i] != buttonStatus)  {
//        Serial << "Iter." << counter << ": MCP23017, Board #" << i << ": ";
//        Serial.println(buttonStatus, BIN);
//        deviation = true;
//        theSpeed = 3;
//      }
//    }
//    delay(theSpeed);
//  }
//
//
//
//  if (firstTime)  Serial << "\n\nPCA9685::\n";
//  for (uint8_t i = 0; i <= 63; i++)  {
//    if (i != 48)  { // Don't address the broadcast address.
//      // Set up each board:
//      ledDriver.begin(i);  // Address pins A5-A0 set to B111000
//
//      if (firstTime)  {
//        if (ledDriver.init())  {
//          Serial << "\nBoard #" << i << " found\n";
//          PCA9685_states[i] = true;
//        }
//        else {
//          Serial << ".";
//          PCA9685_states[i] = false;
//        }
//      }
//      else {
//        bool ledDriverState = ledDriver.init();
//        if (ledDriverState != PCA9685_states[i])  {
//          Serial << "Iter." << counter << ": PCA9685, Board #" << i << " " << (ledDriver.init() ? "found" : "missing");
//          Serial << "\n";
//          deviation = true;
//          theSpeed = 3;
//        }
//        if (ledDriverState)  {
//          if (deviation)  {  // Blinks if there has been a deviation from expected:
//            for (uint8_t ii = 0; ii < 16; ii++)  {
//              ledDriver.setLEDDimmed(ii, counter % 2 ? 100 : 0);
//            }
//          }
//          else {
//            if (counter % 20 < 3)  { // 100% color: (3 seconds)
//              for (uint8_t ii = 0; ii < 16; ii++)  {
//                ledDriver.setLEDDimmed(ii, (counter % 20) * 50);
//              }
//            }
//            else if (counter % 20 < 15)  { // The random color programme:
//              for (uint8_t ii = 0; ii < 16; ii++)  {
//                ledDriver.setLEDDimmed(ii, random(0, 6) * 20);
//              }
//            }
//            else {  // Same intensity for all LEDs:
//              int randColor1 = random(0, 3) * 50;
//              int randColor2 = random(0, 3) * 50;
//              for (uint8_t ii = 0; ii < 16; ii++)  {
//                ledDriver.setLEDDimmed(ii, ii % 2 ? randColor1 : randColor2);
//              }
//            }
//          }
//        }
//      }
//    }
//    delay(theSpeed);
//  }
//  if (counter % 20 < 3)  {
//    delay(3000);
//  }
//
//
//  if (firstTime)  Serial << "\n\n----\nWaiting for Deviations:";
//  if (counter % (30 * 60) == 0)  Serial << "\n" << counter << " ";
//  if (counter % 30 == 0)  Serial << ".";
//
//  counter++;
//  firstTime = false;
//}


/**
   Local delay function
*/
void lDelay(unsigned long timeout)  {
  unsigned long thisTime = millis();
  do {
    AtemSwitcher.runLoop();
    hyperDeck.runLoop();
    //Videohub.runLoop();
    encoders.runLoop();
    HDclipId = hyperDeck.getClipId();
    //    Serial << F(".");
    //    static int j = 1;
    //    static long k = 1;
    //    j++;
    //    if (j>100) {
    //      j=1;
    //      k++;
    //      Serial << F("\nNewLine: ") << k << F(" ");
    //    }
  }
  while (!sTools.hasTimedOut(thisTime, timeout));
}

