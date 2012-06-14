# set this to something else if the FreeWRLEAI headers are not installed in the default location
INCLUDES=-I/usr/include/FreeWRLEAI

# set this if libFreeWRLEAI is not in the ld-path
LIBEAILOCATION=

all: simple-test advanced-test

simple-test: simple.c
	$(CC) $(INCLUDES) -lpthread $(LIBEAILOCATION) -lFreeWRLEAI -o simple-test $<

advanced-test: Ctest_libeai_CRC.c
	$(CC) $(INCLUDES) -lpthread $(LIBEAILOCATION) -lFreeWRLEAI -o advanced-test $<

tests: simple advanced

simple: simple-test
	freewrl --eai simple-test.wrl &
	./simple-test

advanced: advanced-test
	freewrl --eai root.wrl &
	./advanced-test

clean:
	rm -f simple-test advanced-test
