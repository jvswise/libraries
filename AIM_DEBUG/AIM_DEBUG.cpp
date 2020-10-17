/* AIM.cpp */
/******************************************************************************
* AIM Protocol Test
*
* This Arduino library implements the Amenable (Arduino) Interactive Mesh (AIM)
* Protocol framework.
*
* Refer to AIM.h for library comments.
*
* History
* =======
* Version  Date      Author         Desription
* -------  -----     ------         --------------------------------------
* 1.0      13-09-05  J.van Schouwen 1st Library .cpp source file version.
*                                     (Derived from
*                                      .../Test Sketches/AIM_Protocol_Test)
* 1.1      13-09-17  J.van Schouwen 2nd Library .cpp source file version.
*                                     (Derived from
*                                      .../Test Sketches/AIM_Protocol_Test)
* 
*******************************************************************************/

#include "AIM_DEBUG.h"

//Ruler
//345678901234567890123456789012345678901234567890123456789012345678901234567890


/***************************************************************************
 * Test how much free RAM is left on the Arduino, printing the results out
 * to the serial port.
 * IMPORTANT: Avoid calling this function during normal operation. This
 *            should be treated as a temporary diagnostic test only. 
 ***************************************************************************/
void CheckRam()
{  
  unsigned short current;
  void *tmpStore, *newTmpStore;
  current = 30;
  newTmpStore = malloc(current);
  if (newTmpStore == NULL)
  {
    Serial.print(FLASH("Free: ")); Serial.print(FLASH("< "));
    Serial.println(current);
  }
  else
  {
    while (newTmpStore != NULL)
    {
      //Serial.println(current);
      tmpStore = newTmpStore;
      newTmpStore = realloc(tmpStore, ++current);
    }
    free(tmpStore);
    Serial.print(FLASH("Free: ")); Serial.println(current);
  }
}


/************************************************************************
 ****************   Arduino AIM Protocol Base Classes   *****************
 ************************************************************************/


/*************************  AIMPacket class *******************************
 **************************************************************************/

//--------------- Constructor & Destructors ------------------------
AIMPacket::AIMPacket() :
  args(0)
{
  hdr.protocolVersion = 0;
  hdr.event = 0;
  hdr.transId = 0;
  args = new byte[MAX_ARGS_SIZE];
  argPos = 0;
}


AIMPacket::~AIMPacket()
{
  delete[] args;
}


//-----------------------------------------------------------------
// Initialize a packet before writing to it.
// Arguments:
//   event:I  - the event field value to insert into the packet
// Returns: (none)
//-----------------------------------------------------------------
void AIMPacket::initPktWrite(Uint8 event)
{
  hdr.protocolVersion = 0;
  hdr.event = event;
  hdr.transId = 0;
  memset(args, 0, MAX_ARGS_SIZE);
  argPos = 0;
}


//----------------------------------------------------------------
// Initialize a packet before reading from it.
// Arguments: (none)
// Returns: (none)
//----------------------------------------------------------------
void AIMPacket::initPktRead()
{
  argPos = 0;
}


//------------------------------------------------------------------
// Send an AIM protocol UDP packet over a supplied Ethernet port.
// Arguments:
//   udp:I      - the Ethernet port (that supports UDP packets)
//   dstIp:I    - destination IP address
//   dstPort:I  - destination UDP port
// Returns:  (none)
// Notes:
//   The source IP and source UDP port will have already been defined
//   for the udp argument passed in.
//------------------------------------------------------------------
void AIMPacket::sendPacket(EthernetUDP &udp, IPAddress &dstIp, int dstPort)
{
  Uint8 tmp1, tmp2;
  
 #ifdef DEBUG_ON
  Serial.print(FLASH("Sending "));
  printEvent(hdr.event);
  Serial.print(FLASH(" packet to "));
  Serial.print(dstIp);
  Serial.print(FLASH(" on UDP port "));
  Serial.println(dstPort);
  Serial.println();
  show();
 #endif
//JVS??
Serial.print("Pkt Send: Event="); Serial.println(hdr.event);
  udp.beginPacket(dstIp, dstPort);
    // Write out the header
    udp.write(hdr.protocolVersion);
    udp.write(hdr.event);
    uint16ToBytes(hdr.transId, tmp1, tmp2);
    udp.write(tmp1);
    udp.write(tmp2);
    
    // Write out the arguments
    for (int i = 0; i < argPos; i++)
      udp.write(args[i]);
  udp.endPacket();
}


//--------------------------------------------------------------------
// Read in an AIM protocol UDP packet from a supplied Ethernet port
// Arguments:
//   udp:I      - the Ethernet port (that supports UDP packets)
//   pktSize:I  - the number of octets to read (the UDP packet
//                payload size)
//   err:O      - the AIMP error indication (RET_OK, if no error)
// Returns:
//   true iff there is no error in the AIM header or upper packet size
//            limit.
// Notes:
//   The invoker is assumed to have called udp.parsePacket() and has
//   determined both that a UDP packet is available and the packet's
//   payload length.
//----------------------------------------------------------------------
bool AIMPacket::receivePacket(EthernetUDP &udp, int pktSize, int &err)
{
  byte tmpByte1, tmpByte2;
  bool recvOk = true;
  Uint16 bytesLeft;
  
  err = RET_OK;
  bytesLeft = pktSize;
  if (pktSize < sizeof(AimHeader))
  {
    recvOk = false;
    err = RET_MALFORMED;
  }
  
  if (recvOk)
  {
    argPos = 0;
    hdr.protocolVersion = (Uint8) udp.read();
    bytesLeft--;
    
    hdr.event = (Uint8) udp.read();
    bytesLeft--;
    
   #ifdef DEBUG_ON
    Serial.print(FLASH("Receiving Event "));
    printEvent(hdr.event);
    Serial.print(FLASH(" packet from "));
    Serial.print(udp.remoteIP());
    Serial.print(FLASH(" on UDP port "));
    Serial.println(udp.remotePort());
   #endif
    if (!verifyEvent())
    {
      recvOk = false;
      err = RET_EVENT;
    }
  }
  
  if (recvOk)
  {
    tmpByte1 = (Uint8) udp.read();
    tmpByte2 = (Uint8) udp.read();
    bytesLeft -= 2;
    bytesToUint16(tmpByte1, tmpByte2, hdr.transId);
    if (pktSize > MAX_PKT_SIZE)
    {
     #ifdef DEBUG_ON
      Serial.print(FLASH("Rcvd pkt too big ["));
      Serial.print(pktSize);
      Serial.print(FLASH("/"));
      Serial.print(MAX_PKT_SIZE);
      Serial.println(FLASH("]"));
     #endif
      recvOk = false;
      err = RET_TOOLARGE;
    }
  }
  
  if (recvOk)
  {
    if (bytesLeft <= MAX_ARGS_SIZE)
    {
      udp.read(args, bytesLeft);
      bytesLeft -= bytesLeft;
    }
    else
    {
      udp.read(args, MAX_ARGS_SIZE);
      bytesLeft -= MAX_ARGS_SIZE;
    }
    if (bytesLeft)
    {
     #ifdef DEBUG_ON
      Serial.print(bytesLeft);
      Serial.println(FLASH(" bytes left to read in RX pkt!!"));
     #endif
    }  
    return true;
  }
  else
  {
    // Purge the incoming UDP packet buffer
   #ifdef DEBUG_ON
    Serial.print(FLASH("RX: purging  "));
    Serial.print(bytesLeft);
    Serial.println(FLASH(" bytes from UDP buffer"));
   #endif
    while (bytesLeft--)
      tmpByte1 = udp.read();
    return false;
  }
  return false;
}


