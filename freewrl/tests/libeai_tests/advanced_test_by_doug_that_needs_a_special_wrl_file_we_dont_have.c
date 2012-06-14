#include <stdio.h>
#include <FreeWRLEAI/EAI_C.h>

#define SPHERE 		"Sphere"
#define BLUE            "0.2 0.2 0.8"

X3DNode *makeSimpleShape (char * shape, char *colour, char *posn) {
        char myline[2000];

        sprintf (myline, "Transform{translation %s children Shape{" \
                         "  appearance Appearance { \n" \
                         "    material Material {" \
                         "      diffuseColor %s" \
                         "    }" \
                         "  }" \
                         "  geometry %s {}" \
                         " }}" \
			 "Transform { children Shape { } }" ,posn,colour,shape);

        return X3D_createVrmlFromString(myline);
}

int main () {

	X3DNode *root = NULL;
	X3DEventOut* evOut = NULL;
	X3DEventIn* evIn = NULL;
	X3DNode* getvalue = NULL;
	X3DNode* setvalue = NULL;

	float f3[3];
/*
	int i3[2];
	int* rfi;
*/
	float tester = 3.0;
	double dtester = 2.0;
	float f1;
	double d1;
	float f4[4];
	int i1;
	char* mystring = NULL;
	char** rstrings; 

	X3D_initialize ("");

	printf("Testing error checking ... \n");
	root = X3D_getNode("NOSUCHNODE");
	if (!root) {
		printf("\t\t\t Invalid node passed!\n");
	} else {
		printf("\t\t\t Invalid node FAILED!\n");
	}

	root = X3D_getNode ("MAT");

	evOut = X3D_getEventOut(root, "NOSUCHEVENT");
	if (!evOut) {
		printf("\t\t\t Invalid event passed!\n");
	} else {
		printf("\t\t\t Invalid event FAILED!\n");
	}
	/* SF Color */
	printf("Testing SFColor ... \n");
	evOut = X3D_getEventOut(root, "diffuseColor");
	evIn = X3D_getEventIn(root, "diffuseColor");
	setvalue = X3D_newSFColor(1.0, 2.0, 3.0);
	getvalue = X3D_getValue(evOut);
	X3D_getSFColor(getvalue, f3);
	if (f3[0] == 0.0 && f3[1] == 0.0 && f3[2] == 3.0)
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getSFColor(getvalue, f3);
	if (f3[0] == 1.0 && f3[1] == 2.0 && f3[2] == 3.0)
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* SFFloat */
	printf("Testing SFFloat... \n");
	evOut = X3D_getEventOut(root, "ambientIntensity");
	evIn = X3D_getEventIn(root, "ambientIntensity");
	setvalue = X3D_newSFFloat(3.0);
	getvalue = X3D_getValue(evOut);
	X3D_getSFFloat(getvalue, &f1);
	tester = 1.0;
	if (f1 == tester) 
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED (%f)\n", f1);
	tester = 3.0;
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getSFFloat(getvalue, &f1);
	if (f1 == tester)
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");


	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	X3D_freeNode(root);
	root = X3D_getNode ("TEXT"); 


	/* MFString */	
	printf("Testing MFString... \n");
	evOut = X3D_getEventOut(root, "string");
	evIn = X3D_getEventIn(root, "string");

	char tstrings[3][256];
	strcpy(tstrings[0], "text");
	strcpy(tstrings[1], "is");
	strcpy(tstrings[2], "passed");
	setvalue = X3D_newMFString(3, tstrings);
	getvalue = X3D_getValue(evOut);

	X3D_getMFString(getvalue, &rstrings, &i1);
	if ((!strcmp(rstrings[0], "one"))  && (i1 == 1))
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");

	free(rstrings);
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getMFString(getvalue, &rstrings, &i1);
	if ((i1 == 3) && (!strcmp(rstrings[0], "text")) && (!strcmp(rstrings[1], "is")) && (!strcmp(rstrings[2], "passed")))
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");
	free(rstrings);

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	X3D_freeNode(root);
	root = X3D_getNode ("TIMESENSOR"); 

	/* SFBool */
	printf("Testing SFBool ... \n");
	evOut = X3D_getEventOut(root, "enabled");
	evIn = X3D_getEventIn(root, "enabled");
	setvalue = X3D_newSFBool(0);
	getvalue = X3D_getValue(evOut);
	X3D_getSFBool(getvalue, &i1);
	if (i1 == 1) 
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getSFBool(getvalue, &i1);
	if (i1 == 0)
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* SFTime */
	printf("Testing SFTime... \n");
	evOut = X3D_getEventOut(root, "cycleInterval");
	evIn = X3D_getEventIn(root, "cycleInterval");
	setvalue = X3D_newSFTime(3.0);
	getvalue = X3D_getValue(evOut);
	X3D_getSFTime(getvalue, &d1);
	dtester = 2.0;
	if (d1 == dtester) 
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getSFTime(getvalue, &d1);
	dtester = 3.0;
	if (d1 == dtester)
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	X3D_freeNode(root);
	root = X3D_getNode ("ROOT");

	/* SFVec3f */
	printf("Testing SFVec3f... \n");
	evOut = X3D_getEventOut(root, "scale");
	evIn = X3D_getEventIn(root, "scale");
	setvalue = X3D_newSFVec3f(1.0, 2.0, 3.0);
	getvalue = X3D_getValue(evOut);
	X3D_getSFVec3f(getvalue, f3);
	if (f3[0] == 1.0 && f3[1] == 2.0 && f3[2] == 1.0)
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getSFVec3f(getvalue, f3);
	if (f3[0] == 1.0 && f3[1] == 2.0 && f3[2] == 3.0)
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* SFRotation */
	printf("Testing SFRotation ... \n");
	evOut = X3D_getEventOut(root, "rotation");
	evIn = X3D_getEventIn(root, "rotation");
	setvalue = X3D_newSFRotation(1.0, 2.0, 3.0, 4.0);
	getvalue = X3D_getValue(evOut);
	X3D_getSFRotation(getvalue, f4);
	if (f4[0] == 0.0 && f4[1] == 0.0 && f4[2] == 2.0 && f4[3] == 0.0)
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getSFRotation(getvalue, f4);
	if (f4[0] == 1.0 && f4[1] == 2.0 && f4[2] == 3.0 && f4[3] == 4.0)
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	X3D_freeNode(root);
	root = X3D_getNode ("RECTANGLE");

	/* SFVec2f */
	/* FreeWRL BUG
	printf("Testing SFVec2f... \n");
	evOut = X3D_getEventOut(root, "size");
	evIn = X3D_getEventIn(root, "size");
	setvalue = X3D_newSFVec2f(1.0, 2.0);
	getvalue = X3D_getValue(evOut);
	X3D_getSFVec2f(getvalue, f2);
	if (f2[0] == 0.0 && f2[1] == 5.0 )
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getSFVec2f(getvalue, f2);
	if (f2[0] == 1.0 && f2[1] == 2.0)
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	*/

	X3D_freeNode(root);
	root = X3D_getNode ("SWITCH");

	/* SFInt */
	printf("Testing SFInt... \n");
	evOut = X3D_getEventOut(root, "whichChoice");
	evIn = X3D_getEventIn(root, "whichChoice");
	setvalue = X3D_newSFInt32(3);
	getvalue = X3D_getValue(evOut);
	X3D_getSFInt32(getvalue, &i1);
	if (i1 == 7) 
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getSFInt32(getvalue, &i1);
	if (i1 == 3)
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* SFNode */
	printf("Testing SFNode ... \n");
	evOut = X3D_getEventOut(root, "metadata");
	evIn = X3D_getEventIn(root, "metadata");
	setvalue = X3D_getNode("MAT");
	X3D_setValue(evIn, setvalue);
	getvalue = X3D_getValue(evOut);
	X3D_freeEventOut(evOut);
	evOut = X3D_getEventOut(getvalue, "ambientIntensity");
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getSFFloat(getvalue, &f1);
	if (f1 == 3.0)
		printf("\t\t\t ... set/get passed\n");
	else
		printf("\t\t\t ... set/get FAILED\n");

	X3D_freeNode(setvalue);
	setvalue = X3D_getNode("TIMESENSOR");
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventOut(evOut);
	evOut = X3D_getEventOut(root, "metadata");
	getvalue = X3D_getValue(evOut);
	X3D_freeEventOut(evOut);
	evOut  = X3D_getEventOut(getvalue, "cycleInterval");
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getSFTime(getvalue, &d1);
	if (d1 == 3.0)
		printf("\t\t\t ... second set/get passed\n");
	else
		printf("\t\t\t ... second set/get FAILED\n");


	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);


	/* MFNode */
	X3D_freeNode(root);
	root = X3D_getNode ("T2");

	printf("Testing MFNode ... \n");
        evOut = X3D_getEventOut(root, "children");
        evIn = X3D_getEventIn(root, "addChildren");

	setvalue = makeSimpleShape(SPHERE, BLUE, "1.0 1.0 1.0");
	X3D_setValue(evIn, setvalue);
	getvalue = X3D_getValue(evOut);

	if ((getvalue->X3D_MFNode.n == 2) && (getvalue->X3D_MFNode.p[0].adr == setvalue->X3D_MFNode.p[0].adr) && (getvalue->X3D_MFNode.p[1].adr == setvalue->X3D_MFNode.p[1].adr)) 
		printf("\t\t\t ... set/get passed\n");
	else
		printf("\t\t\t ... set/get FAILED\n");

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* SFColorRGBA */
	/* Not in any node */


	X3D_freeNode(root);
	root = X3D_getNode ("ANCHOR");

	/* SFString */
	printf("Testing SFString... \n");
	evOut = X3D_getEventOut(root, "description");
	evIn = X3D_getEventIn(root, "description");
	setvalue = X3D_newSFString("NEW");
	getvalue = X3D_getValue(evOut);
	mystring = X3D_getSFString(getvalue);
	if (!strcmp(mystring, "DEFAULT")) 
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
/*  BUG IN SFString setValue in freewrl C parser 

	X3D_freeNode(getvalue);
	X3D_setValue(evIn, setvalue);
	getvalue = X3D_getValue(evOut);
	free(mystring);
	mystring = X3D_getSFString(getvalue);
	if (!strcmp(mystring, "NEW"))
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");
*/

	free(mystring);
	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* MFInt */
	X3D_freeNode(root);
	root = X3D_getNode("SEQUENCER");

	int ia[3];
	int* ria;

	printf("Testing MFInt... \n");
	evOut = X3D_getEventOut(root, "keyValue");
	evIn = X3D_getEventIn(root, "keyValue");
	ia[0] = 4;
	ia[1] = 5;
	ia[2] = 6;

	setvalue = X3D_newMFInt32(3, ia);
	getvalue = X3D_getValue(evOut);
	X3D_getMFInt32(getvalue, &ria,  &i1);
	if ((i1 == 2)  && (ria[0] == 1) && (ria[1] == 2))
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	free(ria);
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getMFInt32(getvalue, &ria, &i1);
	if ((i1 == 3) && (ria[0] == 4) && (ria[1] == 5) && (ria[2] == 6))
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");
	free(ria);

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);


	X3D_freeNode(root);
	root = X3D_getNode("INTERPOLATOR");

	float*  rfa;

	printf("Testing MFFloat ... \n");
	evOut = X3D_getEventOut(root, "key");
	evIn = X3D_getEventIn(root, "key");
	f3[0] = 4.0;
	f3[1] = 5.0;
	f3[2] = 6.0;

	setvalue = X3D_newMFFloat(3, f3);
	getvalue = X3D_getValue(evOut);
	X3D_getMFFloat(getvalue, &rfa,  &i1);
	if ((i1 == 2)  && (rfa[0] == 1.0) && (rfa[1] == 2))
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	free(rfa);
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getMFFloat(getvalue, &rfa, &i1);
	if ((i1 == 3) && (rfa[0] == 4.0) && (rfa[1] == 5.0) && (rfa[2] == 6.0))
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");
	free(rfa);

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* MFVec3f */
	float **rfaa;
	float faa[3][3];

	printf("Testing MFVec3f ... \n");
	evOut = X3D_getEventOut(root, "keyValue");
	evIn = X3D_getEventIn(root, "keyValue");
	faa[0][0] = 4.0;
	faa[0][1] = 5.0;
	faa[0][2] = 6.0;
	faa[1][0] = 7.0;
	faa[1][1] = 8.0;
	faa[1][2] = 9.0;
	faa[2][0] = 10.0;
	faa[2][1] = 11.0;
	faa[2][2] = 12.0;

	setvalue = X3D_newMFVec3f(3, faa);
	getvalue = X3D_getValue(evOut);
	X3D_getMFVec3f(getvalue, &rfaa,  &i1);
	if ((i1 == 2)  && (rfaa[0][0] == 1.0) && (rfaa[0][1] == 1.0) && (rfaa[0][2] == 1.0) && (rfaa[1][0] == 2.0) && (rfaa[1][1] == 2.0) && (rfaa[1][2] == 2.0))
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	free(rfaa);
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getMFVec3f(getvalue, &rfaa, &i1);
	if ((i1 == 3) && (rfaa[0][0] == 4.0) && (rfaa[0][1] == 5.0) && (rfaa[0][2] == 6.0) && (rfaa[1][0] == 7.0) && (rfaa[1][1] == 8.0) && (rfaa[1][2] == 9.0) && (rfaa[2][0] == 10.0) && (rfaa[2][1] == 11.0) && (rfaa[2][2] == 12.0))
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");
	free(rfaa);

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* MFColor */
	X3D_freeNode(root);
	root = X3D_getNode("COLORINT");

	printf("Testing MFColor ...\n");
	evOut = X3D_getEventOut(root, "keyValue");
	evIn = X3D_getEventIn(root, "keyValue");

	faa[0][0] = 4.0;
	faa[0][1] = 5.0;
	faa[0][2] = 6.0;
	faa[1][0] = 7.0;
	faa[1][1] = 8.0;
	faa[1][2] = 9.0;
	faa[2][0] = 10.0;
	faa[2][1] = 11.0;
	faa[2][2] = 12.0;

	setvalue = X3D_newMFColor(3, faa);
	getvalue = X3D_getValue(evOut);
	X3D_getMFColor(getvalue, &rfaa,  &i1);
	if ((i1 == 2)  && (rfaa[0][0] == 1.0) && (rfaa[0][1] == 1.0) && (rfaa[0][2] == 1.0) && (rfaa[1][0] == 2.0) && (rfaa[1][1] == 2.0) && (rfaa[1][2] == 2.0))
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	free(rfaa);
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getMFColor(getvalue, &rfaa, &i1);
	if ((i1 == 3) && (rfaa[0][0] == 4.0) && (rfaa[0][1] == 5.0) && (rfaa[0][2] == 6.0) && (rfaa[1][0] == 7.0) && (rfaa[1][1] == 8.0) && (rfaa[1][2] == 9.0) && (rfaa[2][0] == 10.0) && (rfaa[2][1] == 11.0) && (rfaa[2][2] == 12.0))
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");
	free(rfaa);

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* MFVec2f*/
	X3D_freeNode(root);
	root = X3D_getNode("COORDINT");

	printf("Testing MFVec2f...\n");
	evOut = X3D_getEventOut(root, "keyValue");
	evIn = X3D_getEventIn(root, "keyValue");

	float faa2[3][2];

	faa2[0][0] = 4.0;
	faa2[0][1] = 5.0;
	faa2[1][0] = 7.0;
	faa2[1][1] = 8.0;
	faa2[2][0] = 10.0;
	faa2[2][1] = 11.0;

	setvalue = X3D_newMFVec2f(3, faa2);
	getvalue = X3D_getValue(evOut);
	X3D_getMFVec2f(getvalue, &rfaa,  &i1);
	if ((i1 == 2)  && (rfaa[0][0] == 1.0) && (rfaa[0][1] == 1.0) && (rfaa[1][0] == 2.0) && (rfaa[1][1] == 2.0))
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	free(rfaa);
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getMFVec2f(getvalue, &rfaa, &i1);
	if ((i1 == 3) && (rfaa[0][0] == 4.0) && (rfaa[0][1] == 5.0) && (rfaa[1][0] == 7.0) && (rfaa[1][1] == 8.0) && (rfaa[2][0] == 10.0) && (rfaa[2][1] == 11.0))
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");
	free(rfaa);

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* MFRotation */
	X3D_freeNode(root);
	root = X3D_getNode("ORINT");

	printf("Testing MFRotation ...\n");
	evOut = X3D_getEventOut(root, "keyValue");
	evIn = X3D_getEventIn(root, "keyValue");

	float faa4[3][4];

	faa4[0][0] = 4.0;
	faa4[0][1] = 5.0;
	faa4[0][2] = 6.0;
	faa4[0][3] = 16.0;
	faa4[1][0] = 7.0;
	faa4[1][1] = 8.0;
	faa4[1][2] = 9.0;
	faa4[1][3] = 19.0;
	faa4[2][0] = 10.0;
	faa4[2][1] = 11.0;
	faa4[2][2] = 12.0;
	faa4[2][3] = 112.0;

	setvalue = X3D_newMFRotation(3, faa4);
	getvalue = X3D_getValue(evOut);
	X3D_getMFRotation(getvalue, &rfaa,  &i1);
	if ((i1 == 2)  && (rfaa[0][0] == 1.0) && (rfaa[0][1] == 1.0) && (rfaa[0][2] == 1.0) && (rfaa[0][3] == 1.0) &&  (rfaa[1][0] == 2.0) && (rfaa[1][1] == 2.0) && (rfaa[1][2] == 2.0) && (rfaa[1][3] == 2.0))
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	free(rfaa);
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getMFRotation(getvalue, &rfaa, &i1);
	if ((i1 == 3) && (rfaa[0][0] == 4.0) && (rfaa[0][1] == 5.0) && (rfaa[0][2] == 6.0) && (rfaa[0][3] == 16.0) && (rfaa[1][0] == 7.0) && (rfaa[1][1] == 8.0) && (rfaa[1][2] == 9.0) && (rfaa[1][3] == 19.0) && (rfaa[2][0] == 10.0) && (rfaa[2][1] == 11.0) && (rfaa[2][2] == 12.0) && (rfaa[2][3] == 112.0))
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");
	free(rfaa);

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* MFColorRGBA*/
	X3D_freeNode(root);
	root = X3D_getNode("COLORRGBA");

	printf("Testing MFColorRGBA...\n");
	evOut = X3D_getEventOut(root, "color");
	evIn = X3D_getEventIn(root, "color");

	faa4[0][0] = 4.0;
	faa4[0][1] = 5.0;
	faa4[0][2] = 6.0;
	faa4[0][3] = 16.0;
	faa4[1][0] = 7.0;
	faa4[1][1] = 8.0;
	faa4[1][2] = 9.0;
	faa4[1][3] = 19.0;
	faa4[2][0] = 10.0;
	faa4[2][1] = 11.0;
	faa4[2][2] = 12.0;
	faa4[2][3] = 112.0;

	setvalue = X3D_newMFColorRGBA(3, faa4);
	getvalue = X3D_getValue(evOut);
	X3D_getMFColorRGBA(getvalue, &rfaa,  &i1);
	if ((i1 == 2)  && (rfaa[0][0] == 1.0) && (rfaa[0][1] == 1.0) && (rfaa[0][2] == 1.0) && (rfaa[0][3] == 1.0) &&  (rfaa[1][0] == 2.0) && (rfaa[1][1] == 2.0) && (rfaa[1][2] == 2.0) && (rfaa[1][3] == 2.0))
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	free(rfaa);
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getMFColorRGBA(getvalue, &rfaa, &i1);
	if ((i1 == 3) && (rfaa[0][0] == 4.0) && (rfaa[0][1] == 5.0) && (rfaa[0][2] == 6.0) && (rfaa[0][3] == 16.0) && (rfaa[1][0] == 7.0) && (rfaa[1][1] == 8.0) && (rfaa[1][2] == 9.0) && (rfaa[1][3] == 19.0) && (rfaa[2][0] == 10.0) && (rfaa[2][1] == 11.0) && (rfaa[2][2] == 12.0) && (rfaa[2][3] == 112.0))
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");
	free(rfaa);

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	X3D_freeEventOut(evOut);

	/* MFBool */
	/* Not yet handled by FreeWRL 
	X3D_freeNode(root);
	root = X3D_getNode ("BOOLSEQ"); 

	printf("Testing MFBool ... \n");
	evOut = X3D_getEventOut(root, "keyValue");
	evIn = X3D_getEventIn(root, "keyValue");

	sleep(5);
	i3[0] = 0;
	i3[1] = 1;
	i3[2] = 0;

	setvalue = X3D_newMFBool(3, i3);
	getvalue = X3D_getValue(evOut);
	X3D_getMFBool(getvalue, &rfi, &i1);
	if ((i1 == 2) && (rfi[0] == 1) && (rfi[1] == 0)) 
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	free(rfi);
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getMFBool(getvalue, &rfi, &i1);
	if ((i1 == 3) && (rfi[0] == 0) && (rfi[1] == 1) && (rfi[2] == 0))
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");

	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	*/

	/* MFVec3d */
	/* Not yet handled by FreeWRL 
	X3D_freeNode(root);
	root = X3D_getNode ("GEOPOS"); 

	double **rdaa;
	double daa[3][3];

	printf("Testing MFVec3d ... \n");
	evOut = X3D_getEventOut(root, "keyValue");
	evIn = X3D_getEventIn(root, "keyValue");
	daa[0][0] = 4.0;
	daa[0][1] = 5.0;
	daa[0][2] = 6.0;
	daa[1][0] = 7.0;
	daa[1][1] = 8.0;
	daa[1][2] = 9.0;
	daa[2][0] = 10.0;
	daa[2][1] = 11.0;
	daa[2][2] = 12.0;

	setvalue = X3D_newMFVec3d(3, daa);
	getvalue = X3D_getValue(evOut);
	X3D_getMFVec3d(getvalue, &rdaa,  &i1);
	if ((i1 == 2)  && (rdaa[0][0] == 1.0) && (rdaa[0][1] == 1.0) && (rdaa[0][2] == 1.0) && (rdaa[1][0] == 2.0) && (rdaa[1][1] == 2.0) && (rdaa[1][2] == 2.0))
		printf("\t\t\t ... get passed\n");
	else
		printf("\t\t\t ... get FAILED\n");
	free(rdaa);
	X3D_setValue(evIn, setvalue);
	X3D_freeNode(getvalue);
	getvalue = X3D_getValue(evOut);
	X3D_getMFVec3d(getvalue, &rdaa, &i1);
	if ((i1 == 3) && (rdaa[0][0] == 4.0) && (rdaa[0][1] == 5.0) && (rdaa[0][2] == 6.0) && (rdaa[1][0] == 7.0) && (rdaa[1][1] == 8.0) && (rdaa[1][2] == 9.0) && (rdaa[2][0] == 10.0) && (rdaa[2][1] == 11.0) && (rdaa[2][2] == 12.0))
		printf("\t\t\t ... set passed\n");
	else
		printf("\t\t\t ... set FAILED\n");
	X3D_freeNode(setvalue);
	X3D_freeNode(getvalue);
	X3D_freeEventIn(evIn);
	*/
	while(1) {
	sleep(5);
	}
/* 	X3D_shutdown (); */
	return 1;
}
