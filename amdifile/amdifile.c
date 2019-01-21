#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include "nettcp.h"
#include "W32trace.h"
#include "crc.h"
#include "schglobal.h"
#define IDATBUF	8192
#define DWORD long
//
// TCP Connection Control Blocks defined here
//
SOCKET s;
PINFBLK ib=NULL;								    // INfo block adrress
//WSADATA data;
struct sockaddr_in server;
struct hostent *h;
unsigned short port=80;

char cmdbuf[20480];									// HTTP Command string
char inetdate[16];
char inettime[16];
DWORD  ThId;											// Thead ID

void StopLClient(SOCKET s,int rc);					// Terminate Client
int DumpArea(char *data,int len);					// Dump a data block to log
int GetAMDValue(char *name);						// Get an AMD REG value
void ConnectToServer(char *ipaddr);					// Open Session
void RecvRespn(void);								// Get Server reponse
void SetupSocket(void);								// Build Socket please
void ChangeBlanks(char * field);					// HTTP Parms must not contain blanks
void FmtDate(char *time,char *itime);				// Format date for HTTP
void FmtTime(char *time,char *itime);				// Format Time for HTTP
void FmtFile(char *time,char *itime);

void BuildIdata(void);								// Build the server data
void ThTimeout(char *type);							// Module timeout Thread

char Inetname[128];
//char PostReg[]="POST /amdweb/amdfile.asp";			// Address for group information page
char PostReg[]="POST /amds/amdsupdfile";			// Address for group information page
char PostFmt[]="%s HTTP/1.0\nContent-type: application/x-www-form-urlencoded\nContent-length: %d\n\n%s";
char Content[]="Content-type: application/x-www-form-urlencoded\nContent-length: 50\n\n";
char idata[IDATBUF]="M=%s&LD=%s&LT=%s&FN=%s&AC=%s&FD=%s&FS=%s";
char mach[64];
char date[12];
char ttime[12];
char fname[_MAX_PATH+1];
char act[8];
char fdate[10];
char size[16];

