#INCLUDE = -I/usr/X11R6/include/ -I/usr/include/sys -I/System/Library/Frameworks/CoreAudio.framework/Headers -I/System/Library/Frameworks/AudioUnit.framework/Headers -I/System/Libraray/Frameworks/AudioToolbox.framework/Headers
INCLUDE = -I/System/Library/Frameworks/CoreAudio.framework/Headers -I/System/Library/Frameworks/AudioUnit.framework/Headers -I/System/Libraray/Frameworks/AudioToolbox.framework/Headers

LIBDIR  = -L. -L/usr/X11R6/lib -L/usr/local/lib -L/usr/lib 

COMPILERFLAGS = -Wall
CC = g++
CFLAGS = $(COMPILERFLAGS) $(INCLUDE)
LIBRARIES = -lm -lstdc++ -lc
SVROBJ = main.o wavmaker.o mixer.o
FRAMEDIR = -F/System/Library/Frameworks/
FRAMEWORKS = -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices

All: main 

main: $(SVROBJ)
	$(CC) $(SVROBJ) $(CFLAGS)  AudioFilePlayImpl/AudioFileData.cpp AudioFilePlayImpl/AudioFilePlay.cpp AudioFilePlayImpl/AudioFilePlayer.cpp AudioFilePlayImpl/AudioFileReaderThread.cpp CAguard.cp CAHostTimeBase.cp CADebugMacros.cp -o FreeWRL_SoundServer $(LIBDIR) $(LIBRARIES) $(FRAMEDIR) $(FRAMEWORKS)
	@ls -al FreeWRL_SoundServer;strip FreeWRL_SoundServer;ls -al FreeWRL_SoundServer

mixer: mixer.o
	$(CC) $(CFLAGS) -o $mixer $(LIBDIR) $< $(LIBRARIES)

wavmaker: wavmaker.o
	$(CC) $(CFLAGS) -o $wavmaker $(LIBDIR) $< $(LIBRARIES) $(FRAMEDIR) $(FRAMEWORKS)

clean:
	rm -f *.o FreeWRL_SoundServer

install:  main
	cp FreeWRL_SoundServer /usr/local/bin/.

