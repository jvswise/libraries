#include <SPI.h>
#include <Ethernet.h>
//#include <EthernetUDP.h>

// Select the appropriate AIM library. For most applications, use AIM.h.
// When debugging the AIM protocol, use AIM_DEBUG.h
//#include <AIM.h>
#include <AIM_DEBUG.h>


/******************************************************************************
* AIM Protocol Test
*
* This sketch implements a sample Amenable Interactive Mesh (AIM) Protocol
* application for remotely controlling a tricolour LED connected to digital
* pins 3, 5, and 6.
* To test it:
*   (1) Connect a tricolour LED (or 3 separate LEDs), each via a 220 ohm
*       resistor to GND and PWM digital pins 3, 5, and 6.
*   (2) Download this code to the Arduino.
*   (3) Run the companion Processing sketch, AIM_Prototype, which contains a
*       very rough GRAM server running the AIM Protocol and a simple HTTP
*       server.
*   (4) If you don't change the local IP address in the Processing example,
*       open a Web browser and enter URL  http://192.168.7.151:7780
*   (5) You should see the "AIM Device Dashboard" page displayed. Initially,
*       you may get the message, "Awaiting Device Discovery". Within 3
*       minutes you should see a table with 3 rows (one for each LED) and
*       with columns, "Id", "Class", "Location", "Command", "New Value",
*       and "Last Error". (The page updates every 2 minutes or when manually
*       refreshing.)
*   (6) To change the brightness of an LED, select "Set Value" command in its
*       row and enter a value in the range 0..255.
*       (Although you can change more than one row at-a-time, the Processing
*       code is very crude and has some concurrency issues that cause it to
*       lose some of the commands some of the time.)
*   (7) By default debug logging is turned on--the RX and TX packets are
*       printed out to the Arduino's serial port. This has a huge impact
*       on performance. To improve performance comment out the following
*       compiler directive located just after these comments:
*           //#define DEBUG_ON
*
*
* In order to minimize the impact to RAM on Arduino modules, the maximum
* AIM packet size is 64 octets.
*
* AIM Packet Format
* =================
*
*    3      2 2      1 1
*    1      4 3      6 5      8 7      0
*   +--------+--------+--------+-------+
*   |Protocol| Event  |   Transaction  |
*   |Version |        |       ID       |
*   +--------+--------+--------+-------+
*   |            Arguments ...         |
*   +--                              --+
*   |         (max 60 octets)          |
*   +--------+--------+--------+-------+
*
* Default destination UDP port for AIM is: 7770
*
* NOTE about function block comments (esp. the <I/O spec>):
*   Block comments for functions follow a standard format as follows:
*   //----------------------------------------------------------
*   // <1 or 2 line synopsis of its purpose>
*   // Arguments:
*   //   <arg-name>:<I/O spec>    - <argument's relevance>
*   //     ...
*   // Returns:
*   //   <specifiation of all return values>
*   // [Notes:
*   //    <important notes>
*   //   ...
*   // ]
*   //------------------------------------------------------------
*
*   <I/O spec> is one of ":I", ":O", or ":IO", meaning that we care
*      about the argument's value upon function invocation (an input
*      value), function termination (an output value), or both.
*   For each argument specify:
*     - the argument's relevance
*     - range restrictions when appropriate
*     - any special information of which the function's invoker should
*       be aware.
*
* NOTES about RAM usage and the use of DHCP in Arduino code:
*   1) Including AIM_DEBUG.h (rather than AIM.h) will add about 6KB to
*      to the program store.
*   2) Function startAimProtocol() currently supports the acquisition of the
*      Arduino's IP address from a DHCP server (typically found on a home
*      router). This adds about 3KB to the program store. To reduce program
*      store usage, refer to the notes in startAimProtocol() in library
*      file AIM.cpp (and AIM_DEBUG.cpp).
*
* History
* =======
* Version  Date      Author         Desription
* -------  -----     ------         --------------------------------------
* 0.1      13-09-05  J.van Schouwen Initial creation. (Derived from
*                                     .../Test Sketches/AIM_Protocol_Test6)
*
*******************************************************************************/