int main(int argn,char *argc[])
{
char *c=NULL;

	OpenLog("amdifile");								// Open the Log file
	strcpy(Inetname,REG_ISERV);						// Copy in reg value for this guy
	GetAMDValue(Inetname);							// Get the internet server address
	if(Inetname[0]<=' ') {
		lmsg(MSG_ERR,"Internet server not defined to AMDCFG.EXE");
		StopLClient(s,8);							// Get out of here please
		}
	if((c=strstr(Inetname,":"))) {						// port supplied
		*(c)='\0';
		c++;
		port =atoi(c);
		lmsg(MSG_INFO,"Port assigned to : [%d] for Host [%s]",port,Inetname);
		}
        if(argn<8) {
                lmsg(MSG_INFO,"Invalid number of args [%d]",argn);
                printf("Usage: amdifile machine date time file action fileDate fileSize\n");
                exit(8);
                }
        strcpy(mach,argc[1]);
        strcpy(date,argc[2]);
        strcpy(ttime,argc[3]);
        strcpy(fname,argc[4]);
        strcpy(act,argc[5]);
        strcpy(fdate,argc[6]);
        strcpy(size,argc[7]);

	ChangeBlanks(mach);
        lmsg(MSG_INFO,"Update Machine");
        ChangeBlanks(fname);   // Replace blanks with +
	ChangeBlanks(fdate);
        trim(size);

	lmsg(MSG_INFO,"File Event....:[%s]",mach);
	lmsg(MSG_INFO,"Date Time.....:[%s %s]",date,ttime);
	lmsg(MSG_INFO,"File Name.....:[%s]",fname);
	lmsg(MSG_INFO,"Action........:[%s]",act);
	lmsg(MSG_INFO,"File Date.....:[%s]",fdate);
	lmsg(MSG_INFO,"File Size......[%s]",size);

	BuildIdata();									// Build the Inet date transfer format

	sprintf(cmdbuf,PostFmt,PostReg,strlen(idata),idata);

	SetupSocket();									// Setup WSData
	ConnectToServer(Inetname);						// Make the connection (may not return if errors)
	send(s,cmdbuf,strlen(cmdbuf),0);				// Send the POST request to the Server
	RecvRespn();									// Wait for server Response
	lmsg(MSG_INFO,"Shutdown of execution");			// Last Message Please
	StopLClient(s,0);
return 0;
}
//
// Setup the socket to be used to contact the Iternet server
//
void SetupSocket(void)
{
int rc=0;

//	WSAStartup(WinSockVersion,&data);				// WinSock Startup
	s=socket(AF_INET,SOCK_STREAM,0);				// Build a socket for TCP s is global socket
	rc=1;
return;
}
//
// Start the TCP/IP Seesion please
//
void ConnectToServer(char *ipname)
{
DWORD ipaddr;
int rc=0;
	ipaddr=inet_addr(ipname);						// Is it an IP address
	if(ipaddr!=INADDR_NONE) server.sin_addr.s_addr=ipaddr;
	else							 				// else its a name
		{
		(struct hostent *)h=gethostbyname(ipname);
		if(h==NULL) {lmsg(MSG_ERR,"Host [%s] not found",ipname); StopLClient(s,8);}
		memcpy(&server.sin_addr,h->h_addr,h->h_length);
		}
	lmsg(MSG_INFO,"AMDIREG using PORT [%d]",port);
	server.sin_port=htons(port);
	server.sin_family=AF_INET;
	if(connect(s,&server,sizeof(server)))
		{
//		rc=WSAGetLastError();
		lmsg(MSG_ERR,"Connect to host [%s] failed rc[%d]",ipname,(void *)rc);
		StopLClient(s,16);
		}
return;
}
//
// Shutdown a client task
//
void StopLClient(SOCKET s,int rc)
{
	shutdown(s,2);
	close(s);									// Now Close it
	CloseLog();
	exit(rc);
}
//
// Get an AMD Reg Value
//
#define AMDCFG   "amdcfg"
int GetAMDValue(char *name)
{
int x=1;
DWORD dwt=0,dwl=_MAX_PATH;
long rc=0;
static char buf[_MAX_PATH+1];

        LoadVbase(AMDCFG);
        findvalue(buf,AMDCFG,name);
        lmsg(MSG_INFO,"Value returned [%s]",buf);
        strcpy(name,buf);
        return (strlen(buf));
}
//
// Wait for the HTTP response
//
void RecvRespn(void)
{
FILE *out;
char *c;
int len=0,flen=0;
char htmlname[1024];

	strcpy(htmlname,W32LOGDIR);						// Build output file name
	strcat(htmlname,"AMDIFILE.HTM");					// Finish it off
	memset(cmdbuf,0,sizeof(cmdbuf));				// Clear it out please
	c=cmdbuf;										// Set a poiter to the bufer adress
	while(1)
		{
		len=recv(s,c,2048,0);						// DO a receive
		if(!len || len<0) break;					// No length then to connetions dropped
		c=c+len;									// move pointer into the bufer
		flen=len+flen;								// keep track of the full length
		}
	out=fopen(htmlname,"w");						// Open up a file please
	fwrite(cmdbuf,flen,1,out);						// Write Thye servers reponse
	fclose(out);									// Close the file please
return;
}

void ChangeBlanks(char * field)
{
int x=0;
	while(field[x]) {
	if(field[x]==' ') field[x]='+';
	x++;
	}
return;
}
void BuildIdata(void)
{
char hold[IDATBUF];
char filename[1024]="";

	FmtDate(date,inetdate); FmtTime(ttime,inettime);							// build POST format date
	FmtFile(fname,filename);
	sprintf(hold,idata,mach,inetdate,inettime,filename,act,fdate,size);	// Build Buffer
	strcpy(idata,hold);
return;
}
void FmtDate(char *date,char *idate)
{
int x=0,y=0;
	while(date[x]) {
		if(date[x]=='/') {
			strcat(idate,"%2F"); x++; y=y+3;
			continue;
			}
		idate[y++]=date[x++];
		}
}
void FmtTime(char *time,char *itime)
{
int x=0,y=0;
	while(time[x]) {
		if(time[x]==':') {
			strcat(itime,"%3A"); x++; y=y+3;
			continue;
			}
		itime[y++]=time[x++];
		}
}
void FmtFile(char *time,char *itime)
{
int x=0,y=0;
	while(time[x]) {
		if(time[x]=='\\') {
			strcat(itime,"%5C"); x++; y=y+3;
			continue;
			}
		if(time[x]==':') {
			strcat(itime,"%3A"); x++; y=y+3;
			continue;
			}
		itime[y++]=time[x++];
		}
}

//
// This thread just sits here and if time limit reached it will exit
//
void ThTimeout(char *type)
{
DWORD wait=120000;										// Wait Two minutes MAX
//	SleepEx(wait,FALSE);
	exit(0);
}
