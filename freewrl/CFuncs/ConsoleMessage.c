/*******************************************************************
 Copyright (C) 2005 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*****************************************

ConsoleMessage.c - when running in a plugin, there is no way
any longer to get the console messages to come up - eg, no
way to say "Syntax error in X3D file". 

Old netscapes used to have a console.

So, now, we pop up xmessages for EACH call here, when running
as a plugin.

NOTE: Parts of this came from on line examples; apologies
for loosing the reference. Also, most if it is found in
"the C programming language" second edition.

**********************************************/

#include "headers.h"
#include <stdarg.h>
/*
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>


#ifndef FALSE
	#define FALSE 0
#endif
#ifndef TRUE
	#define TRUE 1
#endif

int ConsoleMessage (char *fmt, ...);
*/

#define STRING_LENGTH 2000	/* something 'safe'	*/

static char FWbuffer [STRING_LENGTH];



int ConsoleMessage(char *fmt, ...) {
	va_list ap;
	char tempbuf[STRING_LENGTH];
	char format[STRING_LENGTH];
	int count = 0;
	int i, j;
	char c;
	double d;
	unsigned u;
	char *s;
	void *v;


	FWbuffer[0] = '\0';

#ifndef AQUA
	if (RUNNINGASPLUGIN) {
		strcpy (FWbuffer,XMESSAGE);
		strcat (FWbuffer, " ");
	}
#endif
	va_start(ap, fmt);		 /* must be called before work	 */
	while (*fmt) {
		tempbuf[0] = '\0';
		for (j = 0; fmt[j] && fmt[j] != '%'; j++)
			format[j] = fmt[j];	/* not a format string	*/
		if (j) {
			format[j] = '\0';
			count += sprintf(tempbuf, format);/* printf it verbatim				*/
			fmt += j;
		} else {
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
	c = (char) va_arg(ap, int);		/* must cast!			 */
	count += sprintf(tempbuf, format, c);
	break;
			case 's':
	s = va_arg(ap, char *);
	count += sprintf(tempbuf, format, s);
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
	printf("Invalid format specifier in printf().\n");
			}
		}
	if ((strlen(tempbuf) + strlen(FWbuffer)) < 
		(STRING_LENGTH) -10) {
		strcat (FWbuffer,tempbuf);
	}
	}
	
	va_end(ap);				/* clean up				 */

#ifdef AQUA
	sendCM(FWbuffer);
#else
	printf (FWbuffer);
	if (RUNNINGASPLUGIN) {
		freewrlSystem (FWbuffer);
	} else {
		printf (FWbuffer);
		printf ("\n");
	}
#endif
	return count;
}


