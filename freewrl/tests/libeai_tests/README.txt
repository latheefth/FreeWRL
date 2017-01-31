Jan 2017, John Stewart.

1) compiling via the Makefile does not work, as library ordering is not correct (link fails)

Try this:

	gcc simple.c -lFreeWRLEAI -o simple

2) should run it as:

	freewrl --eai simple-test.wrl &
	#freewrl --eai simple-test.x3d &

	sleep 1
	./simple

At this point, the "x3d" format does not work for "getNode" calls.



