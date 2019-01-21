#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "W32trace.h"
#include "crc.h"
#include "nettcp.h"
#include "blowfish.h"

unsigned long initkey[2] = {0x3e4f9a6bL, 0x77073096L};
blf_ctx ctx;				// Our Main ctx for Blowfish
blf_ctx sctx;				// Send Data context
blf_ctx dctx;													// Buffer CTX
static char ourkey[8];		// our key to place in the BDH
static int KeySet=0;		// We are using encryption
//
// Get storage for Info block
//
int InitIB(PINFBLK ib)
{
	if(!ib) return 0;									// Oh Nooooo this is bad...
	memset(ib,0,sizeof(struct InfoBlk));				// clear it out
	ib->srbuf=malloc(SR_BUFF_SIZE);						// Allocate input buffer
	if(!ib->srbuf) {FreeIB(ib); return 0;}				// Noooo free all and go.
	ib->recbuf=malloc(REC_BUFF_SIZE);					// get a recrod buffer
	if(!ib->recbuf) {FreeIB(ib); return 0;}				// Failure get out
        InitializeEnc(&ctx, (unsigned char *) initkey, ENCKEYLEN);
return 1;
}
void FreeIB(PINFBLK ib)
{
	if(ib)
		{
		if(ib->srbuf) free(ib->srbuf);					// Line buffer
		if(ib->recbuf) free(ib->recbuf);				// Record Buffer
		free(ib);										// Client Block
		ib=NULL;										// reset block
		}
return;
}
//
// set our send key please
//
void SetSessionKey(char *key)
{
	memcpy(ourkey,key,ENCKEYLEN);						// Save the key we were given
	enc(&ctx,(unsigned long *)ourkey,1);				// Encrypt it please
	InitializeEnc(&sctx, (unsigned char *) key,8);		// Session Key for sending 
	KeySet=1;											// Set we are doing encytption
}
//
// Recv the next transaction
//
int	RecvTDH(PINFBLK ib)
{
PTDH t;													// Pointer to TDH
char *c;												// pointer to buffer

	if(!ib->srbuf_remain) {if(!RecvBDH(ib)) return 0;}	// Set the transaction 
	(char*)t=ib->srbuf+ib->srbuf_read;					// set adress of next trans
	c=(char *)t+sizeof(struct tdh);						// set to data
	ib->tdhtype=t->tdhtype;								// Set the type
	ib->tdhdesc=t->tdhdesc;								// Set the description
	ib->tdhfunc=t->tdhfunc;								// Set function
	ib->tdhres=t->tdhres;								// set the reserved

	memset(ib->workbuf,0,TDHLEN+1);						// Clear out buffer
	memcpy(ib->workbuf,t->tdhlen,TDHLEN);				// Copy to null term buffer
	ib->recbuf_len=atoi(ib->workbuf);					// Get length of this record
	memcpy(ib->recbuf,c,ib->recbuf_len);				// Copy in the data
	ib->srbuf_read=ib->srbuf_read+ib->recbuf_len+sizeof(struct tdh);// Increment
	ib->srbuf_remain=ib->srbuf_remain-(ib->recbuf_len+sizeof(struct tdh));
	lmsg(MSG_DEBUG,"Read TDH Complete remain[%d],RecLen[%d]",(void*)ib->srbuf_remain,(void *)ib->recbuf_len);

return 1;
}
//
// Get next block in
//
int	RecvTcp(PINFBLK ib)
{
char *c;
int len=0;														// Length of read
int elen=0;														// encryption length
PBDH b;															// Define block header
int blen=0;														// Length for this block
div_t result;
char holdkey[9];												// for number of 8 byte segments

	c=ib->srbuf;
	len=recv(ib->sock,c,sizeof(struct bdh),0);					// Get BDH off input queue
	if(len<=0) return 0;										// Session may be gone
	ib->srbuf_ilen=len;											// reset ilen
	(char *)b=ib->srbuf;										// Point to it
	memset(ib->workbuf,0,BDHLEN+1);								// Clear out buffer
	memcpy(ib->workbuf,b->bdhtln,BDHLEN);						// Copy to null term buffer
	blen=atoi(ib->workbuf);										// Get Length of this message
	elen=blen;													// save this value
	blen-=len;													// Next portion please
	while( (blen) && (len!=0) )									// See if data present
		{
		lmsg(MSG_DEBUG,"Recv loop length[%d]",len);				// Send out the message
		c=ib->srbuf+ib->srbuf_ilen;								// poition at next
		len=recv(ib->sock,c,blen,0);							// Get off input queue
		blen-=len;												// Next portion please
		ib->srbuf_ilen+=len;
		}
	memset(ib->workbuf,0,BDHLEN+1);								// Clear out buffer
	memcpy(ib->workbuf,b->bdhlen,BDHLEN);						// Copy to null term buffer
	ib->srbuf_ilen=atoi(ib->workbuf);    					// Get data Length of this message
	if(b->bdhenc[0]!= '\0') {
                memset(holdkey,0,sizeof(holdkey));
		elen=elen-sizeof(struct bdh);							// round to nearest 8
		(char *)c=(char *)b+sizeof(struct bdh);
 		result=div(elen,8);										// divide it please
		dec(&ctx,(unsigned long *)b->bdhkey,1);
                memcpy(holdkey,b->bdhkey,8);
		InitializeEnc(&dctx, (unsigned char *)holdkey,ENCKEYLEN);
		dec(&dctx,(unsigned long *)c,(int)result.quot);// Decrypt the data
 		}

return ib->srbuf_ilen;
}

