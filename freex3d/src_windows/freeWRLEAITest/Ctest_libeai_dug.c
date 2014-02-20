// Ctest_libeai.c : Defines the entry point for the console application.
//

//#include <config.h>
//#include <system.h>
//
////#include <tchar.h>
//#include <string.h>
//#include <malloc.h>
#include <EAI_C.h>
//#include <EAI_swigMe.h>
/*the first 2 swig-venience functions should be minimally sufficient 
but require parsing code to interpret the stringFromField on the script side*/
X3DNode* X3D_swigFieldFromString(char* fieldtype, char* values); 
char * X3D_swigStringFromField(X3DNode* field);

/* the following 4 would also be sufficient (but do not confuse with any "live" field functions in the main libeai) */
X3DNode *X3D_swigNewMF(char *fieldtype, int num );
void X3D_swigAppendToMF(X3DNode* mfnode, X3DNode* sfitem);
void X3D_swigSetItem(X3DNode* node, int item, X3DNode* value);
X3DNode* X3D_swigGetItem(X3DNode* node, int item);

/* the following is what you call right after a port style Advise notification, to get the field data */
X3DNode* X3D_swigCallbackDataFetch(char *ListenerTableIndex);


int main(int argc, char* argv[])
{
	X3DNode *mff,*sff,*sff2,*sff3;

/*tests for newMF,setItem,getItem,appendToMF */
	if(1)
	{
	printf("before init - please run browser with root.wrl --eai :");
	getchar();
	X3D_initialize("");
	printf("after init:");
	getchar();
	printf("version=%s %s\n",X3D_getVersion(),X3D_getName());
	}
	if(1)
	{
	sff = X3D_newSFFloat(33.33f);
	printf("sff.val=%f type=%d\n",sff->X3D_SFFloat.value,sff->X3D_SFFloat.type);
	mff = X3D_swigNewMF("SFFloat",2);
	X3D_swigSetItem(mff,0,sff);
	sff->X3D_SFFloat.value = 44.44f;
	X3D_swigSetItem(mff,1,sff);
	printf("loaded 2 sfs into an mf\n");
	sff2 = X3D_swigGetItem(mff,0);
	printf("sff2 mf[0] .val=%f .type=%ld\n",sff2->X3D_SFFloat.value,sff2->X3D_SFFloat.type);
	sff3 = X3D_swigGetItem(mff,1);
	printf("sff3 mf[1] .val=%f .type=%ld\n",sff3->X3D_SFFloat.value,sff3->X3D_SFFloat.type);
	X3D_freeNode(sff);
	X3D_freeNode(mff);
	X3D_freeNode(sff2);
	X3D_freeNode(sff3);
	}
	if(1)
	{
	sff = X3D_newSFFloat(33.33f);
	printf("sff.val=%f type=%d\n",sff->X3D_SFFloat.value,sff->X3D_SFFloat.type);
	mff = X3D_swigNewMF("SFFloat",0);
	X3D_swigAppendToMF(mff,sff);
	sff->X3D_SFFloat.value = 44.44f;
	X3D_swigAppendToMF(mff,sff);
	printf("loaded 2 sfs into an mf\n");
	sff2 = X3D_swigGetItem(mff,0);
	printf("sff2 mf[0] .val=%f .type=%ld\n",sff2->X3D_SFFloat.value,sff2->X3D_SFFloat.type);
	sff3 = X3D_swigGetItem(mff,1);
	printf("sff3 mf[1] .val=%f .type=%ld\n",sff3->X3D_SFFloat.value,sff3->X3D_SFFloat.type);
	X3D_freeNode(sff);
	X3D_freeNode(mff);
	X3D_freeNode(sff2);
	X3D_freeNode(sff3);

	}
/* tests for fieldFromString and stringFromField */
	if(1)
	{
	 /*tests the mf = X3D_fieldFromString("MFString","111.11 222.22"); */
	 X3DNode* mfs,*mff,*sf;
	 char *str[4];
	 int i,j;
	 mfs = X3D_swigNewMF("MFString",2);
	 /*mfs2 = X3D_fieldFromString("MFString"," \"Howdy\" \"Partner\" ");*/
	 str[0] = " 1234.567,891011.23 ";
	 str[1] = "333.33 444.44";
	 str[2] = "666.66 -777.77,";
	 str[3] = "'88.8''99.9'";
	 for(i=0;i<4;i++)
	 {
		 printf("str=%s ",str[i]);
		 mff = X3D_swigFieldFromString("MFFloat",str[i]); /*" 1234.567,891011.23 ");*/
		 for(j=0;j<mff->X3D_MFString.n;j++)
		 {
			sf = X3D_swigGetItem(mff,j);
			printf("sf[%d]=%f ",j,sf->X3D_SFFloat.value);
		 }
		 printf("\n");
	 }
	}
	if(1)
	{
	 /*tests the string = X3D_stringFromField(X3DNode* field); */
	 X3DNode* mfs,*mff,*sf;
	 char *str,*str2;
	 int i,j, itype, ismf, count;
	 mff = X3D_swigFieldFromString("MFFloat","111.11 -222.22 333.33 -444.44");
	 str = X3D_swigStringFromField(mff);
	 sscanf(str,"%d %d",&itype,&count);
	 ismf = itype % 2;
	 printf("type=%d count=%d str=%s\n",itype,count,str);
	 /*free(str);*/
	 FREE_IF_NZ(str);
	
	 mfs = X3D_swigFieldFromString("MFString","\"Howdy\" \"from\" \"my ol' script.\"");
	 printf("OK did a field\n");
	 str2 = X3D_swigStringFromField(mfs);
	 printf("mfs str=[%s]\n",str2);
	 //X3D_freeNode(mff);
	 X3D_freeNode(mfs);
	 /* OK now parse string */
 	 sscanf(str2,"%d %d",&itype,&count);
	 printf("type=%d count=%d\n",itype,count);
	 if( itype == findFieldInFIELDTYPES("MFString") )
	 {
		 for(i=0;i<count;i++)
		 {
			 /* hard work like in the EAI_C_Field swig parsing functions or nextToken */
		 }

	 }

	 /*free(str2);*/
	 FREE_IF_NZ(str2);

	 /*
	 delim = " ,";
	 if( itype == FIELDTYPE_MFString || itype == FIELDTYPE_SFString )
		 delim = "\"";

	 str[0] = " 1234.567,891011.23 ";
	 str[1] = "333.33 444.44";
	 str[2] = "666.66 -777.77,";
	 str[3] = "'88.8''99.9'";
	 for(i=0;i<4;i++)
	 {
		 printf("str=%s ",str[i]);
		 mff = X3D_fieldFromString("MFFloat",str[i]); 
		 for(j=0;j<mff->X3D_MFString.n;j++)
		 {
			sf = X3D_getItem(mff,j);
			printf("sf[%d]=%f ",j,sf->X3D_SFFloat.value);
		 }
		 printf("\n");
	 }
	 */
	}

	printf("hi from main\n");
	getchar();
	return 0;
}

