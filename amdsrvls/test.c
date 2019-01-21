
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "W32trace.h"
#define _MAX_PATH       1024
int splitpath(char *in,char *path,char *file);
void SetUpFileLocation(char *fn,char *loc,char *fullname);

int main(int argn, char *argc[])
{
char path[256];
char file[256];
char fullname[256]="";

	OpenLog("AmdTest");
	lmsg(MSG_INFO,"Hello from Log Message");
        splitpath(argc[1],path,file);
        printf("The Path for the file is.: [%s]\n",path);
        printf("The File name is.........: [%s]\n",file);
        printf("Build /home/test1/test2/test3\n");
        SetUpFileLocation(file,path,fullname);
        printf("Full name is [%s]\n",fullname);
}

void SetUpFileLocation(char *fn,char *loc,char *fullname)
{
char buffer[_MAX_PATH+1];
char *c,*s;
	buffer[0]='\0';
	if(strlen(loc)==0) strcpy(loc,"/home/");						// Set the default
	if(loc[strlen(loc)-1]!='/') strcat(loc,"/");					// End with a dir sep
	strcpy(fullname,loc);											// put in the start
	strcat(fullname,fn);											// finish it up
	if( (access( loc, 0 )) != -1 ) return;							// directory already there
	c=(char *)strstr(loc,"/"); s=loc; c++;									// set pointer
	while(c)
		{
		c=(char *)strstr(c,"/");											// Find it please
		if(!c) break;												// At the end now
		*(c)='\0';													// Null it out
		strcat(buffer,s);											// copy in first part
		mkdir(buffer,755);												// create it please
		strcat(buffer,"/");										// Setup for next
		c=c+1; s=c;													// Reset pointers
		}
return ;
}


int splitpath(char *in,char *path,char *file)
{
int len;

        if(!in || (len=strlen(in)) <= 0) {
                lmsg(MSG_ERR,"No input file to splitpath routine");
                }
        while(--len) {
                if(in[len] == '/') {
                        strcpy(path,in);
                        path[len]='\0';
                        strcat(path,"/");
                        strcpy(file,&in[++len]);
                        return 1;
                        }
                }

}