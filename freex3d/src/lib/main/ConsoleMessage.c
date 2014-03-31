/*
=INSERT_TEMPLATE_HERE=

$Id$

When running in a plugin, there is no way
any longer to get the console messages to come up - eg, no
way to say "Syntax error in X3D file".

Old netscapes used to have a console.

So, now, we pop up xmessages for EACH call here, when running
as a plugin.

NOTE: Parts of this came from on line examples; apologies
for loosing the reference. Also, most if it is found in
"the C programming language" second edition.

*/


/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/



#include <config.h>
#include <system.h>
#include <internal.h>

#include <stdio.h>
#include <stdarg.h> 
#include <iglobal.h>


#define STRING_LENGTH 2000	/* something 'safe'	*/
#define MAX_ANDROID_CONSOLE_MESSAGE_SLOTS 5 //max number of message lines per frame
#define MAX_LINE_LENGTH 80  //wrap text here to make it easy for GUI frontends
#define TAB_SPACES 1

typedef struct pConsoleMessage{
	int androidFreeSlot;
	char **androidMessageSlot;
	int androidHaveUnreadMessages;
	char FWbuffer [STRING_LENGTH];
	int maxLineLength;
	int maxLines;
	int tabSpaces;
	void(*callback[2])(char *);
}* ppConsoleMessage;
static void *ConsoleMessage_constructor(){
	void *v = malloc(sizeof(struct pConsoleMessage));
	memset(v,0,sizeof(struct pConsoleMessage));
	return v;
}
void ConsoleMessage_init(struct tConsoleMessage *t){
	//public
	//private
	t->prv = ConsoleMessage_constructor();
	{
		int i;
		ppConsoleMessage p = (ppConsoleMessage)t->prv;
		p->androidFreeSlot = 0;
		p->androidHaveUnreadMessages = 0;
		p->maxLineLength = MAX_LINE_LENGTH;
		p->maxLines = MAX_ANDROID_CONSOLE_MESSAGE_SLOTS;
		p->tabSpaces = TAB_SPACES;
		p->androidMessageSlot = (char**)malloc(MAX_ANDROID_CONSOLE_MESSAGE_SLOTS * sizeof(char*));
		for (i = 0; i < p->maxLines; i++) p->androidMessageSlot[i] = (char*)NULL;
		p->callback[0] = NULL;
		p->callback[1] = NULL;
	}
}


#define MAXMESSAGES 5 
void closeConsoleMessage() {
	gglobal()->ConsoleMessage.consMsgCount = 0;
}



//View / UI part 
void fwg_updateConsoleStatus()
{
	//if you desire to see ConsoleMessages in the View/UI
	//a) if your View/UI is a console program - call this function once per frame
	//b) if your View/UI is a GUI program, do something similar in your View/UI code to fetch and display lines in GUI
	//call once per controller-loop/frame
	//polls ConsoleMessage.c for accumulated messages and updates console
	int nlines, i;
	char *buffer;
	nlines = fwg_get_unread_message_count(); //poll model
	for (i = 0; i<nlines; i++)
	{
		buffer = fwg_get_last_message(); //poll model for state handover - View now owns buffer
		printf(buffer); //update UI(view)
		free(buffer);
	}
}

// Model (backend) INTERFACE part
void fwg_setConsoleParam_maxLines(int maxLines);
void fwg_setConsoleParam_maxLineLength(int maxLineLength);
void fwg_setConsoleParam_replaceTabs(int tabSpaces);
int fwg_get_unread_message_count();
char *fwg_get_last_message();
int fwl_StringConsoleMessage(char* consoleBuffer);
void fwg_updateConsoleStatus(); //for console programs only - sent to printf
void fwg_register_consolemessage_callback(void(*callback)(char *));