//---------------------------------------------------------------------
// Write an unsigned 8-bit integer to the packet
// Arguments:
//   val:I   - value to write
// Returns:
//   0, if successful
//   1, if packet buffer is too full
//----------------------------------------------------------------------
int AIMPacket::writeUint8(Uint8 val)
{
  const int SPACE_NEEDED = sizeof(Uint8);
  
  if (argPos <= (MAX_ARGS_SIZE - SPACE_NEEDED ) )
  {
    args[argPos] = val;
    argPos += SPACE_NEEDED;
  }
  else
  {
   #ifdef DEBUG_ON
    Serial.print(FLASH("writeUint8: ***ERR ["));
    printEvent(hdr.event);
    Serial.print(FLASH("] Buffer full."));
   #endif
    return 1;
  }
  return 0;
}


//---------------------------------------------------------------------
// Write an unsigned 16-bit integer to the packet
// Arguments:
//   val:I   - value to write
// Returns:
//   0, if successful
//   1, if packet buffer is too full
//----------------------------------------------------------------------
int AIMPacket::writeUint16(Uint16 val)
{
  int SPACE_NEEDED = sizeof(Uint16);
  
  if (argPos <= (MAX_ARGS_SIZE - SPACE_NEEDED) )
  {
    uint16ToBytes(val, args[argPos], args[argPos+1]);
    argPos += SPACE_NEEDED;
  }
  else
  {
   #ifdef DEBUG_ON 
    Serial.print(FLASH("writeUint16: ***ERR ["));
    printEvent(hdr.event);
    Serial.print(FLASH("] Buffer full"));
   #endif
    return 1;
  }
  return 0;
}


//---------------------------------------------------------------------
// Write an unsigned 32-bit integer to the packet
// Arguments:
//   val:I   - value to write
// Returns:
//   0, if successful
//   1, if packet buffer is too full
//----------------------------------------------------------------------
int AIMPacket::writeUint32(Uint32 val)
{
  const int SPACE_NEEDED = sizeof(Uint32);
  
  if (argPos <= (MAX_ARGS_SIZE - SPACE_NEEDED ) )
  {
    uint32ToBytes(val, args[argPos], args[argPos+1],
                       args[argPos+2], args[argPos+3]);
    argPos += SPACE_NEEDED;
  }
  else
  {
   #ifdef DEBUG_ON
    Serial.print(FLASH("writeUint32: ***ERR ["));
    printEvent(hdr.event);
    Serial.print(FLASH("] Buffer full"));
   #endif
   return 1;
  }
  return 0;
}


//---------------------------------------------------------------------
// Write a null-terminated ASCII string to the packet
// Arguments:
//   str:I     - string to write
//   maxLex:I  - AIM protocol field size limit for this kind of string.
//   refNum:I  - A user-defined reference number printed to the serial
//               port in error cases. (It has not bearing on the packet
//               contents.)
// Returns:
//   0, if successful
//   1, if str had to be truncated in order to fit the packet field;
//        the packet can still be sent in this case.
//   2, if packet buffer is too full
//----------------------------------------------------------------------
int AIMPacket::writeStr(const char *const str, int maxLen, int refNum)
{
  int len = strlen(str);
  bool truncated = false;
  bool bufferExceeded = false;
  int spaceRemaining = MAX_ARGS_SIZE - argPos;

  if (len > maxLen)
  {
    truncated = true;
    len = maxLen;

  }

  if (spaceRemaining < (len + 1))
  {
    bufferExceeded = true;
    truncated = true;
    len = (spaceRemaining - 1);
  }
  
  strncpy((char *) &args[argPos], str, len);
  args[argPos + len] = '\0';
  argPos += (len + 1);

 #ifdef DEBUG_ON
  if (truncated)
  {
    Serial.print(FLASH("writeStr: ***ERR["));
    printEvent(hdr.event);
    Serial.print(FLASH("]: string #"));
    Serial.print(refNum);
    
    if (bufferExceeded)
    {
      Serial.print(FLASH(" exceeds buffer length ["));
      Serial.print(strlen(str) + 1);
      Serial.print(FLASH("/"));
      Serial.print(spaceRemaining);
    }
    else
    {
      Serial.print(FLASH(" too long ["));
      Serial.print(strlen(str));
      Serial.print(FLASH("/"));
      Serial.print(maxLen);
    }
    Serial.println(FLASH("]. String truncated."));
  }
 #endif
    
  if (bufferExceeded)
    return 2;
  else if (truncated)
    return 1;
  else
    return 0;
}


//---------------------------------------------------------------------
// Read an unsigned 8-bit integer from the packet
// Arguments:
//   index:I/O   - current buffer read position; updated after reading
//                 to point to the next octet to be read.
//   err:O       - error counter (0 if no errors)
// Returns:
//   0, on error
//   n >= 0, (the value read) if err == 0
//----------------------------------------------------------------------
Uint8 AIMPacket::readUint8(int &index, int &err)
{
  const int SPACE_TAKEN = sizeof(Uint8);
  Uint8 val = 0;
  
  if (index <= MAX_ARGS_SIZE - SPACE_TAKEN)
  {
    val = args[index];
    index += SPACE_TAKEN;
  }
  else
  {
    printReadErr("Uint8");
    err++;
  }
  return val;
}


//---------------------------------------------------------------------
// Read an unsigned 16-bit integer from the packet
// Arguments:
//   index:I/O   - current buffer read position; updated after reading
//                 to point to the next octet to be read.
//   err:O       - error counter (0 if no errors)
// Returns:
//   0, on error
//   n >= 0, (the value read) if err == 0
//----------------------------------------------------------------------
Uint16 AIMPacket::readUint16(int &index, int &err)
{
  const int SPACE_TAKEN = sizeof(Uint16);
  Uint16 val = 0;
  
  err = 0;
  if (index <= MAX_ARGS_SIZE - SPACE_TAKEN)
  {
    bytesToUint16(args[index], args[index+1], val);
    index += SPACE_TAKEN;
  }
  else
  {
    printReadErr("Uint16");
    err++;
  }
  return val;
}


//---------------------------------------------------------------------
// Read an unsigned 32-bit integer from the packet
// Arguments:
//   index:I/O   - current buffer read position; updated after reading
//                 to point to the next octet to be read.
//   err:O       - error counter (0 if no errors)
// Returns:
//   0, on error
//   n >= 0, (the value read) if err == 0
//----------------------------------------------------------------------
Uint32 AIMPacket::readUint32(int &index, int &err)
{
  const int SPACE_TAKEN = sizeof(Uint32);
  Uint32 val = 0;
  
  err = 0;
  if (index <= MAX_ARGS_SIZE - SPACE_TAKEN)
  {
    bytesToUint32(args[index], args[index+1],
                  args[index+2], args[index+3],
                  val);
    index += SPACE_TAKEN;
  }
  else
  {
    printReadErr("Uint32");
    err++;
  }
  return val;
}


//---------------------------------------------------------------------
// Read a null-terminated ASCII string from the packet
// Arguments:
//   index:I/O   - current buffer read position; updated after reading
//                 to point to the next octet to be read.
//   maxLen:I    - the AIM protocol maximum allowed field length for
//                 this string.
//   err:O       - error counter (0, no errors; 1, if the string was
//                   truncated upon reading). Truncation is not really
//                   considered an error; err just provides the indication.
// Returns:
//   The, possibly truncated, string read from the packet.
//----------------------------------------------------------------------
char *AIMPacket::readStr(int &index, int maxLen, int &err)
{
  int len = strlen((char *)&args[index]);
  char *val = NULL;
  bool truncated = false;
  bool bufferExceeded = false;
  int Status = 0;
  int spaceRemaining = MAX_ARGS_SIZE - index;
  int indexIncr;
  
  indexIncr = len + 1;
  if (spaceRemaining < (len + 1))
  {
    bufferExceeded = true;
    truncated = true;
    len = (spaceRemaining - 1);
    indexIncr = spaceRemaining;
    Status = 2;
  }
  
  if (len > maxLen)
  {
    truncated = true;
    len = maxLen;
    Status = 1;
  }
  
  val = (char *)&args[index];
  val[len] = '\0';
  index += indexIncr;
 
 #ifdef DEBUG_ON 
  if (truncated)
  {
    printReadErr("readStr");
    err++;
    Serial.print(FLASH(" (Status = "));
    Serial.print(Status);
    Serial.print(FLASH(")"));
  }
 #endif
 
  return val;
}


