/***************************************************************/
/**   Include file for the Configuration Verification Server  **/
/**********************************************
Internal Commands
***********************************************/
#define REBOOT			"REBOOT"
#define POWEROFF		"POWEROFF"
#define LOGOFF			"LOGOFF"
/***********************************************
Define the routine code here
************************************************/
#define MAIN			01
#define	INITIB			10
#define FREEIB			20
#define STOPCLIENT		30
#define INITSB			40
#define RECVTDH			50
#define RECVBDH			60
#define STARTTRANS		70
/***********************************************
Define Description Codes Here
************************************************/
#define DIRDATA			0x50			// Directory type
#define FILEDESC		0x51			// File description
#define FILEDATA		0x52			// File Data
#define REGENTRY		0x53			// Registry entry
#define GLOBALVAR		0x54			// Establish Global Variables
#define FLUSHVAR		0x55			// Flush all global Variables  
/***********************************************
Server Definition Block
************************************************/
typedef struct ServBlk
{
unsigned short port;
unsigned short storagefail;					// number of getmain failures
} SB,*PSB;


SOCKET my_socket;
int		Running;
void    InitSB(PSB sb);						// Init the server block
int		DispTrans(PINFBLK ib);				// Dispatch the correct transaction
int		ExecCmd(PINFBLK ib);				// Execute The Command Please
int		BuildDList(PINFBLK ib,char *type);	// Dir List setup
int		BuildFName(PINFBLK ib,char *type);	// Project or group Name
int		ExeOnly(PINFBLK ib);				// Only Execute a Command
int		SendFile(PINFBLK ib);				// Send Back A requested File
int		DumpArea(char *data,int len);
void	FormatCmd(char *cmd,char *ext,char *p);
int		InitTrans(PINFBLK ib);
int		DumpArea(char *data,int len);
int		SendResponse(PINFBLK ib,int rc);
void	SetUpFileLocation(char *fn,char *loc,char *fullname);
int		CheckInternalCmd(char *cmd,PINFBLK ib);
int		runcmd(char *cmd,PINFBLK ib);
int		SendData(PINFBLK ib,char *fn,char *p);
void	WriteTransBuffer(PINFBLK ib,char *type,char *data,int len);
int		DataOnly(PINFBLK ib);
int		RemoveData(PINFBLK ib);
int		GetAMDValue(char *name);
int		StrtOnly(PINFBLK ib);
int		startcmd(char *cmd,PINFBLK ib);
int		LockData(PINFBLK ib);
int		RemoveD(PINFBLK ib);
int BuildTempFile(char *real,char *temp);
void HeartBeat(PSB sb);
int SetAMDValue(char *name,char *value);
void AppendDefault(void);
void LoadDirItems(PINFBLK ib, char *path);
int splitpath(char *in,char *path,char *file);











