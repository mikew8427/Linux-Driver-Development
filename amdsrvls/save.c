#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "W32trace.h"
#include "amdvdf.h"
#include "crc.h"
#include "nettcp.h"
#include "schglobal.h"
#include "amdsrvmn.h"
int GetRecord(PINFBLK ib);
int shutcmd(char *cmd);

SB sb;												// Server block definition	

void talk(SOCKET s)
{
PINFBLK ib=NULL;
int rc,myoff=0;		

   	ib=malloc(sizeof(struct InfoBlk));				// Get memory for this block
	if(!InitIB(ib)) 								// Allocate thr info block    
		{
		lmsg(MSG_ERR,"Allocation of IB failed");	// Tell'em                    
		StopClient(s);								// Close socket				  
		return;										// End this thread            					
		}
	ib->sock=s;										// save socket in IB
	rc=1;
//	rc=setsockopt(ib->sock,IPPROTO_TCP,TCP_NODELAY,(const char *)&rc,sizeof(int));
	rc=SR_BUFF_SIZE+1024;
	rc=setsockopt(ib->sock,SOL_SOCKET,SO_RCVBUF,(const char *)&rc,sizeof(int));
	rc=SR_BUFF_SIZE+1024;
	rc=setsockopt(ib->sock,SOL_SOCKET,SO_SNDBUF,(const char *)&rc,sizeof(int));
	// Assign a task block for this guy
	ib->task=myoff;
	lmsg(MSG_INFO,"Tranaction Env. Built Waiting for Server Command");
	while(TRUE)
		{
		if(!RecvTDH(ib)) break;						// Read in the Transaction
		if(ib->term_session) break;					// Session Request Ended
		if(!DispTrans(ib)) break;				    // Start the transaction
		}
	StopClient(s);									// End the session
	lmsg(MSG_INFO,"Client task socket stopped Thread exits");
}
BOOL signal_handler(DWORD CtrlType)
{

//	WSACleanup();
//	CloseLog();
//	ExitProcess(0);
return TRUE;
}
//
// Start of main Application Code
//
void StartTalk(char *port)
{
WSADATA data;
static SOCKADDR_IN my_addr, client_addr;
SOCKET client_socket;
char cPORT[64]="PORT";


		OpenLog("AMDLISTN");                                    // Open the Log file          
		InitSB(&sb);											// Build the Server Block     
		Running=1;
		
		// GetAMDValue(cPORT);
		sb.port=(unsigned short)atoi("1931");
		if(!sb.port) sb.port=PortNumber;						// No override set defaul
		lmsg(MSG_INFO,"AMDLSTN Startup using port number [%d]",(void *)sb.port);
		AppendDefault();										// Append AMDTRAN.CSV to DEFAULT.CSV
		gen_crc_table();										// Build crc table
		LoadVbase("BASE");
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)signal_handler,TRUE);
		WSAStartup(WinSockVersion,&data);						// WinSock Startup            
		my_addr.sin_port   =htons(sb.port);						// Setup SOCKET for listen    
		my_addr.sin_family =AF_INET;
		my_addr.sin_addr.s_addr=htonl(INADDR_ANY);
		my_socket=socket(AF_INET,SOCK_STREAM,0);
		bind(my_socket,(LPSOCKADDR)&my_addr,sizeof(my_addr));
		lmsg(MSG_DEBUG,"Wait on listen in Main");
		listen(my_socket,1);									/* Listen for a client		  */

		while(Running)							
			{
			register HANDLE child;
			DWORD childid;
			client_socket=accept(my_socket,(LPSOCKADDR)&client_addr,NULL);	/* Accept the session */
			talk(client_socket);
			}
		ServiceStop();
