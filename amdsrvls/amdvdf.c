#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slst.h"
#include "W32trace.h"
#include "schglobal.h"
#define MAXVBASE		40
#define DATABUFSIZE		8192
#define MAXVNAME		(_MAX_PATH*2)+1	
#define MAXBASE			_MAX_PATH+1
#define MAXVAR			_MAX_PATH+1
#define MAXLINE			1024
int LoadVbase(char *name);
int setvalue(char *base,char *var,char *value);
int SaveVbase(char *name);
int FindVbase(PSLSTBASE vbase,char *name);
int Resolve(char *data);
void splitvar(char *full,char *base,char *var);
void findvalue(char *full,char *base,char *var);
void getvalue(PSLSTBASE base,char *var,char *value);
char *GetPathInfo(char *name);
SLSTBASE Vbase[MAXVBASE];							// Set of 40 variable bases
int		VbaseNext=0;								// Next Open Vbase
char	fname[_MAX_FNAME+1];						// File name holder
char	fext[_MAX_EXT+1];							// File name ext
char	fdrive[_MAX_DRIVE+1];						// File name Drive   
char	fdir[_MAX_DIR+1];							// File directory
//
// Loads up a VDF File into a link list
//
int LoadVbase(char *name)
{
char *buffer;
char fullname[_MAX_PATH+1];
PSLST lent;															// List entry
int tb;																// pointer to This base
char *c;															// work char pointer
int base=-1;
FILE *in;															// Fiule cb for read

        //_splitpath(name,fdrive,fdir,fname,fext);							// Use Global vars for split
        strcpy(fname,name);
	base=FindVbase(&Vbase[0],fname);								// Dose it exits??
	if(base>=0) return 1;											// Yes do not reload
	buffer=malloc(DATABUFSIZE);										// Allocate the Read buffer
	if(!buffer) {lmsg(MSG_ERR,"Unable to allocate buffer of variable read");return 0;}
	tb=VbaseNext++;													// Save the next base entry
	InitLstBase(&Vbase[tb],fname);									// Init the Var Base entry
	if(!fext[0]) strcpy(fext,".vdf");								// Variable Def File Extention
	if(!fdrive[0])	strcpy(fullname,GetPathInfo("VARIABLES"));		// Get the path for variable files
	strcat(fullname,fname); strcat(fullname,fext);					// Build the rest
	lmsg(MSG_DEBUG,"Open for VDF full file name is [%s]",fullname); // put to log
	in=fopen(fullname,"r");											// Open it up
	if(!in) {lmsg(MSG_ERR,"Unable to open VDF [%.100s]",fullname); return 0;}
	while(fgets(buffer,DATABUFSIZE,in))								// Start to read it in
		{
		trim(buffer);												// Get rid of blanks
		c=strstr(buffer,"=");										// find the =
		*(c)=0; c++;												// Put in a NULL and pass it up
		lmsg(MSG_DEBUG,"Var/Data is V[%s] D[%s]",buffer,c);			// Put to Log
		lent=AllocEnt();											// Get new entry
		if(!lent) {lmsg(MSG_ERR,"Unable to allocate Block for Var"); return 0;}													
		lent->entry=malloc(strlen(buffer)+1);						// Get into var name
		lent->entry2=malloc(strlen(c)+1);							// Get for data 
		strcpy(lent->entry,buffer);									// Copy in var
		strcpy(lent->entry2,c);										// Copy in data
		AddSlst(&Vbase[tb],lent);									// Add to list
		}
if(buffer) free(buffer);											// Free work area
if(in) fclose(in);													// Close file
return 1;	
}
//
// Set a Variable to a VDF In storage List
//
int setvalue(char *base,char *var,char *value)
{
int offset=-1;
PSLST work;																// List entry
char hold[MAXVNAME]="";													// Hold area for both names
char lbase[MAXBASE]="BASE";												// Default base

	if(base && base[0]!='\0') strcpy(lbase,base);						// Use default or one provided
	offset=FindVbase(&Vbase[0],lbase);									// Dose it exits??
	if(offset<0) return 0;												// No Must load it first
	work=Vbase[offset].first;											// Get first entry
	while(work)
		{
		if(strcasecmp(work->entry,var)==0)									// We found the Var 
			{
			free(work->entry2);											// Get rid of Old data
			work->entry2=malloc(strlen(value)+1);						// Alloc for data 
			strcpy(work->entry2,value);									// Copy for data
			return 1;
			}				
		work=work->next;												// Next entry
		}
	// Not found above so just add it in As we go
	work=AllocEnt();													// Get new entry
	if(!work) {lmsg(MSG_ERR,"Unable to allocate Block for Var"); return 0;}													
	work->entry=malloc(strlen(var)+1);									// Get into var name
	work->entry2=malloc(strlen(value)+1);								// Get for data 
	strcpy(work->entry,var);											// Copy in var
	strcpy(work->entry2,value);											// Copy in data
	AddSlst(&Vbase[offset],work);										// Add to list
return 1;
}
//
// Save a VDF File out to disk
//
int SaveVbase(char *name)
{
char *buffer;
FILE *in;
char fullname[_MAX_PATH+1];
PSLST work;															// List entry
int base=-1;

        strcpy(fname,name);
	base=FindVbase(&Vbase[0],fname);								// Dose it exits??
	if(base<0) return 1;											// Not Loaded
	buffer=malloc(DATABUFSIZE);										// Allocate the Read buffer
	if(!buffer) {lmsg(MSG_ERR,"Unable to allocate buffer of variable read");return 0;}
	if(!fext[0]) strcpy(fext,".vdf");								// Variable Def File Extention
	if(!fdrive[0])	strcpy(fullname,GetPathInfo("VARIABLES"));		// Get the path for variable files
	strcat(fullname,fname); strcat(fullname,fext);					// Build the rest
	lmsg(MSG_DEBUG,"Open for VDF full file name is [%s]",fullname); // put to log
	in=fopen(fullname,"w");											// Open it up
	if(!in) {lmsg(MSG_ERR,"Unable to open VDF [%.100s]",fullname); return 0;}
	work=Vbase[base].first;											// Get first entry
	while(work)
		{
		strcpy(buffer,work->entry);									// Load in Variable name
		strcat(buffer,"=");											// Equal Sign
		strcat(buffer,work->entry2);								// Load up data
		strcat(buffer,"\n");										// Put in New Line
		fputs(buffer,in);											// Write it back out
		work=work->next;											// Next entry please
		}
if(buffer) free(buffer);											// Free it up
if(in) fclose(in);													// Close file
return 1;	
}
int FindVbase(PSLSTBASE vbase,char *name)
{
int x=0;
	for(x=0; x<VbaseNext; x++)
		{
		if(strcasecmp(Vbase[x].name,name)==0) return x;
		}
return -1;
}
int Resolve(char *data)
{
char *c,*c2;														// Pointers for Vars
int len;															// Lenth of name
char bname[MAXBASE];												// Base Name
char vname[MAXVAR];													// Variable Name
char hold[MAXVNAME];												// Hold area for both names
char line[MAXLINE];													// Hold area for line

	if((c=strstr(data,"<"))==NULL) return 1;						// Nothing to do
	while(c)
		{
		memset(hold,0,sizeof(hold));								// Clear it out
		memset(vname,0,sizeof(vname));								// Clear it out
		memset(bname,0,sizeof(bname));								// Clear it out
		memset(line,0,sizeof(line));								// Clear it out
		c2=strstr(data,">");										// Find the back end
		if(!c2) return 1;											// Not a var
		len=c2-(c+1);												// Get the length
		strncpy(hold,(c+1),len);									// Put name in Hold area
		splitvar(hold,bname,vname);									// Separate Var from base
		findvalue(hold,bname,vname);								// Go get value ino hold
		strncpy(line,data,(c-data));								// Copy first part
		strcat(line,hold);											// Now the value
		strcat(line,++c2);											// Copy over rest
		strcpy(data,line);											// put back into data
		c=strstr(data,"<");											// Next up please
		}
return 0;
}
void splitvar(char *full,char *base,char *var)						
{
char *c;

	c=strstr(full,".");												// Find a Dot ??
	if(!c) {base[0]=0; strcpy(var,full); return; }					// Only a var name
	*(c++)=0;														// stomp on it
	strcpy(base,full);												// Copy in base
	strcpy(var,c);													// Variable name
return;
}
void findvalue(char *full,char *base,char *var)						
{
int basenum=-1;
int copylen=MAXVNAME;												// default copy length

	memset(full,0,MAXVNAME);										// Clear it out
	if(strlen(base)>0)												// We were given a base
		{
		LoadVbase("BASE");											// Load it if needed
		basenum=FindVbase(&Vbase[0],base);							// Set the base
		if(basenum<0) {												// Cant set it
			LoadVbase(base);
			basenum=FindVbase(&Vbase[0],base);
			if(basenum<0) {lmsg(MSG_INFO,"Unable to load [%s]",base); return;}
			}
		getvalue(&Vbase[basenum],var,full);							// Set value for this guy
		return;
		}
	for(basenum=0; basenum<VbaseNext; basenum++)
		{
		getvalue(&Vbase[basenum],var,full);							// Try this guy
		if(full[0]>=' ') return;									// found one
		}
return;
}
void getvalue(PSLSTBASE base,char *var,char *value)
{
PSLST work=NULL;														// Work entry
char *v,*d;

	work=base->first;													// Get first entry
	while(work)
		{
		v=work->entry; d=work->entry2;									// Set up var and data pointers
		if(strcasecmp(v,var)==0) {strcpy(value,d); return;}				// If varthe copy data
		work=work->next;												// Next entry
		}
return;
}
