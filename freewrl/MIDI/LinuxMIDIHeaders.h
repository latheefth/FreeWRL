/*******************************************************************
 Copyright (C) 2007 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.

 Note that these files are stripped of comments, and non-Linux code. The reason for
 this is that Propellerheads' ReWire IP is protected - the Linux code does not 
 contain any ReWire protocol code (despite file names). At the present time, I am
 unsure if the original source, with all comments and #ifdefs, and Propellerheads'
 system calls can be released as open source.

 John Stewart, CRC Canada.
 October 8, 2007.

*********************************************************************/
# 1 "LinuxMIDIHeaders.h"
# 1 "/root/usb/FreeWRL_Reason_MIDI_Interface//"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "LinuxMIDIHeaders.h"
# 12 "LinuxMIDIHeaders.h"
typedef int Byte;
# 35 "LinuxMIDIHeaders.h"
struct MIDIPacket {
 long unsigned int timeStamp;
 int length;
 char data[256];
};
typedef struct MIDIPacket MIDIPacket;
struct MIDIPacketList {
 MIDIPacket *packet;
 int numPackets;
};
typedef struct MIDIPacketList MIDIPacketList;

typedef int * MIDIPortRef;
typedef int * MIDIClientRef;
typedef int *MIDIEndpointRef;
typedef int SInt32;


struct CFStringRef { char *strptr; int len; }; typedef struct CFStringRef CFStringRef;

MIDIPacket *MIDIPacketListAdd(MIDIPacketList *outPacketList, int sz, MIDIPacket* outPacketPtr, int x, int y, const Byte *data);
void initializeCFString (CFStringRef me);
void CFRelease (CFStringRef me);
void CFStringGetCString(CFStringRef pname, char *name, int i, int x);
int MIDISend (int bus, Byte *data, int len);
MIDIPacket * MIDIPacketNext(MIDIPacket *inPacket);
int MIDIGetNumberOfSources (void);
int MIDIGetNumberOfDestinations (void);
MIDIEndpointRef MIDIGetSource (int i);
void MIDIPortConnectSource (MIDIPortRef port, MIDIEndpointRef dev,void *i);
void MIDIPortDisconnectSource (MIDIPortRef port, MIDIEndpointRef dev);
void MIDIObjectGetIntegerProperty(MIDIEndpointRef dev, int i, int *ret);
void MIDIObjectGetStringProperty(MIDIEndpointRef dev, int i, CFStringRef *ret);
MIDIPacket *MIDIPacketListInit(MIDIPacketList *pl);
void readFromMIDIBusses(char *inData, int datalen, int dest);
void dataWaiting(void);