return;
}
// 
// Start of the Server block on the right foot
//
void InitSB(PSB sb)
{
	sb->port=PortNumber;
	sb->storagefail=0;
return;
}
//
// Shutdown a client task
//
void StopClient(SOCKET s)
{
	shutdown(s,2);										// Disable Send/Recv for socket
	closesocket(s);										// Now Close it
}
int DispTrans(PINFBLK ib)
{
int rc;
	switch(ib->tdhtype) {
		case SHUT:		{ib->term_session=1; return 1;}			// Set Terminate Session
		case EXEC:		
			{
			rc=ExecCmd(ib);										// Call routine & get rc
			SendResponse(ib,rc);								// Send Back the Response
			return (1);											// Bacl Home please
			}
		case EXEO:		
			{
			rc=ExeOnly(ib);
			SendResponse(ib,rc);								// Send Back the Response
			return (1);											// Bacl Home please
			}
		case STOP:		
			{
			rc=ExeOnly(ib);
			SendResponse(ib,rc);								// Send Back the Response
			return (1);											// Bacl Home please
			}
		case TRDT:		
			{
			return (1);
			}
		case SNDF:	
			{
			}
		case STRT:		
			{
			}
		case LOCK:		
			{
			}
		case REMV:		
			{
			}
		case STGD:		
			{
			return (1);
			}

		default:		{lmsg(MSG_ERR,"Unknown Tranaction type [%d]",(void *)ib->tdhtype); return 0;}
	}
return 1;
}
int ExecCmd(PINFBLK ib)
{
int count=0,dlen=0;
char type[36];
char f[_MAX_PATH+1];												// Full Name
char p[_MAX_PATH+1];												// Parms
char l[_MAX_PATH+1];												// Loction
char fn[_MAX_PATH];													// File name
char cmd[1024];														// Command to execute
char fext[_MAX_PATH];												// File Extention
FILE *out=NULL;
int rc=0,BuiltFile=0;
PBUFENT pb;															// Buffer mapper

	memset(type,0,sizeof(type));									// Clear out Type Name
	memcpy(type,ib->recbuf,ib->recbuf_len);							// Save type away
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
			_splitpath(f,NULL,NULL,fn,fext);						// Break it Up
			strcpy(f,fn); strcat(f,fext);							// Build it up
			strcpy(fn,f);											// Both hold file name
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
	if(rc<0) return 16;												// Bad return
	if (out) {
		lmsg(MSG_INFO,"Number of records Written were[%d]",count);	// Dump out number of records	
		fclose(out);
		}
	if(!BuiltFile) SetUpFileLocation(fn,l,f);						// Do this Now
	if(stricmp(type,"EXEC")==0)										// Is this an EXEC Type
		{
		strcpy(cmd,f);												// Build Buffer
		FormatCmd(cmd,fext,p);										// See if we need to rebuild
		lmsg(MSG_INFO,"Execute Command [%s]",cmd);					// Tell What we are doing
		rc=runcmd(cmd,ib);											// Run it Please												
		lmsg(MSG_INFO,"Exit    Command Routine RC[%d]",rc);			// Let'em Know
		return rc;
		}
return 1;
}
void SetUpFileLocation(char *fn,char *loc,char *fullname)
{
char buffer[_MAX_PATH+1];
char *c,*s;
	buffer[0]='\0';
	if(strlen(loc)==0) strcpy(loc,W32LOGDIR);						// Set the default
	if(loc[strlen(loc)-1]!='\\') strcat(loc,"\\");					// End with a dir sep
	strcpy(fullname,loc);											// put in the start
	strcat(fullname,fn);											// finish it up
	if( (_access( loc, 0 )) != -1 ) return;							// directory already there
	c=strstr(loc,"\\"); s=loc; c++;									// set pointer
	while(c) 
		{
		c=strstr(c,"\\");											// Find it please
		if(!c) break;												// At the end now
		*(c)='\0';													// Null it out
		strcat(buffer,s);											// copy in first part
		mkdir(buffer);												// create it please
		strcat(buffer,"\\");										// Setup for next
		c=c+1; s=c;													// Reset pointers
		}
return;
}
void FormatCmd(char *cmd,char *ext,char *p)
{
char buffer[1024];
char fn[_MAX_PATH];
char fext[_MAX_PATH];


	memset(buffer,0,sizeof(buffer));						// Clear out Buffer
	if( (_access( cmd, 0 )) == -1 )							// Not found in This Directory
		{
		_splitpath(cmd,NULL,NULL,fn,fext);
		strcpy(cmd,fn); strcat(cmd,fext);					// Let System search for it
		}
	if(stricmp(ext,".BAT")==0)
		{
		strcpy(buffer,W32LOGDIR);
		strcat(buffer,"AMD.PIF /c ");
		strcat(buffer,cmd);
		strcpy(cmd,buffer);
		}
	if(stricmp(ext,".CMD")==0)
		{
		strcpy(buffer,"CMD.EXE /c ");
		strcat(buffer,cmd);
		strcpy(cmd,buffer);
		}
	if(stricmp(ext,".UTL")==0)
		{
		sprintf(buffer,"%sAMDUTIL.EXE \"%s\"",W32LOGDIR,cmd);
		strcpy(cmd,buffer);
		}
	if(stricmp(ext,".FLS")==0)
		{
		sprintf(buffer,"%sAMDFLS.EXE \"%s\"",W32LOGDIR,cmd);
		strcpy(cmd,buffer);
		}
	if(stricmp(ext,".FSC")==0)
		{
		sprintf(buffer,"%sAMDSFILE.EXE \"%s\"",W32LOGDIR,cmd);
		strcpy(cmd,buffer);
		}
	if(stricmp(ext,".RSC")==0)
		{
		sprintf(buffer,"%sAMDSREG.EXE \"%s\"",W32LOGDIR,cmd);
		strcpy(cmd,buffer);
		}

	if(p && strlen(p)>0) {strcat(cmd," "); strcat(cmd,p);}
return;
}
//
// Debug Routine to dump the buffers to the Log
//
int DumpArea(char *data,int len)
{
	return 1;
	while(len >0)
		{
		sprintf(LogBuf,"%.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X %.2X%.2X%.2X%.2X",*(data),*(data+1),*(data+2),*(data+3),
		*(data+4),*(data+5),*(data+6),*(data+7),*(data+8),*(data+9),*(data+10),*(data+11),
		*(data+12),*(data+13),*(data+14),*(data+15));
		lmsg(MSG_DEBUG,LogBuf);
		data=data+16;
		len=len-16;
		}
return 1;
}
//
// Start a process and wait
//
int runcmd(char *cmd,PINFBLK ib)
{
	lmsg(MSG_IMFO,"Would have executed : [%s]",cmd);
return 0;
}
//
// Start a process and get out
//
int startcmd(char *cmd,PINFBLK ib)
{
	lmsg(MSG_INFO,"Exex and No Wait for [%s]",cmd);
return 0;
}
int CheckInternalCmd(char *cmd,PINFBLK ib)
{
UINT ShutFlg=128;
char cbuf[_MAX_PATH];

	strcpy(cbuf,W32LOGDIR); strcat(cbuf,"AMDSHUT.EXE ");
	if(stricmp(REBOOT,cmd)==0)   {strcat(cbuf,REBOOT); ShutFlg=0;   }
	if(stricmp(POWEROFF,cmd)==0) { strcat(cbuf,POWEROFF); ShutFlg=0;}
	if(ShutFlg==128) return 0;
	lmsg(MSG_INFO,"Shut down System");
	SendResponse(ib,999);											// Tell AMDSCHED we are in reboot
	SleepEx(10,FALSE);
	shutcmd(cbuf);
	lmsg(MSG_INFO,"Shut Command [%s]",cbuf);
	Running=0;														// Hey no more please
	shutdown(my_socket,2);											// Disable Send/Recv for socket
	closesocket(my_socket);											// Now Close it
return 999;
}
//
// Start a process and get out
//
int shutcmd(char *cmd)
{
	lmsg(MSG_INFO,"Shut Command [%s]",cmd);
return 0;
}
int GetRecord(PINFBLK ib)
{
	memset(ib->recbuf,0,REC_BUFF_SIZE);								// Setup for record
	ib->recbuf_len=0;												// Set length to 0
	while(!ib->recbuf_len)
		{
		if(!RecvTDH(ib)) {ib->term_session=1; return -1;}			// Get entry off line
		if(ib->tdhdesc==ENDSECTION) return 0;						// Are we at the end
		if(ib->recbuf_len==0)		continue;						// Null Record (Start Trans)
		}
return 1;
}
int SendResponse(PINFBLK ib,int rc)
{
	memset(ib->recbuf,0,16);										// Clean it out please
	sprintf(ib->recbuf,"%.3d",rc);									// Char Rc
	ib->recbuf_len=strlen(ib->recbuf);								// Set the length
	StartTrans(ib,(char)RSPNS,0,TRUE);								// Set the transaction type to Verfile
return 1;
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
PBUFENT pb;															// Buffer mapper

	memset(type,0,sizeof(type));									// Clear out Type Name
	memcpy(type,ib->recbuf,ib->recbuf_len);							// Save type away
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
			memcpy(l,pb->data,dlen);								// Copy in the Parms
			Resolve(l);
			}
		}
	if(rc<0) return 16;												// Bad return
	EnterCriticalSection(&DATAO);
	rc=SendData(ib,f,p);											// Send data to requestor
	LeaveCriticalSection(&DATAO);
	if(rc) SendResponse(ib,rc);										// If rc then an error has occurred
