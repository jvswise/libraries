/////////////////////////////////////////////////////////////////////////////////
// ArduinoSlaveConsts.h
//
// These are the constant definitions for messages transferred between a PC
// and a tethered Arduino that is being used as a collection of digital and
// analog I/O ports. 
//
// Messages are sent back and forth between the PC and Arduino using the
// following format:
//  SYNC   <transactionId> <function> [<arguments> ...]   END
// (byte)     (byte)          (byte)      (byte)          (byte)
//
// <transactionId> in {0x00, 0x01, ..., 0xF0} to avoid having to be escaped.
// (Refer to the note under Special octet values.)
//
// Basic digital input/output, analog input/output, and tone generation
// is supported.
//
// Copyleft 2010 J.van Schouwen, under GNU public license.
// (You may freely modify or distribute this code as long as the source code
// remains in the public domain without further copyright restrictions.)
//
// Version  Date        Author            Description
// -------  ----------  ----------------  ---------------------------------------
// 1.0      2010-04-22  J.van Schouwen    Initial creation.
// 1.1      2010-05-12  J.van Schouwen    Added debug tracing codes and new
//                                         error codes.
//
/////////////////////////////////////////////////////////////////////////////////


#define maxBlockSize  128  // Maximum data block (buffer) size


////////////////////////////////////////////////////////////////////////////
// Special octet values.
#define SYNC 0xFD  // FD hex (11111101) signifies the start of a new block
                   // of data on the the serial port.
#define END  0xFE  // FE hex signifies the end of the data block.
#define ESCP 0xFF  // FF hex is the escape character. The next octet is the
                   // escaped octet value (see below).
// NOTE: Room is left for possible future expansion such that the smallest
//       possible special octet value is 0xF0.

// ESCaPed codes
//  When adding an escaped code value:
//  - the value cannot overlap with the Function numbers defined immediately
//    below.
//  - for hex value 0xNN, use the format:
//      #define xNN  0xMM,   where MM = 0xff - 0xNN + 1
//  - update the value of LAST_SPECIAL
#define LAST_SPECIAL  xFD
#define FIRST_SPECIAL xFF

#define xFD  0x03  // ESCP 0x03 = FD hex (since FD hex is SYNC)
#define xFE  0x02  // ESCP 0x02 = FE hex (since FE hex is END)
#define xFF  0x01  // ESCP 0x01 = FF hex (since FF hex is ESCP)
////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////
// Functions (Function numbers are prefixed by ESCP)
#define RETN 0x10  // ESCP RETN = (return value from a function)

#define MODE 0x20  // ESCP MODE = pinMode(byte pin, byte mode)
#define DIGW 0x21  // ESCP DIGW = digitalWrite(byte pin, byte value)
#define DIGR 0x22  // ESCP DIGR = byte digtalRead(byte pin)

#define ANAF 0x30  // ESCP ANAF = analogReference(byte type)
#define ANAW 0x31  // ESCP ANAW = analogWrite(byte pin, byte value)
#define ANAR 0x32  // ESCP ANAR = byte analogRead(byte pin)

#define ADTN 0x40  // ESCP ADTN = tone(byte pin, int frequency, [int duration])
#define ADNT 0x41  // ESCP ADNT = noTone(byte pin)

#define ERR  0x50  // ESCP ERR  = Error reply (see "Error codes" below). These
                   //             can be sent asynchronously and independent of
                   //             command reply messages. The format of an error
                   //             reply is:
                   //               ESCP ERR <error code> [<ascii string>]

#define DBG  0x60  // ESCP DBG  = (debug commands and logs)
                   //           = DBG (byte msgType, [byte[] additionalData)
                   //             where msgType can have the following values:
   #define DBGLEVEL0  0x00 // Disable debug mode
   #define DBGLEVEL1  0x01 // start debug level 1 on Arduino
   #define DBGLEVEL2  0x02 // start debug level 2 on Arduino
   #define DBGLEVEL3  0x03 // start debug level 3 on Arduino
   #define DBGLEVEL4  0x04 // start debug level 4 on Arduino
   #define DBGLEVEL5  0x05 // start debug level 5 on Arduino
   #define DBGLEVEL6  0x06 // start debug level 6 on Arduino
   #define DBGREPLY   0x10 // Arduino replies with additionalData that can be
                           //   logged/displayed on PC


#define USER 0x70  // ESCP USER = (user-defined data block payload format)
                   //             NOTE: not being used at this time
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
// Error codes  (when sending an error code, use the ERR function id)
// Don't define any new error code values greater than GENERR.
#define NOERR     0x00  // There was no error.
                        // (There's no need to ever send this code.)
#define NOFUNC    0x01  // Function is unknown.  ERR <functionId>
#define CORRUPT   0x02  // Received data block appears to be corrupted.
#define BADTXBUF  0x03  // The egress buffer is invalid.
#define GENERR    0x7F  // Generic error (unclassified).


///////////////////////////////////////////////////////////////////////////