/*
 * =====================================================================================
 *
 *       Filename:  NetWorkThread.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/06/2018 03:22:03 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include <netdb.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "BlockEngine/BlockEngine.h"
#include "DAO/CWalletDB.h"
#include "WalletService/CWalletTx.h"
#include "WalletService/WalletServ.h"
#include "NetWorkServ.h"
using namespace Enze;

void* ThreadMessageHandler(void* parg);
void* ThreadSocketHandler(void* parg);
void* ThreadOpenConnections(void* parg);
extern vector<pthread_t> gThreadList;

//
// Global state variables
//
bool fShutdown = false; // ����Ƿ�ر�
boost::array<bool, 10> vfThreadRunning; // �߳�����״̬��ǣ�0������ʾsockethandler��1������ʾopenConnection��2������ʾ������Ϣ

// �����ڵ�
bool NetWorkServ::StartNode()
{
    string strError = "";
    // Get local host ip
    char pszHostName[255]= {0};
    if (gethostname(pszHostName, 255) == SOCKET_ERROR)
    {
        strError = strprintf("Error: Unable to get IP address of this computer (gethostname returned error %d)", errno);
        printf("%s\n", strError.c_str());
        return false;
    }
    struct hostent* phostent = gethostbyname(pszHostName);
    if (!phostent)
    {
        strError = strprintf("Error: Unable to get IP address of this computer (gethostbyname returned error %d)", errno);
        printf("%s\n", strError.c_str());
        return false;
    }
    m_cAddrLocalHost = CAddress(*(long*)(phostent->h_addr_list[0]),
                             DEFAULT_PORT,
                             m_nLocalServices);

    printf("m_cAddrLocalHost = %s\n", m_cAddrLocalHost.ToString().c_str());

    // Create socket for listening for incoming connections
    SOCKET hListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hListenSocket == INVALID_SOCKET)
    {
        strError = strprintf("Error: Couldn't open socket for incoming connections (socket returned error %d)", errno);
        printf("%s\n", strError.c_str());
        return false;
    }

    // Set to nonblocking, incoming connections will also inherit this
    int flag = 0;
    flag = fcntl(hListenSocket, F_GETFL, NULL);
    fcntl(hListenSocket, F_SETFL, flag|O_NONBLOCK);

    // Reuse Addr
    int opt = 1;
    socklen_t opt_len = sizeof(opt);
    setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, opt_len);
    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound
    int nRetryLimit = 15;
    struct sockaddr_in sockaddr = m_cAddrLocalHost.GetSockAddr();
    if (bind(hListenSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
    {
        int nErr = errno;
        if (nErr == EADDRINUSE)
            strError = strprintf("Error: Unable to bind to port %s on this computer. The program is probably already running.", m_cAddrLocalHost.ToString().c_str());
        else
            strError = strprintf("Error: Unable to bind to port %s on this computer (bind returned error %d)", m_cAddrLocalHost.ToString().c_str(), nErr);
        printf("%s\n", strError.c_str());
        return false;
    }
    printf("bound to m_cAddrLocalHost = %s\n\n", m_cAddrLocalHost.ToString().c_str());

    // Listen for incoming connections
    if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf("Error: Listening for incoming connections failed (listen returned error %d)", errno);
        printf("%s\n", strError.c_str());
        return false;
    }
    CAddress& addrIncoming = WalletServ::getInstance()->addrIncoming;
    // Get our external IP address for incoming connections
    if (addrIncoming.ip)
        m_cAddrLocalHost.ip = addrIncoming.ip;
    if (GetMyExternalIP(m_cAddrLocalHost.ip))
    {
        addrIncoming = m_cAddrLocalHost;
        //CWalletDB().WriteSetting("addrIncoming", addrIncoming);
    }

    /*IRC��Internet Relay Chat ��Ӣ����д������һ���Ϊ�������м����졣
	�����ɷ�����Jarkko Oikarinen��1988���״���һ����������Э�顣
	����ʮ��ķ�չ��Ŀǰ�������г���60�������ṩ��IRC�ķ���IRC�Ĺ���ԭ��ǳ��򵥣�
	��ֻҪ���Լ���PC�����пͻ��������Ȼ��ͨ����������IRCЭ�����ӵ�һ̨IRC�������ϼ��ɡ�
	�����ص����ٶȷǳ�֮�죬����ʱ����û���ӳٵ����󣬲���ֻռ�ú�С�Ĵ�����Դ��
	�����û�������һ������Ϊ\"Channel\"��Ƶ�����ĵط���ĳһ������н�̸����̸��
	ÿ��IRC��ʹ���߶���һ��Nickname���ǳƣ���
	IRC�û�ʹ���ض����û�������������ӵ�IRC��������
	ͨ���������м����������ӵ���һ�������ϵ��û�������
	����IRC��������Ϊ���������м����족��
	*/
    pthread_t pid = 0;
