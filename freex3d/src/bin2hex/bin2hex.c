#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv){
	char *inname;
	int ncol = 15;
	if(argc > 1){
		printf("/* argc= %d  argv = [",argc);
		for(int i=0;i<argc;i++)
			printf("%s ",argv[i]);
		printf("] */\n");
		inname = argv[1];
		if(argc > 2) sscanf(argv[2],"%d",&ncol);
		printf("/* inname=%s ncol=%d */\n",inname,ncol);

		FILE *fin = fopen(inname,"r+b");
		if(fin){
			unsigned char *buf = malloc(ncol + 1);
			//convert ..\ProggyClean.ttf to ProggyClean_ttf
			char *bufname = strdup(inname);
			char *ir = strrchr(bufname,'\\'); //other / for linux
			if(ir) bufname = &ir[1];
			ir = strrchr(bufname,'.');
			if(ir) ir[0] = '_';
			//print data
			printf("unsigned char %s_data[] = \n",bufname);
			char *sep = "{";
			int more = 1;
			int m = 0;
			do{
				int nc;
				nc = ncol;
				nc = fread(buf,1,nc,fin);
				if(nc < ncol) more = 0;
				for(int j=0;j<nc;j++){
					printf("%s",sep);
					unsigned int hh = buf[j];
					printf("0x%.2x",hh);
					sep = ",";
				}
				if(more) printf("\n");
				m += nc;
			}while(more);
			printf("};\n");
			//print size
			printf("int %s_size = %d;\n",bufname,m);
		}
	} else {
		printf("testBin2hex <filename_of_bin> [<number_of_columns_default_15>] [> <outfile_name_default_console>] ");
	}

	getchar();
	return 0;
}