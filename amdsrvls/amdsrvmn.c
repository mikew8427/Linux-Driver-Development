#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include "W32trace.h"
#include "amdvdf.h"
#include "crc.h"
#include "nettcp.h"
#include "schglobal.h"
#include "amdsrvmn.h"


#define PORTNUMBER 1931
unsigned short MainPort = PORTNUMBER;
int GetRecord(PINFBLK ib);
void talk(int sok);

SB sb;

int main (int argn, char *argc[])
{
int ClientSok=0,trueval=1,worker=0,length=0;
struct protoent *protocol;
struct sockaddr_in socka;

        OpenLog("amdsrvls");                                    // Open a log file
        sb.port=MainPort;                                       // Set Default
        sb.storagefail=0;

        if(argn == 2) {
                sb.port = (unsigned int) atoi(argc[1]);         // Get New Port Number
                }
        lmsg(MSG_INFO,"AMDSRVLS Selected Port [%d]",sb.port);   // Let them all know

        gen_crc_table();
        protocol = getprotobyname("tcp");
        socka.sin_family = AF_INET;
        socka.sin_addr.s_addr = INADDR_ANY;
        socka.sin_port = htons(sb.port);
        ClientSok = socket(PF_INET,SOCK_STREAM,protocol->p_proto);
        if(ClientSok < 0 ) {
                lmsg(MSG_INFO,"Unable to establish the Listener Socket\n");
                return 16;
                }
        if(bind(ClientSok,&socka,sizeof(socka)) <0 ) {
                lmsg(MSG_ERR,"Unable to Bind Socket");
                return 16;
                }
        setsockopt(ClientSok,SOL_SOCKET,SO_REUSEADDR, &trueval, sizeof(trueval));
	if( listen(ClientSok,0) < 0) {
		lmsg(MSG_ERR,"Listen Failure - AMDLISTN Exits");
		return 16;
		}
        lmsg(MSG_INFO,"AMDSRVLS  Listening for Connections...");
        while(1) {
                worker = accept(ClientSok,&socka,&length);
                talk(worker);
                }
        lmsg(MSG_INFO,"AMDLISTN is Exiting");
} // End of main

void talk(int sok)
{
PINFBLK ib=NULL;
int rc;

        ib=(PINFBLK)malloc(sizeof(struct InfoBlk));
        if(!ib) {
                lmsg(MSG_ERR,"Unable to build Session Control Block");
                }
        InitIB(ib);
        ib->sock=sok;
        while(1) {
                if(!RecvTDH(ib)) break;
                if(ib->term_session) break;
                if(!DispTrans(ib)) break;
                }

        lmsg(MSG_INFO,"Transaction completed Thread Exits");
} // End of Function talk

int DispTrans(PINFBLK ib)
{
int rc;
        switch(ib->tdhtype) {
                case SHUT: {ib->term_session=1; return 1;}                              // process the EndOf Block transaction
                case EXEC: {rc=ExecCmd(ib); SendResponse(ib,rc); return rc;  }          // Execute a command and send the Executable
                case EXEO: {rc=ExecOnly(ib); SendResponse(ib,rc); return rc; }          // Execute a command and it must exsit locally
                case TRDT: {rc=DataOnly(ib); SendResponse(ib,rc); return rc; }          // Receive a file please
                case SNDF: {rc=SendFile(ib); return rc;}                                // Send a file to the caller
                }// End Switch
        lmsg(MSG_INFO,"Unknown Transaction [%d]",ib->tdhtype);
        return 0;

} // End of Disp Trans