//-------------------------------------------------------------------
// Verify that the event value stored in the packet header is one
// that we currently support.
// Arguments: (none)
// Returns:
//   true iff the event is supported
//-------------------------------------------------------------------
bool AIMPacket::verifyEvent()
{
  switch (hdr.event)
  {
    case PKT_ACK:
    case PKT_NACK:
    case PKT_HELLO:
    case PKT_IAMHERE:
    case PKT_UARE:
    case PKT_FORGET:
    case PKT_UALIVE:
    case PKT_QUERY:
    case PKT_ATTRS:
    case PKT_ATTRCHG:
    case PKT_CONTROL:
    case PKT_REPORT:
      return true;
      
    case PKT_MENUGET:
    case PKT_MENUITEM:
    case PKT_MENUCTRL:
    case PKT_REQCTRL:
    case PKT_HANDOFF:
     #ifdef DEBUG_ON 
      Serial.print(FLASH("RX unimplemented AIM event ["));
     #endif
      break;
      
    default:
     #ifdef DEBUG_ON 
      Serial.print(FLASH("RX unknown AIM event ["));
     #endif
     ;
  }
 #ifdef DEBUG_ON 
  Serial.print(hdr.event);
  Serial.println(FLASH("]"));
 #endif
  return false;
}


//--------------------------------------------------------------------
// Encode an ACK packet (for sending).
// Arguments:
//   code:I    - Error code (RET_OK, if no error)
//   diag:I    - Optional human-readable diagnostic info
// Returns:
//   Number of error encountered while writing the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::writeAck(Uint8 code, const char *const diag)
{
  int ret = 0;
  
  initPktWrite(PKT_ACK);
  ret += writeUint8(code);
  ret += writeStr(diag, DIAG_MAX, 1);
  return ret;
}


//--------------------------------------------------------------------
// Decode a (received) ACK packet.
// Arguments:
//   code:O    - Error code
//   diag:O    - Optional human-readable diagnostic info
// Returns:
//   Number of error encountered while reading the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::readAck(Uint8 &code, char *&diag)
{
  int err = 0;
  
  initPktRead();
  code = readUint8(argPos, err);
  diag = readStr(argPos, DIAG_MAX, err);
  return err;
}


//---------------------------------------------------------------------------
// Print the packet arguments to the Arduino's serial port in readable form.
// Arguments: (none)
// Returns: (none)
//---------------------------------------------------------------------------
void AIMPacket::showAck()
{
 #ifdef DEBUG_ON 
  int tmpPos = 0;
  int err = 0;
  
  Serial.println(FLASH("ACK Args:"));
  Serial.print(FLASH("Code: ")); Serial.println(readUint8(tmpPos, err));
  Serial.print(FLASH("Diag: ")); Serial.println(readStr(tmpPos, DIAG_MAX, err));
  printErrCount(err);
 #endif
}


//--------------------------------------------------------------------
// Encode a NACK packet (for sending).
// Arguments:
//   code:I    - Error code (shouldn't be RET_OK)
//   diag:I    - Optional human-readable diagnostic info
// Returns:
//   Number of error encountered while writing the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::writeNack(Uint8 code, const char *const diag)
{
  int ret = 0;
  
  ret += writeAck(code, diag);
  hdr.event = PKT_NACK;
  return ret;
}


//--------------------------------------------------------------------
// Decode a (received) NACK packet.
// Arguments:
//   code:O    - Error code
//   diag:O    - Optional human-readable diagnostic info
// Returns:
//   Number of error encountered while reading the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::readNack(Uint8 &code, char *&diag)
{
  int err = 0;
  
  err = readAck(code, diag);
  return err;
}


//---------------------------------------------------------------------------
// Print the packet arguments to the Arduino's serial port in readable form.
// Arguments: (none)
// Returns: (none)
//---------------------------------------------------------------------------
void AIMPacket::showNack()
{
 #ifdef DEBUG_ON 
  int tmpPos = 0;
  int err = 0;
  
  Serial.println(FLASH("NACK Args:"));
  Serial.print(FLASH("Code: ")); Serial.println(readUint8(tmpPos, err));
  Serial.print(FLASH("Diag: ")); Serial.println(readStr(tmpPos, DIAG_MAX, err));
  printErrCount(err);
 #endif
}


//--------------------------------------------------------------------
// Encode a HELLO packet (for sending).
// Arguments: (none)
// Returns:  0
//---------------------------------------------------------------------
int AIMPacket::writeHello()
{
  initPktWrite(PKT_HELLO);
  return 0;
}


// void AIMPacket::showHello() { ; }


//--------------------------------------------------------------------
// Encode an IAMHERE packet (for sending).
// Arguments:
//   id:I      - device ID
//   grpSz:I   - size of device group (1 if there's only one device)
//   grpId:I   - unique sequence number of the device within the group
//                 (1 <= grpId <= grpSz)
// Returns:
//   Number of error encountered while writing the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::writeIAmHere(Uint16 id, Uint8 grpSz, Uint8 grpId)
{
  int ret = 0;
  
  initPktWrite(PKT_IAMHERE);
  ret += writeUint16(id);
  ret += writeUint8(grpSz);
  ret += writeUint8(grpId);
  return ret;
}


//--------------------------------------------------------------------
// Decode a (received) IAMHERE packet.
// Arguments:
//   id:O      - device ID
//   grpSz:O   - size of device group (1 if there's only one device)
//   grpId:O   - unique sequence number of the device within the group
//                 (1 <= grpId <= grpSz)
// Returns:
//   Number of error encountered while reading the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::readIAmHere(Uint16 &id, Uint8 &grpSz, Uint8 &grpId)
{
  int err = 0;
  
  initPktRead();
  id = readUint16(argPos, err);
  grpSz = readUint8(argPos, err);
  grpId = readUint8(argPos, err);
  if ( (grpId < 1) || (grpId > grpSz) )
    err++;
  return err;
}


//---------------------------------------------------------------------------
// Print the packet arguments to the Arduino's serial port in readable form.
// Arguments: (none)
// Returns: (none)
//--------------------------------------------------------------------------
void AIMPacket::showIAmHere()
{
 #ifdef DEBUG_ON 
  int tmpPos = 0;
  int err = 0;
  
  Serial.println(FLASH("IAMHERE Args:"));
  Serial.print(FLASH("Id: ")); Serial.println(readUint16(tmpPos, err));
  Serial.print(FLASH("Group Size: ")); Serial.println(readUint8(tmpPos, err));
  Serial.print(FLASH("Group ID: ")); Serial.println(readUint8(tmpPos, err));
  printErrCount(err);
 #endif
}


//--------------------------------------------------------------------
// Encode a UARE packet.
// Arguments:
//   grpId:I   - unique sequence number of the device within the group
//   id:I      - device ID
//   loc:I     - the Location string
// Returns:
//   Number of error encountered while writing the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::writeUAre(Uint8 grpId, Uint16 id, char *loc)
{
  int len = strlen(loc);
  int ret = 0;
  
  initPktWrite(PKT_UARE);
  ret += writeUint8(grpId);
  ret += writeUint16(id);
  ret += writeStr(loc, LOC_MAX, 1);
  return ret;
}


//--------------------------------------------------------------------
// Decode a UARE packet.
// Arguments:
//   grpId:O   - unique sequence number of the device within the group
//   id:O      - device ID
//   loc:O     - the Location string
// Returns:
//   Number of error encountered while reading the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::readUAre(Uint8 &grpId, Uint16 &id, char *&loc)
{
  int err = 0;
  
  initPktRead();
  grpId = readUint8(argPos, err);
  id = readUint16(argPos, err);
  loc = readStr(argPos, LOC_MAX, err);
  return err;
}