#if 0
    // Get addresses from IRC and advertise ours
    if (pthread_create(&pid, NULL, ThreadIRCSeed, NULL) == -1)
        printf("Error: pthread_create(ThreadIRCSeed) failed\n");
    gThreadList.push_back(pid);
#endif
	// �����߳�
    //
    // Start threads
    //
    
    if (pthread_create(&pid, NULL, ThreadSocketHandler, (void*)new SOCKET(hListenSocket)) == -1)
    {
        strError = "Error: pthread_create(ThreadSocketHandler) failed";
        printf("%s\n", strError.c_str());
        return false;
    }
    gThreadList.push_back(pid);
    
    if (pthread_create(&pid, NULL, ThreadMessageHandler, NULL) == -1)
    {
        strError = "Error: pthread_create(ThreadMessageHandler) failed";
        printf("%s\n", strError.c_str());
        return false;
    } 
    gThreadList.push_back(pid);
    
    if (pthread_create(&pid, NULL, ThreadOpenConnections, NULL) == -1)
    {
        strError = "Error: pthread_create(ThreadOpenConnections) failed";
        printf("%s\n", strError.c_str());
        return false;
    }
    gThreadList.push_back(pid);
    return true;
}

bool NetWorkServ::StopNode()
{
    printf("StopNode()\n");
    fShutdown = true;
    WalletServ::getInstance()->nTransactionsUpdated++;
    int64 nStart = GetTime();
    while (vfThreadRunning[0] || vfThreadRunning[2] || vfThreadRunning[3])
    {
        if (GetTime() - nStart > 15)
            break;
        sleep(20);
    }
    if (vfThreadRunning[0]) printf("ThreadSocketHandler still running\n");
    if (vfThreadRunning[1]) printf("ThreadOpenConnections still running\n");
    if (vfThreadRunning[2]) printf("ThreadMessageHandler still running\n");
    if (vfThreadRunning[3]) printf("ThreadBitcoinMiner still running\n");
    while (vfThreadRunning[2])
        sleep(20);
    sleep(50);

    // Sockets shutdown
 //   WSACleanup();
    return true;
}

void NetWorkServ::CheckForShutdown(int n)
{
    if (fShutdown)
    {
        if (n != -1)
            vfThreadRunning[n] = false;
        if (n == 0)
            foreach(CNode* pnode, m_cNodeLst)
                close(pnode->hSocket);
    }
}