//
// Start the COmmand that comes in from a client
//
int ExecCmd(PINFBLK ib)
{
char type[36];
char f[_MAX_PATH];
char p[_MAX_PATH];
char l[_MAX_PATH];
char fn[_MAX_PATH];
char fext[_MAX_EXT];
char cmd[_MAX_PATH];
char path[_MAX_PATH];
int BuiltFile=0;
PBUFENT pb;
int rc,dlen,count=0;
FILE *out=NULL;

        memset(type,0,sizeof(type));
        memcpy(type,ib->recbuf,ib->recbuf_len);
	memset(f,0,_MAX_PATH);
	memset(p,0,_MAX_PATH);
	memset(l,0,_MAX_PATH);
        pb=(PBUFENT)ib->recbuf;
        while( (rc=GetRecord(ib))>0) {
		dlen=ib->recbuf_len-1;										// Save the data length
		if(pb->type=='F') {
			memcpy(f,pb->data,dlen);								// Copy in the File Name
			Resolve(f);
                        splitpath(f,path,cmd);
			strcpy(fn,cmd);										// Both hold file name
			}
		if(pb->type=='P') {
			memcpy(p,pb->data,dlen);								// Copy in the Parms
			Resolve(p);
			}
		if(pb->type=='L') {
			memcpy(l,pb->data,dlen);								// Copy in the Parms
			Resolve(l);
			}
		if(pb->type=='D')	{
			if(!out){SetUpFileLocation(fn,l,f); out=fopen(f,"wb");}	// Open it Up
			BuiltFile=1;											// tell them we did it
			if(out) {fwrite(pb->data,dlen,1,out); count++;}			// Write it out
			}

                }
 	if (out) {
		lmsg(MSG_INFO,"Number of records Written were[%d]",count);	// Dump out number of records
		fclose(out);
		}
	if(!BuiltFile) SetUpFileLocation(fn,l,f);						// Do this Now
        sprintf(cmd,"chmod 755 %s",f);
        system(cmd);
        lmsg(MSG_INFO,"Execute [%s] ",f);
        lmsg(MSG_INFO,"      Remote Path[%s]", path);
        lmsg(MSG_INFO,"      With Parms [%s]", p);
        lmsg(MSG_INFO,"      Location   [%s]", l);
        sprintf(cmd,"%s %s",f,p);
        system(cmd);
   return 1;
}
//
// Start the COmmand that come in from client
//
int ExecOnly(PINFBLK ib)
{
char type[36];
char f[_MAX_PATH];
char p[_MAX_PATH];
char l[_MAX_PATH];
char fn[_MAX_PATH];
char fext[_MAX_EXT];
char cmd[_MAX_PATH];
char path[_MAX_PATH];
PBUFENT pb;
int rc,dlen;

        memset(type,0,sizeof(type));
        memcpy(type,ib->recbuf,ib->recbuf_len);
	memset(f,0,_MAX_PATH);
	memset(p,0,_MAX_PATH);
	memset(l,0,_MAX_PATH);
        pb=(PBUFENT)ib->recbuf;
        while( (rc=GetRecord(ib))>0) {
		dlen=ib->recbuf_len-1;										// Save the data length
		if(pb->type=='F') {
			memcpy(f,pb->data,dlen);								// Copy in the File Name
			Resolve(f);
                        splitpath(f,path,cmd);
			strcpy(fn,cmd);											// Both hold file name
			}
		if(pb->type=='P') {
			memcpy(p,pb->data,dlen);								// Copy in the Parms
			Resolve(p);
			}
		if(pb->type=='L') {
			memcpy(l,pb->data,dlen);								// Copy in the Parms
			Resolve(l);
			}
                }
	if(rc<0) return 16;						    		// Bad return
	if(strlen(l)>0) {strcpy(cmd,l); strcat(cmd,f);}					// Location supplied
	else strcpy(cmd,f);						                // Build Buffer
        strcat(cmd," "); strcat(cmd,p);
	lmsg(MSG_INFO,"Execute Command [%s]",cmd);					// Tell What we are doing
        system(cmd);
	lmsg(MSG_INFO,"Exit    Command Routine RC[%d]",rc);				// Let'em Know
	return rc;
   return 1;
}
//
// The routine responds to a Send File request from a client (File Receive)
//
int DataOnly(PINFBLK ib)
{
FILE *out=NULL;
int count=0,dlen=0;
char type[36];
char f[_MAX_PATH+1];												// Full Name
char path[_MAX_PATH+1];
char p[_MAX_PATH+1];												// Parms
char l[_MAX_PATH+1];												// Loction
char fn[_MAX_PATH];													// File name
char fext[_MAX_PATH];												// File Extention
int rc=0,BuiltFile=0;
int datasent=0;														// Was data sent??
PBUFENT pb;
struct tm tm1;
time_t tmt;
struct utimbuf timebuf;
char perm[24];
char cmd[_MAX_PATH];
char group[128]="";
char owner[128]="";


	memset(type,0,36);												// Clear out Type Name
        memset(&tm1,0,sizeof(tm1));
        memset(&timebuf,0,sizeof(timebuf));
	memcpy(type,ib->recbuf,ib->recbuf_len);							// Save type away
	memset(f,0,_MAX_PATH+1);											// Clear it out
	memset(p,0,_MAX_PATH+1);
	memset(l,0,_MAX_PATH+1);
	pb=(PBUFENT)ib->recbuf;											// point to it
	while((rc=GetRecord(ib))>0)
		{
		pb=(PBUFENT)ib->recbuf;											// point to it
		dlen=ib->recbuf_len-1;										// Save the data length
		if(pb->type=='F') {
			memcpy(f,pb->data,dlen);								// Copy in the File Name
			Resolve(f);
                        splitpath(f,path,fn);
			}
		if(pb->type=='L') {
			memcpy(l,pb->data,dlen);								// Copy in the Location
			Resolve(l);
			}
		if(pb->type=='P') {
			memcpy(p,pb->data,dlen);								// Parms are the Date and time
			Resolve(p);
			}
		if(pb->type=='D')	{
			if(!out){SetUpFileLocation(fn,l,f); out=fopen(f,"wb");}	// Open it Up
			BuiltFile=1; datasent=1;								// tell them we did it
			if(out)	{fwrite(pb->data,(size_t)dlen,(size_t)1,out); count++;}			// Write it out
			}
		}
	if(rc<0) return 16;										// Bad return
	if(!out && datasent) return 16;									// Very Bad in deed
	if (out) fclose(out);										// Close it Please
	if(strlen(p)>0) {
                lmsg(MSG_INFO,"Parm data for File transfer [%s]",p);
		sscanf(p,"%d %d %d %d %d %d %s %s %s",&tm1.tm_year,&tm1.tm_mon,&tm1.tm_mday,&tm1.tm_hour,&tm1.tm_min,&tm1.tm_sec,perm,group,owner);
                tm1.tm_year-=1900;
                tm1.tm_mon-=1;
                tmt=mktime(&tm1);
                timebuf.actime = tmt;
                timebuf.modtime = tmt;
                sprintf(cmd,"chmod %s %s",perm,f);
                system(cmd);
                sprintf(cmd,"chgrp %s %s",group,f);
                system(cmd);
                sprintf(cmd,"chown %s %s",owner,f);
                system(cmd);
                utime(f,&timebuf);
		// RememberData(&st,f,rn,p,"WRITE");
		}
	lmsg(MSG_INFO,"Data File Transfer Complete for [%s] Recourd Count [%d]",f,count);
return 0;
}
//
// Someone asked us for a file Please comply
//
int SendFile(PINFBLK ib)
{
char type[36];
char f[_MAX_PATH+1];												// Full Name
char p[_MAX_PATH+1];												// Parms
char l[_MAX_PATH+1];												// Loction
int rc=0,BuiltFile=0,dlen=0;
PBUFENT pb;													// Buffer mapper

	memset(type,0,sizeof(type));									        // Clear out Type Name
	memcpy(type,ib->recbuf,ib->recbuf_len);							                // Save type away
	memset(f,0,sizeof(f));											// Clear it out
	memset(p,0,sizeof(p));
	memset(l,0,sizeof(l));
	pb=(PBUFENT)ib->recbuf;											// point to it
	while((rc=GetRecord(ib))>0)
		{
		dlen=ib->recbuf_len-1;										// Save the data length
		if(pb->type=='F') {
			memcpy(f,pb->data,dlen);								// Copy in the File Name
			Resolve(f);
			}
		if(pb->type=='P') {
			memcpy(p,pb->data,dlen);								// Copy in the Parms
			Resolve(p);
			}
		if(pb->type=='L') {
			memcpy(l,pb->data,dlen);								// Copy in the location
			Resolve(l);
			}
		}
	if(rc<0) return 16;											// Bad return
	rc=SendData(ib,f,p);											// Send data to requestor
	if(rc) SendResponse(ib,rc);										// If rc then an error has occurred
return 0;
}
//
// Send data for this Block, 'fn' is local name and 'p' is remote Name
//
int SendData(PINFBLK ib,char *fn,char *p)
{
int count=0;
FILE *in=NULL;

	if(!p) return 16;									// Nadda
	in=fopen(fn,"rb");									// Open for read the CMD File file
	if(!in) return 4;									// Not Found
	InitTrans(ib);										// Setup for Sends
	WriteTransBuffer(ib,REC_FILE,p,strlen(p));			// Write Out remote file Name
	SendDataOnTrans(ib);								// Send it out
	while(ib->recbuf_len=fread(&ib->recbuf[1],1,(REC_BUFF_SIZE-1),in))
		{
		ib->recbuf[0]='D';								// Set data Type
		ib->recbuf_len++;								// Add in One
		SendDataOnTrans(ib);							// Send it out
		memset(ib->recbuf,0,REC_BUFF_SIZE);				// clear out the buffer
		}
	fclose(in);											// Free it up please
	StopDataOnTrans(ib);								// We be done and stuff
	SendTrans(ib);										// Send it out now please
	lmsg(MSG_INFO,"Send for File Complete [%s]",fn);
return 1;
}
void WriteTransBuffer(PINFBLK ib,char *type,char *data,int len)
{
	ib->recbuf_len=len+1;								// Set the length
	ib->recbuf[0]=type[0];								// Set File Name Please
	memcpy(&ib->recbuf[1],data,len);					// Copy it in
return;
}
//
//
// Utility Section
//
int GetRecord(PINFBLK ib)
{
        memset(ib->recbuf,0,REC_BUFF_SIZE);                             // Setup for record
        ib->recbuf_len=0;                                               // Set length to 0
        while(!ib->recbuf_len)
                {
                if(!RecvTDH(ib)) {ib->term_session=1; return -1;}       // Get entry off line
                if(ib->tdhdesc==ENDSECTION) return 0;                   // Are we at the end
                if(ib->recbuf_len==0) continue;                         // Null Record (Start Trans)
                }
return 1;
}
//
// Respond to Calling System
//
int SendResponse(PINFBLK ib,int rc)
{
	memset(ib->recbuf,0,16);										// Clean it out please
	sprintf(ib->recbuf,"%.3d",rc);									// Char Rc
	ib->recbuf_len=strlen(ib->recbuf);								// Set the length
	StartTrans(ib,(char)RSPNS,0,TRUE);								// Set the transaction type to Verfile
return 1;
}
int splitpath(char *in,char *path,char *file)                                                                                                				// See if we need to rebuild
{
int len;

        if(!in || (len=strlen(in)) <= 0) {
                lmsg(MSG_ERR,"No input file to splitpath routine");
                }
        while(--len) {
                if(in[len] == '/' || in[len] == '\\' ) {
                        strcpy(path,in);
                        path[len]='\0';
                        strcpy(file,&in[++len]);
                        return 1;
                        }
                }

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
