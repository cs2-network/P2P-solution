#ifdef LINUX
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h> 
#include <sys/types.h>
#include <sys/time.h> 
#include <signal.h> 
#include <netinet/in.h>
#include <netdb.h> 
#include <net/if.h>
#include <string.h>
#include <sched.h>
#include <stdarg.h>
#include <dirent.h>
#include <arpa/inet.h>  // inet_ntoa
#endif
#ifdef WIN32DLL
#include <Winsock2.h>
#include <windows.h>
#include <stdio.h>
#endif //// #ifdef WIN32DLL

#ifdef WIN32DLL
#include "../../../../Include/PPCS/PPCS_API.h"
#else
#include "../../../Include/PPCS/PPCS_API.h"
#endif

#define CONNECT_TEST				// 连接测试
//#define RW_TEST					// 读写测试
//#define FT_TEST					// 文件传输测试
//#define RW_TEST1					// 读写测试1
//#define MULTI_CONNECT_TEST		// 多连接测试 多个DID测试
//#define MULTI_SESSION_TEST
//#define RW_TEST2					// 读写测试2
//#define RW_TEST3					// 读写测试3
//#define CONNECT_TEST_BY_SERVER	// 跨平台连接测试
//#define PktSR_TEST				// 即时传输测试（丢包不重传，不保证送达）

//// This InitString is CS2 PPCS InitString, you must Todo: Modify this for your own InitString 
//// 用户需改为自己平台的 InitString
const char *g_DefaultInitString = "EBGAEIBIKHJJGFJKEOGCFAEPHPMAHONDGJFPBKCPAJJMLFKBDBAGCJPBGOLKIKLKAJMJKFDOOFMOBECEJIMM";

#ifdef WIN32DLL
UINT32 MyGetTickCount() {return GetTickCount();}
#endif
#ifdef LINUX
CHAR bFlagGetTickCount= 0;
struct timeval gTime_Begin;
UINT32 MyGetTickCount()
{
	if(!bFlagGetTickCount)
	{
		bFlagGetTickCount = 1;
		gettimeofday(&gTime_Begin, NULL);
		return 0;
	}
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//printf("%d %d %d %d\n",tv.tv_sec , gTime_Begin.tv_sec, tv.tv_usec ,gTime_Begin.tv_usec);
	return (tv.tv_sec - gTime_Begin.tv_sec)*1000 + (tv.tv_usec - gTime_Begin.tv_usec)/1000;
}
#endif


void mSecSleep(UINT32 ms)
{
#ifdef WIN32DLL
			Sleep(ms);
#endif //// WIN32DLL
#ifdef LINUX
			usleep(ms * 1000);
#endif //// LINUX
}

// 获取错误信息
const char *getP2PErrorCodeInfo(int err)
{
    if (0 < err)
        return "NoError";
    switch (err)
    {
        case 0: return "ERROR_P2P_SUCCESSFUL";
        case -1: return "ERROR_P2P_NOT_INITIALIZED";
        case -2: return "ERROR_P2P_ALREADY_INITIALIZED";
        case -3: return "ERROR_P2P_TIME_OUT";
        case -4: return "ERROR_P2P_INVALID_ID";
        case -5: return "ERROR_P2P_INVALID_PARAMETER";
        case -6: return "ERROR_P2P_DEVICE_NOT_ONLINE";
        case -7: return "ERROR_P2P_FAIL_TO_RESOLVE_NAME";
        case -8: return "ERROR_P2P_INVALID_PREFIX";
        case -9: return "ERROR_P2P_ID_OUT_OF_DATE";
        case -10: return "ERROR_P2P_NO_RELAY_SERVER_AVAILABLE";
        case -11: return "ERROR_P2P_INVALID_SESSION_HANDLE";
        case -12: return "ERROR_P2P_SESSION_CLOSED_REMOTE";
        case -13: return "ERROR_P2P_SESSION_CLOSED_TIMEOUT";
        case -14: return "ERROR_P2P_SESSION_CLOSED_CALLED";
        case -15: return "ERROR_P2P_REMOTE_SITE_BUFFER_FULL";
        case -16: return "ERROR_P2P_USER_LISTEN_BREAK";
        case -17: return "ERROR_P2P_MAX_SESSION";
        case -18: return "ERROR_P2P_UDP_PORT_BIND_FAILED";
        case -19: return "ERROR_P2P_USER_CONNECT_BREAK";
        case -20: return "ERROR_P2P_SESSION_CLOSED_INSUFFICIENT_MEMORY";
        case -21: return "ERROR_P2P_INVALID_APILICENSE";
        case -22: return "ERROR_P2P_FAIL_TO_CREATE_THREAD";
        default:
            return "Unknow, something is wrong!";
    }
}

