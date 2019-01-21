#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include "schglobal.h"
/**************************************/
/** Define message types to use      **/
/**************************************/
#define MSG_ERR				1
#define MSG_INFO			10
#define MSG_DEBUG			25
#define K1				1024

FILE *W32LOG=NULL;
char W32LOGDIR[K1];
char LogDate[12];
char LogTime[9];
char buf[1024];
char tempname[1024];
char msgbuf [1024];
int msgtype;
int  newlog=0;
int OpenLog(char *);
int trim(char *);
void lmsg(int type,const char *format,...);
void SetLogType(int type);
static char * TransType(int type);
//
//  Close the Log file 
//
int CloseLog(void)
{
	if(W32LOG) fclose(W32LOG);
	W32LOG=NULL;
return 1;
}
int OpenLog(char *name)
{
int x=1;
long rc=0;
char second[1024];
time_t timet;
struct tm *tm;

	memset(W32LOGDIR,0,sizeof(W32LOGDIR));
	strcpy(W32LOGDIR,"/home/amd/");
	strcpy(tempname,W32LOGDIR);
	strcat(tempname,name);
	strcpy(second,tempname);
	strcat(tempname,".log");
	strcat(second,".bak");
	W32LOG=fopen(tempname,"w");
	sprintf(LogDate,"%4d/%2d/%2d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday);
	sprintf(LogTime,"%2d:%2d:%2d",tm->tm_hour,tm->tm_min,tm->tm_sec);
	msgtype=MSG_INFO;											// Set for info by default
	lmsg(MSG_INFO,"Log [%s] Opened",name);						// Log it to the system
return 1;
}
int WriteLog(char *line)
{
time_t timet;
struct tm *currt;

	if(!W32LOG) W32LOG=fopen(tempname,"a+");
	timet=time(NULL);
	currt=localtime(&timet);
	sprintf(LogDate,"%4d/%2d/%2d",currt->tm_year+1900,currt->tm_mon+1,currt->tm_mday);
	sprintf(LogTime,"%2d:%2d:%2d",currt->tm_hour,currt->tm_min,currt->tm_sec);
	sprintf(buf,"[%s] [%s] %s\n",LogDate,LogTime,line);
	if(W32LOG) fputs(buf,W32LOG);
	CloseLog();
	return 1;
}
int trim(char *data)
{
int x=0;

	x=strlen(data);
	if(x<1) return 0;
	x--;
	while((data[x]<=' ') && x) {data[x]=0; x--;}
return x;
}
//
// Build a message from a variable list of args
//
void lmsg(int type,const char *format,...)
{
va_list args;
	
	if (type > msgtype) return;		// check message type (default is MSG_INFO)
	va_start(args,format);			// Start the args
	vsprintf(buf,format,args);		// Format the buffer
	va_end(args);					// End args
	sprintf(msgbuf,"[%s] %s",TransType(type),buf);
	WriteLog(msgbuf);				// Send it to the log
return;
}
void SetLogType(int type)
{
	msgtype=type;					// Reset the message type
}
static char * TransType(int type)
{
	if(type==MSG_INFO)	return ("INFO ");
	if(type==MSG_ERR)	return ("ERROR"); 
	if(type==MSG_DEBUG)	return ("DEBUG");
	return ("UNKN ");
}
char *GetPathInfo(char *name)
{
static char pathbuf[_MAX_PATH+1];

	strcpy(pathbuf,"/home/amd/");
	if(strcmp(name,"VARIABLES") == 0) {
		strcpy(pathbuf,"home/amd/var/");
		}

return (pathbuf);
}