//
// Get next block in
//
int	RecvBDH(PINFBLK ib)
{
PBDH b;													// Define block header

	memset(ib->srbuf,0,SR_BUFF_SIZE);					// Clear ou the bufer
	if(!RecvTcp(ib)) return 0;
	lmsg(MSG_DEBUG,"Recv complete with total length[%d]",(void *)ib->srbuf_ilen); 
	// DumpArea(ib->srbuf,ib->srbuf_ilen);
	if(ib->srbuf_ilen==0 ||								// Check for errors 
	   ib->srbuf_ilen==SOCKET_ERROR) {ib->error=BADBLK; return 0;}
	(char *)b=ib->srbuf;										// Point to it
	if(b->bdhcrc[0]>' ')								// If CRC provided
	  {
	  GenCrc((char *)b+BDHSIZE,ib->srbuf_ilen-BDHSIZE,ib->bdhcrc);// See if data is the same
	  if(strncasecmp(ib->bdhcrc,b->bdhcrc,8)!=0)
		{
		ib->error=BADBDH;
		lmsg(MSG_ERR,"Invalid CRC for block I:[%.8s] O:[%.8s]",(void*)b->bdhcrc,(void *)ib->bdhcrc);
		return 0;
		}
	  }
	ib->srbuf_remain=ib->srbuf_ilen-sizeof(struct bdh);	// Data that remains
	ib->srbuf_read=sizeof(struct bdh);					// How Many bytes read in

	memset(ib->workbuf,0,MSGLEN+1);						// Clear out buffer
	memcpy(ib->workbuf,b->bdhmsg,MSGLEN);				// Copy to null term buffer
	ib->msgnum=atoi(ib->workbuf);						// Set mesage number
	ib->srbuf_slen=0;									// reset send length
	lmsg(MSG_DEBUG,"Block num[%d] Len[%d] CRC[%.8s] read complete",(void*)ib->msgnum,(void *)ib->srbuf_ilen,ib->bdhcrc);
	return 1;
}
//
// Send data on the line
//
int	SendTrans(PINFBLK ib)
{
PBDH b;	
int rc,len;
div_t result;													// for number of 8 byte segments
char *c;

														// Define block header
	if(ib->srbuf_slen<1) 
		{
		lmsg(MSG_ERR,"Send Transaction called with no data in buffer len[%d]",(void*)ib->srbuf_slen);
		return 1;												// No data to send
		}
	(char *)b=ib->srbuf;										// Point to it
	(char *)c=(char *)b+sizeof(struct bdh);						// Point past BDH - first TDH
	sprintf(b->bdhlen,"%.5d",ib->srbuf_slen);					// Put in the length
	sprintf(b->bdhtln,"%.5d",ib->srbuf_slen);					// Put in the total length

	len=ib->srbuf_slen;											// save away this length
	result=div(len,8);											// Get the # of 8 byte segments
	if(result.rem>0) {
		len=len+7;												// Not on boundry redo
		result=div(len,8);										// Get the # of 8 byte segments
		len=result.quot * 8;									// calc the size
		sprintf(b->bdhtln,"%.5d",len);							// Put in the total length
		}

    GenCrc((char *)b+CRCLEN,ib->srbuf_slen-CRCLEN,b->bdhcrc);	// Punt in the CRC Value

	if(KeySet) {												// We Are doing Encryption
		memcpy(b->bdhkey,ourkey,ENCKEYLEN);						// Copy into BDH
		b->bdhenc[0]='1';										// Tell other side to check key
		result.quot-=BDHOFFSET;									// remove BDH length
		enc(&sctx,(unsigned long *)c,result.quot);				// Encrypt
		}

	rc=1;
	//rc=setsockopt(ib->sock,IPPROTO_TCP,TCP_NODELAY,(const char*)&rc,sizeof(int));
	rc=send(ib->sock,ib->srbuf,ib->srbuf_slen,0);
	if(rc<=0) {
		ib->error=8;
		lmsg(MSG_DEBUG,"Send completed with RC[%d] Length[%d]",(void*)ib->error,(void*)ib->srbuf_slen);
		sprintf(ib->recbuf,"%d",ib->error);
		return 0;
		}
	ib->srbuf_slen=0;											// Reset send length
	ib->srbuf_remain=0;											// Reset the length
	return 1;
}
//
// Start Trans always starts the buffer so we know where the tdh lives and  leave
// room for the bdh
//
int StartTrans(PINFBLK ib,char type,char func,int data)
{
PTDH t;
char *c;

	memset(ib->srbuf,0,SR_BUFF_SIZE);						// Clear out send buffer
	ib->srbuf_slen=sizeof(struct bdh)+sizeof(struct tdh);	// Clear out send size
	(char *)t=(ib->srbuf+sizeof(struct bdh));				// Set tdh position
	c=(char *)t+sizeof(struct tdh);							// Set where data goes
	t->tdhtype=type;										// Set the type
	ib->tdhtype=type;										// Save in IB
	t->tdhdesc=STARTSECTION;								// Set the description
	t->tdhfunc=func;										// Set the function
	ib->tdhfunc=func;										// Save in IB
	if(data) 
		{
		sprintf(t->tdhlen,"%.4d",ib->recbuf_len);			// Set the data length with the 
		memcpy(c,ib->recbuf,ib->recbuf_len);		 		// move data into bufer
		ib->srbuf_slen+=ib->recbuf_len;						// Next position in buffer
		}
	if(!SendTrans(ib)) return 0;
return 1;
}
//
// Start Trans always starts the buffer so we know where the tdh lives and leave
// room for the bdh
//
int InitTrans(PINFBLK ib)
{

	memset(ib->srbuf,0,SR_BUFF_SIZE);						// Clear out send buffer
	ib->srbuf_slen=sizeof(struct bdh);						// Clear out send size
return 1;
}