void fwg_setConsoleParam_maxLines(int maxLines)
{
	ppConsoleMessage p;
	ttglobal tg = gglobal();
	if (!tg) return;
	p = (ppConsoleMessage)tg->ConsoleMessage.prv;
	if (maxLines > 0){
		int i;
		p->androidMessageSlot = realloc(p->androidMessageSlot, maxLines*sizeof(char*)); //array of pointers
		for (i = p->maxLines; i<maxLines; i++) p->androidMessageSlot[i] = (char *)NULL;
		p->maxLines = maxLines;
	}

}
void fwg_setConsoleParam_maxLineLength(int maxLineLength)
{
	ppConsoleMessage p;
	ttglobal tg = gglobal();
	if (!tg) return;
	p = (ppConsoleMessage)tg->ConsoleMessage.prv;
	if (maxLineLength > 0)
		p->maxLineLength = maxLineLength;
}
void fwg_setConsoleParam_replaceTabs(int tabSpaces)
{
	ppConsoleMessage p;
	ttglobal tg = gglobal();
	if (!tg) return;
	p = (ppConsoleMessage)tg->ConsoleMessage.prv;
	if (tabSpaces > 0)
		p->tabSpaces = tabSpaces;

}
void fwg_register_consolemessage_callback(void(*callback)(char *))
{
	//if your frontend is in C, you can register something like printf here as a callback
	//advantage over polling once per loop: when debugging you may want to see console output
	//more often during a single loop - this should come out as soon as written in the program
	//if message ends in \n
	//you can call 0,1 or 2 times during program run ie to set a printf and a logfile
	// \t and \n will still be in the string (it won't be pre-split)
	int iback;
	ppConsoleMessage p;
	ttglobal tg = gglobal();
	if (!tg) return;
	p = (ppConsoleMessage)tg->ConsoleMessage.prv;
	iback = 0;
	if (p->callback[iback]) iback++;
	p->callback[iback] = callback;
}

// tell the UI how many unread console messages we have.
int fwg_get_unread_message_count() {
	ppConsoleMessage p;
	ttglobal tg = gglobal();
	if (!tg) return 0;
	p = (ppConsoleMessage)tg->ConsoleMessage.prv;
	return p->androidHaveUnreadMessages;
}

char *fwg_get_last_message() {
	/*
	Transfers ownership of a ConsoleMessage line to the View/UI caller
	- returns NULL if no more messages waiting on this frame (check again next frame)
	- there is no \n in string, it has already been split into screen lines
	- by default, \t is replaced with one space
	- long lines will already be split to maxLineLength
	- on each frame, loop over fwg_get_unread_message_count() 
	    or until fwg_get_last_message() returns null to get all the messages
	*/
	ppConsoleMessage p;
	ttglobal tg = gglobal();
	int whm, nmess;

	if (!tg) return "NO GGLOBAL - NO MESSAGES";
	p = (ppConsoleMessage)tg->ConsoleMessage.prv;

	// reset the "number of messages" counter.
	nmess = p->androidHaveUnreadMessages;
	p->androidHaveUnreadMessages--;

	// which message from our rotating pool do we want?
	whm = p->androidFreeSlot - nmess; // +whichOne;
	if (whm < 0) whm += p->maxLines;
	if (whm < 0)
		return NULL; // strdup(""); //none left - View is asking for too many on this frame.
	if (p->androidMessageSlot[whm] == NULL)
		return NULL; // strdup(""); //blank string - likely a programming error 

	return strdup(p->androidMessageSlot[whm]);
}

int fwl_StringConsoleMessage(char* consoleBuffer) {
	return ConsoleMessage(consoleBuffer);
}


