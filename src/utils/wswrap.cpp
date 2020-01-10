
//#include "stdafx.h"
#include "wswrap.h"
#include "windefsws.h"

#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)

#ifdef WIN32
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "winmm.lib")
//#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "kernel32.lib")
#endif //_WIN32


#ifdef _WIN32
DWORD WINAPI EventsThread(LPVOID pParam);
#else
void* EventsThread(LPVOID pParam);
#endif


static int ws_callback(lws *wsi, lws_callback_reasons reason, void *user, void *in, size_t len)
{
    CWSClientWrapper* inst = (CWSClientWrapper*)user;

    struct per_vhost_data__minimal *vhd =
        (struct per_vhost_data__minimal *)
        lws_protocol_vh_priv_get(lws_get_vhost(wsi),
            lws_get_protocol(wsi));

    if (!inst)
    {
        //logprintf(DEBUG_LEVEL_DEBUG, "inst==NULL\n");
        return 0;
    }

    if (inst->cbProc(inst, (int)reason, in, len))
        return lws_callback_http_dummy((lws*)inst->GetWsi(), reason, user, in, len);
    else
        return 0;
}

CWSClientWrapper::CWSClientWrapper(WSWRAP_CB_PROC OnEvent, void *userData)
	:Log("CWSClientWrapper", 2)
{
    m_OnEvent = OnEvent;
	m_UserData = userData;

	Log.d("=>CWSClientWrapper");
    m_bDisconnectFlag  = 0;
    m_bConnected    = 0;
    m_hPendingThread= (HANDLE)NULL;
    m_hEventsThread = (HANDLE)NULL;
    m_pWsi = NULL;
    m_nMaxEventsSize = 50;
    m_nMaxSendDataSize = 50;
    m_SendData.clear();
    m_Events.clear();
    m_nSendRetCode = 0;
    m_bDestroyFlag = 0;
	m_tLastPongTime = 0;

#ifdef _DEBUG
    //lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_DEBUG, NULL);
    //lws_set_log_level(LLL_ERR | LLL_WARN, NULL);
    lws_set_log_level(0, NULL);
#else
    lws_set_log_level(LLL_ERR | LLL_WARN, NULL);
#endif

#ifdef USE_WINDOWS_CERT_STORE
    X509 *x509 = NULL;
    X509_STORE *store = X509_STORE_new();
    m_pCertContext = NULL;
    m_hCertStore = CertOpenSystemStore(NULL, L"ROOT");

    //logprintf(DEBUG_LEVEL_DEBUG, "m_hCertStore = 0x%X\n", m_hCertStore);

    if (!m_hCertStore)
        return;

    while (m_pCertContext = CertEnumCertificatesInStore(m_hCertStore, m_pCertContext))
    {
        //uncomment the line below if you want to see the certificates as pop ups
        //CryptUIDlgViewContext(CERT_STORE_CERTIFICATE_CONTEXT, m_pCertContext,   NULL, NULL, 0, NULL);
        x509 = NULL;
        x509 = d2i_X509(NULL, (const unsigned char **)&m_pCertContext->pbCertEncoded, m_pCertContext->cbCertEncoded);
        if (x509)
        {
            int i = X509_STORE_add_cert(store, x509);
            //if (i == 1)logprintf(DEBUG_LEVEL_DEBUG, "certificate added\n");
            X509_free(x509);
        }
    }
#endif //USE_WINDOWS_CERT_STORE

    DWORD dwThreadId = 0;
    m_hEventsThread = CreateThread(0, 0, EventsThread, this, 0, &dwThreadId);

};