//---------------------------------------------------------------------------
// Print the packet arguments to the Arduino's serial port in readable form.
// Arguments: (none)
// Returns: (none)
//--------------------------------------------------------------------------
void AIMPacket::showUAre()
{
 #ifdef DEBUG_ON 
  int tmpPos = 0;
  int err = 0;
  
  Serial.println(FLASH("UARE Args:"));
  Serial.print(FLASH("GroupId: ")); Serial.println(readUint8(tmpPos, err));
  Serial.print(FLASH("ID: ")); Serial.println(readUint16(tmpPos, err));
  Serial.print(FLASH("Location: ")); Serial.println(readStr(tmpPos, LOC_MAX, err));
  printErrCount(err);
 #endif
}


//--------------------------------------------------------------------
// Encode a FORGET packet.
// Arguments:
//   grpId:I   - unique sequence number of the device within the group
// Returns:
//   Number of error encountered while writing the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::writeForget(Uint8 grpId)
{
  int ret = 0;
  
  initPktWrite(PKT_FORGET);
  ret += writeUint8(grpId);
  return ret;
}


//--------------------------------------------------------------------
// Decode a FORGET packet.
// Arguments:
//   grpId:O   - unique sequence number of the device within the group
// Returns:
//   Number of error encountered while reading the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::readForget(Uint8 &grpId)
{
  int err = 0;
  
  initPktRead();
  grpId = readUint8(argPos, err);
  return err;
}


//---------------------------------------------------------------------------
// Print the packet arguments to the Arduino's serial port in readable form.
// Arguments: (none)
// Returns: (none)
//--------------------------------------------------------------------------
void AIMPacket::showForget()
{
 #ifdef DEBUG_ON 
  int tmpPos = 0;
  int err = 0;
  
  Serial.println(FLASH("FORGET Args:"));
  Serial.print(FLASH("GroupId: ")); Serial.println(readUint8(tmpPos, err));
  printErrCount(err);
 #endif
}


//--------------------------------------------------------------------
// Encode a UALIVE packet.
// Arguments:
//   id:I      - AIM device ID
// Returns:
//   Number of error encountered while writing the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::writeUAlive(Uint16 id)
{
  int ret = 0;
  
  initPktWrite(PKT_UALIVE);
  ret += writeUint16(id);
  return ret;
}


//--------------------------------------------------------------------
// Decode a UALIVE packet.
// Arguments:
//   id:O      - AIM device ID
// Returns:
//   Number of error encountered while reading the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::readUAlive(Uint16 &id)
{
  int err = 0;
  
  initPktRead();
  id = readUint16(argPos, err);
  return err;
}


//---------------------------------------------------------------------------
// Print the packet arguments to the Arduino's serial port in readable form.
// Arguments: (none)
// Returns: (none)
//--------------------------------------------------------------------------
void AIMPacket::showUAlive()
{
 #ifdef DEBUG_ON 
  int tmpPos = 0;
  int err = 0;
  
  Serial.println(FLASH("UALIVE Args:"));
  Serial.print(FLASH("ID: ")); Serial.println(readUint16(tmpPos, err));
  printErrCount(err);
 #endif
}


//--------------------------------------------------------------------
// Encode a QUERY packet.
// Arguments:
//   grpId:I   - unique sequence number of the device within the group
// Returns:
//   Number of error encountered while writing the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::writeQuery(Uint8 grpId)
{
  int ret = 0;
  
  initPktWrite(PKT_QUERY);
  ret += writeUint8(grpId);
  return ret;
}


//--------------------------------------------------------------------
// Decode a QUERY packet.
// Arguments:
//   grpId:O   - unique sequence number of the device within the group
// Returns:
//   Number of error encountered while reading the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::readQuery(Uint8 &grpId)
{
  int err = 0;
  
  initPktRead();
  grpId = readUint8(argPos, err);
  return err;
}


//---------------------------------------------------------------------------
// Print the packet arguments to the Arduino's serial port in readable form.
// Arguments: (none)
// Returns: (none)
//--------------------------------------------------------------------------
void AIMPacket::showQuery()
{
 #ifdef DEBUG_ON 
  int tmpPos = 0;
  int err = 0;
  
  Serial.println(FLASH("QUERY Args:"));
  Serial.print(FLASH("GroupId: ")); Serial.println(readUint8(tmpPos, err));
  printErrCount(err);
 #endif
}


//--------------------------------------------------------------------
// Encode an ATTRS packet.
// Arguments:
//   id:I        - AIM device ID
//   loc:I       - Location string
//   grpSz:I     - size of device group (1 if there's only one device)
//   grpId:I     - unique sequence number of the device within the group
//                   (1 <= grpId <= grpSz)
//   type:I      - AIM device type identifier
//   scale:I     - Scaling to be applied on values reported by the device
//                   (Actual-Value = Reported-Value * 10^scale)
//   dev_class:I - Human-readable string to identify the device's
//                   classification
//   rngl:I      - The device's lowest valid value
//   rnglh:I     - The device's highest valid value
//   zero:I      - The value indicating the zero point in the device's
//                   value range
//   units:I     - Human-readable string to indicate the measurement units
//                   to be associated with the device's values
//                   (e.g. inches, centimeters, degree celcius)
// Returns:
//   Number of error encountered while writing the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::writeAttrs(Uint16 id, char *loc, Uint8 grpSz, Uint8 grpId,
                          Uint8 type, Int8 scale, char *dev_class, Int16 rngl,
                          Int16 rngh, Int16 zero, char *units)
{
  int ret = 0;
  
  initPktWrite(PKT_ATTRS);
  ret += writeUint16(id);
  ret += writeStr(loc, LOC_MAX, 1);
  ret += writeUint8(grpSz);
  ret += writeUint8(grpId);
  ret += writeUint8(type);
  ret += writeUint8((Uint8) scale);
  ret += writeStr(dev_class, CLASS_MAX, 2);
  ret += writeUint16(rngl);
  ret += writeUint16(rngh);
  ret += writeUint16(zero);
  ret += writeStr(units, UNITS_MAX, 3);

  return ret;
}


//--------------------------------------------------------------------
// Decode an ATTRS packet.
// Arguments:
//   id:O        - AIM device ID
//   loc:O       - Location string
//   grpSz:O     - size of device group (1 if there's only one device)
//   grpId:O     - unique sequence number of the device within the group
//                   (1 <= grpId <= grpSz)
//   type:O      - AIM device type identifier
//   scale:O     - Scaling to be applied on values reported by the device
//                   (Actual-Value = Reported-Value * 10^scale)
//   dev_class:O - Human-readable string to identify the device's
//                   classification
//   rngl:O      - The device's lowest valid value
//   rnglh:O     - The device's highest valid value
//   zero:O      - The value indicating the zero point in the device's
//                   value range
//   units:O     - Human-readable string to indicate the measurement units
//                   to be associated with the device's values
//                   (e.g. inches, centimeters, degree celcius)
// Returns:
//   Number of error encountered while reading the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::readAttrs(Uint16 &id, char *&loc, Uint8 &grpSz, Uint8 &grpId,
                         Uint8 &type, Int8 &scale, char *&dev_class,
                         Int16 &rngl, Int16 &rngh, Int16 &zero, char *&units)
{
  int err = 0;
  
  initPktRead();
  id = readUint16(argPos, err);
  loc = readStr(argPos, LOC_MAX, err);
  grpSz = readUint8(argPos, err);
  grpId = readUint8(argPos, err);
  type = readUint8(argPos, err);
  scale = (Int8) readUint8(argPos, err);
  dev_class = readStr(argPos, CLASS_MAX, err);
  rngl = (Int16) readUint16(argPos, err);
  rngh = (Int16) readUint16(argPos, err);
  zero = (Int16) readUint16(argPos, err);
  units = readStr(argPos, UNITS_MAX, err);
  return err;
}


