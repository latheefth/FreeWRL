char *EAI_GetNode(const char *str) {
	printf ("EAI_GetNode %s\n",str);
}

	
void EAI_GetType(unsigned int nodenum, const char *fieldname, const char *direction,
        int *nodeptr,
        int *dataoffset,
        int *datalen,
        int *nodetype,
        int *scripttype) {

	printf ("EAI_GetType %d %s %s\n",nodenum, fieldname, direction);
}


char *EAI_GetTypeName (unsigned int uretval) {
	printf ("EAI_GetTypeName %d\n",uretval);
	return "unknownType";
}


int SAI_IntRetCommand (char cmnd, const char *fn) {
	printf ("SAI_IntRetCommand, %c, %s\n",cmnd,fn);
	return 0;
}

char * SAI_StrRetCommand (char cmnd, const char *fn) {
	printf ("SAI_StrRetCommand, %c, %s\n",cmnd,fn);
	return "iunknownreturn";
}

char* EAI_GetValue(unsigned int nodenum, const char *fieldname, const char *nodename) {
	printf ("EAI_GetValue, %d, %s %s\n",nodenum, fieldname, nodename);
}

unsigned int EAI_GetViewpoint(const char *str) {
	printf ("EAI_GetViewpoint %s\n",str);
	return 0;
}