//Ruler
//345678901234567890123456789012345678901234567890123456789012345678901234567890


// struct for tracking device-specific data for the LED device group
struct Device
{
 public:
  Uint16        myId;
  Uint8         myGrpId;
  char          myClass[AIMPacket::CLASS_MAX + 1];
  char          myLoc[AIMPacket::LOC_MAX + 1];
  Int16         myValue;
  int           reportPeriod;
  long          reportTimer;
  bool          sendReport;
};



/***************************************************************************
 * Global Definitions 
 ***************************************************************************/

const Uint8   myGrpSz = 3;

//---- AIM network parameters
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // Arduino MAC address
IPAddress ip(0, 0, 0, 0);                 // Triggers DHCP address acquisition
//IPAddress ip(192, 168, 7, 155);           // Arduino static IP

int aimUdpPort = 7770;                                // Default AIM UDP port
// NOTES:
// (1)The MAC address should really be pre-configured somehow and
//    stored in EEPROM; it would have to be unique across the entire
//    Ethernet LAN.
// (2) It would be better to use DHCP (to acquire the IP address
//     dynamically from an attached router) rather than a static IP; it
//     is supported by the Ethernet library. (But, it's ok for this example.)
// (3) We should to calculate the broadcast IP based on the Arduino's IP
//     address--especially when DHCP is used.


//---- Declare our application-specific protocol handler
class DevProtHandler;  // Class definition follows the global variables
DevProtHandler *aim;


//---- Attributes common to the device group
Uint8         myType = AIMPacket::DEV_CTRL;
Int8          myScale = 0;
Int16         myRngL = 0;
Int16         myRngH = 255;
Int16         myZero = 0;
char          myUnits[] = "<none>";


//---- Instantiate and initialize our 3 LEDs
const int numDevs = 3;
Device dev[3] = { { 0, 1,    "Dim LED R", "",   50},
                  { 0, 2,    "Dim LED G", "",   50},
                  { 0, 3,    "Dim LED B", "",   50} };
//                  ^  ^      ^           ^     ^
//                  |  |      |           |     |
//                 id  grpId  class       loc   value
// On the very 1st powerup, id = 0 and loc = "". GRAMS should subsequently
// update these--at which point, we should be storing them in EEPROM.


//---- Misc globals
const int iAmHere_period = 10000;
bool initialized = false;
Device *tmp;
TransactionId tid;
IPAddress     storedDstIp;
int           storedDstPort;
IPAddress     reportDstIp;
int           reportDstPort;
long lastTime, currTime;

 

 
/***************************************************************************
 * DevProtHandler is the application-specific AIM protocol handling
 * class. Here you can see the AIMProtocol virtual function overrides
 * that allow us to handle a device group of 3 LEDs (a tricolour LED?).
 ***************************************************************************/
class DevProtHandler : public AIMProtocol
{  
 private:
   static bool instantiated;
   static DevProtHandler *instance;
   DevProtHandler() { ; };
   
 public:
  ~DevProtHandler() {;}
  
