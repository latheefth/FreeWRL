#ifdef AQUA
#include <syslog.h> //TODO: configure check
/* for sending text to the System Console */
//static int logFileOpened = FALSE;
//char ConsoleLogName[200];
/* JAS - make the Console Log write to the log app on OSX for all invocations of the library */

int logFileOpened = FALSE;
char ConsoleLogName[200];


/* try to open a file descriptor to the Console Log - on OS X
this should display the text on the "Console.app" */
void openLogfile()
{
	if (!logFileOpened) {
		logFileOpened = TRUE;
		openlog("freewrl", LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);
		setlogmask(LOG_UPTO(LOG_ERR));
		syslog(LOG_ALERT, "FreeWRL opened Console Log");
	}
}
// you would register writeToLogFile with fwg_register_consolemessage_callback
void writeToLogFile(char *buffer)
{
	if (!logFileOpened) openLogfile();
	/* print this to the console log */
	syslog(LOG_ALERT, buffer);
}
void closeLogFile() {

	if (logFileOpened) syslog(LOG_ALERT, "FreeWRL loading a new file");
	logFileOpened = FALSE;
}

#endif