return 1;

return 0;
}
int ExeOnly(PINFBLK ib)
{
char type[36];
char f[_MAX_PATH+1];												// Full Name
char p[_MAX_PATH+1];												// Parms
char l[_MAX_PATH+1];												// Loction
char fn[_MAX_PATH];													// File name
char cmd[1024];														// Command to execute
char fext[_MAX_PATH];												// File Extention
int rc=0,BuiltFile=0,dlen=0;
PBUFENT pb;															// Buffer mapper

	memset(type,0,sizeof(type));									// Clear out Type Name
	memcpy(type,ib->recbuf,ib->recbuf_len);							// Save type away
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
			_splitpath(f,NULL,NULL,fn,fext);						// Break it Up
			strcpy(f,fn); strcat(f,fext);							// Build it up
			strcpy(fn,f);											// Both hold file name
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
	if(rc<0) return 16;												// Bad return
	if(strlen(l)>0) {strcpy(cmd,l); strcat(cmd,f);}					// Location supplied
	else strcpy(cmd,f);												// Build Buffer
	FormatCmd(cmd,fext,p);											// See if we need to rebuild
	lmsg(MSG_INFO,"Execute Command [%s]",cmd);						// Tell What we are doing
	rc=runcmd(cmd,ib);												// Run it Please												
	lmsg(MSG_INFO,"Exit    Command Routine RC[%d]",rc);				// Let'em Know
	return rc;
}

