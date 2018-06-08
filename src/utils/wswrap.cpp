
//#include "stdafx.h"
#include "wswrap.h"
#include "windefsws.h"

#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)

#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "../../Debug/websockets_static.lib")
#pragma comment(lib, "../../Debug/libsslMTd.lib")
#pragma comment(lib, "../../Debug/libcryptoMTd.lib")
#else
#pragma comment(lib, "../../Release/websockets_static.lib")
#pragma comment(lib, "../../Release/libsslMT.lib")
#pragma comment(lib, "../../Release/libcryptoMT.lib")
#endif //DEBUG
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
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
	:Log(TAG, LOG_LEVEL)
{
    m_OnEvent = OnEvent;
	m_UserData = userData;

	Log.v("=>CWSClientWrapper");
    m_bDisconnectFlag  = 0;
    m_bConnected    = 0;
    m_hPendingThread= NULL;
    m_hEventsThread = NULL;
    m_pWsi = NULL;
    m_nMaxEventsSize = 50;
    m_nMaxSendDataSize = 50;
    m_SendData.clear();
    m_Events.clear();
    m_nSendRetCode = 0;
    m_bDestroyFlag = 0;

#ifdef _DEBUG
    //lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_DEBUG, NULL);
    lws_set_log_level(LLL_ERR | LLL_WARN, NULL);
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
    //logprintf(DEBUG_LEVEL_DEBUG, "Destructor\n");
    Disconnect();

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
            Log.v("m_hEventsThread stop failed!!!");

        //TerminateThread(m_hEventsThread, 0);
        m_hEventsThread = NULL;
    }

	Log.v("<=CWSClientWrapper");
}

int CWSClientWrapper::Disconnect()
{
    //logprintf(DEBUG_LEVEL_DEBUG, "Disconnect\n");
	Log.v("=>Disconnect");

    m_bDisconnectFlag = 1;

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
            Log.v("m_hPendingThread stop failed!!!");

        //TerminateThread(m_hPendingThread, 0);
        m_hPendingThread = NULL;
    }

    if (m_pContext)
    {
        lws_cancel_service(m_pContext);
        lws_context_destroy(m_pContext);
    }

    if(!m_pWsi && m_OnEvent)
        m_OnEvent(m_UserData, WEBSOCKWRAP_DISCONNECT, NULL, 0);

    PurgeEvents();
    PurgeSendData();

    m_bConnected = 0;
    m_pContext = NULL;

	Log.v("<=Disconnect");
	return 0;
}