//
// Send data for Started transaction
//
int SendDataOnTrans(PINFBLK ib)
{
PTDH t;
char *c;
			
	if(ib->tdhtype==UNKN)
		{
		WriteLog("ERROR: SendDataOnTrans Called Without StartTran");
		return 0;
		}
    if( (ib->recbuf_len+ib->srbuf_slen+sizeof(struct tdh)) > SR_BUFF_SIZE) // No More room
		{
		SendTrans(ib);											// Send out the data
		memset(ib->srbuf,0,SR_BUFF_SIZE);						// Clear out send buffer
		ib->srbuf_slen=sizeof(struct bdh)+sizeof(struct tdh);	// Clear out send size
		(char *)t=(ib->srbuf+sizeof(struct bdh));				// Set tdh position
		}
	else (char *)t=(ib->srbuf+ib->srbuf_slen);					// Set tdh position
	c=(char *)t+sizeof(struct tdh);								// Set where data goes
	t->tdhtype=ib->tdhtype;										// Set the type
	t->tdhdesc=DATAONLY;										// Set the description
	t->tdhfunc=ib->tdhfunc;										// Set the function
	sprintf(t->tdhlen,"%.4d",ib->recbuf_len);					// Set the data length with the
	memcpy(c,ib->recbuf,ib->recbuf_len);		 				// move data into bufer
	ib->srbuf_slen+=ib->recbuf_len+sizeof(struct tdh);			// Next position in 
return 1;
}
//
// Signal No more data for transaction
//
int StopDataOnTrans(PINFBLK ib)
{
PTDH t;

    if( (ib->recbuf_len+sizeof(struct tdh)) > SR_BUFF_SIZE)		// No More room
		{
		SendTrans(ib);											// Send out the data
		memset(ib->srbuf,0,SR_BUFF_SIZE);						// Clear out send buffer
		ib->srbuf_slen=sizeof(struct bdh)+sizeof(struct tdh);	// Clear out send size
		(char *)t=(ib->srbuf+sizeof(struct bdh));				// Set tdh position
		}
	else (char *)t=(ib->srbuf+ib->srbuf_slen);					// Set tdh position
	t->tdhtype=ib->tdhtype;										// Set the type
	t->tdhdesc=ENDSECTION;										// Set the description
	t->tdhfunc=ib->tdhfunc;										// Set the function
	sprintf(t->tdhlen,"%.4s","0000");							// Set to 0
	ib->srbuf_slen+=sizeof(struct tdh);							// Next position in buffer
	ib->tdhtype=UNKN;											// Reset
	ib->tdhdesc=UNKN;											// the
	ib->tdhfunc=UNKN;											// Transaction
	
return 1;
}