void WriteTransBuffer(PINFBLK ib,char *type,char *data,int len)
{
	ib->recbuf_len=len+1;								// Set the length
	ib->recbuf[0]=type[0];								// Set File Name Please
	memcpy(&ib->recbuf[1],data,len);					// Copy it in
return;
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
	lmsg(MSG_INFO,"Task[%d] Send for File Complete [%s]",ib->task,fn);
return 0;
}
//
// The routine Expects only Data to be passed to it
//
int DataOnly(PINFBLK ib)
{
FILE *out=NULL;
int count=0,dlen=0;
char type[36];
char f[_MAX_PATH+1];												// Full Name
char rn[_MAX_PATH+1];
char p[_MAX_PATH+1];												// Parms
char l[_MAX_PATH+1];												// Loction
char fn[_MAX_PATH];													// File name
char fext[_MAX_PATH];												// File Extention
int rc=0,BuiltFile=0;
int datasent=0;														// Was data sent??
PBUFENT pb;	
HANDLE fle;
SYSTEMTIME st;
FILETIME ft;
FILETIME lt;


	memset(type,0,36);												// Clear out Type Name
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
			strcpy(rn,f);											// Remeber remote name
			_splitpath(f,NULL,NULL,fn,fext);						// Break it Up
			strcpy(f,fn); strcat(f,fext);							// Build it up
			strcpy(fn,f);											// Both hold file name
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
	if(rc<0) return 16;												// Bad return
	if(!out && datasent) return 16;									// Very Bad in deed
	if (out) fclose(out);											// Close it Please
	if(strlen(p)>0) {
		fle=CreateFile(f,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
		sscanf(p,"%d %d %d %d %d %d",&st.wYear,&st.wMonth,&st.wDay,&st.wHour,&st.wMinute,&st.wSecond);
		SystemTimeToFileTime(&st,&ft);
		LocalFileTimeToFileTime(&ft,&lt);
		if(!SetFileTime(fle,&lt,&lt,&lt)) {
			lmsg(MSG_ERR,"Set File Date/Time Failed %d",GetLastError());
			}
		CloseHandle(fle);
		RememberData(&st,f,rn,p,"WRITE");
		}
	lmsg(MSG_INFO,"Data File Transfer Complete for [%s]",f);
return 0;
}
//
// Given a file name and path information Delete a file
//
int RemoveData(PINFBLK ib)
{
FILE *out=NULL;
int count=0,dlen=0;
char type[36];
char f[_MAX_PATH+1];												// Full Name
char l[_MAX_PATH+1];												// Loction
char p[_MAX_PATH+1];												// Parms
char fn[_MAX_PATH];													// File name
char fext[_MAX_PATH];												// File Extention
int rc=0,BuiltFile=0;
PBUFENT pb;	
SYSTEMTIME st;


	memset(type,0,36);												// Clear out Type Name
	memcpy(type,ib->recbuf,ib->recbuf_len);							// Save type away
	memset(f,0,_MAX_PATH+1);											// Clear it out
	memset(l,0,_MAX_PATH+1);
	memset(&st,0,sizeof(SYSTEMTIME));
	pb=(PBUFENT)ib->recbuf;											// point to it
	while((rc=GetRecord(ib))>0)
		{
		pb=(PBUFENT)ib->recbuf;											// point to it
		dlen=ib->recbuf_len-1;										// Save the data length
		if(pb->type=='F') {
			memcpy(f,pb->data,dlen);								// Copy in the File Name
			Resolve(f);
			_splitpath(f,NULL,NULL,fn,fext);						// Break it Up
			strcpy(f,fn); strcat(f,fext);							// Build it up
			strcpy(fn,f);											// Both hold file name
			}
		if(pb->type=='P') {
			memcpy(p,pb->data,dlen);								// Copy in the Parms
			Resolve(p);
			}
		if(pb->type=='L') {
			memcpy(l,pb->data,dlen);								// Copy in the Location
			Resolve(l);
			}
		}
	SetUpFileLocation(fn,l,f);									// Build the complete file
	SetFileAttributes(f,(DWORD)0);								// Remove attributes
	if(stricmp(p,"STGD")==0) {
		MoveFileEx((LPCTSTR)f,(LPCTSTR)NULL,MOVEFILE_DELAY_UNTIL_REBOOT);
		lmsg(MSG_INFO,"Data File Deleted Delayed for [%s]",f);
		RememberData(&st,f,"N/A","N/A","STAGE_DELETE");
		return 0;
		}
	if(DeleteFile(f))												// Remove File
		lmsg(MSG_INFO,"Data File Deleted [%s]",f);
	else lmsg(MSG_ERR,"Delete Failed for [%s]",f);
	RememberData(&st,f,"N/A","N/A","DELETE");

return 0;
}
//
// Given a path remove all the files in it
//
int RemoveD(PINFBLK ib)
{
FILE *out=NULL;
int count=0,rc=0,dlen=0;
char type[36];
char f[_MAX_PATH+1];
char ndrive[_MAX_PATH+1];												// Full Name
char BuildFile[_MAX_PATH+1];
PBUFENT pb;	
HANDLE fhan=0;														// Handle from find first
WIN32_FIND_DATA fdata;

	memset(type,0,36);												// Clear out Type Name
	memcpy(type,ib->recbuf,ib->recbuf_len);							// Save type away
	memset(f,0,_MAX_PATH+1);											// Clear it out
	pb=(PBUFENT)ib->recbuf;											// point to it
	while((rc=GetRecord(ib))>0)
		{
		pb=(PBUFENT)ib->recbuf;										// point to it
		dlen=ib->recbuf_len-1;										// Save the data length
		if(pb->type=='F') {
			memcpy(f,pb->data,dlen);								// Copy in the File Name
			Resolve(f);
			if(f[strlen(f)-1]!='\\') strcat(f,"\\");
			}
		}
	lmsg(MSG_INFO,"Remove File in [%s]",f);
	strcpy(ndrive,f); strcat(ndrive,"*.*");
	if(!(fhan=FindFirstFile(ndrive,&fdata))) return 8;					// Get the handle
	if(fhan==INVALID_HANDLE_VALUE) return 8;
	while(1)
		{
		if(fdata.cFileName[0]=='.') {if(!FindNextFile(fhan,&fdata)) break; continue;}
		if(!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
			{
			sprintf(BuildFile,"%s%s",f,fdata.cFileName);
			DeleteFile(BuildFile);
			count++;
			}
		if(!FindNextFile(fhan,&fdata)) break;
		}
	lmsg(MSG_INFO,"Removed [%d] Files",count);
FindClose(fhan);
return 0;
}

int GetAMDValue(char *name)
{
int x=1;
DWORD dwt=0,dwl=_MAX_PATH;
HKEY hkey;
long rc=0;
char buf[_MAX_PATH+1];

	if((rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_KEY,0,KEY_READ,&hkey))!=ERROR_SUCCESS) 
		{
		strcpy(name,"");
		return 0;
		}
	else 
		{
		strcpy(buf,name); name[0]='\0';
		RegQueryValueEx(hkey,buf,NULL,&dwt,name,&dwl);
		RegCloseKey(hkey);
		}
return 1;
}
//
// Receive the information and Start a process ONLY
//
int StrtOnly(PINFBLK ib)
{
char type[36];
char f[_MAX_PATH+1];												// Full Name
char p[_MAX_PATH+1];												// Parms
char l[_MAX_PATH+1];												// Loction
char fn[_MAX_PATH];													// File name
char cmd[1024];														// Command to execute
char fext[_MAX_PATH];												// File Extention
int rc=0,BuiltFile=0,dlen=0;
PBUFENT pb;															// Buffer mapper

	memset(type,0,sizeof(type));									// Clear out Type Name
	memcpy(type,ib->recbuf,ib->recbuf_len);							// Save type away
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
			_splitpath(f,NULL,NULL,fn,fext);						// Break it Up
			strcpy(f,fn); strcat(f,fext);							// Build it up
			strcpy(fn,f);											// Both hold file name
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
	if(rc<0) return 16;												// Bad return
	if(strlen(l)>0) {strcpy(cmd,l); strcat(cmd,f);}					// Location supplied
	else strcpy(cmd,f);												// Build Buffer
	FormatCmd(cmd,fext,p);											// See if we need to rebuild
	lmsg(MSG_INFO,"Execute Command [%s]",cmd);						// Tell What we are doing
	rc=startcmd(cmd,ib);												// Run it Please												
	lmsg(MSG_INFO,"Exit    Command Routine RC[%d]",rc);				// Let'em Know
	return rc;
}
//
// The routine Expects only Data to be passed to it
//
int LockData(PINFBLK ib)
{
FILE *out=NULL;
int count=0,dlen=0;
char type[36];
char f[_MAX_PATH+1];												// Full Name
char rn[_MAX_PATH+1];
char p[_MAX_PATH+1];												// Parms
char l[_MAX_PATH+1];												// Loction
char fn[_MAX_PATH];													// File name
char fext[_MAX_PATH];												// File Extention
char TempF[1024];
int rc=0,BuiltFile=0;
int datasent=0;														// Was data sent??
PBUFENT pb;	
HANDLE fle;
SYSTEMTIME st;
FILETIME ft;
FILETIME lt;


	memset(type,0,36);												// Clear out Type Name
	memcpy(type,ib->recbuf,ib->recbuf_len);							// Save type away
	memset(f,0,_MAX_PATH+1);										// Clear it out
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
			strcpy(rn,f);											// Save real name
			_splitpath(f,NULL,NULL,fn,fext);						// Break it Up
			strcpy(f,fn); strcat(f,fext);							// Build it up
			strcpy(fn,f);											// Both hold file name
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
			if(!out) {
				SetUpFileLocation(fn,l,f);							// Build the real File name
				BuildTempFile(f,TempF);								// Build a Temp File of the same drive
				out=fopen(TempF,"wb");
				} 
			BuiltFile=1; datasent=1;								// tell them we did it
			if(out)	{fwrite(pb->data,(size_t)dlen,(size_t)1,out); count++;}			// Write it out
			}
		}
	if(rc<0) {lmsg(MSG_ERR,"Bad Return From COMM RC=16"); return 16;}						// Bad return
	if(!out && datasent) {lmsg(MSG_ERR,"Bad DataSet Name R[%s] T[%s]",f,TempF); return 8;}	// Very Bad in deed
	if (out) fclose(out);											// Close it Please
	if(strlen(p)>0) {
		fle=CreateFile(TempF,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
		sscanf(p,"%d %d %d %d %d %d",&st.wYear,&st.wMonth,&st.wDay,&st.wHour,&st.wMinute,&st.wSecond);
		SystemTimeToFileTime(&st,&ft);
		LocalFileTimeToFileTime(&ft,&lt);
		if(!SetFileTime(fle,&lt,&lt,&lt)) {
			lmsg(MSG_ERR,"Set File Date/Time Failed %d",GetLastError());
			}
		CloseHandle(fle);
		RememberData(&st,f,rn,p,"STAGED WRITE");
		}
	if(!MoveFileEx(TempF,f,MOVEFILE_DELAY_UNTIL_REBOOT)) lmsg(MSG_ERR,"MoveFileEx Rc [%d]",GetLastError());
	lmsg(MSG_INFO,"Temp File [%s] Built",TempF);
	lmsg(MSG_INFO,"Data File [%s] Will be moved on Next Reboot",f);
return 0;
}
//
// Used with MoveFileEX to build a temp file on the same drive as the Real one
//
int BuildTempFile(char *real,char *temp)
{
char pname[1024];											// Path Information
char hold[_MAX_PATH+1];										// Your Basic Hold Area
char *c,*s;													// Your Basic Work Pointers

	_splitpath(real,hold,NULL,NULL,NULL);					// Break it Up
	strcpy(pname,hold);										// Start it off please
	strcat(pname,"\\AMDSTAGE\\");							// Build a stage area on that drive
	if( (_access( pname, 0 )) == -1 ) {						// directory not there 
		hold[0]='\0';
		c=strstr(pname,"\\"); s=pname; c++;					// set pointer
		while(c) {
			c=strstr(c,"\\");								// Find it please
			if(!c) break;									// At the end now
			*(c)='\0';										// Null it out
			strcat(hold,s);									// copy in first part
			mkdir(hold);									// create it please
			lmsg(MSG_INFO,"Directory Build For [%s]",hold);
			strcat(hold,"\\");								// Setup for next
			c=c+1; s=c;										// Reset pointers
			}												// END of While
		}													// End of IF ACCESS
	GetTempFileName(pname,"AMD",0,temp);					// Create Temp File Name
return 1;
}
int RememberData(SYSTEMTIME *st,char *f,char *rn,char *p,char *action)
{
char crc[9]="00000000";
char buffer[4096];
FILE *o;
char dfile[_MAX_PATH+1];
int fle=0;
WIN32_FIND_DATA fdata;								// Data from find file
HANDLE fh;											// Handle for call
long size;
SYSTEMTIME tme;
char currdate[16];

	GetLocalTime(&tme);
	sprintf(currdate,"%.4d/%.2d/%.2d %.2d:%.2d:%.2d",tme.wYear,tme.wMonth,tme.wDay,tme.wHour,tme.wMinute,tme.wSecond);
	GenCrc(rn,(int)strlen(rn),crc);
	sprintf(dfile,"%sAMDTRAN.CSV",W32LOGDIR);
	sprintf(buffer,"KEY;Date Received;File Name;Server Name;Last Write Date/Time;File Size;Update Type\n");
	if(!(fh=FindFirstFile(f,&fdata))) return 0;
	size=fdata.nFileSizeHigh+fdata.nFileSizeLow;
	FindClose(fh);
	EnterCriticalSection(&DATAO);
	if( (_access( dfile, 0 )) != -1 ) fle=1;
	o=fopen(dfile,"a+");
	if(!o) {LeaveCriticalSection(&DATAO); return 0;}
	if(!fle) fwrite(buffer,strlen(buffer),1,o);
	sprintf(buffer,"%s%.4d%.2d%.2d%.2d%.2d%.2d;%s;%s;%s;%s;%d;%s\n",crc,(9999-st->wYear),(99-st->wMonth),(99-st->wDay),(99-st->wHour),(99-st->wMinute),(99-st->wSecond),currdate,f,rn,p,size,action);
	fwrite(buffer,strlen(buffer),1,o);	
	fclose(o);
	LeaveCriticalSection(&DATAO);
return 1;
}