// socket ����parag��Ӧ���Ǳ��ؽڵ㿪���ļ���socket
void* ThreadSocketHandler(void* parg)
{
//  IMPLEMENT_RANDOMIZE_STACK(ThreadSocketHandler(parg));

    loop
    {
        vfThreadRunning[0] = true;
        //CheckForShutdown(0);
        try
        {
            NetWorkServ::getInstance()->SocketHandler(parg);
        }
        CATCH_PRINT_EXCEPTION("ThreadSocketHandler()")
        vfThreadRunning[0] = false;
        sleep(5000);
    }
}
// socket ����parag��Ӧ���Ǳ��ؽڵ㿪���ļ���socket
void NetWorkServ::SocketHandler(void* parg)
{
    printf("ThreadSocketHandler started\n");
    SOCKET hListenSocket = *(SOCKET*)parg; // ��ü���socket
    list<CNode*> m_cNodeLstDisconnected;
    int nPrevNodeCount = 0;

    loop
    {
        // Disconnect nodes
        {
            // Disconnect duplicate connections �ͷ�ͬһ��ip�ظ����Ӷ�Ӧ�Ľڵ㣬�����ǲ�ͬ�˿�
            map<unsigned int, CNode*> mapFirst;
            foreach(CNode* pnode, m_cNodeLst)
            {
                if (pnode->fDisconnect)
                    continue;
                unsigned int ip = pnode->addr.ip;
				// ��������ip��ַ��Ӧ����0���������е�ip��ַ��Ӧ�ô������ip
                if (mapFirst.count(ip) && m_cAddrLocalHost.ip < ip)
                {
                    // In case two nodes connect to each other at once,
                    // the lower ip disconnects its outbound connection
                    CNode* pnodeExtra = mapFirst[ip];

                    if (pnodeExtra->GetRefCount() > (pnodeExtra->fNetworkNode ? 1 : 0))
                        printf("error %s_%d\n", __FILE__, __LINE__);
                        //swap(pnodeExtra, pnode);

                    if (pnodeExtra->GetRefCount() <= (pnodeExtra->fNetworkNode ? 1 : 0))
                    {
                        printf("(%d nodes) disconnecting duplicate: %s\n", m_cNodeLst.size(), pnodeExtra->addr.ToString().c_str());
                        if (pnodeExtra->fNetworkNode && !pnode->fNetworkNode)
                        {
                            pnode->AddRef();
                            printf("error %s_%d\n", __FILE__, __LINE__);
                            //swap(pnodeExtra->fNetworkNode, pnode->fNetworkNode);
                            pnodeExtra->Release();
                        }
                        pnodeExtra->fDisconnect = true;
                    }
                }
                mapFirst[ip] = pnode;
            }
			// �Ͽ���ʹ�õĽڵ�
            // Disconnect unused nodes
            vector<CNode*> m_cNodeLstCopy = m_cNodeLst;
            foreach(CNode* pnode, m_cNodeLstCopy)
            {
				// �ڵ�׼���ͷ����ӣ����Ҷ�Ӧ�Ľ��պͷ��ͻ��������ǿ�
                if (pnode->ReadyToDisconnect() && pnode->vRecv.empty() && pnode->vSend.empty())
                {
					// �ӽڵ��б����Ƴ�
                    // remove from m_cNodeLst
                    m_cNodeLst.erase(remove(m_cNodeLst.begin(), m_cNodeLst.end(), pnode), m_cNodeLst.end());
                    pnode->Disconnect();

					// ����Ӧ׼���ͷŵĽڵ���ڶ�Ӧ�Ľڵ��ͷ����ӳ��У��ȴ���Ӧ�ڵ�����������ͷ�
                    // hold in disconnected pool until all refs are released
                    pnode->nReleaseTime = max(pnode->nReleaseTime, GetTime() + 5 * 60); // ����Ƴ�5����
                    if (pnode->fNetworkNode)
                        pnode->Release();
                    m_cNodeLstDisconnected.push_back(pnode);
                }
            }

			// ɾ���˿ڵ����ӵĽڵ㣺ɾ���������Ƕ�Ӧ�ڵ������С�ڵ���0
            // Delete disconnected nodes
            list<CNode*> m_cNodeLstDisconnectedCopy = m_cNodeLstDisconnected;
            foreach(CNode* pnode, m_cNodeLstDisconnectedCopy)
            {
                // wait until threads are done using it
                if (pnode->GetRefCount() <= 0)
                {
                    bool fDelete = false;
       //             //TRY_CRITICAL_BLOCK(pnode->cs_vSend)
       //              //TRY_CRITICAL_BLOCK(pnode->cs_vRecv)
       //               //TRY_CRITICAL_BLOCK(pnode->cs_mapRequests)
       //                //TRY_CRITICAL_BLOCK(pnode->cs_inventory)
                        fDelete = true;
                    if (fDelete)
                    {
                        m_cNodeLstDisconnected.remove(pnode);
                        delete pnode;
                    }
                }
            }
        }
        if (m_cNodeLst.size() != nPrevNodeCount)
        {
            nPrevNodeCount = m_cNodeLst.size(); // ��¼ǰһ�νڵ��Ӧ������
            //MainFrameRepaint();
        }


        // �ҳ���һ��socket������Ҫ����
        // Find which sockets have data to receive
        //
        struct timeval timeout;
        timeout.tv_sec  = 0;
        timeout.tv_usec = 50000; // frequency to poll pnode->vSend ��ѯ�ڵ��Ƿ�������Ҫ���͵�Ƶ��

        fd_set fdsetRecv; // ��¼���нڵ��Ӧ��socket����ͼ���socket���
        fd_set fdsetSend; // ��¼�����д�������Ϣ�Ľڵ��Ӧ��socket���
        FD_ZERO(&fdsetRecv);
        FD_ZERO(&fdsetSend);
        SOCKET hSocketMax = 0;
        FD_SET(hListenSocket, &fdsetRecv); // FD_SET��hListenSocket ����fdsetRecv��Ӧ����������
        hSocketMax = max(hSocketMax, hListenSocket);
        //CRITICAL_BLOCK(cs_m_cNodeLst)
        {
            foreach(CNode* pnode, m_cNodeLst)
            {
                FD_SET(pnode->hSocket, &fdsetRecv);
                hSocketMax = max(hSocketMax, pnode->hSocket); // �ҳ����нڵ��Ӧ��socket�����ֵ����������socket
  //              TRY_CRITICAL_BLOCK(pnode->cs_vSend)
                    if (!pnode->vSend.empty())
                        FD_SET(pnode->hSocket, &fdsetSend); // FD_SET �ֶ�����
            }
        }

        vfThreadRunning[0] = false;
		// �����ο���https://blog.csdn.net/rootusers/article/details/43604729
		/*ȷ��һ�������׽ӿڵ�״̬������������ȷ��һ�������׽ӿڵ�״̬����ÿһ���׽ӿڣ������߿ɲ�ѯ���Ŀɶ��ԡ���д�Լ�����״̬��Ϣ����fd_set�ṹ����ʾһ��ȴ������׽ӿڣ��ڵ��÷���ʱ������ṹ��������һ���������׽ӿ�����Ӽ�������select()���������������׽ӿڵ���Ŀ��
			����˵select�������һ����õ�socket���������������֮һ����ʱ��
			1.���Զ�ȡ��sockets������Щsocket������ʱ������Щsocket��ִ��recv/accept�Ȳ��������������;
			2.����д���sockets������Щsocket������ʱ������Щsocket��ִ��send�Ȳ����������;
			3.�����д����sockets��
			select()�Ļ������ṩһfd_set�����ݽṹ��ʵ������һlong���͵����飬ÿһ������Ԫ�ض�����һ�򿪵��ļ����(������������socket������ļ������ܵ���)������ϵ��������ϵ�Ĺ���ʵ�����ɳ���Ա��ɣ�������select()��ʱ�����ں˸���IO״̬�޸�fd_set�����ݣ��ɴ���ִ֪ͨ����select()�Ľ�����һsocket���ļ��ɶ���
		*/
        int nSelect = select(hSocketMax + 1, &fdsetRecv, &fdsetSend, NULL, &timeout);
        vfThreadRunning[0] = true;
        CheckForShutdown(0);
        if (nSelect == SOCKET_ERROR)
        {
            printf("select failed: %m\n");
            for (int i = 0; i <= hSocketMax; i++)
            {
                FD_SET(i, &fdsetRecv); // ���е�ֵ����һ��
                FD_SET(i, &fdsetSend);
            }
            sleep(timeout.tv_usec/1000);
        }
		// ����������ӣ����ܼ���
        RandAddSeed();

        //// debug print
    /*
        foreach(CNode* pnode, m_cNodeLst)
        {
            printf("vRecv = %-5d ", pnode->vRecv.size());
            printf("vSend = %-5d    ", pnode->vSend.size());
        }
        printf("\n");
    */
        // ���fdsetRecv���м���socket������ոļ���socket��Ӧ���������󣬲���������������Ϊ�µĽڵ�
        // Accept new connections
        // �жϷ��ͻ��������Ƿ��ж�Ӧ��socket�������������µĽ���
        if (FD_ISSET(hListenSocket, &fdsetRecv))
        {
            struct sockaddr_in sockaddr;
            socklen_t len = sizeof(sockaddr);
            SOCKET hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
            CAddress addr(sockaddr);
            if (hSocket == INVALID_SOCKET)
            {
                //if (errno != EWOULDBLOCK)
                  if (errno == EWOULDBLOCK)
                    printf("ERROR ThreadSocketHandler accept failed: %m\n");
            }
            else
            {
                printf("accepted connection from %s\n", addr.ToString().c_str());
                CNode* pnode = new CNode(hSocket, addr, true); // ���µ�socket���ӣ����½���Ӧ�Ľڵ㣬�����ڵ��ڼ��뱾�ؽڵ��б���
                pnode->AddRef();
                m_cNodeLst.push_back(pnode);
            }
        }


        // ��ÿһ��socket���з�����
        // Service each socket
        //
        vector<CNode*> m_cNodeLstCopy;
        m_cNodeLstCopy = m_cNodeLst;
        foreach(CNode* pnode, m_cNodeLstCopy)
        {
            CheckForShutdown(0);
            SOCKET hSocket = pnode->hSocket; // ��ȡÿһ���ڵ��Ӧ��socket

            // �ӽڵ��Ӧ��socket�ж�ȡ��Ӧ�����ݣ������ݷ���ڵ�Ľ��ջ�������
            // Receive
            //
            if (FD_ISSET(hSocket, &fdsetRecv))
            {
                printf("error %s_%d\n", __FILE__, __LINE__);
#if 0
                CDataStream& vRecv = pnode->vRecv;
                unsigned int nPos = vRecv.size();

                const unsigned int nBufSize = 0x10000;
                vRecv.resize(nPos + nBufSize);// �������ջ������Ĵ�С
                int nBytes = recv(hSocket, &vRecv[nPos], nBufSize, MSG_DONTWAIT);// ��socket�н��ն�Ӧ������
                vRecv.resize(nPos + max(nBytes, 0));
                if (nBytes == 0)
                {
                    // socket closed gracefully ��socket���ŵĹرգ�
                    if (!pnode->fDisconnect)
                        printf("recv: socket closed\n");
                    pnode->fDisconnect = true;
                }
                else if (nBytes < 0)
                {
                    // socket error
                    int nErr = errno;
                    if (nErr != EWOULDBLOCK && nErr != EAGAIN && nErr != EINTR && nErr != EINPROGRESS)
                    {
                        if (!pnode->fDisconnect)
                            printf("recv failed: %d\n", nErr);
                        pnode->fDisconnect = true;
                    }
                }
#endif
            }

            // ���ڵ��Ӧ�ķ��ͻ����е����ݷ��ͳ�ȥ
            // Send
            //
            if (FD_ISSET(hSocket, &fdsetSend))
            {
                printf("error %s_%d\n", __FILE__, __LINE__);
#if 0
                {
                    CDataStream& vSend = pnode->vSend;
                    if (!vSend.empty())
                    {
                        int nBytes = send(hSocket, &vSend[0], vSend.size(), 0); // �ӽڵ��Ӧ�ķ��ͻ������з������ݳ�ȥ
                        if (nBytes > 0)
                        {
                            vSend.erase(vSend.begin(), vSend.begin() + nBytes);// �ӷ��ͻ��������Ƴ����͹�������
                        }
                        else if (nBytes == 0)
                        {
                            if (pnode->ReadyToDisconnect())
                                pnode->vSend.clear();
                        }
                        else
                        {
                            printf("send error %d\n", nBytes);
                            if (pnode->ReadyToDisconnect())
                                pnode->vSend.clear();
                        }
                    }
                }
#endif
            }

        }
        sleep(10);
    }
}




