/* AIM.h */
#ifndef AIM_h
#define AIM_h

/******************************************************************************
* AIM Protocol Test
*
* This Arduino library implements the Amenable (Arduino) Interactive Mesh (AIM)
* Protocol framework.
*
* Refer to example sketch, AIM_Protocol_Tester, for a sample application and
* the hardware setup required to run the test.
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
* 1.0      13-09-05  J.van Schouwen 1st Library .cpp source file version.
*                                     (Derived from
*                                      .../Test Sketches/AIM_Protocol_Test6)
*
*******************************************************************************/


//Ruler
//345678901234567890123456789012345678901234567890123456789012345678901234567890


#include "string.h"
#include <avr/pgmspace.h>
#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include "IPAddress.h"



/********** Enable/Disable debugging via serial monitor ************/
/* Comment out the following line to disable debugging info */
#define DEBUG_ON


// Macro for defining strings that are stored in flash (program) memory rather
// than in RAM. Arduino defines the non-descript F("string") syntax.
#define FLASH(x) F(x)


#ifndef byte
  typedef uint8_t byte;
#endif


// Define fixed-width signed and unsigned integer types.
#ifndef Int8
  typedef signed char  Int8;
#endif
#ifndef Int16
  typedef signed int   Int16;
#endif
#ifndef Int32
  typedef signed long  Int32;
#endif
#ifndef Uint8
  typedef unsigned char  Uint8;
#endif
#ifndef Uint16
  typedef unsigned int   Uint16;
#endif
#ifndef Uint32
  typedef unsigned long  Uint32;
#endif


void CheckRam();


/************************************************************************
 ****************   Arduino AIM Protocol Base Classes   *****************
 ************************************************************************/


/**********************************************************************
 * AIMPacket provides the facilities for encoding and decoding AIM
 * packets.
 * Unsupported at this time is handling for the following AIM v1 packets:
 *   MENUGET
 *   MENUITEM
 *   MENUCTRL
 *   REQCTRL
 *   HANDOFF
 *
 * API:
 *   For application code, subclassed off of AIMProtocol, the following
 *   will be typically accessed:
 *   - Most certainly:
 *       writeAck(...);     // When a positive acknowledgement is required
 *       writeNack(...);    // Whenever an error condition occurs
 *       writeIAmHere(...); // Sent periodically to announce presence
 *       writeAttrs(...);   // Reply to a QUERY (see readQuery(...))
 *       writeReport(...);  // Report back your value (in response to CONTROL)
 *     Note that applications must not use the AIMPacket read#####() functions;
 *     the reading (decoding) is handled in the base behaviour of the
 *     AIMProtocol class. Applications will need to override that class'
 *     handleRx_NNNNN() functions where the appropriate argument values
 *     become readily available.
/************************************************************************
 ****************   Arduino AIM Protocol Base Classes   *****************
 ************************************************************************/


/************************** AIMPacket class ****************************
 ***********************************************************************/


/**********************************************************************
 * AIMPacket provides the facilities for encoding and decoding AIM
 * packets.
 * Unsupported at this time is handling for the following AIM v1 packets:
 *   MENUGET
 *   MENUITEM
 *   MENUCTRL
 *   REQCTRL
 *   HANDOFF
 *
 * API:
 *   For application code, subclassed off of AIMProtocol, the following
 *   will be typically accessed:
 *   - Most certainly:
 *       writeAck(...);     // When a positive acknowledgement is required
 *       writeNack(...);    // Whenever an error condition occurs
 *       writeIAmHere(...); // Sent periodically to announce presence
 *       writeAttrs(...);   // Reply to a QUERY (see readQuery(...))
 *       writeReport(...);  // Report back your value (in response to CONTROL)
 *     Note that applications must not use the AIMPacket read#####() functions;
 *     the reading (decoding) is handled in the base behaviour of the
 *     AIMProtocol class. Applications will need to override that class'
 *     handleRx_NNNNN() functions where the appropriate argument values
 *     become readily available.
 *
 *   - Perhaps:
 *       writeAttrChg(...); // Announce that you've changed an attr value
 *
 *   - Optional:
 *       show();            // Print packet contents to serial port.
 **********************************************************************/