void AppendDefault(void)
{
FILE *o;
FILE *i;
char dfile[_MAX_PATH+1];
char ofile[_MAX_PATH+1];
char buffer[4096];
int exist=0;

	sprintf(dfile,"%sAMDTRAN.CSV",W32LOGDIR);
	sprintf(ofile,"%sData\\DEFAULT.FLS",W32LOGDIR);
	if( (_access( dfile, 0 )) == -1 ) return ;				// No AMDTRAN.CSV to Copy over
	if( (_access( ofile, 0 )) != -1 ) exist=1;				// Do not copy over th header
	EnterCriticalSection(&DATAO);
	o=fopen(ofile,"a+");
	i=fopen(dfile,"r");
	if(!o || !i) {LeaveCriticalSection(&DATAO); return;}
	fgets(buffer,sizeof(buffer),i);							// Read in the header
	if(!exist) fwrite(buffer,strlen(buffer),1,o);			// write out header if not exists
	while(fgets(buffer,sizeof(buffer),i)) {
		fwrite(buffer,strlen(buffer),1,o);
		}
	fclose(o); fclose(i);
	DeleteFile(dfile);										// Clean it out
	LeaveCriticalSection(&DATAO);
return;
}
//
// Dir List Pre Process
//
int BuildDList(PINFBLK ib,char *loc)
{
return 1;
}
//
// Load up requested directory items
//
void LoadDirItems(PINFBLK ib, char *path)
{
return;
}