//---------------------------------------------------------------------------
// Print the packet arguments to the Arduino's serial port in readable form.
// Arguments: (none)
// Returns: (none)
//--------------------------------------------------------------------------
void AIMPacket::showAttrs()
{
 #ifdef DEBUG_ON 
  int    tmpPos = 0;
  int    err = 0;

  Serial.println(FLASH("ATTRS Args:"));
  Serial.print(FLASH("ID: ")); Serial.println(readUint16(tmpPos, err));
  Serial.print(FLASH("Location: ")); Serial.println(readStr(tmpPos, LOC_MAX, err));
  Serial.print(FLASH("Group Size: ")); Serial.println(readUint8(tmpPos, err));
  Serial.print(FLASH("Group ID: ")); Serial.println(readUint8(tmpPos, err));
  Serial.print(FLASH("Type: ")); printDevType(readUint8(tmpPos, err)); Serial.println();
  Serial.println(FLASH("Values:"));
  Serial.print(FLASH("  Scale: ")); Serial.println((Int8)readUint8(tmpPos, err), DEC);
  Serial.print(FLASH("  Class: ")); Serial.println(readStr(tmpPos, CLASS_MAX, err));
  Serial.println(FLASH("  Values:"));
  Serial.print(FLASH("    Lower bound: "));
    Serial.println((Int16)readUint16(tmpPos, err));
  Serial.print(FLASH("    Upper bound: "));
    Serial.println((Int16)readUint16(tmpPos, err));
  Serial.print(FLASH("    Zero point: "));
    Serial.println((Int16)readUint16(tmpPos, err));
  Serial.print(FLASH("    Units: ")); Serial.println(readStr(tmpPos, UNITS_MAX, err));
  printErrCount(err);
 #endif
}


//--------------------------------------------------------------------
// Encode an ATTRCHG packet.
// Arguments:
//   id:I      - AIM device ID
// Returns:
//   Number of error encountered while writing the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::writeAttrChg(Uint16 id)
{
  int ret = 0;
  
  initPktWrite(PKT_ATTRCHG);
  ret += writeUint16(id);
  return ret;
}


//--------------------------------------------------------------------
// Decode an ATTRCHG packet.
// Arguments:
//   id:I      - AIM device ID
// Returns:
//   Number of error encountered while reading the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::readAttrChg(Uint16 &id)
{
  int err = 0;
  
  initPktRead();
  id = readUint16(argPos, err);
  return err;
}


//---------------------------------------------------------------------------
// Print the packet arguments to the Arduino's serial port in readable form.
// Arguments: (none)
// Returns: (none)
//--------------------------------------------------------------------------
void AIMPacket::showAttrChg()
{
 #ifdef DEBUG_ON 
  int tmpPos = 0;
  int err = 0;
  
  Serial.println(FLASH("ATTRCHG Args:"));
  Serial.print(FLASH("ID: ")); Serial.println(readUint16(tmpPos, err));
  printErrCount(err);
 #endif
}


//--------------------------------------------------------------------
// Encode a CONTROL packet.
// Arguments:
//   id:I      - AIM device ID
//   action:I  - Control action to take (e.g. ACT_SET, ACT_READ)
//   value:I   - Control action value:
//                 - for ACT_SET, the desired value to set the device to
//                 - for ACT_READ:
//                     0 = report device value once
//                     n > 0, report device value every n milliseconds
// Returns:
//   Number of error encountered while writing the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::writeControl(Uint16 id, Actions action, Int16 value)
{
  int ret = 0;
  
  initPktWrite(PKT_CONTROL);
  ret += writeUint16(id);
  ret += writeUint8((Uint8) action);
  ret += writeUint16((Uint16) value);
  return ret;
}


//--------------------------------------------------------------------
// Decode a CONTROL packet.
// Arguments:
//   id:O      - AIM device ID
//   action:O  - Control action to take (e.g. ACT_SET, ACT_READ)
//   value:O   - Control action value:
//                 - for ACT_SET, the desired value to set the device to
//                 - for ACT_READ:
//                     0 = report device value once
//                     n > 0, report device value every n milliseconds
// Returns:
//   Number of error encountered while reading the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::readControl(Uint16 &id, Uint8 &action, Int16 &value)
{
  int err = 0;
  
  initPktRead();
  id = readUint16(argPos, err);
  action = (Actions) readUint8(argPos, err);
  value = (Int16) readUint16(argPos, err);
  return err;
}


//---------------------------------------------------------------------------
// Print the packet arguments to the Arduino's serial port in readable form.
// Arguments: (none)
// Returns: (none)
//--------------------------------------------------------------------------
void AIMPacket::showControl()
{
 #ifdef DEBUG_ON
  int tmpPos = 0;
  int err = 0;
  
  Serial.println(FLASH("CONTROL Args:"));
  Serial.print(FLASH("ID: ")); Serial.println(readUint16(tmpPos, err));
  Serial.print(FLASH("Action: "));
  printAction((Actions)readUint8(tmpPos, err));
  Serial.println();
  Serial.print(FLASH("Value: ")); Serial.println((Int16)readUint16(tmpPos, err));
  printErrCount(err);
 #endif
}


//--------------------------------------------------------------------
// Encode a REPORT packet.
// Arguments:
//   id:I      - AIM device ID
//   value:I   - The value associated with the device
// Returns:
//   Number of error encountered while writing the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::writeReport(Uint16 id, Int16 value)
{
  int ret = 0;
  
  initPktWrite(PKT_REPORT);
  ret += writeUint16(id);
  ret += writeUint16((Uint16) value);
  return ret;
}


//--------------------------------------------------------------------
// Decode a REPORT packet.
// Arguments:
//   id:O      - AIM device ID
//   value:O   - The value associated with the device
// Returns:
//   Number of error encountered while reading the fields
//     (0 if no error).
//---------------------------------------------------------------------
int AIMPacket::readReport(Uint16 &id, Int16 &value)
{
  int err = 0;
  
  initPktRead();
  
  id = readUint16(argPos, err);
  value = (Int16) readUint16(argPos, err);
  return err;
}


//---------------------------------------------------------------------------
// Print the packet arguments to the Arduino's serial port in readable form.
// Arguments: (none)
// Returns: (none)
//--------------------------------------------------------------------------
void AIMPacket::showReport()
{
 #ifdef DEBUG_ON 
  int tmpPos = 0;
  int err = 0;
  
  Serial.println(FLASH("REPORT Args:"));
  Serial.print(FLASH("ID: ")); Serial.println(readUint16(tmpPos, err));
  Serial.print(FLASH("Value: ")); Serial.println((Int16)readUint16(tmpPos, err));
 #endif
}


//-------------------------------------------------------------------
// Print a dotted IP v4 address to the Arduino's serial port.
// Arguments:
//   ip:I   - the 4 octets of the IP address
// Returns: (none)
//-------------------------------------------------------------------
void AIMPacket::printIpAddr(byte *ip)
{
 #ifdef DEBUG_ON 
  for (int i = 0; i < 4; i++)
  {
    Serial.print(ip[i], DEC);
    if (i < 3)
      Serial.print(".");
  }
 #endif
}


