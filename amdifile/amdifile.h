/***************************************************************/
/**   Include file for the Configuration Verification Server  **/
/***************************************************************/
/***********************************************
Server Definition Block
************************************************/
typedef struct ServBlk
{
unsigned short port;
unsigned short storagefail;					// number of getmain failures
} SB,*PSB;

void    InitSB(PSB sb);						// Init the server block
int		DispTrans(PINFBLK ib);				// Dispatch the correct transaction
int		ExecCmd(PINFBLK ib);				// Execute The Command Please
int		TransEx(PINFBLK ib); 
int		DumpArea(char *data,int len);
void	FormatCmd(char *cmd,char *ext,char *p);
int		InitTrans(PINFBLK ib);
int		DumpArea(char *data,int len);
int		SendResponse(PINFBLK ib,int rc);
int		SendExec(PINFBLK ib,char *type,char *fn,char *p,char *l);
int		ReqData(PINFBLK ib,char *type,char *fn,char *p,char *l);
int		GetFileData(PINFBLK ib);
int		GetRecord(PINFBLK ib);
int		TransData(PINFBLK ib,char *type,char *fn,char *p,char *l);
int		GetAMDValue(char *name);
int		StartExec(PINFBLK ib,char *type,char *fn,char *p,char *l);
int		HoldData(PINFBLK ib,char *type,char *fn,char *p,char *l);
int		GetAMDValue(char *name);