#ifdef _WIN32
DWORD WINAPI EventsThread(LPVOID pParam)
#else
void* EventsThread(LPVOID pParam)
#endif
{
    CWSClientWrapper* pWrapper = (CWSClientWrapper*)pParam;
    pWrapper->Log.v("=>EventsThread");

    while (!pWrapper->m_bDestroyFlag)
    {
        event_data* vdata = NULL;
        WSWRAP_CB_PROC evCb = pWrapper->GetEventsCb();
        if (pWrapper->GetEvent(&vdata))
        {
            if (evCb)
                evCb(pWrapper->GetUserData(), vdata->reason, vdata->buffer, vdata->size);

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
    pWrapper->m_hEventsThread = NULL;
    pWrapper->Log.v("<=EventsThread");
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
	pWrapper->Log.v("=>PendingThread");

	while (!pWrapper->m_bDisconnectFlag && pWrapper->GetWsi())
    {
        if (pWrapper->isConnected())
        {
            //CAutoLock lock(&pWrapper->m_csProcLock);
            lws_context* ctx = (lws_context*)pWrapper->GetContext();
            lws_service(ctx, pWrapper->m_nPendingTimeout);
            lws_callback_on_writable_all_protocol(ctx, (lws_protocols*)(pWrapper->GetProtocols()));
            //lws_callback_on_writable((lws*)pWrapper->GetWsi());
        }
        Sleep(10);
    }
    pWrapper->m_hPendingThread = NULL;
	pWrapper->Log.v("<=PendingThread");

    return 0;
}

int CWSClientWrapper::Connect(const char* url, const char* method, int pending_timeout_ms, const char* cert_path, const char* key_path)
{

    const char* proto = NULL;
    const char* addr = NULL;
    const char* path = NULL;
    int port = 80;
    char* szUrl = NULL;

    if (url && strlen(url))
    {
        szUrl = (char*)malloc(strlen(url) +1);
        strcpy(szUrl, url);
        lws_parse_uri(szUrl, &proto, &addr, &port, &path);
        Log.v("Url proto=%s, addr=%s, port=%d, path=%s\n ", proto, addr, port, path);
    }
    else
    {
		Log.v("Invalid url\n ");
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
	Log.v("use_ssl=%d\n", use_ssl);

    m_Protocols[0].name = m_szProto;
    m_Protocols[0].callback = &ws_callback;
    m_Protocols[0].user = this;

    memset(&m_CreateInfo, 0, sizeof m_CreateInfo);
    m_CreateInfo.port = CONTEXT_PORT_NO_LISTEN;
    m_CreateInfo.protocols = m_Protocols;
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

	Log.v("Connect\n");
    memset(&m_ClientInfo, 0, sizeof(m_ClientInfo));
    m_pContext = lws_create_context(&m_CreateInfo);

    if (m_pContext == NULL)
    {
		Log.v("Context is NULL\n");
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

    lws_client_connect_via_info(&m_ClientInfo);

    if (!m_pWsi)
    {
		Log.v("Wsi create error\n");
        if (szUrl)free(szUrl);
        if (szPath)free(szPath);
        return -1;
    }

	Log.v("Wsi create success 0x%X\n" , m_pWsi);

    m_bDisconnectFlag = 0;
    DWORD dwThreadId = 0;
    m_hPendingThread = CreateThread(0, 0, PendingThread, this, 0, &dwThreadId);

    if (szUrl)free(szUrl);
    if (szPath)free(szPath);

    m_bConnected = 1;

    return 0;
}

int CWSClientWrapper::cbProc(void* _inst, int reason, void *in, size_t len)
{

    if (0x3A == reason)//_inst is not CWSClientWrapper* instance, don't use
        return 1;

    CWSClientWrapper* inst = (CWSClientWrapper*)_inst;

    switch (reason)
    {
        case LWS_CALLBACK_PROTOCOL_INIT:
        {
            inst->Log.v("LWS_CALLBACK_PROTOCOL_INIT\n");
            break;
        }
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        {
            inst->AddEvent(WEBSOCKWRAP_ERROR, NULL, 0);
            lws_cancel_service(lws_get_context(inst->m_pWsi));
            inst->m_pWsi = NULL;
            inst->m_bConnected = 0;
            break;
        }

        case LWS_CALLBACK_CLIENT_WRITEABLE:
        {
            //inst->Log.v("LWS_CALLBACK_CLIENT_WRITEABLE\n");
            send_data* vdata = NULL;
            if (inst->GetSendData(&vdata))
            {
                if (vdata->buffer && vdata->size)
                {
                    int len = sizeof(unsigned char)*(LWS_SEND_BUFFER_PRE_PADDING + vdata->size + LWS_SEND_BUFFER_POST_PADDING);
                    unsigned char* out = (unsigned char*)malloc(len);
                    memset(out, 0, len);
                    memcpy(out + LWS_SEND_BUFFER_PRE_PADDING, vdata->buffer, vdata->size);
                    int n = lws_write((lws*)inst->GetWsi(), (unsigned char*)out + LWS_SEND_BUFFER_PRE_PADDING, vdata->size, LWS_WRITE_TEXT);
                    inst->Log.v("lws_write ret=%d\n", n);
                    free(vdata->buffer);
                    free(vdata);
                    free(out);
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
            return 0;
        }

        /* uninterpreted http content */
        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
        {
            char buffer[1024 + LWS_PRE];
            char *px = buffer + LWS_PRE;
            int lenx = sizeof(buffer) - LWS_PRE;
            if (lws_http_client_read(inst->m_pWsi, &px, &lenx) < 0)
                return -1;
        }
        return 0; /* don't passthru */

        case LWS_CALLBACK_CLIENT_ESTABLISHED:
        case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:// LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
        {
            lws_callback_on_writable(inst->m_pWsi);
            inst->AddEvent(WEBSOCKWRAP_CONNECT, NULL, 0);
            inst->m_bConnected = 1;
            break;
        }

        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
        {
            inst->AddEvent(WEBSOCKWRAP_DISCONNECT, NULL, 0);
            inst->m_bConnected = 0;
            lws_cancel_service(lws_get_context(inst->m_pWsi));
            break;
        }
        default:
            //inst->AddEvent(WEBSOCKWRAP_LWS_OTHER, NULL, 0);
            //inst->Log.v("WEBSOCKWRAP_LWS_OTHER reason = %d\n", reason);
            break;
    }
    return 1;
}

int CWSClientWrapper::GetEvent(event_data** vdata)
{
    CAutoLock lock(&m_csEventsLock);
    std::vector<event_data*>::iterator it = m_Events.begin();
    if (it != m_Events.end())
    {
        *vdata = *it;
        m_Events.erase(it);
        Log.v("GetEvent %d, size=%d\n", (*vdata)->reason, m_Events.size());
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

    Log.v("AddEvent %d, size=%d\n", reason, m_Events.size());

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
        Log.v("AddSendData failed, m_nMaxSendDataSize=%d reached\n", m_nMaxSendDataSize);
        return 0;
    }

    send_data* vdata = (send_data*)malloc(sizeof(send_data));

    if (in && len)
    {
        vdata->buffer = (char*)malloc(len);
        memcpy(vdata->buffer, in, len);
        vdata->size = len;
        m_SendData.push_back(vdata);
        //Log.v("AddSendData %s\n", in);
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
        Log.v("WriteBack failed, not connected\n", str);
        return 0;
    }

    Log.v("WriteBack %s\n", str);

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

    Log.v("WriteBack ret=%d\n", m_nSendRetCode);

    int ret = m_nSendRetCode;
    m_nSendRetCode = 0;
    return ret;
}