//-------------------------------------------------------------------
// Print an event to the Arduino's serial port in human-readable form.
// Arguments:
//   event:I  - the event to print
// Returns:  (none)
//--------------------------------------------------------------------
void AIMPacket::printEvent(Uint8 event)
{
 #ifdef DEBUG_ON 
  Serial.print(event);
  Serial.print(FLASH(" ["));
  switch (event)
  {
    case PKT_UNDEF:    Serial.print(FLASH("<undefined>"));  break;
    case PKT_ACK:      Serial.print(FLASH("PKT_ACK"));      break;
    case PKT_NACK:     Serial.print(FLASH("PKT_NACK"));     break;
    case PKT_HELLO:    Serial.print(FLASH("PKT_HELLO"));    break;
    case PKT_IAMHERE:  Serial.print(FLASH("PKT_IAMHERE"));  break;
    case PKT_UARE:     Serial.print(FLASH("PKT_UARE"));     break;
    case PKT_FORGET:   Serial.print(FLASH("PKT_FORGET"));   break;
    case PKT_UALIVE:   Serial.print(FLASH("PKT_UALIVE"));   break;
    case PKT_QUERY:    Serial.print(FLASH("PKT_QUERY"));    break;
    case PKT_ATTRS:    Serial.print(FLASH("PKT_ATTRS"));    break;
    case PKT_ATTRCHG:  Serial.print(FLASH("PKT_ATTRCHG"));  break;
    case PKT_CONTROL:  Serial.print(FLASH("PKT_CONTROL"));  break;
    case PKT_REPORT:   Serial.print(FLASH("PKT_REPORT"));   break;
//JVS?? The following are not yet supported
//    case PKT_MENUGET:  Serial.print(FLASH("PKT_MENUGET"));  break;
//    case PKT_MENUITEM: Serial.print(FLASH("PKT_MENUITEM")); break;
//    case PKT_MENUCTRL: Serial.print(FLASH("PKT_MENUCTRL")); break;
//    case PKT_REQCTRL:  Serial.print(FLASH("PKT_REQCTRL"));  break;
//    case PKT_HANDOFF:  Serial.print(FLASH("PKT_HANDOFF"));  break;
    
    default:
      Serial.print(FLASH("<UNSUPPORTED>"));
  }
  Serial.print(FLASH("]"));
 #endif
}


//-----------------------------------------------------------------------
// Print an AIM CONTROL packet action to the Arduino's serial port
// in human-readable form.
// Arguments:
//   action:I   - the action to print (e.g. ACT_SET printed as "ACT_SET")
// Returns:  (none)
//------------------------------------------------------------------------
void AIMPacket::printAction(Actions action)
{
 #ifdef DEBUG_ON 
  Serial.print(action);
  Serial.print(FLASH(" ["));
  switch (action)
  {
    case ACT_SET:  Serial.print(FLASH("ACT_SET"));  break;
    case ACT_READ: Serial.print(FLASH("ACT_READ")); break;
    
    default:
      Serial.print(FLASH("<unsupported>"));
  }
  Serial.print(FLASH("]"));
 #endif
}


//---------------------------------------------------------------------
// Print a device type to the Arduino's serial port in human-readable
// form.
// Arguments:
//   type:I   - the device type to print
// Returns:  (none)
// Notes:
//   We can only print out the known device types. Types may be
//   added in the future; but, we don't necessarily care.
//---------------------------------------------------------------------
void AIMPacket::printDevType(Uint8 type)
{
 #ifdef DEBUG_ON 
  Serial.print(type);
  Serial.print(FLASH(" ["));
  switch (type)
  {
    case DEV_SRV:   Serial.print(FLASH("DEV_SRV"));    break;
    case DEV_MON:   Serial.print(FLASH("DEV_MON"));    break;
    case DEV_CTRL:  Serial.print(FLASH("DEV_CTRL"));   break;
    case DEV_SMART: Serial.print(FLASH("DEV_SMART"));  break;
    case DEV_MENU:  Serial.print(FLASH("DEV_MENU"));   break;
    
    default:
      Serial.print(FLASH("<unsupported>"));
  }
  Serial.print(FLASH("]"));
 #endif
}


//------------------------------------------------------------------
// Print, to the Arduino's serial port, a 'read past buffer' error
// for a given function name and an implied AIM event (taken from
// the packet header).
// Arguments:
//   progName:I    - a string intended to name the function in which
//                   the error occurred.
// Returns:  (none)
//------------------------------------------------------------------
void AIMPacket::printReadErr(char *progName)
{
 #ifdef DEBUG_ON 
  Serial.print(FLASH("read"));
  Serial.print(progName);
  Serial.print(FLASH(": ***ERR["));
  printEvent(hdr.event);
  Serial.print(FLASH("] Read past buffer"));
 #endif
}


//----------------------------------------------------------------
// Print a specified error count to the Arduino's serial port.
// Arguments:
//   err:I   - the error count
// Returns:  (none)
//-----------------------------------------------------------------
void AIMPacket::printErrCount(int err)
{
 #ifdef DEBUG_ON 
  if (err > 0)
  {
    Serial.print(FLASH("Error count: "));
    Serial.println(err);
  }
 #endif
}


//------------------------------------------------------------------
// Print an AIM return code to the Arduino's serial port.
// Argument:
//   code:I   - the return code to print
// Returns:  (none)
//------------------------------------------------------------------
void AIMPacket::printRetCode(Uint8 code)
{
 #ifdef DEBUG_ON 
  switch (code)
  {
    RET_OK:        Serial.print(FLASH("OK")); break;
    RET_ERR:       Serial.print(FLASH("Error")); break;
    RET_PROTOCOL:  Serial.print(FLASH("Prot Vers Err")); break;
    RET_EVENT:     Serial.print(FLASH("Unsupported event")); break;
    RET_TOOLARGE:  Serial.print(FLASH("Packet too large")); break;
    RET_MALFORMED: Serial.print(FLASH("Malformed pkt content")); break;
    RET_ARGUMENT:  Serial.print(FLASH("Arg value out of range")); break;
    
    default:       Serial.print(FLASH("<unknown>"));
  }
 #endif
}


//-----------------------------------------------------------------------
// Print the contents of the packet to the Arduino's serial port in
// human-readable form.
// Arguments:  (none)
// Returns:  (none)
//------------------------------------------------------------------------
void AIMPacket::show()
{
 #ifdef DEBUG_ON 
  Serial.println(FLASH("AIM Packet"));
  Serial.println(FLASH("=========="));
  Serial.print(FLASH("AIM header length: ")); Serial.println(sizeof(AimHeader));
  Serial.print(FLASH("AIM arguments length: ")); Serial.println(argPos);
  Serial.print(FLASH("Total: ")); Serial.println(sizeof(AimHeader) + argPos);
  Serial.println(FLASH("AIM Packet Header:"));
  Serial.print(FLASH("  Protocol Version: "));
    Serial.println(hdr.protocolVersion);
  Serial.print(FLASH("  Event: ")); printEvent(hdr.event); Serial.println();
  Serial.print(FLASH("  Transaction Id: ")); Serial.println(hdr.transId);
  Serial.println(FLASH("  Arguments:"));
  Serial.println(FLASH("  ----------"));
  Serial.println(FLASH("    Hex dump:"));
  if (argPos > 0)
  {
    for (int i = 0; i < argPos; i++)
    {
      Serial.print(args[i], HEX);
      Serial.print(FLASH("  "));
    }
    Serial.println();
  }
  else
    Serial.println(FLASH("      <no args to dump>"));
  switch (hdr.event)
  {
    case PKT_ACK:      showAck();      break;
    case PKT_NACK:     showNack();     break;
    case PKT_HELLO:    showHello();    break;
    case PKT_IAMHERE:  showIAmHere();  break;
    case PKT_UARE:     showUAre();     break;
    case PKT_FORGET:   showForget();   break;
    case PKT_UALIVE:   showUAlive();   break;
    case PKT_QUERY:    showQuery();    break;
    case PKT_ATTRS:    showAttrs();    break;
    case PKT_ATTRCHG:  showAttrChg();  break;
    case PKT_CONTROL:  showControl();  break;
    case PKT_REPORT:   showReport();   break;
//JVS?? The following aren't supported yet.
//    case PKT_MENUGET:  showMenuGet();  break;
//    case PKT_MENUITEM: showMenuItem(); break;
//    case PKT_MENUCTRL: showMenuCtrl(); break;
//    case PKT_REQCTRL:  showReqCtrl();  break;
//    case PKT_HANDOFF:  showHandoff();  break;
    default:
      Serial.println(FLASH("  <Unrecognized event. No args available.>"));
  }
  Serial.println(); Serial.println();
 #endif
}



