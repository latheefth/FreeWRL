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
# 1 "localMIDIInterface.h"
# 1 "/root/usb/FreeWRL_Reason_MIDI_Interface//"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "localMIDIInterface.h"
void checkInstalledMIDIDevices (void);
void startUpReWire(void);
void closeReWire(void);
void sendMIDIToReason (int bus,int channel, int controllerOrVelocity, int controlType, int value);
void startUpLocalMIDI ();
void fatalError (char *str);

extern int haveNewReWireConfig;
extern int haveNewLocalMIDIConfig;
extern char *localMidiString;
extern int localMidiStringSize;




struct localMidiDevicesStruct {
 char *name;
 int midiSource;
 int midiSink;
 int present;
 SInt32 uniqueID;
 SInt32 olduniqueID;
 MIDIEndpointRef outDevice;
 MIDIEndpointRef inDevice;
};
typedef struct localMidiDevicesStruct localMidiDevicesStruct;

extern localMidiDevicesStruct localDev[];