class AIMPacket
{
 public:

  /*********************************************
   * Public types and constants
   *********************************************/

  // Return Code Definitions
  // =======================
  static const Uint8 RET_OK        = 0; // (no error)
  static const Uint8 RET_ERROR     = 1; // Generic error (avoid this value)
  static const Uint8 RET_PROTOCOL  = 2; // Unsupported protocol version
  static const Uint8 RET_EVENT     = 3; // Unsupported event
  static const Uint8 RET_TOOLARGE  = 4; // Packet is too large
  static const Uint8 RET_MALFORMED = 5; // Malformed packet contents
  static const Uint8 RET_ARGUMENT  = 6; // Argument value out of range


  // Packet string field maximums (null character excluded)
  static const int LOC_MAX = 12;
  static const int MENUNAME_MAX = 12;
  static const int CLASS_MAX = 12;
  static const int UNITS_MAX = 12;


  // AIM Protocol Type Definitions
  // =============================
  enum Actions
  {
    ACT_SET = 1,
    ACT_READ
  };

  enum MenuActions
  {
    // Menu System Version 1 (MVERS = 1) Actions:
    M1ACT_PREV = 1,
    M1ACT_NEXTSEL,
    M1ACT_MINUS,
    M1ACT_PLUS,

    // Menu System Version 2 Actions:
    // <TBD>
  };

  enum DeviceTypes
  {
    // Device Types
    DEV_SRV = 1,    // AIM Server--i.e. "Gateway for Remote Access and
                    //   Mesh Server (GRAMS)
    DEV_MON,        // Monitored device
    DEV_CTRL,       // Controlled device
    DEV_SMART,      // Smart (menu-driven) device
    DEV_MENU,       // Menu device (local menu-driver & controller)
    DEV_DISP        // Display device
  };


  // AIM Protocol Event Identifier Definitions
  // =========================================
  static const byte PKT_UNDEF    =   0; // Initial, undefined event value

  //   Acknowledgements:
  static const byte PKT_ACK      =   1;
  static const byte PKT_NACK     =   2;

  //   Discovery and Liveness:
  static const byte PKT_HELLO    =   3;
  static const byte PKT_IAMHERE  =   4;
  static const byte PKT_UARE     =   5;
  static const byte PKT_FORGET   =   6;
  static const byte PKT_UALIVE   =   7;
  static const byte PKT_QUERY    =   8;
  static const byte PKT_ATTRS    =   9;
  static const byte PKT_ATTRCHG  =  10;

  //   Device Monitoring and Control:
  static const byte PKT_CONTROL  =  50;
  static const byte PKT_REPORT   =  51;

  //     - Menu Interations
  static const byte PKT_MENUGET  = 100;
  static const byte PKT_MENUITEM = 101;
  static const byte PKT_MENUCTRL = 102;

  //   Local Controller and GRAMS Interactions:
  static const byte PKT_REQCTRL  = 150;
  static const byte PKT_HANDOFF  = 151;
  // ===================================================


 private:
  typedef struct
  {
    Uint8     protocolVersion;
    Uint8     event;
    Uint16    transId;
  } AimHeader;


 public:
  static const int MAX_PKT_SIZE = 64;
  static const int MAX_ARGS_SIZE = MAX_PKT_SIZE - sizeof(AimHeader);
  static const int DIAG_MAX = MAX_ARGS_SIZE - sizeof(Uint8) - 1;



  /****************** Private Member Variables *********************/
  AimHeader     hdr;
  byte         *args;
  int           argPos;


 public:
  /****************** Public Member Functions *************************/
  AIMPacket();
  ~AIMPacket();