/**************************  AIMProtocol class *****************************
 ***************************************************************************/


// Static member variable initialization
bool AIMProtocol::instantiated = false;
AIMProtocol  *AIMProtocol::instance = NULL;



//-------------------------------------------------------------------------
// Startup the AIM protocol.
// Arguments:
//   mac:I          - MAC address of the Arduino Ethernet shield
//                    (It seems they don't have preprogrammed MAC
//                    addresses)
//   ip:I           - The Arduino's local IP address. If the address is
//                    0.0.0.0, DHCP will be used to acquire the IP address
//                    dynamically; otherwise the value of 'ip' denotes a
//                    static address.
//   udpPort:I      - The local UDP port used for AIM communciations by
//                    this Arduino.
//   protVers:I     - Identifies the most recent AIM protocol version
//                    that this Arduino supports.
// Returns:  (none)
// Notes:
//   1) Using DHCP adds about 3KB to the size of program store. Recommend
//      removing that part of the code and allow only static IP addresses;
//      We would need to have a mechanism by which the local static IP
//      address could be datafilled (and stored in EEPROM for use during
//      powerup). (To remove DHCP code, check the code dealing with variable
//      defaultIp--you won't need that variable any longer.)
//   2) AIM devices are assumed to reside on a subnet using a 24 bit
//      netmask (i.e. 255.255.255.0)
//-------------------------------------------------------------------------
void AIMProtocol::startAimProtocol(byte *mac, IPAddress &ip,
                                   Uint16 udpPort,
                                   Uint8 protVers)
{
  bool defaultIp = true;
  
  memset(local_mac, 0, 6);
  memcpy(local_mac, local_mac, 6);
  local_udp_port = udpPort;
  this->protVers = protVers;
  for (int i = 0; i < 4; i++)
    if (ip[i] != 0)
    {
      defaultIp = false;
      break;
    }
  if (defaultIp)
    Ethernet.begin(local_mac); // JVS?? This adds ~3KB to program store
  else
    Ethernet.begin(local_mac, local_ip);
  local_ip = Ethernet.localIP();
  broadcast_ip = local_ip;
  broadcast_ip[3] = 255;
  udp.begin(local_udp_port);
  protStarted = true;
}


//------------------------------------------------------------------------
// Send an AIM packet.
// Arguments:
//   dstIp:I   - the destination IP address; IAMHERE and ATTRCHG packets
//               ignore this value and use the broadcast address instead
//   dstPort:I - the destination AIM UDP port (typically the default port,
//               7770).
//   transId:I - the transaction identifier to be used; note that ACK/NACK
//               is supposed to use the transaction ID of the packet to
//               which they are responding.
// Returns:  (none)
//------------------------------------------------------------------------
void AIMProtocol::sendAim(const IPAddress &dstIp,
                          Uint16 dstPort, Uint16 transId)
{
  IPAddress actualDstIp;
  
  switch (outPkt.getEvent())
  {
    case AIMPacket::PKT_IAMHERE:
    case AIMPacket::PKT_ATTRCHG:
      actualDstIp = broadcast_ip;
      break;
    default:
      actualDstIp = dstIp;
  }
  outPkt.setProtocolVersion(protVers);
  outPkt.setTransId(transId);
  outPkt.sendPacket(udp, actualDstIp, dstPort);
}


//------------------------------------------------------------------------
// Retrieves a UDP packet if one has been received.
// Arguments:
//   event:O   - identifies the AIM packet event field;
//               unknown/unsupported events are ignored quietly
//   transId:O - the packet's transaction ID
//   srcIp:O   - source IP address
//   srcPort:O - source AIM UDP port
//   err:O     - error indication (AIM return code)
// Returns:
//   true iff a valid AIM packet was received.
//------------------------------------------------------------------------
bool AIMProtocol::packetRcvd(Uint8 &event, Uint16 &transId,
                             IPAddress &srcIp, Uint16 &srcPort, int &err)
{
  byte tmpByte1, tmpByte2;
  bool recvOk;
  Uint16 bytesLeft;
  int pktSize = 0;
 
  err = AIMPacket::RET_OK;
  event = AIMPacket::PKT_UNDEF;
  transId = 0;
  if (!protStarted)
    return false;

  pktSize = udp.parsePacket();
  if (pktSize)
  {
    bytesLeft = pktSize;
    srcIp = udp.remoteIP();
    srcPort = udp.remotePort();
    recvOk = inPkt.receivePacket(udp, bytesLeft, err);
    if (!verifyProtVers(inPkt.getProtocolVersion()))
    {
      err = AIMPacket::RET_PROTOCOL;
      return false;
    }
    if (recvOk)
    {
      event = inPkt.getEvent();
      transId = inPkt.getTransId();
      return true;
    }
    else
    {
      if (err != AIMPacket::RET_OK)
      {
        outPkt.writeNack(err, "Can't parse pkt");
        sendAim(srcIp, srcPort, transId); // transId could be 0
      }
    }
  }
  return false;
}


//------------------------------------------------------------------------
// Check if a valid AIM packet has been received and, if so, invoke the
// corresponding handleRx####() function.
// Arguments:  (none)
// Returns:  (none)
//------------------------------------------------------------------------
void AIMProtocol::handleRxAimPacket()
{
  Uint8     event;
  Uint16    transId;
  IPAddress dstIp;
  Uint16    dstPort;
  int       err;
  
  err = AIMPacket::RET_OK;
  diag[0] = '\0';
  if (packetRcvd(event, transId, dstIp, dstPort, err))
  {
//JVS??
Serial.print("Pkt Rcvd: Event="); Serial.println(event);
    switch (event)
    {
      case AIMPacket::PKT_ACK:
        {
          Uint8  code;
          char  *diag;
          
          inPkt.readAck(code, diag);
          err = handleRx_Ack(transId, dstIp, dstPort, code, diag);
          break;
        }
      case AIMPacket::PKT_NACK:
        {
          Uint8  code;
          char  *diag;
         
          inPkt.readNack(code, diag);
          err = handleRx_Nack(transId, dstIp, dstPort, code, diag); 
          break;
        }
      case AIMPacket::PKT_HELLO:
        {
          err = handleRx_Hello(transId, dstIp, dstPort);
          break;
        }
      case AIMPacket::PKT_IAMHERE:
        {
          Uint16  id;
          Uint8   grpSz, grpId;
          
          inPkt.readIAmHere(id, grpSz, grpId);
          err = handleRx_IAmHere(transId, dstIp, dstPort, id, grpSz, grpId);
          break;
        }
      case AIMPacket::PKT_UARE:
        {
          Uint8   grpId;
          Uint16  id;
          char   *loc;
          
          inPkt.readUAre(grpId, id, loc);
          err = handleRx_UAre(transId, dstIp, dstPort, grpId, id, loc);
          break;
        }
      case AIMPacket::PKT_FORGET:
        {
          Uint8   grpId;
          
          inPkt.readForget(grpId);
          err = handleRx_Forget(transId, dstIp, dstPort, grpId);
          break;
        }
      case AIMPacket::PKT_UALIVE:
        {
          Uint16  id;
          
          inPkt.readUAlive(id);
          err = handleRx_UAlive(transId, dstIp, dstPort, id);
          break;
        }
      case AIMPacket::PKT_QUERY:
        {
          Uint8  grpId;
          
          inPkt.readQuery(grpId);
          err = handleRx_Query(transId, dstIp, dstPort, grpId);
          break;
        }
      case AIMPacket::PKT_ATTRS:
        {
          Uint8   grpSz, grpId, devType;
          Int8    scale;
          Uint16  id;
          Int16   rngH, rngL, zero;
          char   *loc, *devClass, *units;

          inPkt.readAttrs(id, loc, grpSz, grpId, devType, scale, devClass,
                    rngH, rngL, zero, units);
          err = handleRx_Attrs(transId, dstIp, dstPort,
                               id, loc, grpSz, grpId, devType, scale, devClass,
                               rngH, rngL, zero, units);
          break;
        }
      case AIMPacket::PKT_ATTRCHG:
        {
          Uint16  id;
          
          inPkt.readAttrChg(id);
          err = handleRx_AttrChg(transId, dstIp, dstPort, id);
          break;
        }
      case AIMPacket::PKT_CONTROL:
        {
          Uint8   action;
          Uint16  id;
          Int16   value;
          
          inPkt.readControl(id, action, value);
          err = handleRx_Control(transId, dstIp, dstPort, id, action, value);
          break;
        }
      case AIMPacket::PKT_REPORT:
        {
          Uint16  id;
          Int16   value;
          
          inPkt.readReport(id, value);
          err = handleRx_Report(transId, dstIp, dstPort, id, value);
          break;
        }
      default:
        Serial.print(FLASH("RX unsupported packet. Event="));
        Serial.println(event);
    }
  }
  
  // Catch unhandled exceptions.
  if (err != AIMPacket::RET_OK)
  {
    outPkt.writeNack(err, "<Cause unknown>");
    sendAim(dstIp, dstPort, transId);
  }
}