void* ThreadOpenConnections(void* parg)
{
//    IMPLEMENT_RANDOMIZE_STACK(ThreadOpenConnections(parg));

    loop
    {
        vfThreadRunning[1] = true;
        //CheckForShutdown(1);
        try
        {
            NetWorkServ::getInstance()->OpenConnections(parg);
        }
        CATCH_PRINT_EXCEPTION("ThreadOpenConnections()")
        vfThreadRunning[1] = false;
        sleep(5000);
    }
}
// ����ÿһ���򿪽ڵ�����ӣ����нڵ�֮����Ϣͨ�ţ���ýڵ��Ӧ��������Ϣ������ڵ��Ӧ��֪����ַ���н�����
void NetWorkServ::OpenConnections(void* parg)
{
    printf("ThreadOpenConnections started\n");

	// ��ʼ����������
    // Initiate network connections
    int nTry = 0;
    bool fIRCOnly = false;
    const int nMaxConnections = 15; // ���������
    loop
    {
        // Wait
        vfThreadRunning[1] = false;
        sleep(5);
        while (m_cNodeLst.size() >= nMaxConnections || m_cNodeLst.size() >= m_cMapAddresses.size())
        {
            CheckForShutdown(1);
            sleep(50);
        }
        vfThreadRunning[1] = true;
        CheckForShutdown(1);


		// Ip��Ӧ��C���ַ����ͬ��C���ַ����һ��
        fIRCOnly = !fIRCOnly;
        fIRCOnly = false;
#if 0
        if (mapIRCAddresses.empty())
            fIRCOnly = false;
        else if (nTry++ < 30 && m_cNodeLst.size() < nMaxConnections/2)
            fIRCOnly = true;
#endif
        // Make a list of unique class C's
        unsigned char pchIPCMask[4] = { 0xff, 0xff, 0xff, 0x00 };
        unsigned int nIPCMask = *(unsigned int*)pchIPCMask;
        vector<unsigned int> vIPC;
        //CRITICAL_BLOCK(cs_mapIRCAddresses)
        //CRITICAL_BLOCK(cs_m_cMapAddresses)
        {
            vIPC.reserve(m_cMapAddresses.size());
            unsigned int nPrev = 0;
			// mapAddress�Ѿ����������ˣ�Ĭ������Ч����
            foreach(const PAIRTYPE(string, CAddress)& item, m_cMapAddresses)
            {
                const CAddress& addr = item.second;
                if (!addr.IsIPv4())
                    continue;
#if 0
                if (fIRCOnly && !mapIRCAddresses.count(item.first))
                    continue;
#endif
                // Taking advantage of m_cMapAddresses being in sorted order,
                // with IPs of the same class C grouped together.
                unsigned int ipC = addr.ip & nIPCMask;
                if (ipC != nPrev)
                    vIPC.push_back(nPrev = ipC);
            }
        }
        if (vIPC.empty())
            continue;

        // IPѡ��Ĺ���
        // The IP selection process is designed to limit vulnerability������ to address flooding.
        // Any class C (a.b.c.?) has an equal chance of being chosen, then an IP is
        // chosen within the class C.  An attacker may be able to allocate many IPs, but
        // they would normally be concentrated in blocks of class C's.  They can hog��ռ the
        // attention within their class C, but not the whole IP address space overall.
        // A lone node in a class C will get as much attention as someone holding all 255
        // IPs in another class C.
        //
        bool fSuccess = false;
        int nLimit = vIPC.size();
        while (!fSuccess && nLimit-- > 0)
        {
            // Choose a random class C �����ȡһ��C����ĵ�ַ
            unsigned int ipC = vIPC[GetRand(vIPC.size())];

            // Organize all addresses in the class C by IP
            map<unsigned int, vector<CAddress> > mapIP;
            int64 nDelay = ((30 * 60) << m_cNodeLst.size());
            if (!fIRCOnly)
            {
                nDelay *= 2;
                if (m_cNodeLst.size() >= 3)
                    nDelay *= 4;
                
                //if (!mapIRCAddresses.empty())
                //    nDelay *= 100;  
            }
					
            /*
            map::lower_bound(key):����map�е�һ�����ڻ����key�ĵ�����ָ��
            map::upper_bound(key):����map�е�һ������key�ĵ�����ָ��
            */
            for (map<string, CAddress>::iterator mi = m_cMapAddresses.lower_bound(CAddress(ipC, 0).GetKey());
                 mi != m_cMapAddresses.upper_bound(CAddress(ipC | ~nIPCMask, 0xffff).GetKey());
                 ++mi)
            {
                const CAddress& addr = (*mi).second;
                //if (fIRCOnly && !mapIRCAddresses.count((*mi).first))
                //    continue;

                int64 nRandomizer = (addr.nLastFailed * addr.ip * 7777U) % 20000;
                // ��ǰʱ�� - ��ַ��������ʧ�ܵ�ʱ�� Ҫ���ڶ�Ӧ�ڵ������ļ��ʱ��
                if (GetTime() - addr.nLastFailed > nDelay * nRandomizer / 10000)
                    mapIP[addr.ip].push_back(addr); //ͬһ����ַ���β�ͬ��ַ�� ͬһ����ַ�Ĳ�ͬ�˿ڣ����ж�Ӧͬһ��ip���ж����ַ
            }
            
            if (mapIP.empty())
                break;

            // Choose a random IP in the class C
            map<unsigned int, vector<CAddress> >::iterator mi = mapIP.begin();
			//boost::iterators::advance_adl_barrier::advance(mi, GetRand(mapIP.size())); // ��ָ�붨λ�����λ��
			advance(mi, GetRand(mapIP.size())); // ��ָ�붨λ�����λ��

			// ����ͬһ��ip��Ӧ�����в�ͬ�˿�
            // Once we've chosen an IP, we'll try every given port before moving on
            foreach(const CAddress& addrConnect, (*mi).second)
            {
               // printf("OpenConnection,LocalIP[%s], addrConnect_ip[%s]\n", m_cAddrLocalHost.ToStringIP().c_str(), addrConnect.ToStringIP().c_str());
				// ip�����Ǳ���ip���Ҳ����Ƿ�ipV4��ַ����Ӧ��ip��ַ���ڱ��صĽڵ��б���
                CheckForShutdown(1);
                if (addrConnect.ip == m_cAddrLocalHost.ip || !addrConnect.IsIPv4() || FindNode(addrConnect.ip))
                    continue;
				// ���Ӷ�Ӧ��ַ��Ϣ�Ľڵ�
                vfThreadRunning[1] = false;
                CNode* pnode = ConnectNode(addrConnect);
                vfThreadRunning[1] = true;
                CheckForShutdown(1);
                if (!pnode)
                    continue;
                pnode->fNetworkNode = true;

				// �������������ַ�ܹ�����·�ɣ�����Ҫ�㲥���ǵĵ�ַ
                if (m_cAddrLocalHost.IsRoutable())
                {
                    // Advertise our address
                    vector<CAddress> vAddrToSend;
                    vAddrToSend.push_back(m_cAddrLocalHost);
                    pnode->PushMessage("addr", vAddrToSend); // ����Ϣ���ͳ�ȥ����vsend�У�����Ϣ�����߳��н��д���
                }

				// �Ӵ����Ľڵ��þ����ܶ�ĵ�ַ��Ϣ��������Ϣ������Ϣ�����߳��н��д���
                // Get as many addresses as we can
                pnode->PushMessage("getaddr");

                ////// should the one on the receiving end do this too?
                // Subscribe our local subscription list
				// �½��Ľڵ�Ҫ�������Ǳ����������ĵĶ�Ӧͨ��
                const unsigned int nHops = 0;
                for (unsigned int nChannel = 0; nChannel < m_pcNodeLocalHost->vfSubscribe.size(); nChannel++)
                    if (m_pcNodeLocalHost->vfSubscribe[nChannel])
                        pnode->PushMessage("subscribe", nChannel, nHops);

                fSuccess = true;
                break;
            }
        }
    }
}