  inline Uint8  getProtocolVersion() { return hdr.protocolVersion; };
  inline void   setProtocolVersion(Uint8 vers) { hdr.protocolVersion = vers; };
  inline Uint8  getEvent() { return hdr.event; };
  inline Uint16 getTransId() { return hdr.transId; };
  inline void   setTransId(Uint16 tid) { hdr.transId = tid; };


  void sendPacket(EthernetUDP &udp, IPAddress &dstIp, int dstPort);
  bool receivePacket(EthernetUDP &udp, int pktSize, int &err);


  int writeAck(Uint8 code, const char *const diag);
  int readAck(Uint8 &code, char *&diag);
  void showAck();

  int writeNack(Uint8 code, const char *const diag);
  int readNack(Uint8 &code, char *&diag);
  void showNack();

  int writeHello();
  int readHello() { return 0; };
  void showHello() { ; };

  int writeIAmHere(Uint16 id, Uint8 grpSz, Uint8 grpId);
  int readIAmHere(Uint16 &id, Uint8 &grpSz, Uint8 &grpId);
  void showIAmHere();

  int writeUAre(Uint8 grpId, Uint16 id, char *loc);
  int readUAre(Uint8 &grpId, Uint16 &id, char *&loc);
  void showUAre();

  int writeForget(Uint8 grpId);
  int readForget(Uint8 &grpId);
  void showForget();

  int writeUAlive(Uint16 id);
  int readUAlive(Uint16 &id);
  void showUAlive();

  int writeQuery(Uint8 grpId);
  int readQuery(Uint8 &grpId);
  void showQuery();

  int writeAttrs(Uint16 id, char *loc, Uint8 grpSz, Uint8 grpId,
                 Uint8 type, Int8 scale, char *dev_class,
                 Int16 rngl, Int16 rngh, Int16 zero, char *units);
  int readAttrs(Uint16 &id, char *&loc, Uint8 &grpSz, Uint8 &grpId,
                Uint8 &type, Int8 &scale, char *&dev_class,
                Int16 &rngl, Int16 &rngh, Int16 &zero, char *&units);
  void showAttrs();

  int writeAttrChg(Uint16 id);
  int readAttrChg(Uint16 &id);
  void showAttrChg();

  int writeControl(Uint16 id, Actions action, Int16 value);
  int readControl(Uint16 &id, Uint8 &action, Int16 &value);
  void showControl();

  int writeReport(Uint16 id, Int16 value);
  int readReport(Uint16 &id, Int16 &value);
  void showReport();


  void printEvent(Uint8 event);
  void printAction(Actions action);
  void printDevType(Uint8 type);
  void printRetCode(Uint8 code);
  void printErrCount(int err);
  void show();


 private:
  /******************  Private Functions ***********************/

  //----- Conversions between unsigned integers & constituent bytes -----
  inline void uint16ToBytes(Uint16 val, byte &byteH, byte &byteL)
  {
    byteH = (byte) ((val & 0xFF00) >> 8);
    byteL = (byte) (val & 0x00FF);
  }

  inline void uint32ToBytes(Uint32 val, byte &byte1MSB, byte &byte2,
                                        byte &byte3, byte &byte4LSB)
  {
    byte1MSB = (byte) ((val & 0xFF000000) >> 24);
    byte2    = (byte) ((val & 0x00FF0000) >> 16);
    byte3    = (byte) ((val & 0x0000FF00) >> 8);
    byte4LSB = (byte) (val & 0x000000FF);
  }

  inline void bytesToUint16(byte byteH, byte byteL, Uint16 &val)
  {
    val = (((Uint16) byteH) << 8) | ((Uint16) byteL);
  }

  inline void bytesToUint32(byte byte1MSB, byte byte2, byte byte3,
                            byte byte4LSB, Uint32 &val)
  {
    val = (((Uint32) byte1MSB) << 24) | (((Uint32) byte2) << 16) |
          (((Uint32) byte3) << 8)     |  ((Uint32) byte4LSB);
  }

