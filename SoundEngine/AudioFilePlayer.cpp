/*	Copyright: 	© Copyright 2003 Apple Computer, Inc. All rights reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under AppleÕs
			copyrights in this original Apple software (the "Apple Software"), to use,
			reproduce, modify and redistribute the Apple Software, with or without
			modifications, in source and/or binary forms; provided that if you redistribute
			the Apple Software in its entirety and without modifications, you must retain
			this notice and the following text and disclaimers in all such redistributions of
			the Apple Software.  Neither the name, trademarks, service marks or logos of
			Apple Computer, Inc. may be used to endorse or promote products derived from the
			Apple Software without specific prior written permission from Apple.  Except as
			expressly stated in this notice, no other rights or licenses, express or implied,
			are granted by Apple herein, including but not limited to any patent rights that
			may be infringed by your derivative works or by other works in which the Apple
			Software may be incorporated.

			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
			WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
			WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
			PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
			COMBINATION WITH YOUR PRODUCTS.

			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
			CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
			GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
			ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
			OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
			(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
			ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	AudioFilePlayer.cpp
//
#include "AudioFilePlayer.h"

#pragma mark __these are the calls for a v1 unit
OSStatus 	AudioFileManager::FileInputProc (void 						*inRefCon, 
										AudioUnitRenderActionFlags 		inActionFlags,
										const AudioTimeStamp 			*inTimeStamp, 
										UInt32 							inBusNumber, 
										AudioBuffer 					*ioData)
{
	AudioFileManager* THIS = (AudioFileManager*)inRefCon;
	return THIS->Render (*ioData);
}

OSStatus	AudioFileManager::Render (AudioBuffer &ioData)
{
	OSStatus result = AudioConverterFillBuffer(mParentConverter, 
									AudioFileManager::ACInputProc, 
									this, 
									&ioData.mDataByteSize, 
									ioData.mData);
		if (result)	fprintf (stderr, "AudioConverterFillBuffer:%ld\n", result);
	
	if (!result)
		AfterRender ();
	else
		GetParent().DoNotification (result);
	
	return result;
}

OSStatus 	AudioFileManager::ACInputProc (AudioConverterRef			inAudioConverter,
											UInt32*						outDataSize,
											void**						outData,
											void*						inUserData)
{
	AudioFileManager* THIS = (AudioFileManager*)inUserData;
	return THIS->GetFileData (outData, outDataSize);
}

#pragma mark __these are the calls for a v2 unit

#if SUPPORT_V2_AU
OSStatus 	AudioFileManager::FileRenderProc (void 						*inRefCon, 
										AudioUnitRenderActionFlags		*inActionFlags,
										const AudioTimeStamp 			*inTimeStamp, 
										UInt32 							inBusNumber,
										UInt32							inNumFrames, 
										AudioBufferList 				*ioData)
{
	AudioFileManager* THIS = (AudioFileManager*)inRefCon;
	return THIS->Render (*ioData, inNumFrames);
}

	//we're only supporting the reading of CBR based data here...
	// and we're only assuming that we have one AudioBuffer
	// which is safe enough in the AudioUnit case...
	
	// if we're dealing with PCM data, then the numberOfPackets we need to produce
	// is equal to the number of frames (as there is 1 frame per packet)
	
	// however, if we have non-PCM data, we can have some num Frames in a packet
	// of data, so we should do that translation...
OSStatus	AudioFileManager::Render (AudioBufferList &ioData, UInt32 inNumFrames)
{
	UInt32 numDataPacketsNeeded = inNumFrames / GetParent().GetFileFormat().mFramesPerPacket;
	OSStatus result = AudioConverterFillComplexBuffer(mParentConverter, 
								ACComplexInputProc, 
								this,
								&numDataPacketsNeeded, 
								&ioData, 
								0);
			
		if (result)	fprintf (stderr, "AudioConverterFillComplexBuffer:%ld,%-4.4s\n", result, (char*)&result);

#if 0
		//debug test assumes framePerPacket == 1
	if (numDataPacketsNeeded != inNumFrames) 
		printf ("after conv:%ld,%ld\n", numDataPacketsNeeded, inNumFrames);
#endif
	
	if (!result)
		AfterRender ();
	else
		GetParent().DoNotification (result);
	
	return result;
}

OSStatus 	AudioFileManager::ACComplexInputProc (AudioConverterRef		inAudioConverter,
								UInt32*								ioNumberDataPackets,
								AudioBufferList*					ioData,
								AudioStreamPacketDescription** 		outDataPacketDescription,
								void*								inUserData)
{
	AudioFileManager* THIS = (AudioFileManager*)inUserData;
	AudioBuffer* firstBuffer = ioData->mBuffers;

#if 0
	// we can leave the channels alone.. this would have been set for us
	// (that's why ioData is both in and out -> in has channels set
	UInt32 size = firstBuffer->mDataByteSize;
	void* ptr = firstBuffer->mData;
	UInt32 framesIn = *ioNumberDataPackets;
#endif

	OSStatus res = THIS->GetFileData (&firstBuffer->mData, &firstBuffer->mDataByteSize);

	if (firstBuffer->mDataByteSize == 0)
		*ioNumberDataPackets = 0;
	else {
		// we should calculate the number of packets of data
		// as we're cbr or pcm data, this is a simple calc.
		// as in both of these cases, there is a simple ratio of bytes per packet
		// and 1 frame per packet.
		
		// in a vbr case, this is more complicated, as each packet has a different
		// number of bytes
		
		// in that case, the GetFileData call is not sufficiently complex
		// as it would have to not only reset the data pointer, but potentially
		// tell me how many packets, and their boundaries, there are in the
		// data provided to it...
		const AudioStreamBasicDescription& desc	= THIS->GetParent().GetFileFormat();
		*ioNumberDataPackets = firstBuffer->mDataByteSize / desc.mBytesPerPacket;
	}

#if 0
// this is debug code to just check out how the data handling is doing
	printf ("fIn = %ld, fOut=%ld, bytesIn=%ld, bytesOut=%ld, ptrIn=%x, ptrOut=%x\n",
					framesIn, *ioNumberDataPackets, 
					size, firstBuffer->mDataByteSize,
					(int)ptr, (int)firstBuffer->mData);
#endif
	
	return res;
}

#endif

AudioFileManager::~AudioFileManager ()
{
	if (mFileBuffer) {
		free (mFileBuffer);
		mFileBuffer = 0;
	}
}

AudioFilePlayer::AudioFilePlayer (const FSRef& 			inFileRef)
	: mConnected (false),
	  mAudioFileManager (0),
	  mConverter (0),
	  mNotifier (0)
{
	SInt64 fileDataSize  = 0;
	
	OpenFile (inFileRef, fileDataSize);
		
		// we'll automatically load files that are smaller than 256k
	if (fileDataSize < (1024 * 256)) {
		mAudioFileManager = new AudioFileData (*this, mAudioFileID, fileDataSize);
		mUsingReaderThread = false;
	} else {
		// we'll also check to see if the file is smaller
		// than what we would create buffers for
					
			// we want about a seconds worth of data for the buffer
		int secsBytes = UInt32 (mFileDescription.mSampleRate * mFileDescription.mBytesPerFrame);
		
#if DEBUG
		PrintStreamDesc (&mFileDescription);
#endif
		
			//round to a 32K boundary
		if ((secsBytes & 0xFFFF8000) > (128 * 1024))
			secsBytes &= 0xFFFF8000;
		else
			secsBytes = (secsBytes + 0x7FFF) & 0xFFFF8000;
				
		if (fileDataSize > (secsBytes * 2)) {
			mAudioFileManager = new AudioFileReaderThread (*this, 
														mAudioFileID, 
														fileDataSize,
														secsBytes);
			mUsingReaderThread = true;
		} else {
			mAudioFileManager = new AudioFileData (*this, mAudioFileID, fileDataSize);
			mUsingReaderThread = false;
		}
	}
}

// you can put a rate scalar here to play the file faster or slower
// by multiplying the same rate by the desired factor 
// eg fileSampleRate * 2 -> twice as fast
// before you create the AudioConverter
void	AudioFilePlayer::SetDestination (AudioUnit				&inDestUnit, 
								int			 					inBusNumber)
{
	if (mConnected) throw static_cast<OSStatus>(-1); //can't set dest if already engaged
 
	mPlayUnit = inDestUnit;
	mBusNumber = inBusNumber;

	OSStatus result = noErr;
	
	if (mConverter) {
		result = AudioConverterDispose (mConverter);
			THROW_RESULT("AudioConverterDispose")
	}
	
	AudioStreamBasicDescription		destDesc;
	UInt32	size = sizeof (destDesc);
	result = AudioUnitGetProperty (inDestUnit,
							kAudioUnitProperty_StreamFormat,
							kAudioUnitScope_Input,
							inBusNumber,
							&destDesc,
							&size);
		THROW_RESULT("AudioUnitGetProperty")

#if DEBUG
	PrintStreamDesc (&destDesc);
#endif

		//we can "down" cast a component instance to a component
	ComponentDescription desc;
	result = GetComponentInfo ((Component)inDestUnit, &desc, 0, 0, 0);
		THROW_RESULT("GetComponentInfo")
		
		// we're going to use this to know which convert routine to call
		// a v1 audio unit will have a type of 'aunt'
		// a v2 audio unit will have one of several different types.
	mIsAUNTUnit = (desc.componentType == kAudioUnitComponentType);
	
#if !SUPPORT_V2_AU	
	if (!mIsAUNTUnit) {
		result = badComponentInstance;
		THROW_RESULT("BAD COMPONENT")
	}
#endif

// 	a "neat" trick:
// 	if you want to play the file back faster or slower then you can 
// 	alter the sample rate of the fileDescription before you create the converter.

// thus...
//	mFileDescription.mSampleRate *= 2; // the file will play back twice as fast
// 	mFileDescription.mSampeRate *= 0.5; // half speed

	result = AudioConverterNew (&mFileDescription, &destDesc, &mConverter);
		THROW_RESULT("AudioConverterNew")

	// if we have a mono source, we're going to copy each channel into
	// the destination's channel source...
	if (mFileDescription.mChannelsPerFrame == 1) {
		SInt32* channelMap = new SInt32 [destDesc.mChannelsPerFrame];
		for (unsigned int i = 0; i < destDesc.mChannelsPerFrame; ++i)
			channelMap[i] = 0; //set first channel to all output channels
			
		result = AudioConverterSetProperty(mConverter,
							kAudioConverterChannelMap,
							(sizeof(SInt32) * destDesc.mChannelsPerFrame),
							channelMap);
			THROW_RESULT("AudioConverterSetProperty")
		
		delete [] channelMap;
	}
	
#if 0
	// this uses the better quality SRC
	UInt32 srcID = kAudioUnitSRCAlgorithm_Polyphase;
	result = AudioConverterSetProperty(mConverter,
					kAudioConverterSampleRateConverterAlgorithm, 
					sizeof(srcID), 
					&srcID);
		THROW_RESULT("AudioConverterSetProperty")
#endif
}

AudioFilePlayer::~AudioFilePlayer()
{
	Disconnect();
		
	if (mAudioFileManager) {
		delete mAudioFileManager;
		mAudioFileManager = 0;
	}
	
	if (mAudioFileID) {
		::AudioFileClose (mAudioFileID);
		mAudioFileID = 0;
	}

	if (mConverter) {
		AudioConverterDispose (mConverter);
		mConverter = 0;
	}
}

void 	AudioFilePlayer::Connect()
{
#if DEBUG
	printf ("Connect:%x,%ld, engaged=%d\n", (int)mPlayUnit, mBusNumber, (mConnected ? 1 : 0));
#endif
	if (!mConnected)
	{			
		mAudioFileManager->Connect(mConverter);
				
		// set the render callback for the file data to be supplied to the sound converter AU
		if (mIsAUNTUnit) {
			mInputCallback.inputProc = AudioFileManager::FileInputProc;
			mInputCallback.inputProcRefCon = mAudioFileManager;

			OSStatus result = AudioUnitSetProperty (mPlayUnit, 
								kAudioUnitProperty_SetInputCallback, 
								kAudioUnitScope_Input, 
								mBusNumber,
								&mInputCallback, 
								sizeof(mInputCallback));
			THROW_RESULT("AudioUnitSetProperty")
#if SUPPORT_V2_AU
		} else {
			mRenderCallback.inputProc = AudioFileManager::FileRenderProc;
			mRenderCallback.inputProcRefCon = mAudioFileManager;
			
			OSStatus result = AudioUnitSetProperty (mPlayUnit, 
								kAudioUnitProperty_SetRenderCallback, 
								kAudioUnitScope_Input, 
								mBusNumber,
								&mRenderCallback, 
								sizeof(mRenderCallback));
			THROW_RESULT("AudioUnitSetProperty")	
#endif
		}
		mConnected = true;
	}
}

#warning This should redirect the calling of notification code to some other thread
void 	AudioFilePlayer::DoNotification (OSStatus inStatus) const
{
	AudioFilePlayer* THIS = const_cast<AudioFilePlayer*>(this);
	if (mNotifier) {
		(*mNotifier) (mRefCon, inStatus);
	} else {
		if (inStatus == kAudioFilePlay_FileIsFinished)
			THIS->Disconnect();
		else if (inStatus != kAudioFilePlayErr_FilePlayUnderrun)
			THIS->Disconnect();
	}
}

void 	AudioFilePlayer::Disconnect ()
{
#if DEBUG
	printf ("Disconnect:%x,%ld, engaged=%d\n", (int)mPlayUnit, mBusNumber, (mConnected ? 1 : 0));
#endif
	if (mConnected)
	{
		mConnected = false;
			
		if (mIsAUNTUnit) {
			mInputCallback.inputProc = 0;
			mInputCallback.inputProcRefCon = 0;
			OSStatus result = AudioUnitSetProperty (mPlayUnit, 
											kAudioUnitProperty_SetInputCallback, 
											kAudioUnitScope_Input, 
											mBusNumber,
											&mInputCallback, 
											sizeof(mInputCallback));
			if (result) 
				fprintf(stderr, "AudioUnitSetProperty:RemoveInputCallback:%ld", result);

#if SUPPORT_V2_AU
		
		} else {
			mRenderCallback.inputProc = 0;
			mRenderCallback.inputProcRefCon = 0;
			
			OSStatus result = AudioUnitSetProperty (mPlayUnit, 
								kAudioUnitProperty_SetRenderCallback, 
								kAudioUnitScope_Input, 
								mBusNumber,
								&mRenderCallback, 
								sizeof(mRenderCallback));
			if (result) 
				fprintf(stderr, "AudioUnitSetProperty:RemoveRenderCallback:%ld", result);
#endif
		}
		
		mAudioFileManager->Disconnect();
	}
}

void	AudioFilePlayer::SetLooping (bool inLoop) 
{ 
	mAudioFileManager->SetLooping (inLoop); 
}
	
bool	AudioFilePlayer::IsLooping () const 
{
	return mAudioFileManager->IsLooping(); 
}

void	AudioFilePlayer::OpenFile (const FSRef& inRef, SInt64& outFileDataSize)
{		
	OSStatus result = AudioFileOpen (&inRef, fsRdPerm, 0, &mAudioFileID);
		THROW_RESULT("AudioFileOpen")
		
	UInt32 dataSize = sizeof(AudioStreamBasicDescription);
	result = AudioFileGetProperty (mAudioFileID, 
							kAudioFilePropertyDataFormat, 
							&dataSize, 
							&mFileDescription);
		THROW_RESULT("AudioFileGetProperty")
	
	dataSize = sizeof (SInt64);
	result = AudioFileGetProperty (mAudioFileID, 
							kAudioFilePropertyAudioDataByteCount, 
							&dataSize, 
							&outFileDataSize);
		THROW_RESULT("AudioFileGetProperty")
}