  static DevProtHandler* getInstance()
  {
    if (!instantiated)
    {
      instantiated = true;
      instance = new DevProtHandler();
    }
    return instance;
  }


//  virtual int handleRx_Ack(Uint16 transId, const IPAddress &dstIp,
//                           Uint16 dstPort, Uint8 code, char *diag);
//  virtual int handleRx_Nack(Uint16 transId, const IPAddress &dstIp,
//                            Uint16 dstPort, Uint8 code, char *diag);
  virtual int handleRx_Hello(Uint16 transId, IPAddress const &dstIp,
                             Uint16 dstPort);
//  virtual int handleRx_IAmHere(Uint16 transId, const IPAddress &dstIp,
//                    Uint16 dstPort, Uint16 id, Uint8 grpSz, Uint8 grpId);
  virtual int handleRx_UAre(Uint16 transId, const IPAddress &dstIp,
                    Uint16 dstPort, Uint8 grpId, Uint16 id, char *loc);
  virtual int handleRx_Forget(Uint16 transId, IPAddress const &dstIp,
                              Uint16 dstPort, Uint8 grpId);
  virtual int handleRx_UAlive(Uint16 transId, IPAddress const &dstIp,
                               Uint16 dstPort, Uint16 id);
  virtual int handleRx_Query(Uint16 transId, IPAddress const &dstIp,
                              Uint16 dstPort, Uint8 grpId);
//  virtual int handleRx_Attrs(Uint16 transId, IPAddress const &dstIp,
//           Uint16 dstPort, Uint16 id, char *loc, Uint8 grpSz, Uint8 grpId,
//           Uint8 devType, Int8 scale, char *devClass,
//           Int16 rngH, Int16 rngL, Int16 zero, char *units);
//  virtual int handleRx_AttrChg(Uint16 transId, IPAddress const &dstIp,
//                                Uint16 dstPort, Uint16 id);
  virtual int handleRx_Control(Uint16 transId, IPAddress const &dstIp,
                    Uint16 dstPort, Uint16 id, Uint8 action, Int16 value);
//  virtual int handleRx_Report(Uint16 transId, IPAddress const &dstIp,
//                               Uint16 dstPort, Uint16 id, Int16 value);


  void sendIAmHere(Uint16 id, Uint8 grpSz, Uint8 grpId);
  void sendAimReport(Uint16 id, Int16 value);
};

bool DevProtHandler::instantiated = false;
DevProtHandler *DevProtHandler::instance = NULL;


int DevProtHandler::handleRx_Hello(Uint16 transId, IPAddress const &dstIp,
                             Uint16 dstPort)
{
  inPkt.show();
  for (int i = 0; i < numDevs; i++)
  {
    dev[i].reportPeriod = 0;
    outPkt.writeIAmHere(dev[i].myId, myGrpSz, dev[i].myGrpId);
    sendAim(dstIp, dstPort, transId);
  }
  return AIMPacket::RET_OK;
}


int DevProtHandler::handleRx_UAre(Uint16 transId, const IPAddress &dstIp,
                    Uint16 dstPort, Uint8 grpId, Uint16 id, char *loc)
{
  inPkt.show();
  if (grpId <= numDevs)
  {
    dev[grpId-1].reportPeriod = 0;
    dev[grpId-1].myId = id;
    strcpy(dev[grpId-1].myLoc, loc);
    outPkt.writeAck(0, "");
    sendAim(dstIp, dstPort, transId);
    return AIMPacket::RET_OK;
  }
  return AIMPacket::RET_ERROR;
}


int DevProtHandler::handleRx_Forget(Uint16 transId, IPAddress const &dstIp,
                              Uint16 dstPort, Uint8 grpId)
{
  inPkt.show();
  if (grpId <= numDevs)
  {
    dev[grpId-1].reportPeriod = 0;
    dev[grpId-1].myId = 0;
    strcpy(dev[grpId-1].myLoc, "");
    outPkt.writeAck(0, "");
    sendAim(dstIp, dstPort, transId);
    return AIMPacket::RET_OK;
  }
  return AIMPacket::RET_ERROR;
}


int DevProtHandler::handleRx_UAlive(Uint16 transId, IPAddress const &dstIp,
                               Uint16 dstPort, Uint16 id)
{
  inPkt.show();
  for (int i = 0; i < numDevs; i++)
  {
    if (dev[i].myId == id)
    {
      outPkt.writeAck(0, "");
      sendAim(dstIp, dstPort, transId);
      return AIMPacket::RET_OK;
    }
  }
  return AIMPacket::RET_ERROR;
}


int DevProtHandler::handleRx_Query(Uint16 transId, IPAddress const &dstIp,
                              Uint16 dstPort, Uint8 grpId)
{
  inPkt.show();
  if (grpId <= numDevs)
  {
    dev[grpId-1].reportPeriod = 0;
    Device &tmp = dev[grpId-1];
    outPkt.writeAttrs(tmp.myId, tmp.myLoc, myGrpSz, tmp.myGrpId,
                      myType, myScale, tmp.myClass,
                      myRngL, myRngH, myZero, myUnits);
    sendAim(dstIp, dstPort, transId);
    return AIMPacket::RET_OK;
  }
  return AIMPacket::RET_ERROR;
}