CWSClientWrapper::~CWSClientWrapper()
{

    Log.d("=>CWSClientWrapper");

    Disconnect();

Log.d("%s", __FUNCTION__);

#ifdef USE_WINDOWS_CERT_STORE
    if(m_pCertContext)
        CertFreeCertificateContext(m_pCertContext);
    if(m_hCertStore)
        CertCloseStore(m_hCertStore, 0);
#endif //USE_WINDOWS_CERT_STORE

    if (m_hEventsThread)
    {
        m_bDestroyFlag = 1;
        //Sleep(m_nPendingTimeout * 2);
        int nsleep = 100;
        while (m_hEventsThread && nsleep>0)
        {
            Sleep(10);
            nsleep--;
        }
        if (m_hEventsThread)
            Log.d("m_hEventsThread stop failed!!!");

        //TerminateThread(m_hEventsThread, 0);
        m_hEventsThread = (HANDLE)NULL;
    }

	Log.d("<=CWSClientWrapper");
}

int CWSClientWrapper::Disconnect()
{
    //logprintf(DEBUG_LEVEL_DEBUG, "Disconnect\n");
	Log.d("=>Disconnect");

	if (!m_bConnected && !m_pContext)
	{
		Log.d("Already disconnected");
		return 0;
	}

    m_bDisconnectFlag = 1;

	CAutoLock lock(&m_csProcLock);

    if (m_hPendingThread)
    {
        Sleep(m_nPendingTimeout * 2);
        int nsleep = 100;
        while (m_hPendingThread && nsleep>0)
        {
            Sleep(10);
            nsleep--;
        }
        if(m_hPendingThread)
            Log.d("m_hPendingThread stop failed!!!");

        //TerminateThread(m_hPendingThread, 0);
        m_hPendingThread = (HANDLE)NULL;
    }

Log.d("Disconnect 1");

    if (m_pContext)
    {
Log.d("Disconnect 2");
        lws_cancel_service(m_pContext);
Log.d("Disconnect 3");
        lws_context_destroy(m_pContext);
Log.d("Disconnect 4_1");
    }

    if(!m_pWsi && m_OnEvent)
    {
Log.d("Disconnect 4_2");
        m_OnEvent(m_UserData, WEBSOCKWRAP_DISCONNECT, NULL, 0);
Log.d("Disconnect 5");
    }

    PurgeEvents();

Log.d("Disconnect 6");

    PurgeSendData();

Log.d("Disconnect 7");

    m_bConnected = 0;
    m_pContext = NULL;

    Log.d("<=Disconnect");
    return 0;
}

#ifdef _WIN32
DWORD WINAPI EventsThread(LPVOID pParam)
#else
void* EventsThread(LPVOID pParam)
#endif
{
    CWSClientWrapper* pWrapper = (CWSClientWrapper*)pParam;
    pWrapper->Log.d("=>EventsThread");

    while (!pWrapper->m_bDestroyFlag)
    {
        event_data* vdata = NULL;
        WSWRAP_CB_PROC evCb = pWrapper->GetEventsCb();
        if (pWrapper->GetEvent(&vdata))
        {
            if (evCb)
            {
//                pWrapper->Log.d("=>evCb");
                evCb(pWrapper->GetUserData(), vdata->reason, vdata->buffer, vdata->size);
//                pWrapper->Log.d("<=evCb");
            }

            if (vdata->buffer)
                free(vdata->buffer);
            if (vdata)
                free(vdata);

        }
        else
        {
            Sleep(10);
        }
    }
    pWrapper->m_hEventsThread = (HANDLE)NULL;
    pWrapper->Log.d("<=EventsThread");
	pthread_exit(NULL);
    return 0;
}