// Model (backend) internal part
static void android_save_log(char *thislog) {
	/*
	processes thislog, and accumulates an array simple lines:
	- splits thislog on each \n
	- if no \n, holds the pointer on the current line
	- replaces \t with a blank by default
	- replaces \n with \0
	- overwrites array at maxLines (wrap-around use of limited array on each frame)
	- you can retrieve the array of lines with another function, usually once per frame
	- or if you don't retrieve, it will continue to wrap around harmlessly
	*/
	int i, more;
	char *ln, *buf;
	ttglobal tg = gglobal();
	ppConsoleMessage p = (ppConsoleMessage)tg->ConsoleMessage.prv;
	// sanity check the string, otherwise dalvik can croak if invalid chars
	for (i = 0; i<strlen(thislog); i++) {
		thislog[i] = thislog[i] & 0x7f;
	}

	buf = thislog;
	more = (buf && *buf > '\0');
	while (more)
	{
		BOOL eol = FALSE;
		int len, ipos = 0;
		ln = strchr(buf, '\n');
		len = strlen(buf);
		if (ln)
		{
			*ln = '\0';
			eol = TRUE;
			len = strlen(buf);
			*ln = '\n';
		}


		/* free our copy of this string if required; then set the pointer for this slot
		to our free slot */
		//problem: strdup and strcat fragment memory if used a lot
		if (len || eol)
		{
			if (p->androidMessageSlot[p->androidFreeSlot]){
				//do no-end-of-line-on-last-one continuation line concatonation
				char *catsize, *oldsize;
				int len1, len2;
				len1 = strlen(p->androidMessageSlot[p->androidFreeSlot]);
				len2 = len+1;
				//will need a string buffer to hold combined last line and current continuation line
				//(there is a re_strcat() function in JScript.c about line 808 that might work here -no time to try it)
				catsize = (char*)malloc(len1 + len2 + 1);
				memcpy(catsize, p->androidMessageSlot[p->androidFreeSlot], len1 + 1);
				oldsize = p->androidMessageSlot[p->androidFreeSlot];
				p->androidMessageSlot[p->androidFreeSlot] = catsize;
				free(oldsize);
				p->androidMessageSlot[p->androidFreeSlot] = strcat(p->androidMessageSlot[p->androidFreeSlot], buf);
			}
			else
				p->androidMessageSlot[p->androidFreeSlot] = strdup(buf);

			//tab expansion (into spaces) might go in here before checking line length
			if (p->tabSpaces)
			{
				char *tt = strchr(p->androidMessageSlot[p->androidFreeSlot], '\t');
				while (tt) {
					*tt = ' ';  //currently it only replaces 1:1 tab with space - feel free to really tab
					tt = strchr(p->androidMessageSlot[p->androidFreeSlot], '\t');
				} 
			}

			//check for line length and wrap-around if necessary
			if (strlen(p->androidMessageSlot[p->androidFreeSlot]) > p->maxLineLength){
				buf = strdup(&p->androidMessageSlot[p->androidFreeSlot][p->maxLineLength - 2]); //how remember to delete this?
				free(thislog);
				thislog = buf;
				p->androidMessageSlot[p->androidFreeSlot][p->maxLineLength - 2] = '\n';
				p->androidMessageSlot[p->androidFreeSlot][p->maxLineLength - 1] = '\0';
				eol = TRUE;
			}else{
				if (eol){
					buf = &ln[1];
				}
				else{
					buf = NULL;
				}
			}
			// indicate we have messages
			if (eol) {
				char *ln = strchr(p->androidMessageSlot[p->androidFreeSlot], '\n');
				if (ln)
					*ln = '\0'; //clear \n
				/* go to next slot, wrap around*/
				p->androidFreeSlot++;
				if (p->androidFreeSlot >= p->maxLines) p->androidFreeSlot = 0;
				if (p->androidMessageSlot[p->androidFreeSlot] != NULL) 
					FREE_IF_NZ(p->androidMessageSlot[p->androidFreeSlot]);
				p->androidHaveUnreadMessages++;
			}
		}
		more = (buf && *buf > '\0');
	}
	free(thislog);
	p->androidHaveUnreadMessages = min(p->androidHaveUnreadMessages, p->maxLines -1);
}
int fwvsnprintf(char *buffer, int buffer_length, const char *fmt, va_list ap)
{
	int i, j, count;
	//char tempbuf[STRING_LENGTH];
	//char format[STRING_LENGTH];
	char *tempbuf;
	char *format;
	char c;
	double d;
	unsigned u;
	char *s;
	void *v;
	tempbuf = malloc(buffer_length);
	format = malloc(buffer_length);
	count = 0;
	buffer[0] = '\0';
	while (*fmt)
	{
		tempbuf[0] = '\0';
		for (j = 0; fmt[j] && fmt[j] != '%'; j++) {
			format[j] = fmt[j];	/* not a format string	*/
		}

		if (j) {
			format[j] = '\0';
			count += sprintf(tempbuf, format);/* printf it verbatim				*/
			fmt += j;
		}
		else {
			for (j = 0; !isalpha(fmt[j]); j++) {	 /* find end of format specifier */
				format[j] = fmt[j];
				if (j && fmt[j] == '%')				/* special case printing '%'		*/
					break;
			}
			format[j] = fmt[j];			/* finish writing specifier		 */
			format[j + 1] = '\0';			/* don't forget NULL terminator */
			fmt += j + 1;

			switch (format[j]) {			 /* cases for all specifiers		 */
			case 'd':
			case 'i':						/* many use identical actions	 */
				i = va_arg(ap, int);		 /* process the argument	 */
				count += sprintf(tempbuf, format, i); /* and printf it		 */
				break;
			case 'o':
			case 'x':
			case 'X':
			case 'u':
				u = va_arg(ap, unsigned);
				count += sprintf(tempbuf, format, u);
				break;
			case 'c':
				c = (char)va_arg(ap, int);		/* must cast!			 */
				count += sprintf(tempbuf, format, c);
				break;
			case 's':
				s = va_arg(ap, char *);
				/* limit string to a certain length */
				if ((strlen(s) + count) > buffer_length) {
					char tmpstr[100];
					int ltc;
					ltc = (int)strlen(s);
					if (ltc>80) ltc = 80;
					strncpy(tmpstr, s, ltc);
					tmpstr[ltc] = '.'; ltc++;
					tmpstr[ltc] = '.'; ltc++;
					tmpstr[ltc] = '.'; ltc++;
					tmpstr[ltc] = '\0';

					count += sprintf(tempbuf, format, tmpstr);
				}
				else count += sprintf(tempbuf, format, s);
				break;
			case 'f':
			case 'e':
			case 'E':
			case 'g':
			case 'G':
				d = va_arg(ap, double);
				count += sprintf(tempbuf, format, d);
				break;
			case 'p':
				v = va_arg(ap, void *);
				count += sprintf(tempbuf, format, v);
				break;
			case 'n':
				count += sprintf(tempbuf, "%d", count);
				break;
			case '%':
				count += sprintf(tempbuf, "%%");
				break;
			default:
				ERROR_MSG("ConsoleMessage: invalid format specifier: %c\n", format[j]);
			}
		}
		if ((strlen(tempbuf) + strlen(buffer)) < (buffer_length)-10)
		{
			strcat(buffer, tempbuf);
		}
	}
	free(tempbuf);
	free(format);
	return 1;
}
int ConsoleMessage0(const char *fmt, va_list args){
	int retval;
	ppConsoleMessage p;
	ttglobal tg = gglobal();
	if (!tg) return 0;
	p = (ppConsoleMessage)tg->ConsoleMessage.prv;

	retval = fwvsnprintf(p->FWbuffer, STRING_LENGTH - 1, fmt, args); /*hope STRING_LENGTH is long enough, else -1 skip */
	if (retval >= 0){
		if (p->callback[0])
			p->callback[0](p->FWbuffer);
		if (p->callback[1])
			p->callback[1](p->FWbuffer);
		android_save_log(STRDUP(p->FWbuffer)); //passing ownerhsip in
	}
	return retval;
}


int ConsoleMessage(const char *fmt, ...)
{
	/*
		There's lots I don't understand such as aqua vs motif vs ??? and plugin vs ??? and the sound/speaker method
		Q. if we call ConsoleMessage from any thread, should there be a thread lock on something, 
		for example s_list_t *conlist (see hudConsoleMessage() in statusbarHud.c)?
	*/

	va_list args;
	va_start( args, fmt );
	return ConsoleMessage0(fmt,args);
}