int DevProtHandler::handleRx_Control(Uint16 transId, IPAddress const &dstIp,
                    Uint16 dstPort, Uint16 id, Uint8 action, Int16 value)
{
  inPkt.show();
  for (int i = 0; i < numDevs; i++)
  {
    if (dev[i].myId == id)
    {
      Device &tmp = dev[i];
      if (action == AIMPacket::ACT_SET)
      {
        if ((value >= myRngL) && (value <= myRngH))
        {
          tmp.myValue = value;
          return AIMPacket::RET_OK;
        }
        else
        {
          strcpy(diag, "Val Rng Exceeded");
          return AIMPacket::RET_ARGUMENT;
        }
      }
      else
      {
        if (action == AIMPacket::ACT_READ)
        {
          storedDstIp = dstIp;
          storedDstPort = dstPort;
          dev[i].reportPeriod = value;
          dev[i].reportTimer = millis();
          dev[i].sendReport = true;
        }
        return AIMPacket::RET_OK;
      }
    }
  }
  // At this point the id didn't match any of our devices.
  return AIMPacket::RET_ERROR;
}


void DevProtHandler::sendIAmHere(Uint16 id, Uint8 grpSz, Uint8 grpId)
{
  outPkt.writeIAmHere(id, grpSz, grpId);
  sendAim(broadcast_ip, DEFAULT_AIMP_UDP_PORT, tid.incr());
}


void DevProtHandler::sendAimReport(Uint16 id, Int16 value)
{
  outPkt.writeReport(id, value);
  sendAim(storedDstIp, storedDstPort, tid.incr());
}
/************************  End DevProtHandler class ***********************
 **************************************************************************/
 




void setup() {
  // Startup the AIM protocol (and Ethernet): 
  for (int i = 0; i < numDevs; i++)
  {
    memcpy(dev[i].myLoc, 0, sizeof(dev[i].myLoc));
    dev[i].sendReport = false;
    dev[i].reportPeriod = 0;
    dev[i].reportTimer = 0;
  }
  aim = DevProtHandler::getInstance();
  aim->startAimProtocol(mac, ip, aimUdpPort);
  
  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  
  lastTime = millis();
  Serial.begin(9600);
}



void loop() {
  currTime = millis();
  
  // Kludgey way to handle millis() wraparound. OK for now.
  if (currTime < lastTime)
    lastTime = currTime;
    
  // Send periodic IAMHERE packets for each device.
  if (!initialized || (currTime - lastTime) >= iAmHere_period)
  {
    initialized = true;
    lastTime = currTime;
    // Sending one IAMHERE takes about 400 milliseconds with
    // debugging output turned off. Ouch.
    // Be careful with apps requiring fine timing requirements.
    for (int i = 0; i < numDevs; i++)
    {
      tmp = &dev[i];
      aim->sendIAmHere(tmp->myId, myGrpSz, tmp->myGrpId);
    }
    // Comment out the following unless debugging to see how much RAM is avail.
    CheckRam();
  }
  
  // Handle one-time and periodic reports.
  for (int i = 0; i < numDevs; i++)
  {
    if (dev[i].reportPeriod > 0)
    {
      if ((currTime - dev[i].reportTimer) >= dev[i].reportPeriod)
      {
        dev[i].reportTimer = currTime;
        dev[i].sendReport = true;
      }
    }  
    if (dev[i].sendReport)
    {
      dev[i].sendReport = false;
      aim->sendAimReport(dev[i].myId, dev[i].myValue);
    }
  }
  
  // Check for, and handle, a received AIM packet.
  aim->handleRxAimPacket();
  
  // Update the LED values. These could have been updated via receipt
  // of a CONTROL packet.
  for (int i = 0; i < numDevs; i++)
  {
    switch (i)
    {
      case 0:
        analogWrite(3, dev[i].myValue);
        break;
      case 1:
        analogWrite(5, dev[i].myValue);
        break;
      case 2:
        analogWrite(6, dev[i].myValue);
        break;
    }
  }
  //CheckRam();
}