#ifdef _WIN32
DWORD WINAPI PendingThread(LPVOID pParam)
#else
void* PendingThread(LPVOID pParam)
#endif
{
    //logprintf(DEBUG_LEVEL_DEBUG, "PendingThread started\n");
    CWSClientWrapper* pWrapper = (CWSClientWrapper*)pParam;
	pWrapper->Log.d("=>PendingThread %d",pWrapper->m_bDisconnectFlag);

    while (!pWrapper->m_bDisconnectFlag && pWrapper->GetWsi())
    {
		  //pWrapper->Log.d("=>PendingThread %d",pWrapper->isConnected()); 
		{
			CAutoLock lock(&pWrapper->m_csProcLock);
			if (pWrapper->isConnected())
			{
				lws_context* ctx = (lws_context*)pWrapper->GetContext();
				if (ctx)
				{
					lws_service(ctx, pWrapper->m_nPendingTimeout);
					lws_callback_on_writable_all_protocol(ctx, (lws_protocols*)(pWrapper->GetProtocols()));
					//lws_callback_on_writable((lws*)pWrapper->GetWsi());
				}
			}
		}
        Sleep(10);
    }

	pWrapper->Log.d("pWrapper->m_bDisconnectFlag=%d" , pWrapper->m_bDisconnectFlag);
	pWrapper->Log.d("pWrapper->GetWsi()=%p", pWrapper->GetWsi());

    pWrapper->m_hPendingThread = (HANDLE)NULL;
	pWrapper->Log.d("<=PendingThread");
	pthread_exit(NULL);
    return 0;
}

int CWSClientWrapper::Connect(const char* url, const char* method, int pending_timeout_ms, const char* cert_path, const char* key_path)
{

	CAutoLock lock(&m_csProcLock);

    const char* proto = NULL;
    const char* addr = NULL;
    const char* path = NULL;
    int port = 80;
    char* szUrl = NULL;

    if (url && strlen(url))
    {
        szUrl = (char*)malloc(strlen(url) +1);
        strcpy(szUrl, url);
        int e = lws_parse_uri(szUrl, &proto, &addr, &port, &path);
        Log.d("Url proto=%s, addr=%s, port=%d, path=%s\n ", proto, addr, port, path);
    }
    else
    {
		Log.d("Invalid url\n ");
        return -1;
    }


    memset(&m_Protocols, 0, sizeof m_Protocols);
    if(proto)
        strcpy(m_szProto, proto);
    else
        strcpy(m_szProto, "http");

    int use_ssl = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED;
    if (!strcmp(proto, "http") || !strcmp(proto, "ws"))
        use_ssl = 0;
	Log.d("use_ssl=%d", use_ssl);

    m_Protocols[0].name = m_szProto;
    m_Protocols[0].callback = &ws_callback;
    m_Protocols[0].user = this;

    memset(&m_CreateInfo, 0, sizeof m_CreateInfo);
    m_CreateInfo.port = CONTEXT_PORT_NO_LISTEN;
    m_CreateInfo.protocols = m_Protocols;
	 m_CreateInfo.ka_time				= 3;
	 m_CreateInfo.ka_probes				= 3;
	 m_CreateInfo.ka_interval 			= 1;
	 m_CreateInfo.keepalive_timeout	= 30;

    
//    m_CreateInfo.gid = -1;
//    m_CreateInfo.uid = -1;
    if (use_ssl)
    {
        m_CreateInfo.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        m_CreateInfo.ssl_cert_filepath = cert_path;
        m_CreateInfo.ssl_private_key_filepath = key_path;
    }
    //m_CreateInfo.max_http_header_pool = 20;
    //m_CreateInfo.user = this;

    if(pending_timeout_ms > 0)
        m_nPendingTimeout = pending_timeout_ms;
    else
        m_nPendingTimeout = 100;

	Log.d("Connect");
    memset(&m_ClientInfo, 0, sizeof(m_ClientInfo));
    m_pContext = lws_create_context(&m_CreateInfo);

    if (m_pContext == NULL)
    {
		Log.d("Context is NULL");
        if (szUrl)free(szUrl);
        return -1;
    }

    m_ClientInfo.context = m_pContext;
    m_ClientInfo.port = port;
    if(addr)
        m_ClientInfo.address = addr;

    m_ClientInfo.ssl_connection = use_ssl;
    m_ClientInfo.host = m_ClientInfo.address;
    m_ClientInfo.origin = m_ClientInfo.address;
    m_ClientInfo.ietf_version_or_minus_one = -1;
    m_ClientInfo.protocol = m_Protocols[0].name;

    if (!strcmp(proto, "http") || !strcmp(proto, "https"))
    {
        if (method)
            m_ClientInfo.method = method;
        else
            m_ClientInfo.method = "GET";
    }

    m_ClientInfo.protocol = m_Protocols[0].name;

    int pathlen = 0;
    if (path)pathlen = strlen(path);
    char* szPath = (char*)malloc(pathlen +2);
    szPath[0] = '/';
    szPath[1] = 0;
    if (path)strcpy(szPath + 1, path);
    m_ClientInfo.path = szPath;

    m_ClientInfo.pwsi = &m_pWsi;
    m_ClientInfo.userdata = this;

	 struct lws *cwsi;

    cwsi = lws_client_connect_via_info(&m_ClientInfo);

	 Log.d("lws_client_connect_via_info  %x : %x" , cwsi, m_pWsi); 
	 
    if (m_pWsi == NULL || cwsi == NULL)
    {
		Log.d("Wsi create error");
        if (szUrl)free(szUrl);
        if (szPath)free(szPath);
        return -1;
    }

	Log.d("Wsi create success 0x%X" , m_pWsi);

    m_bDisconnectFlag = 0;
    DWORD dwThreadId = 0;
    m_hPendingThread = CreateThread(0, 0, PendingThread, this, 0, &dwThreadId);

    if (szUrl)free(szUrl);
    if (szPath)free(szPath);

    m_bConnected = 1;

//  lws_set_log_level(LLL_DEBUG, NULL);

    return 0;
}