  //--- Reading & writing values from/to the packet buffer
  int writeUint8(Uint8 val);
  int writeUint16(Uint16 val);
  int writeUint32(Uint32 val);
  int writeStr(const char *const str, int maxLen, int refNum = 0);
  Uint8 readUint8(int &index, int &err);
  Uint16 readUint16(int &index, int &err);
  Uint32 readUint32(int &index, int &err);
  char *readStr(int &index, int maxLen, int &err);
                                        
  //---- Initializing a packet for reading or writing
  void initPktWrite(Uint8 event = PKT_UNDEF);
  void initPktRead(); 
    
  //---- Check that event is supported
  bool verifyEvent();

  //---- Misc functions for printing to the serial port
  void printIpAddr(byte *ip);
  void printReadErr(char *progName);
};



/************************* AIMProtocol class ***************************
 ***********************************************************************/


/**************************************************************************
 * The AIMProtocol class provides basic support for AIM protocol handling.
 * AIMProtocol provides basic packet handling capabilities only. It does
 * not implement the AIM protocol semantics; that must be provided by
 * the application within the context of the devices it handles.
 *
 * It provides functions to:
 *   - start-up the AIM protocol
 *         startAimProtocol(...)
 *   - send AIM packets (once they've been written by the application)
 *         sendAim(...)
 *   - check for receipt of an AIM packet and provide top-level processing
 *     for it. An application should subclass off of this class and
 *     override the handleRx_####() functions (see below)
 *         handleRxAimPacket()
 *
 * Virtual functions are provided to allow an application to process
 * incoming AIM packets in a way that is appropriate for the composite
 * device.
 *    A composite device is an Arduino board that controls some
 *    number of devices in a user-defined device group. A device group is a
 *    collection of one or more devices. Each device controlled by the
 *    Arduino can be uniquely identified by: (1) the IP address and AIM UDP
 *    port number of the Arduino plus the device's group ID; and, (2) the
 *    AIM system-wide ID that it is assigned by the AIM "Gateway for Remote
 *    Access and Mesh Server" (GRAMS).)
 *
 *    - handleRx_#####()
 *       For any given ##### AIM packet the application expects to receive,
 *       it should override the corresponding function.
 *         e.g. To handle the UARE packet, override the handleRx_UAre()
 *              function.
 * 
 * An application should: 
 *   - invoke startAimProtocol() in its setup() function
 *   - override handleRx_#####() functions for those AIM events it expects
 *     to handle; the other events have their packets logged (by default),
 *     but are otherwise ignored
 *   - regularly invoke handleRxAimPacket() somewhere in its loop()
 *     function
 *   - send AIM packets by: 
 *       - invoking one of the AIMPacket::write#####() functions to
 *         datafill a packet
 *       - invoking sendAim() to send the datafilled packet
 *   - ensure that it periodically sends an IAMHERE packet (using
 *       AIMPacket::writeIAmHere(...) and sendAim()) using the timing
 *     parameters given in the Amenable (Arduino) Interactive Mesh protocol
 *     specification.
 *    
 **************************************************************************/
class AIMProtocol
{
 protected:
 
  /*************** Protected Constants *************************/
  static const int PROTOCOL_VERSION = 1;
  static const int DEFAULT_AIMP_UDP_PORT = 7770;
         
  /****************** Protected Member Variables *********************/
  IPAddress     local_ip; 
  IPAddress     broadcast_ip;
  Uint16        local_udp_port;
  AIMPacket     inPkt;
  AIMPacket     outPkt;
  char          diag[AIMPacket::DIAG_MAX]; 
 
 /****************** Private Member Variables ************************/
 private:
  byte          local_mac[6];
  EthernetUDP   udp;
  bool          protStarted;
  Uint8         protVers;  
         
