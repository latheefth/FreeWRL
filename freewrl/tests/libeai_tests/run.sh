#!/bin/sh

# try either XML or classic encoding
#valgrind --tool=memcheck freewrl --eai simple-test.wrl &
freewrl --eai simple-test.wrl &
#freewrl --eai simple-test.x3d &

# run the test program
sleep 1
./simple