int CWSClientWrapper::cbProc(void* _inst, int reason, void *in, size_t len)
{


    CWSClientWrapper* inst = (CWSClientWrapper*)_inst;

	 if (!(reason == 34 || reason == 35 || reason == 36 || reason == 10)) 
			 inst->Log.d("CWSClientWrapper::cbProc reason %d %d",reason, len); 

    if (0x3A == reason)//_inst is not CWSClientWrapper* instance, don't use
        return 1;

    switch (reason)
    {
        case LWS_CALLBACK_PROTOCOL_INIT:
        {
            inst->Log.d("LWS_CALLBACK_PROTOCOL_INIT");
            break;
        }
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        {
            inst->AddEvent(WEBSOCKWRAP_ERROR, NULL, 0);	
            if(inst->m_pWsi)
                lws_cancel_service(lws_get_context(inst->m_pWsi));
            inst->m_pWsi = NULL;
            inst->m_bConnected = 0;
            inst->Log.d("LWS_CALLBACK_CLIENT_CONNECTION_ERROR - %s", (char*)in);
            break;
        }

        case LWS_CALLBACK_CLIENT_WRITEABLE:
        {
            //inst->Log.d("LWS_CALLBACK_CLIENT_WRITEABLE");
            send_data* vdata = NULL;
            if (inst->GetSendData(&vdata))
            {
                if (vdata->buffer && vdata->size)
                {
					int n = 0;
					if ((4 == vdata->size) && (0 == memcmp(vdata->buffer, "PING", 4)))
					{
						uint8_t ping[LWS_PRE + 125];
						int m = lws_snprintf((char *)ping + LWS_PRE, 125, "ping body");
						//inst->Log.d("Sending PING %d...\n", m);
						n = lws_write((lws*)inst->GetWsi(), ping + LWS_PRE, m, LWS_WRITE_PING);
						(m > n) ? inst->Log.d("sending ping failed: %d", n):inst->Log.d("sending ping succeeded: %d", n);
					}
					else
					{
						int len = sizeof(unsigned char)*(LWS_SEND_BUFFER_PRE_PADDING + vdata->size + LWS_SEND_BUFFER_POST_PADDING);
						unsigned char* out = (unsigned char*)malloc(len);
						memset(out, 0, len);
						memcpy(out + LWS_SEND_BUFFER_PRE_PADDING, vdata->buffer, vdata->size);
						n = lws_write((lws*)inst->GetWsi(), (unsigned char*)out + LWS_SEND_BUFFER_PRE_PADDING, vdata->size, LWS_WRITE_TEXT);
						inst->Log.d("lws_write ret=%d", n);
						free(out);
					}
					free(vdata->buffer);
					free(vdata);
                    inst->m_nSendRetCode = n;
                }
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE:
        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
        {
            lws* wsi = (lws*)inst->GetWsi();
            inst->AddEvent(WEBSOCKWRAP_RECEIVE, in, len);
			inst->m_tLastPongTime = time(NULL);
            return 0;
        }

        /* uninterpreted http content */
        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
        {
            char buffer[1024 + LWS_PRE];
            char *px = buffer + LWS_PRE;
            int lenx = sizeof(buffer) - LWS_PRE;
            if(!inst->m_pWsi)return -2;
            if (lws_http_client_read(inst->m_pWsi, &px, &lenx) < 0)
                return -1;
			inst->m_tLastPongTime = time(NULL);
        }
        return 0; /* don't passthru */

        case LWS_CALLBACK_CLIENT_ESTABLISHED:
        case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:// LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
        {
            if(!inst->m_pWsi)break;
            lws_callback_on_writable(inst->m_pWsi);
            inst->AddEvent(WEBSOCKWRAP_CONNECT, NULL, 0);
            inst->m_bConnected = 1;
			inst->m_tLastPongTime = time(NULL);
            break;
        }

        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
        {
            inst->AddEvent(WEBSOCKWRAP_DISCONNECT, NULL, 0);
            inst->m_bConnected = 0;
            if(inst->m_pWsi)
                lws_cancel_service(lws_get_context(inst->m_pWsi));
            inst->m_pWsi = NULL;
            break;
        }

		case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
		{
			inst->Log.d("LWS_CALLBACK_CLIENT_RECEIVE_PONG, len=%d", len);
			inst->m_tLastPongTime = time(NULL);
		}

        default:
            //inst->AddEvent(WEBSOCKWRAP_LWS_OTHER, NULL, 0);
            //inst->Log.d("WEBSOCKWRAP_LWS_OTHER reason = %d", reason);
            break;
    }
    return 1;
}

int CWSClientWrapper::GetEvent(event_data** vdata)
{
    CAutoLock lock(&m_csEventsLock);
	if(0==m_Events.size())
		return 0;

    std::vector<event_data*>::iterator it = m_Events.begin();
    if (it != m_Events.end())
    {
        *vdata = *it;
        m_Events.erase(it);
        Log.d("GetEvent %d, size=%d", (*vdata)->reason, m_Events.size());
        return 1;
    }
    return 0;
}

int CWSClientWrapper::AddEvent(WEBSOCKWRAP_EVENT reason, void* in, int len)
{
    CAutoLock lock(&m_csEventsLock);

    if (m_Events.size() >= m_nMaxEventsSize)
        return 0;

    event_data* vdata = (event_data*)malloc(sizeof(event_data));
    vdata->reason = reason;

    if (in && len)
    {
        vdata->buffer = (char*)malloc(len);
        memcpy(vdata->buffer, in, len);
        vdata->size = len;
    }
    else
    {
        vdata->buffer = NULL;
        vdata->size = 0;
    }

    m_Events.push_back(vdata);

    Log.d("AddEvent %d, size=%d", reason, m_Events.size());

    return 1;
}

int CWSClientWrapper::GetEventsCount()
{
    CAutoLock lock(&m_csEventsLock);
    return m_Events.size();
}

int CWSClientWrapper::PurgeEvents()
{
    CAutoLock lock(&m_csEventsLock);
    while (m_Events.size() > 0)
    {
        std::vector<event_data*>::iterator it = m_Events.begin();
        event_data* vdata = *it;
        if (vdata->buffer)
            free(vdata->buffer);
        free(vdata);
        m_Events.erase(it);

    }
    return 1;
}

int CWSClientWrapper::GetSendData(send_data** vdata)
{
    CAutoLock lock(&m_csSendDataLock);
    if (m_SendData.size() > 0)
    {
        std::vector<send_data*>::iterator it = m_SendData.begin();
        if (it != m_SendData.end())
        {
            *vdata = *it;
            m_SendData.erase(it);
            return 1;
        }
    }
    return 0;
}

int CWSClientWrapper::AddSendData(const char* in, int len)
{
    CAutoLock lock(&m_csSendDataLock);

    if (m_SendData.size() >= m_nMaxSendDataSize)
    {
        Log.d("AddSendData failed, m_nMaxSendDataSize=%d reached", m_nMaxSendDataSize);
        return 0;
    }

    send_data* vdata = (send_data*)malloc(sizeof(send_data));

    if (in && len)
    {
        vdata->buffer = (char*)malloc(len);
        memcpy(vdata->buffer, in, len);
        vdata->size = len;
        m_SendData.push_back(vdata);
        //Log.d("AddSendData %s", in);
        return 1;
    }
    return 0;
}

int CWSClientWrapper::GetSendDataCount()
{
    CAutoLock lock(&m_csSendDataLock);
    return m_SendData.size();
}

int CWSClientWrapper::PurgeSendData()
{
    CAutoLock lock(&m_csSendDataLock);
    while (m_SendData.size() > 0)
    {
        vector<send_data*>::iterator it = m_SendData.begin();
        send_data* vdata = *it;
        if (vdata->buffer)
            free(vdata->buffer);
        free(vdata);
        m_SendData.erase(it);

    }
    return 1;
}

int CWSClientWrapper::WriteBack(const char *str, size_t size_in)
{
    if (!str)
        return -1;

    int nTimeToWait = 10000;
    int nSleepPeriod = 100;

    while (nTimeToWait>0 && !isConnected())
    {
        if (m_bDisconnectFlag)break;
        Sleep(nSleepPeriod);
        nTimeToWait -= nSleepPeriod;
    }

    if (!isConnected())
    {
        Log.d("WriteBack failed, not connected");
        return -1;
    }

    Log.d("WriteBack %s", str);

    int len = 0;
    if (size_in < 1)
        len = strlen(str);
    else
        len = size_in;

    if (!AddSendData(str, len))
    {
        return 0;
    }

    int nCount = 0;
    m_nSendRetCode = 0;
    while(m_nSendRetCode == 0)//wait for lws_write
    {
        Sleep(10);
        nCount++;
        if (nCount > 1000 || m_bDisconnectFlag)
            break;
    }

    Log.d("WriteBack ret=%d", m_nSendRetCode);

    int ret = m_nSendRetCode;
    m_nSendRetCode = 0;
    return ret;
}

int CWSClientWrapper::Ping()
{
	int nTimeToWait = 10000;
	int nSleepPeriod = 100;

	while (nTimeToWait > 0 && !isConnected())
	{
		if (m_bDisconnectFlag)break;
		Sleep(nSleepPeriod);
		nTimeToWait -= nSleepPeriod;
	}

	if (!isConnected())
	{
		Log.d("Ping failed, not connected");
		return -1;
	}

	//Log.d("%s", __FUNCTION__);

	if (!AddSendData("PING", 4))
	{
		return -2;
	}

	int nCount = 0;
	m_nSendRetCode = 0;
	while (m_nSendRetCode == 0)//wait for lws_write
	{
		Sleep(10);
		nCount++;
		if (nCount > 1000 || m_bDisconnectFlag)
			break;
	}

	//Log.d("%s ret=%d", __FUNCTION__, m_nSendRetCode);

	int ret = m_nSendRetCode;
	m_nSendRetCode = 0;
	return ret;

}