  static bool          instantiated;
  static AIMProtocol  *instance;
 
       
 protected:
  // Subclasses should be singleton classes as well.
  AIMProtocol() :
    local_ip(0,0,0,0),
    broadcast_ip(0,0,0,0),
    local_udp_port(0),
    protStarted(false),
    protVers(0),
    udp(),
    inPkt(),
    outPkt()
  {
    memset(local_mac, 0, 6);
  }
  
  
 public:        
  // We only allow one instance of an AIMProtocol. Additional instances
  // would cause problems with the handling of Ethernet and EthernetUDP
  static AIMProtocol* getInstance()
  {
    if (!instantiated)
    {
      instantiated = true; 
      instance = new AIMProtocol();
    }
    return instance;  
  }
       
  
  // Top-level protocol functions
  void startAimProtocol(byte *mac, IPAddress &ip,
                        Uint16 udpPort = DEFAULT_AIMP_UDP_PORT,
                        Uint8 protVers = PROTOCOL_VERSION);
  void sendAim(const IPAddress &dstIp, Uint16 dstPort,
               Uint16 transId);
  void handleRxAimPacket();
    
    
  // Virtual handlers for received AIM packets
  virtual int handleRx_Ack(Uint16 transId, const IPAddress &dstIp,
                           Uint16 dstPort, Uint8 code, char *diag);
  virtual int handleRx_Nack(Uint16 transId, const IPAddress &dstIp,
                            Uint16 dstPort, Uint8 code, char *diag);
  virtual int handleRx_Hello(Uint16 transId, IPAddress const &dstIp,
                             Uint16 dstPort);
  virtual int handleRx_IAmHere(Uint16 transId, const IPAddress &dstIp,
                    Uint16 dstPort, Uint16 id, Uint8 grpSz, Uint8 grpId);
  virtual int handleRx_UAre(Uint16 transId, const IPAddress &dstIp,
                    Uint16 dstPort, Uint8 grpId, Uint16 id, char *loc);
  virtual int handleRx_Forget(Uint16 transId, IPAddress const &dstIp,
                               Uint16 dstPort, Uint8 grpId);
  virtual int handleRx_UAlive(Uint16 transId, IPAddress const &dstIp,
                               Uint16 dstPort, Uint16 id);
  virtual int handleRx_Query(Uint16 transId, IPAddress const &dstIp,
                              Uint16 dstPort, Uint8 grpId);
  virtual int handleRx_Attrs(Uint16 transId, IPAddress const &dstIp,
           Uint16 dstPort, Uint16 id, char *loc, Uint8 grpSz, Uint8 grpId,
           Uint8 devType, Int8 scale, char *devClass,
           Int16 rngH, Int16 rngL, Int16 zero, char *units);
  virtual int handleRx_AttrChg(Uint16 transId, IPAddress const &dstIp,
                                Uint16 dstPort, Uint16 id);
  virtual int handleRx_Control(Uint16 transId, IPAddress const &dstIp,
                    Uint16 dstPort, Uint16 id, Uint8 action, Int16 value);
  virtual int handleRx_Report(Uint16 transId, IPAddress const &dstIp,
                               Uint16 dstPort, Uint16 id, Int16 value);
    
    
 private:
  bool packetRcvd(Uint8 &event, Uint16 &transId, 
                  IPAddress &srcIp, Uint16 &srcPort, int &err);
  bool verifyProtVers(Uint8 protVers);
};                          
  

/************************ TransactionId class **************************
 ***********************************************************************/

/*********************************************************************
 * TransactionId is a simple class for maintaining transaction
 * identifiers locally. It takes care of wraparound and ensures
 * that 0 is never used. Although the protocol spec doesn't care
 * whether or not 0 is used, it's helpful to keep that value
 * reserved as an invalid/undefined transaction ID; some applications
 * might want to leverage it.
 **********************************************************************/
class TransactionId
{
 private:
  Uint16 tid;

 public:
  //JVS?? Might just want to overload the ++ operator instead of
  //      incr().
  inline Uint16 incr() { if (++tid == 0) tid = 1; return tid; };
  inline Uint16 getTransId() { return tid; };
};

/***********************************************************************
 *****************  End of AIM Protocol Base Classes  ******************
 ***********************************************************************/


#endif
