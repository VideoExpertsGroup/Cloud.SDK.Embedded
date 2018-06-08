#ifndef _CWEBSOCKWRAP_H
#define _CWEBSOCKWRAP_H

#include "utils.h"
#include "MLog.h"
#include <stdint.h>
#include <map>
#include <string>
#include <list>
#include <stdio.h>
#include <ctime>
#include <iostream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
//#include <Processthreadsapi.h>
#endif

using namespace std;

#ifdef  __cplusplus
extern "C" {
#endif
#include "../../external_libs/libwebsockets/src/lib/libwebsockets.h"
#ifdef  __cplusplus
}
#endif

#undef USE_WINDOWS_CERT_STORE

#ifdef _WIN32
//#define USE_WINDOWS_CERT_STORE
#ifdef USE_WINDOWS_CERT_STORE
#include "openssl\x509.h"
#include <wincrypt.h>
#include <cryptuiapi.h>
#endif
#endif

typedef enum WEBSOCKWRAP_EVENT
{
    WEBSOCKWRAP_CONNECT = 0x00,
    WEBSOCKWRAP_DISCONNECT = 0x01,
    WEBSOCKWRAP_RECEIVE = 0x02,
    WEBSOCKWRAP_ERROR = 0x03,
    WEBSOCKWRAP_LWS_OTHER = 0x11,
}WEBSOCKWRAP_EVENT;

typedef struct event_data
{
    WEBSOCKWRAP_EVENT reason;
    char*   buffer;
    int     size;
}event_data;


typedef struct send_data
{
    char*   buffer;
    int     size;
}send_data;

typedef int(*WSWRAP_CB_PROC)(void* inst, WEBSOCKWRAP_EVENT reason, void *in, size_t len);

class CWSClientWrapper
{
	const char *TAG = "CWSClientWrapper";
	const int LOG_LEVEL = 2; //Log.VERBOSE;

public:
	MLog Log;

    CWSClientWrapper(WSWRAP_CB_PROC OnEvent, void *userData);
    virtual ~CWSClientWrapper();
    int Connect(const char* url, const char* method=NULL, int pending_timeout_ms=0, const char* cert_path = NULL, const char* key_path = NULL);
    int Disconnect();
    int WriteBack(const char *str, size_t size_in=0);
    int isConnected() { return m_bConnected; };

    static int cbProc(void* inst, int reason, void *in, size_t len);
    int m_bDisconnectFlag;
    int m_bDestroyFlag;
    int m_nPendingTimeout;
    void* GetContext() { return (void*)m_pContext; };
    void* GetProtocols() { return (void*)m_Protocols;};
    void* GetWsi() { return (void*)m_pWsi; };
    void* GetUserData() { return m_UserData; };
    WSWRAP_CB_PROC  GetEventsCb() { return m_OnEvent; };

    HANDLE                      m_hPendingThread;
    CCritSec                    m_csProcLock;

    HANDLE                      m_hEventsThread;

    int                         GetEvent(event_data** vdata);
    int                         AddEvent(WEBSOCKWRAP_EVENT reason, void* in, int len);
    int                         GetEventsCount();
    int                         PurgeEvents();

    int                         GetSendData(send_data** vdata);
    int                         AddSendData(const char* in, int len);
    int                         GetSendDataCount();
    int                         PurgeSendData();
    int                         m_nSendRetCode;

private:
    int                         m_nPort;
    lws_protocols               m_Protocols[2];
    lws_context*                m_pContext;
    lws_context_creation_info   m_CreateInfo;
    lws_client_connect_info     m_ClientInfo;
    lws*                        m_pWsi;
    int                         m_bConnected;
    WSWRAP_CB_PROC              m_OnEvent;
	void*						m_UserData;
	char                        m_szProto[32];

    vector <event_data*>        m_Events;
    size_t                      m_nMaxEventsSize;
    CCritSec                    m_csEventsLock;

    vector <send_data*>         m_SendData;
    CCritSec                    m_csSendDataLock;
    size_t                      m_nMaxSendDataSize;

#ifdef USE_WINDOWS_CERT_STORE
    HCERTSTORE                  m_hCertStore;
    PCCERT_CONTEXT              m_pCertContext;
#endif
};

#endif //_CWEBSOCKWRAP_H
