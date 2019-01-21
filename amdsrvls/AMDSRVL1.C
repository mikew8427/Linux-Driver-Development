#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include "service.h"
extern SOCKET my_socket;
extern int Running;

//***********************************************
//** This Event will be set for Service Stop   **
//***********************************************
HANDLE  hServerStopEvent = NULL;
HANDLE  child			 = NULL;
DWORD	childid			 = 0;

VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{
int x;
    //************************************************
    //**           Service initialization           **
    //************************************************
	InitializeCriticalSection(&TASKS);							// Allow Access to child list
	InitializeCriticalSection(&DATAO);							// Allow Access to child list
	for (x=0;x<MAXTASK;x++) {AmdTask[x]=0; AmdSock[x]=0;}		// Set them all to NG
    if (!ReportStatusToSCMgr(SERVICE_START_PENDING,NO_ERROR,3000)) return;
	child=CreateThread(NULL,8192,(LPTHREAD_START_ROUTINE)StartTalk,(LPVOID)lpszArgv,0,&childid);
	if(!child) return;

    hServerStopEvent = CreateEvent(NULL,TRUE,FALSE,NULL);		// Create the Event
    if ( hServerStopEvent == NULL) return;						// Not there get out

    if (!ReportStatusToSCMgr(SERVICE_RUNNING,NO_ERROR,0)) return;
    WaitForSingleObject( hServerStopEvent, INFINITE );			// Wait here for termination
	//
	// Get rid of All the Child Tasks first
	//
	EnterCriticalSection(&TASKS);
	for(x=0;x<MAXTASK;x++) 
		{
		if(AmdTask[x]>0) 
			{
			shutdown(AmdSock[x],2);										// Disable Send/Recv for socket
			closesocket(AmdSock[x]);									// Now Close it
	//		TerminateThread(AmdTask[x],64); CloseHandle(AmdTask[x]);
			}
		}
	LeaveCriticalSection(&TASKS);
	//
	// Now get rid of the Main Dispatcher
	//
	TerminateThread(child,64);									// Get rid of the thread
	CloseHandle(child);
	DeleteCriticalSection(&TASKS);
	DeleteCriticalSection(&DATAO);
    if (hServerStopEvent) CloseHandle(hServerStopEvent);		// Close this handle
return;
}

/****************************************************/
/**        Signal the service is to end            **/
/****************************************************/
VOID ServiceStop()
{
    if ( hServerStopEvent )
        SetEvent(hServerStopEvent);
}