/*===================================================================
 * Virtual functions for handling received AIM packets. The base
 * class functionality is simply to print the packet contents to
 * the Arduino's serial port (this only happens if DEBUG_ON is 
 * defined).
 * Applications are expected to override each function that handles
 * a packet the application cares about.
 *
 * IMPORTANT: Refer to the notes within each function below.
 * =========
 *====================================================================
 */
int AIMProtocol::handleRx_Ack(Uint16 transId, IPAddress const &dstIp,
                              Uint16 dstPort, Uint8 code, char *diag)
{
  // You only need to handle this if you're sending an AIM packet
  // that would require an ACK; even then you might chose to ignore it.
  inPkt.show();
  return AIMPacket::RET_OK;
}


int AIMProtocol::handleRx_Nack(Uint16 transId, IPAddress const &dstIp,
                               Uint16 dstPort, Uint8 code, char *diag)
{
  // You only need to handle this if you're sending an AIM packet
  // that would require an ACK--you can expect a NACK rather than
  // an ACK; even then you might chose to ignore it.
  inPkt.show();
  return AIMPacket::RET_OK;
}


int AIMProtocol::handleRx_Hello(Uint16 transId, IPAddress const &dstIp,
                                Uint16 dstPort)
{
  // This currently isn't supported as it is likely to be removed from
  // the protocol as the Arduino's Ethernet library can't currently
  // handle IP broadcasts. The workaround is to require every non-GRAMS
  // AIM device to broadcast an IAMHERE packet periodically.
  inPkt.show();
  return AIMPacket::RET_OK;
}


int AIMProtocol::handleRx_IAmHere(Uint16 transId, const IPAddress &dstIp,
                    Uint16 dstPort, Uint16 id, Uint8 grpSz, Uint8 grpId)
{
  // You can ignore this. Since they are broadcast, you're not going
  // to see them from other AIM devices (including GRAMS). (Refer to
  // handleRx_Hello().)
  inPkt.show();
  return AIMPacket::RET_OK;
}


int AIMProtocol::handleRx_UAre(Uint16 transId, const IPAddress &dstIp,
                    Uint16 dstPort, Uint8 grpId, Uint16 id, char *loc)
{
  // You must override this function as GRAMS will send these to you
  // when it wants you to change your ID or Location. You are expected
  // to remember the ID and Location values--store them in EEPROM so
  // that they survive over power cycles. (GRAMS can tolerate it if
  // you don't store the values in EEPROM. However, users will likely
  // get annoyed if their assigned Location is forgotten.)
  // IMPORTANT: Be sure to reply with an ACK (using the sender's
  //            transaction ID)
  inPkt.show();
  return AIMPacket::RET_OK;
}


int AIMProtocol::handleRx_Forget(Uint16 transId, IPAddress const &dstIp,
                               Uint16 dstPort, Uint8 grpId)
{
  // You must override this function as GRAMS will send these when it
  // wants you to forget your ID and Location. When received, set your
  // ID to 0 and your Location to the empty string; you should store
  // these new values in EEPROM (refer to handleRx_UAre()).
  // Be sure to send an ACK with the sender's transaction ID.
  inPkt.show();
  return AIMPacket::RET_OK;
}


int AIMProtocol::handleRx_UAlive(Uint16 transId, IPAddress const &dstIp,
                               Uint16 dstPort, Uint16 id)
{
//JVS?? This function hasn't been tested yet.
  // Normally, you don't need to override this function.
  // You may choose to override it. However, be sure you
  // send an ACK with the sender's transaction ID if you wish to
  // acknowledge that you're still on the AIM network.
  inPkt.show();
  outPkt.writeAck(AIMPacket::RET_OK, "");
  sendAim(dstIp, dstPort, transId);
  return AIMPacket::RET_OK;
}


int AIMProtocol::handleRx_Query(Uint16 transId, IPAddress const &dstIp,
                              Uint16 dstPort, Uint8 grpId)
{
  // You must override this function. The expectation is that you
  // reply with an ATTRS packet for the device having the specified
  // group ID.
  inPkt.show();
  return AIMPacket::RET_OK;
}


int AIMProtocol::handleRx_Attrs(Uint16 transId, IPAddress const &dstIp,
           Uint16 dstPort, Uint16 id, char *loc, Uint8 grpSz, Uint8 grpId,
           Uint8 devType, Int8 scale, char *devClass,
           Int16 rngH, Int16 rngL, Int16 zero, char *units)
{
  // If you are a display device or a local controller device, you
  // should override this function as you will likely be sending QUERY
  // packets to other devices--they will reply with ATTRS packets.
  // Otherwise, you can ignore this function.
  inPkt.show();
  return AIMPacket::RET_OK;
}


int AIMProtocol::handleRx_AttrChg(Uint16 transId, IPAddress const &dstIp,
                                Uint16 dstPort, Uint16 id)
{
  // (See the comments in handleRx_Attrs().)
  // You may wish to override this function if you care about attribute
  // changes in other devices on the network.
  inPkt.show();
  return AIMPacket::RET_OK;
}


int AIMProtocol::handleRx_Control(Uint16 transId, IPAddress const &dstIp,
                    Uint16 dstPort, Uint16 id, Uint8 action, Int16 value)
{
  // You must override this function. The expectation is that you
  // respond with an ACK or one or more REPORT packets, depending
  // on the action.
  inPkt.show();
  return AIMPacket::RET_OK;
}


int AIMProtocol::handleRx_Report(Uint16 transId, IPAddress const &dstIp,
                               Uint16 dstPort, Uint16 id, Int16 value)
{
  // (See the comments in handleRx_AttrChg().)
  inPkt.show();
  return AIMPacket::RET_OK;
}

//=================  End AIMProtocol Virtual Functions  ====================


//------------------------------------------------------------------
// Verify that we can handle the specified protocol version.
// Arguments:
//   protVers:I  - the protocol version in quesion
// Returns:
//   true iff we are able to handle the specified version of the AIM
//            protocol.
//------------------------------------------------------------------
bool AIMProtocol::verifyProtVers(Uint8 protVers)
{
  if (protVers <= PROTOCOL_VERSION)
    return true;
  return false;
}


/************************************************************************
 ************   END of Arduino AIM Protocol Base Classes   **************
 ************************************************************************/ 
 