#ifdef MULTI_CONNECT_TEST
#ifdef WIN32DLL
DWORD WINAPI ThreadMultiConnect(void* arg)
#endif
#ifdef LINUX
void *ThreadMultiConnect(void *arg)
#endif
{
	INT32 ret;
	CHAR *DID = (CHAR*) arg;
	printf("MULTI_CONNECT_TEST: PPCS_Connect(%s, 0, 0)...\n\n", DID);
	for (int i = 0; i < 100; i++)
	{
		printf("DID = %s, cointer = %d\n", DID, i);
		ret = PPCS_Connect(DID, 0, 0);
		if (ret < 0)
			printf("PPCS_Connect failed : %d. [%s]\n", ret, getP2PErrorCodeInfo(ret));
		else
		{
			st_PPCS_Session Sinfo;	
			if(PPCS_Check(ret, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
			{
				printf("-------------- Session Ready (%s): -%s------------------\n", DID,(Sinfo.bMode ==0)? "P2P":"RLY");
				//printf("Socket : %d\n", Sinfo.Skt);
				printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
				//printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
				//printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
				//printf("Connection time : %d second before\n", Sinfo.ConnectTime);
				//printf("DID : %s\n", Sinfo.DID);
				//printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
				//printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
				//printf("------------End of Session info (%d): ---------------\n", i);
			}
			PPCS_Close(ret);
	#ifdef WIN32DLL
			Sleep(1000);
	#endif
	#ifdef LINUX
			usleep(1000000);
	#endif
			
		}
	}
	#ifdef WIN32DLL
		return 0;
	#endif
	#ifdef LINUX
		pthread_exit(0);
	#endif
}
#endif //// #ifdef MULTI_CONNECT_TEST

#ifdef RW_TEST3
#define TEST_WRITE_SIZE 16064  // (251 * 64), 251 is a prime number
#define TOTAL_WRITE_SIZE (4*1024*TEST_WRITE_SIZE)
#define TEST_NUMBER_OF_CHANNEL 1

INT32 gTheSessionHandle;
CHAR gFlagWorking = 0;

#ifdef WIN32DLL
DWORD WINAPI ThreadWrite(void* arg)
#endif
#ifdef LINUX
void *ThreadWrite(void *arg)
#endif
{
	INT32 Channel = *((INT32*)arg);	
	UCHAR *Buffer = (UCHAR *)malloc(TEST_WRITE_SIZE);
	INT32 TotalSize = 0; 
	if(Buffer == NULL) 
	{
		printf("malloc Failed!!\n");
	}
	else
	{
		for(INT32 i = 0 ; i< TEST_WRITE_SIZE; i++)	Buffer[i] = i%251;
		printf("ThreadWrite %d running... \n",Channel);
	}
 /*
	UINT32 WriteSize;
	while(PPCS_Check_Buffer(gTheSessionHandle, Channel, &WriteSize, NULL) == ERROR_PPCS_SUCCESSFUL)
	{
		if((WriteSize < 256*1024) && (TotalSize < TOTAL_WRITE_SIZE))
		{
			INT32 n1,n2,n3,n4;
			n1 = rand() % (TEST_WRITE_SIZE/3);
			n2 = rand() % (TEST_WRITE_SIZE/3);
			n3 = rand() % (TEST_WRITE_SIZE/3);
			n4 = TEST_WRITE_SIZE - n1 -n2 -n3;
			PPCS_Write(gTheSessionHandle, Channel, (CHAR*)Buffer, n1);
			PPCS_Write(gTheSessionHandle, Channel, (CHAR*)(Buffer+n1), n2);
			PPCS_Write(gTheSessionHandle, Channel, (CHAR*)(Buffer+n1+n2), n3);
			PPCS_Write(gTheSessionHandle, Channel, (CHAR*)(Buffer+n1+n2+n3), n4);
			TotalSize += TEST_WRITE_SIZE;
		}
		else if(TotalSize >= TOTAL_WRITE_SIZE)
			break;
		else
			mSecSleep(1);
		gFlagWorking = 1;
	} //// */
	free(Buffer);
	printf("\nThreadWrite %d Exit. TotalSize = %d ",Channel,TotalSize);
#ifdef WIN32DLL
	return 0;
#endif
#ifdef LINUX
	pthread_exit(0);
#endif
}

#ifdef WIN32DLL
DWORD WINAPI ThreadRead(void* arg)
#endif
#ifdef LINUX
void *ThreadRead(void *arg)
#endif
{
	INT32 Channel = *((INT32*) arg);	
	INT32 i = 0;

	UINT32 tick=MyGetTickCount();
	printf("ThreadRead %d running... \n",Channel);
	while(1)
	{
		UCHAR zz;
		INT32 ReadSize=1;
		INT32 ret;
		ret = PPCS_Read(gTheSessionHandle, Channel, (CHAR*)&zz, &ReadSize, 100);
		if((ret < 0) && (ret != ERROR_PPCS_TIME_OUT))
		{
			printf("Channel:%d PPCS_Read ret = %d , i=%d\n", Channel, ret,i);
			break;
		}
		if((i >= TOTAL_WRITE_SIZE) && (ret == ERROR_PPCS_TIME_OUT))
			break;
		gFlagWorking = 1;
		if(ReadSize == 0)
			continue;
		if((i%251) != zz)
		{
			printf("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Channel:%d Error!! i=%d zz= %d\n",Channel,  i,zz);
			break;
		}
		else
		{
			if(i%(5*1024*1024) == (5*1024*1024-1)) 
			{
				printf("%d",Channel); 
				fflush(NULL);
			}
		}
		i++;
	}
	tick = MyGetTickCount() - tick - 100;
	printf("\nThreadRead %d Exit - Channel:%d, Time: %d.%d sec, %d KByte/sec",Channel, Channel,tick/1000, tick%1000, i/tick);

#ifdef WIN32DLL
	return 0;
#endif
#ifdef LINUX
	pthread_exit(0);
#endif
}
#endif //// #ifdef RW_TEST3


INT32 main(INT32 argc, CHAR **argv)
{
	if (argc < 2)
	{
#ifdef MULTI_CONNECT_TEST		
#ifdef WIN32DLL
		printf("Usage: DID1 DID2 DID3 ...\n");
		printf("\tYou can input multiple DID to test\n");
		printf("Example:\n");
		printf("ABCD-000000-ABCDEF\n");
		printf("ABCD-000000-ABCDEF ABCD-000001-ABCDEF\n");
		printf("ABCD-000000-ABCDEF ABCD-000001-ABCDEF ABCD-000002-ABCDEF ...\n");
		printf("\nPlease press any key to exit the main thread ...");
		getchar();
#else
		printf("Usage: ./PPCS_Client DID1 DID2 DID3 ...\n");
		printf("\tYou can input multiple DID to test\n");
		printf("Example:\n");
		printf("\t ./PPCS_Client ABCD-000000-ABCDEF\n");
		printf("\t ./PPCS_Client ABCD-000000-ABCDEF ABCD-000001-ABCDEF\n");
		printf("\t ./PPCS_Client ABCD-000000-ABCDEF ABCD-000001-ABCDEF ABCD-000002-ABCDEF ...\n");
#endif // ifdef WIN32DLL
#else 
#ifdef WIN32DLL
		printf("Usage: DID\n");
		printf("\nPlease press any key to exit the main thread ...");
		getchar();
#else
		printf("Usage: ./PPCS_Client DID\n");
		printf("Example:\n");
		printf("\t ./PPCS_Client ABCD-123456-ABCDEF\n");
#endif
#endif // ifdef MULTI_CONNECT_TEST
		return 0;
	}
	
	UINT32 APIVersion = PPCS_GetAPIVersion();
	printf("\nPPCS_API Version: %d.%d.%d.%d\n", (APIVersion & 0xFF000000)>>24, (APIVersion & 0x00FF0000)>>16, (APIVersion & 0x0000FF00)>>8, (APIVersion & 0x000000FF) >> 0 );
	
	INT32 ret = PPCS_Initialize((CHAR*)g_DefaultInitString);
	
	st_PPCS_NetInfo NetInfo;
	ret = PPCS_NetworkDetect(&NetInfo, 0);
	if (ret < 0)
	{
		printf("PPCS_NetworkDetect() ret= %d. [%s]\n", ret, getP2PErrorCodeInfo(ret));
	}
	
	printf("-------------- NetInfo: -------------------\n");
	printf("Internet Reachable     : %s\n", (NetInfo.bFlagInternet == 1) ? "YES":"NO");
	printf("P2P Server IP resolved : %s\n", (NetInfo.bFlagHostResolved == 1) ? "YES":"NO");
	printf("P2P Server Hello Ack   : %s\n", (NetInfo.bFlagServerHello == 1) ? "YES":"NO");
	printf("Local NAT Type         :");

	switch (NetInfo.NAT_Type)
	{
	case 0:
		printf(" Unknow\n");
		break;
	case 1:
		printf(" IP-Restricted Cone\n");
		break;
	case 2:
		printf(" Port-Restricted Cone\n");
		break;
	case 3:
		printf(" Symmetric\n");
		break;
	}
	printf("My Wan IP : %s\n", NetInfo.MyWanIP);
	printf("My Lan IP : %s\n", NetInfo.MyLanIP);
	printf("-------------------------------------------\n\n");

#ifdef MULTI_CONNECT_TEST
#ifdef WIN32DLL
	HANDLE hthread[256];
#endif
#ifdef LINUX
	pthread_t threadID[256];
#endif
	INT32 max = argc;
	if (max > 256) 
		max = 256;
	for (INT32 i = 1; i < max; i++)
	{
#ifdef WIN32DLL
		hthread[i] = CreateThread(NULL, 0, ThreadMultiConnect, (void *) argv[i], 0, NULL);
#endif
#ifdef LINUX
		pthread_create(&threadID[i], NULL, &ThreadMultiConnect, (void *) argv[i]);
#endif
	}
	for (INT32 i = 1; i < argc; i++)
	{
#ifdef WIN32DLL
		WaitForSingleObject(hthread[i], INFINITE);
#endif
#ifdef LINUX
		pthread_join(threadID[i], NULL);
#endif
	}

#endif //// #ifdef MULTI_CONNECT_TEST

#ifdef CONNECT_TEST_BY_SERVER
	printf("PPCS_ConnectByServer(%s, 0, 0, %s)...\n\n", argv[1], g_DefaultInitString);
	
	for (int i = 0; i < 10000000; i++)
	{
		ret = PPCS_ConnectByServer(argv[1], 0, 0, (char*)g_DefaultInitString);
		
		if (ret < 0)
			printf("PPCS_Connect failed : %d. [%s]\n", ret, getP2PErrorCodeInfo(ret));
		else
		{
			st_PPCS_Session Sinfo;	
			if (PPCS_Check(ret, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
			{
				printf("-------%d, Session Ready (%d): -%s------------------\n", i,ret,(Sinfo.bMode ==0)? "P2P":"RLY");
				//printf("Socket : %d\n", Sinfo.Skt);
				//printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
				//printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
				//printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
				//printf("Connection time : %d second before\n", Sinfo.ConnectTime);
				//printf("DID : %s\n", Sinfo.DID);
				//printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
				//printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
				//printf("------------End of Session info (%d): ---------------\n", i);
			}
#ifdef WIN32DLL
			Sleep(50);
#endif
#ifdef LINUX
			usleep(50000);
#endif
			PPCS_Close(ret);
		}
	}
#endif //// #define CONNECT_TEST_BY_SERVER
#ifdef CONNECT_TEST
	printf("CONNECT_TEST: PPCS_Connect(%s, 1, 0)...\n\n", argv[1]);
	for (int i = 0; i < 10000000; i++)
	{	
		// 发起P2P连接请求
		ret = PPCS_Connect(argv[1], 1, 0);
		
		if (ret < 0)
			printf("CONNECT_TEST: PPCS_Connect failed : %d. [%s]\n", ret, getP2PErrorCodeInfo(ret));
		else
		{
			st_PPCS_Session Sinfo;	
			if (PPCS_Check(ret, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
			{
				printf("-------%d, Session Ready (%d): -%s------------------\n", i,ret,(Sinfo.bMode ==0)? "P2P":"RLY");
				//printf("Socket : %d\n", Sinfo.Skt);
				//printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
				//printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
				//printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
				//printf("Connection time : %d second before\n", Sinfo.ConnectTime);
				//printf("DID : %s\n", Sinfo.DID);
				//printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
				//printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
				//printf("------------End of Session info (%d): ---------------\n", i);
			}
#ifdef WIN32DLL
			Sleep(50);
#endif
#ifdef LINUX
			usleep(50000);
#endif
			PPCS_Close(ret);
		}
	}
#endif //// CONNECT_TEST

#ifdef PktSR_TEST
	INT32 SessionHandle;
	printf("PktSR_TEST: PPCS_Connect(%s, 1, 0)...\n", argv[1]);
	SessionHandle = PPCS_Connect(argv[1], 1, 0);
	
	if (SessionHandle < 0)
		printf("PktSR_TEST: PPCS_Connect failed : %d. [%s]\n", SessionHandle, getP2PErrorCodeInfo(SessionHandle));
	else
	{
		printf("PktSR_TEST: Connect Success!! SessionHandle= %d.\n\n", SessionHandle);
		
		INT32 Counter = 0;
		CHAR ExpectValue = 0;
		st_PPCS_Session Sinfo;	
		if (PPCS_Check(SessionHandle, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("-------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("------------End of Session info : ---------------\n");
		}
		
		printf("PPCS_PktRecv ...\n\n");
		while (1)
		{
			CHAR PktBuf[1240];
			INT32 PktSize = sizeof(PktBuf);
			memset(PktBuf, 0, PktSize);
			
			ret = PPCS_PktRecv(SessionHandle, 0, PktBuf, &PktSize, 0xFFFFFFFF);
			//printf("PPCS_PktRecv ret = %d\n", ret);
			if (ret == ERROR_PPCS_SESSION_CLOSED_TIMEOUT)
			{
				printf("Session TimeOUT!!\n");
				break;
			}
			else if (ret == ERROR_PPCS_SESSION_CLOSED_REMOTE)
			{
				printf("Session Remote Close!!\n");
				break;
			}
			if (PktSize != 1024) //// we send 1024 byte packet
				printf("Packet size error!! PktSize=%d, should be 1024\n", PktSize); 
			if (ExpectValue != PktBuf[0]) 
			{
				printf("Packet Lost Detect!! Value = %d (should be %d)\n", PktBuf[0], ExpectValue); 
				ExpectValue = (PktBuf[0] + 1) % 100;
			}
			else
				ExpectValue = (ExpectValue + 1 ) % 100;
			
			if (Counter % 1000 == 999)
				printf("Receive %dK packets\n",(Counter + 1) / 1000);
			Counter++;
		}
		PPCS_Close(SessionHandle);
	}
#endif //// PktSR_TEST

#ifdef RW_TEST
	INT32 SessionHandle;
	printf("RW_TEST: PPCS_Connect(%s, 0, 0)...\n", argv[1]);
	SessionHandle = PPCS_Connect(argv[1], 0, 0);
	
	if (SessionHandle < 0)
		printf("RW_TEST: PPCS_Connect failed : %d. [%s]\n", SessionHandle, getP2PErrorCodeInfo(SessionHandle));
	else
	{
		printf("RW_TEST: Connect Success!! SessionHandle= %d.\n\n", SessionHandle);
		INT32 i = 0;
		st_PPCS_Session Sinfo;	
		if (PPCS_Check(SessionHandle, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("-------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("------------End of Session info : ---------------\n");
		}
		
		while (1)
		{
			//UINT32 ReadSize;
			//PPCS_Check_Buffer(SessionHandle, 0, NULL, &ReadSize);
			INT32 zz;
			INT32 DataSize = 4;
			ret = PPCS_Read(SessionHandle, 0, (CHAR*)&zz, &DataSize, 0xFFFFFFFF);
			if (ret == ERROR_PPCS_SESSION_CLOSED_TIMEOUT)
			{
				printf("Session TimeOUT!!\n");
				break;
			}
			else if (ret == ERROR_PPCS_SESSION_CLOSED_REMOTE)
			{
				printf("Session Remote Close!!\n");
				break;
			}
			if ((i * 2) == zz) 
			{
				if (i%1000 == 999)
					printf("i=%d, %d\n",i, zz ); 
			}
			else
			{
				printf("Error: i=%d, %d\n",i, zz ); 
				break;
			}
			i++;
		}
		PPCS_Close(SessionHandle);
	}
#endif //// RW_TEST
#ifdef FT_TEST
	INT32 SessionHandle;
	printf("FT_TEST: PPCS_Connect(%s, 1, 0)...\n", argv[1]);
	SessionHandle = PPCS_Connect(argv[1], 1, 0);
	
	if (SessionHandle < 0)
		printf("FT_TEST: PPCS_Connect failed : %d. [%s]\n", SessionHandle, getP2PErrorCodeInfo(SessionHandle));
	else
	{
		printf("FT_TEST: Connect Success!! SessionHandle= %d.\n\n", SessionHandle);
		UINT32 Counter = 0;
		st_PPCS_Session Sinfo;	
		if (PPCS_Check(ret, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("-------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("------------End of Session info : ---------------\n");
		}
		FILE *fp = fopen("2.txt", "wb");
		if (!fp)
		{
			printf("***Error: failed to open file: %s\n", "2.7z");
			PPCS_Close(SessionHandle);
			PPCS_DeInitialize();
			printf("PPCS_DeInitialize done!!\n");
			return 0;
		}
		
		while (1)
		{
			CHAR buf[1000];
			memset(buf, 0, sizeof(buf));
			INT32 DataSize = 1000;			
			INT32 ret = PPCS_Read(SessionHandle, 1, buf, &DataSize, 0xFFFFFFFF);
			if (ret == ERROR_PPCS_SESSION_CLOSED_TIMEOUT)
			{
				printf("Session TimeOUT!!\n");
				break;
			}
			else if (ret == ERROR_PPCS_SESSION_CLOSED_REMOTE)
			{
				printf("Session Remote Close!!\n");
				break;
			}
			Counter = Counter + DataSize;
			fwrite(buf, DataSize, 1, fp);

			if (Counter % 10000 == 0)
				printf("%d\n", Counter);
		}
		printf("File Transfer done!! file size = %d\n",Counter );
		fclose(fp);
	}
#endif //// FT_TEST

#ifdef RW_TEST1
	INT32 SessionHandle;
	
	printf("RW_TEST1: PPCS_Connect(%s, 1, 0)...\n", argv[1]);
	SessionHandle = PPCS_Connect(argv[1], 1, 0);
	
	if (SessionHandle < 0)
		printf("RW_TEST1: PPCS_Connect failed : %d. [%s]\n", SessionHandle, getP2PErrorCodeInfo(SessionHandle));
	else
	{
		printf("RW_TEST1: Connect Success!! SessionHandle= %d.\n\n", SessionHandle);
		st_PPCS_Session Sinfo;	
		if (PPCS_Check(ret, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("-------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("------------End of Session info : ---------------\n");
		}
		
		
		printf("PPCS Writeing ...\n\n");
		for (LONG i = 0 ; i < 1000000000; i++)
		{
			/*
			UINT32 WriteByte;
			ret = PPCS_Check_Buffer(SessionHandle, 2, &WriteByte, NULL);
			if (ret == ERROR_PPCS_SESSION_CLOSED_TIMEOUT)
			{
				printf("Session TimeOUT!!\n");
				break;
			}
			else if (ret == ERROR_PPCS_SESSION_CLOSED_REMOTE)
			{
				printf("Session Remote Close!!\n");
				break;
			}
			if (WriteByte > (32 * 1024 * 1024))
			{
				mSecSleep(1);
				i--;
				continue;
			}*/

			LONG zz = i * 2;
			ret = PPCS_Write(SessionHandle, 2, (CHAR*)&zz, 4);
			if (ret == ERROR_PPCS_SESSION_CLOSED_TIMEOUT)
			{
				printf("Session TimeOUT!!\n");
				break;
			}
			else if (ret == ERROR_PPCS_SESSION_CLOSED_REMOTE)
			{
				printf("Session Remote Close!!\n");
				break;
			}
			if (i % 10000000 == 999999)
				printf("i = %ld\n", i);
		}
		PPCS_Close(SessionHandle);
	}
#endif //// RW_TEST1

#ifdef RW_TEST2

#define TEST_WRITE_SIZE (1024*128)

	INT32 SessionHandle;
	printf("RW_TEST2: PPCS_Connect(%s, 1, 0)...\n", argv[1]);
	SessionHandle = PPCS_Connect(argv[1], 1, 0);
	
	if (SessionHandle < 0)
		printf("RW_TEST2: PPCS_Connect failed : %d. [%s]\n", SessionHandle, getP2PErrorCodeInfo(SessionHandle));
	else
	{
		printf("RW_TEST2: Connect Success!! SessionHandle= %d.\n\n", SessionHandle);
		st_PPCS_Session Sinfo;	
		if (PPCS_Check(SessionHandle, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("-------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("------------End of Session info : ---------------\n");
		}

		UCHAR *Buffer = (UCHAR *)malloc(TEST_WRITE_SIZE);
		if (!Buffer) 
		{
			printf("***Error: malloc Failed!!\n");
			PPCS_Close(SessionHandle);
			PPCS_DeInitialize();
			printf("PPCS_DeInitialize done!!\n");
			return 0;
		}
		else
		{
			INT32 Counter = 1000;
			for (INT32 i = 0 ; i < TEST_WRITE_SIZE; i++) 
				Buffer[i] = i % 256;			
			
			PPCS_Write(SessionHandle, 0, (CHAR*)Buffer, TEST_WRITE_SIZE);
			
			UINT32 WriteSize;
			while (PPCS_Check_Buffer(SessionHandle, 0, &WriteSize, NULL) == ERROR_PPCS_SUCCESSFUL)
			{
				//printf("WriteSize=%d\n",WriteSize);
				if (WriteSize < TEST_WRITE_SIZE)
				{
					if (Counter > 0)
					{
						PPCS_Write(SessionHandle, 0, (CHAR*)Buffer, TEST_WRITE_SIZE);
						printf("Counter= %d\n", Counter);
					}
					else
						mSecSleep(1);
					Counter--;
				}
				else
					mSecSleep(1);
			}
		}
		PPCS_Close(SessionHandle);
	}
#endif //// RW_TEST2

#ifdef RW_TEST3
	printf("RW_TEST3: PPCS_Connect(%s, 1, 0)...\n", argv[1]);
	gTheSessionHandle = PPCS_Connect(argv[1], 1, 0);
	
	if (gTheSessionHandle < 0)
		printf("RW_TEST3: PPCS_Connect failed : %d. [%s]\n", gTheSessionHandle, getP2PErrorCodeInfo(gTheSessionHandle));
	else
	{
		printf("RW_TEST3: Connect Success!! SessionHandle= %d.\n", gTheSessionHandle);
		st_PPCS_Session Sinfo;	
		if(PPCS_Check(gTheSessionHandle, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("-------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("------------End of Session info : ---------------\n");
		}

#ifdef WIN32DLL
		HANDLE hThreadWrite[TEST_NUMBER_OF_CHANNEL];
		HANDLE hThreadRead[TEST_NUMBER_OF_CHANNEL];
#endif
#ifdef LINUX
		pthread_t ThreadWriteID[TEST_NUMBER_OF_CHANNEL];
		pthread_t ThreadReadID[TEST_NUMBER_OF_CHANNEL];
#endif

		for (INT32 i = 0; i < TEST_NUMBER_OF_CHANNEL; i++)
		{
#ifdef WIN32DLL
			hThreadWrite[i] = CreateThread(NULL, 0, ThreadWrite, (void *) &i, 0, NULL);
			mSecSleep(10);
			hThreadRead[i] = CreateThread(NULL, 0, ThreadRead, (void *) &i, 0, NULL);
			mSecSleep(10);
#endif
#ifdef LINUX
			pthread_create(&ThreadWriteID[i], NULL, &ThreadWrite, (void *) &i);
			mSecSleep(10);
			pthread_create(&ThreadReadID[i], NULL, &ThreadRead, (void *) &i);
			mSecSleep(10);
#endif
		}
		gFlagWorking = 1;
		while (1 == gFlagWorking)
		{
			gFlagWorking = 0;
			mSecSleep(200);
		}
		PPCS_Close(gTheSessionHandle);
		
		for (INT32 i = 0; i < TEST_NUMBER_OF_CHANNEL; i++)
		{	
#ifdef WIN32DLL
			WaitForSingleObject(hThreadRead[i], INFINITE);
			WaitForSingleObject(hThreadWrite[i], INFINITE);
#endif
#ifdef LINUX
			pthread_join(ThreadReadID[i], NULL);
			pthread_join(ThreadWriteID[i], NULL);
#endif
		}
	}
#endif

	ret = PPCS_DeInitialize();
	printf("PPCS_DeInitialize done!!\n");
#ifdef WIN32DLL
	printf("Job Done!! press any key to exit ...\n");
	getchar();
#endif
	return 0;
}