// ��Ϣ�����߳�
void* ThreadMessageHandler(void* parg)
{
 //   IMPLEMENT_RANDOMIZE_STACK(ThreadMessageHandler(parg));

    loop
    {
        //vfThreadRunning[2] = true;
        //CheckForShutdown(2);
        try
        {
            NetWorkServ::getInstance()->MessageHandler(parg);
        }
        CATCH_PRINT_EXCEPTION("ThreadMessageHandler()")
        //vfThreadRunning[2] = false;
        sleep(5000);
    }
}
// ��Ϣ�����߳�
void NetWorkServ::MessageHandler(void* parg)
{
    printf("ThreadMessageHandler started\n");
    //SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    loop
    {
		// ��ѯ���ӵĽڵ�������Ϣ����
        // Poll the connected nodes for messages
        vector<CNode*> m_cNodeLstCopy;
        m_cNodeLstCopy = m_cNodeLst;
		// ��ÿһ���ڵ������Ϣ����������Ϣ�ͽ�����Ϣ
        foreach(CNode* pnode, m_cNodeLstCopy)
        {

            pnode->AddRef();

            // Receive messages
            ProcessMessages(pnode);

            // Send messages
            SendMessages(pnode);

            pnode->Release();
        }

        // Wait and allow messages to bunch up
        vfThreadRunning[2] = false;
        sleep(10);
        vfThreadRunning[2] = true;
        CheckForShutdown(2);
    }
}



/* EOF */

