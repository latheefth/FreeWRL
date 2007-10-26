/* header file for the X3D parser, only items common between the X3DParser files should be here. */

#include "expat.h"

#define PARSING_NODES 1
#define PARSING_SCRIPT 2
#define PARSING_PROTODECLARE  3
#define PARSING_PROTOINTERFACE  4
#define PARSING_PROTOBODY       5
#define PARSING_PROTOINSTANCE   6

/* for our internal PROTO tables, and, for initializing the XML parser */
#define PROTOINSTANCE_MAX_LEVELS 30
#define PROTOINSTANCE_MAX_PARAMS 20

int freewrl_XML_GetCurrentLineNumber();
#define LINE freewrl_XML_GetCurrentLineNumber()
#define TTY_SPACE {int tty; for (tty = 0; tty < parentIndex; tty++) putchar('\t');}
extern int parserMode;

#define PARENTSTACKSIZE 256
extern int parentIndex;
extern struct X3D_Node *parentStack[PARENTSTACKSIZE];
extern char *scriptText;

/* function protos */
void parseProtoDeclare (const char **atts);
void parseProtoInterface (const char **atts);
void parseProtoBody (const char **atts);
void registerX3DScriptField(int scriptNotProto, int myScriptNumber,int type,int kind, int myFieldOffs, char *name, char *value);
void parseProtoInstance (const char **atts);
void parseProtoInstanceFields(const char *name, const char **atts);
void dumpProtoBody (const char *name, const char **atts);
void endDumpProtoBody (const char *name);
void parseScriptProtoField(int fromScriptNotProto, const char *name, const char **atts);
int getFieldFromScript (char *fieldName, int scriptno, int *offs, int *type);
void expandProtoInstance(struct X3D_Group * myGroup);
void freeProtoMemory (void);
