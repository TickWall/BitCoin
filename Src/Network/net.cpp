// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include <netdb.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <boost/algorithm/string.hpp>
#include "headers.h"
#include "market.h"
#include "BlockEngine.h"
#include "Network/net.h"
#include "Db/CAddrDB.h"
#include "Db/CWalletDB.h"
#include "TX/CWalletTx.h"

using namespace boost;
void ThreadMessageHandler2(void* parg);
void ThreadSocketHandler2(void* parg);
void ThreadOpenConnections2(void* parg);
extern vector<pthread_t> gThreadList;





//
// Global state variables
//
bool fClient = false;
uint64 nLocalServices = (fClient ? 0 : NODE_NETWORK);
CAddress addrLocalHost(0, DEFAULT_PORT, nLocalServices);// ����������ַ
CNode nodeLocalHost(INVALID_SOCKET, CAddress("127.0.0.1", nLocalServices));
CNode* pnodeLocalHost = &nodeLocalHost; // ���ؽڵ�
bool fShutdown = false; // ����Ƿ�ر�
boost::array<bool, 10> vfThreadRunning; // �߳�����״̬��ǣ�0������ʾsockethandler��1������ʾopenConnection��2������ʾ������Ϣ
vector<CNode*> vNodes; // �뵱ǰ�ڵ������Ľڵ��б�
//CCriticalSection cs_vNodes;
map<vector<unsigned char>, CAddress> mapAddresses; // �ڵ��ַӳ�䣺key��Ӧ����ip��ַ+�˿ڣ�value��CAddress����
//CCriticalSection cs_mapAddresses;
map<CInv, CDataStream> mapRelay; // ����ת��������
deque<pair<int64, CInv> > vRelayExpiration; // �ز����ڼ�¼
//CCriticalSection cs_mapRelay;
map<CInv, int64> mapAlreadyAskedFor; // �Ѿ�����������󣺶�Ӧ��valueΪ����ʱ�䣨��λ��΢�룩



CAddress addrProxy; // �����ַ

//socket ���ӣ����ݵ�ַ��Ϣ������Ӧ��socket��Ϣ
bool ConnectSocket(const CAddress& addrConnect, SOCKET& hSocketRet)
{
    hSocketRet = INVALID_SOCKET;

    SOCKET hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hSocket == INVALID_SOCKET)
        return false;
	// �ж��Ƿ��ܹ�·�ɣ�������ip��ַ��Ӧ����10.x.x.x���߶�Ӧ����192.168.x.x�����ܽ���·��
    bool fRoutable = !(addrConnect.GetByte(3) == 10 || (addrConnect.GetByte(3) == 192 && addrConnect.GetByte(2) == 168));
    // �����ǣ����ܹ�·�ɿ϶����ܹ������ܹ�·�ɻ�Ҫ����Ӧ��ip��ַ������ȫ0
	bool fProxy = (addrProxy.ip && fRoutable);
	// �ܹ������ʹ�ô����ַ�����ܴ���������ӵ�ַ
    struct sockaddr_in sockaddr = (fProxy ? addrProxy.GetSockAddr() : addrConnect.GetSockAddr());
	// ��Ӧָ���ĵ�ַ����socket���ӣ������ض�Ӧ��socket���
    if (connect(hSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
    {
        close(hSocket);
        return false;
    }
	// ����ܹ��������������ӵ�ַ���ص�socket������Ǵ����ַ��Ӧ�ľ�����������ӵ�ַ��Ӧ�ľ��
    if (fProxy)
    {
        printf("Proxy connecting to %s\n", addrConnect.ToString().c_str());
        char pszSocks4IP[] = "\4\1\0\0\0\0\0\0user";
        memcpy(pszSocks4IP + 2, &addrConnect.port, 2); // ����pszSocks4IPΪ"\4\1port\0\0\0\0\0user"
        memcpy(pszSocks4IP + 4, &addrConnect.ip, 4);// ����pszSocks4IPΪ"\4\1port+ip\0user"
        char* pszSocks4 = pszSocks4IP;
        int nSize = sizeof(pszSocks4IP);

        int ret = send(hSocket, pszSocks4, nSize, 0);
        if (ret != nSize)
        {
            close(hSocket);
            return error("Error sending to proxy\n");
        }
        char pchRet[8];
        if (recv(hSocket, pchRet, 8, 0) != 8)
        {
            close(hSocket);
            return error("Error reading proxy response\n");
        }
        if (pchRet[1] != 0x5a)
        {
            close(hSocket);
            return error("Proxy returned error %d\n", pchRet[1]);
        }
        printf("Proxy connection established %s\n", addrConnect.ToString().c_str());
    }

    hSocketRet = hSocket;
    return true;
}



bool GetMyExternalIP2(const CAddress& addrConnect, const char* pszGet, const char* pszKeyword, unsigned int& ipRet)
{
    SOCKET hSocket;
    if (!ConnectSocket(addrConnect, hSocket))
        return error("GetMyExternalIP() : connection to %s failed\n", addrConnect.ToString().c_str());

    send(hSocket, pszGet, strlen(pszGet), 0);

    char Buf[512] = {0};
    int len = recv(hSocket, Buf, 512, 0);
    if (len > 0)
    {
        //printf("GetMyExternalIP() received %s\n", Buf);
        char BufIP[32] = {0};
        sscanf(strstr(Buf,"utf-8")+9,"%*[^\n]\n%[^\n]",BufIP);
       // printf("GetMyExternalIP() received IP %s\n", BufIP);
        CAddress addr(BufIP);
        if (addr.ip == 0 || !addr.IsRoutable())
            return false;
        ipRet = addr.ip;
        return true;
    }
    else {
        close(hSocket);
        return false;
    }
    close(hSocket);
    return error("GetMyExternalIP() : connection closed\n");
}


bool GetMyExternalIP(unsigned int& ipRet)
{
    return false;
    CAddress addrConnect;
    char* pszGet;
    char* pszKeyword;

    for (int nLookup = 0; nLookup <= 1; nLookup++)
    for (int nHost = 1; nHost <= 2; nHost++)
    {
        if (nHost == 1)
        {
            addrConnect = CAddress("118.184.176.13:80"); // www.3322.org

            if (nLookup == 1)
            {
                struct hostent* phostent = gethostbyname("www.3322.org");
                if (phostent && phostent->h_addr_list && phostent->h_addr_list[0])
                    addrConnect = CAddress(*(u_long*)phostent->h_addr_list[0], htons(80));
            }

            pszGet = "GET /dyndns/getip HTTP/1.1\r\n"
                     "Host:www.3322.org\r\n"
                     "ACCEPT:*/*\r\n"
                     "Connection: close\r\n"
                     "\r\n";

            pszKeyword = "IP:";
        }
        else if (nHost == 2)
        {
            addrConnect = CAddress("162.88.100.200:80"); // checkip.dyndns.org

            if (nLookup == 1)
            {
                struct hostent* phostent = gethostbyname("checkip.dyndns.org");
                if (phostent && phostent->h_addr_list && phostent->h_addr_list[0])
                    addrConnect = CAddress(*(u_long*)phostent->h_addr_list[0], htons(80));
            }

            pszGet = "GET / HTTP/1.1\r\n"
                     "Host: checkip.dyndns.org\r\n"
                     "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)\r\n"
                     "Connection: close\r\n"
                     "\r\n";

            pszKeyword = "Address:";
        }

        if (GetMyExternalIP2(addrConnect, pszGet, pszKeyword, ipRet))
            return true;
    }

    return false;
}





bool AddAddress(CAddrDB& addrdb, const CAddress& addr)
{
	// ��ַ����·�ɣ��򲻽��˵�ַ�����ַ����
    if (!addr.IsRoutable())
        return false;
    if (addr.ip == addrLocalHost.ip)
        return false;
    ////CRITICAL_BLOCK(cs_mapAddresses)
    {
		// ���ݵ�ַ��ip+port����ѯ��Ӧ���ڴ����mapAddresses��
        map<vector<unsigned char>, CAddress>::iterator it = mapAddresses.find(addr.GetKey());
        if (it == mapAddresses.end())
        {
            // New address
            mapAddresses.insert(make_pair(addr.GetKey(), addr));
            addrdb.WriteAddress(addr);
            return true;
        }
        else
        {
            CAddress& addrFound = (*it).second;
            if ((addrFound.m_nServices | addr.m_nServices) != addrFound.m_nServices)
            {
				// ���ӵ�ַ��Ӧ�ķ������ͣ�������д�����ݿ�
                // Services have been added
                addrFound.m_nServices |= addr.m_nServices;
                addrdb.WriteAddress(addrFound);
                return true;
            }
        }
    }
    return false;
}





void AbandonRequests(void (*fn)(void*, CDataStream&), void* param1)
{
    // If the dialog might get closed before the reply comes back,
    // call this in the destructor so it doesn't get called after it's deleted.
    ////CRITICAL_BLOCK(cs_vNodes)
    {
        foreach(CNode* pnode, vNodes)
        {
     //       //CRITICAL_BLOCK(pnode->cs_mapRequests)
            {
                for (map<uint256, CRequestTracker>::iterator mi = pnode->mapRequests.begin(); mi != pnode->mapRequests.end();)
                {
                    CRequestTracker& tracker = (*mi).second;
                    if (tracker.fn == fn && tracker.param1 == param1)
                        pnode->mapRequests.erase(mi++);
                    else
                        mi++;
                }
            }
        }
    }
}







//
// Subscription methods for the broadcast and subscription system.
// Channel numbers are message numbers, i.e. MSG_TABLE and MSG_PRODUCT.
//
// The subscription system uses a meet-in-the-middle strategy.
// With 100,000 nodes, if senders broadcast to 1000 random nodes and receivers
// subscribe to 1000 random nodes, 99.995% (1 - 0.99^1000) of messages will get through.
//

bool AnySubscribed(unsigned int nChannel)
{
    if (pnodeLocalHost->IsSubscribed(nChannel))
        return true;
   // //CRITICAL_BLOCK(cs_vNodes)
        foreach(CNode* pnode, vNodes)
            if (pnode->IsSubscribed(nChannel))
                return true;
    return false;
}

bool CNode::IsSubscribed(unsigned int nChannel)
{
    if (nChannel >= vfSubscribe.size())
        return false;
    return vfSubscribe[nChannel];
}

void CNode::Subscribe(unsigned int nChannel, unsigned int nHops)
{
    if (nChannel >= vfSubscribe.size())
        return;

    if (!AnySubscribed(nChannel))
    {
        // Relay subscribe
     //   //CRITICAL_BLOCK(cs_vNodes)
            foreach(CNode* pnode, vNodes)
                if (pnode != this)
                    pnode->PushMessage("subscribe", nChannel, nHops);
    }

    vfSubscribe[nChannel] = true;
}

void CNode::CancelSubscribe(unsigned int nChannel)
{
    if (nChannel >= vfSubscribe.size())
        return;

    // Prevent from relaying cancel if wasn't subscribed
    if (!vfSubscribe[nChannel])
        return;
    vfSubscribe[nChannel] = false;

    if (!AnySubscribed(nChannel))
    {
        // Relay subscription cancel
       // //CRITICAL_BLOCK(cs_vNodes)
            foreach(CNode* pnode, vNodes)
                if (pnode != this)
                    pnode->PushMessage("sub-cancel", nChannel);

        // Clear memory, no longer subscribed
        if (nChannel == MSG_PRODUCT)
         //   //CRITICAL_BLOCK(cs_mapProducts)
                mapProducts.clear();
    }
}








// ����ip�ڱ��ش洢�Ľڵ��б�vNodes�в��Ҷ�Ӧ�Ľڵ�
CNode* FindNode(unsigned int ip)
{
 //   //CRITICAL_BLOCK(cs_vNodes)
    {
        foreach(CNode* pnode, vNodes)
            if (pnode->addr.ip == ip)
                return (pnode);
    }
    return NULL;
}

CNode* FindNode(CAddress addr)
{
    //CRITICAL_BLOCK(cs_vNodes)
    {
        foreach(CNode* pnode, vNodes)
            if (pnode->addr == addr)
                return (pnode);
    }
    return NULL;
}
// ���Ӷ�Ӧ��ַ�Ľڵ�
CNode* ConnectNode(CAddress addrConnect, int64 nTimeout)
{
    if (addrConnect.ip == addrLocalHost.ip)
        return NULL;

	// ʹ��ip��ַ�ڱ��ض�Ӧ�Ľڵ��б��в��Ҷ�Ӧ�Ľڵ㣬���������ֱ�ӷ��ض�Ӧ�Ľڵ�
    // Look for an existing connection
    CNode* pnode = FindNode(addrConnect.ip);
    if (pnode)
    {
        if (nTimeout != 0)
            pnode->AddRef(nTimeout); // �Ƴٽڵ��Ӧ���ͷ�ʱ��
        else
            pnode->AddRef(); // ���ӽڵ��Ӧ������
        return pnode;
    }

    /// debug print
 //   printf("trying %s\n", addrConnect.ToString().c_str());

	// ������ĵ�ַ��������
    // Connect
    SOCKET hSocket;
    if (ConnectSocket(addrConnect, hSocket))
    {
        /// debug print
        printf("connected %s\n", addrConnect.ToString().c_str());

        // Set to nonblocking
        u_long nOne = 1;
#if 0
        if (ioctlsocket(hSocket, FIONBIO, &nOne) == SOCKET_ERROR)
            printf("ConnectSocket() : ioctlsocket nonblocking setting failed, error %m\n");
#endif
        int flag = 0;
        flag = fcntl(hSocket, F_GETFL, NULL);
        fcntl(hSocket, F_SETFL, flag|O_NONBLOCK);
        // Add node
        CNode* pnode = new CNode(hSocket, addrConnect, false);
        if (nTimeout != 0)
            pnode->AddRef(nTimeout);
        else
            pnode->AddRef();
   //     //CRITICAL_BLOCK(cs_vNodes)
            vNodes.push_back(pnode);

     //   //CRITICAL_BLOCK(cs_mapAddresses)
            mapAddresses[addrConnect.GetKey()].nLastFailed = 0;
        return pnode;
    }
    else
    {
        //CRITICAL_BLOCK(cs_mapAddresses)
            mapAddresses[addrConnect.GetKey()].nLastFailed = GetTime();
        return NULL;
    }
}

void CNode::Disconnect()
{
    printf("disconnecting node %s\n", addr.ToString().c_str());

    close(hSocket);

    // If outbound and never got version message, mark address as failed
    if (!fInbound && nVersion == 0)
       // //CRITICAL_BLOCK(cs_mapAddresses)
            mapAddresses[addr.GetKey()].nLastFailed = GetTime();

    // All of a nodes broadcasts and subscriptions are automatically torn down
    // when it goes down, so a node has to stay up to keep its broadcast going.

    ////CRITICAL_BLOCK(cs_mapProducts)
        for (map<uint256, CProduct>::iterator mi = mapProducts.begin(); mi != mapProducts.end();)
            AdvertRemoveSource(this, MSG_PRODUCT, 0, (*(mi++)).second);

    // Cancel subscriptions
    for (unsigned int nChannel = 0; nChannel < vfSubscribe.size(); nChannel++)
        if (vfSubscribe[nChannel])
            CancelSubscribe(nChannel);
}












// socket ����parag��Ӧ���Ǳ��ؽڵ㿪���ļ���socket
void* ThreadSocketHandler(void* parg)
{
//  IMPLEMENT_RANDOMIZE_STACK(ThreadSocketHandler(parg));

    loop
    {
        vfThreadRunning[0] = true;
        CheckForShutdown(0);
        try
        {
            ThreadSocketHandler2(parg);
        }
        CATCH_PRINT_EXCEPTION("ThreadSocketHandler()")
        vfThreadRunning[0] = false;
        sleep(5000);
    }
}
// socket ����parag��Ӧ���Ǳ��ؽڵ㿪���ļ���socket
void ThreadSocketHandler2(void* parg)
{
    printf("ThreadSocketHandler started\n");
    SOCKET hListenSocket = *(SOCKET*)parg; // ��ü���socket
    list<CNode*> vNodesDisconnected;
    int nPrevNodeCount = 0;

    loop
    {
        // Disconnect nodes
        {
            // Disconnect duplicate connections �ͷ�ͬһ��ip�ظ����Ӷ�Ӧ�Ľڵ㣬�����ǲ�ͬ�˿�
            map<unsigned int, CNode*> mapFirst;
            foreach(CNode* pnode, vNodes)
            {
                if (pnode->fDisconnect)
                    continue;
                unsigned int ip = pnode->addr.ip;
				// ��������ip��ַ��Ӧ����0���������е�ip��ַ��Ӧ�ô������ip
                if (mapFirst.count(ip) && addrLocalHost.ip < ip)
                {
                    // In case two nodes connect to each other at once,
                    // the lower ip disconnects its outbound connection
                    CNode* pnodeExtra = mapFirst[ip];

                    if (pnodeExtra->GetRefCount() > (pnodeExtra->fNetworkNode ? 1 : 0))
                        swap(pnodeExtra, pnode);

                    if (pnodeExtra->GetRefCount() <= (pnodeExtra->fNetworkNode ? 1 : 0))
                    {
                        printf("(%d nodes) disconnecting duplicate: %s\n", vNodes.size(), pnodeExtra->addr.ToString().c_str());
                        if (pnodeExtra->fNetworkNode && !pnode->fNetworkNode)
                        {
                            pnode->AddRef();
                            swap(pnodeExtra->fNetworkNode, pnode->fNetworkNode);
                            pnodeExtra->Release();
                        }
                        pnodeExtra->fDisconnect = true;
                    }
                }
                mapFirst[ip] = pnode;
            }
			// �Ͽ���ʹ�õĽڵ�
            // Disconnect unused nodes
            vector<CNode*> vNodesCopy = vNodes;
            foreach(CNode* pnode, vNodesCopy)
            {
				// �ڵ�׼���ͷ����ӣ����Ҷ�Ӧ�Ľ��պͷ��ͻ��������ǿ�
                if (pnode->ReadyToDisconnect() && pnode->vRecv.empty() && pnode->vSend.empty())
                {
					// �ӽڵ��б����Ƴ�
                    // remove from vNodes
                    vNodes.erase(remove(vNodes.begin(), vNodes.end(), pnode), vNodes.end());
                    pnode->Disconnect();

					// ����Ӧ׼���ͷŵĽڵ���ڶ�Ӧ�Ľڵ��ͷ����ӳ��У��ȴ���Ӧ�ڵ�����������ͷ�
                    // hold in disconnected pool until all refs are released
                    pnode->nReleaseTime = max(pnode->nReleaseTime, GetTime() + 5 * 60); // ����Ƴ�5����
                    if (pnode->fNetworkNode)
                        pnode->Release();
                    vNodesDisconnected.push_back(pnode);
                }
            }

			// ɾ���˿ڵ����ӵĽڵ㣺ɾ���������Ƕ�Ӧ�ڵ������С�ڵ���0
            // Delete disconnected nodes
            list<CNode*> vNodesDisconnectedCopy = vNodesDisconnected;
            foreach(CNode* pnode, vNodesDisconnectedCopy)
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
                        vNodesDisconnected.remove(pnode);
                        delete pnode;
                    }
                }
            }
        }
        if (vNodes.size() != nPrevNodeCount)
        {
            nPrevNodeCount = vNodes.size(); // ��¼ǰһ�νڵ��Ӧ������
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
        //CRITICAL_BLOCK(cs_vNodes)
        {
            foreach(CNode* pnode, vNodes)
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
        foreach(CNode* pnode, vNodes)
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
                vNodes.push_back(pnode);
            }
        }


        // ��ÿһ��socket���з�����
        // Service each socket
        //
        vector<CNode*> vNodesCopy;
        vNodesCopy = vNodes;
        foreach(CNode* pnode, vNodesCopy)
        {
            CheckForShutdown(0);
            SOCKET hSocket = pnode->hSocket; // ��ȡÿһ���ڵ��Ӧ��socket

            // �ӽڵ��Ӧ��socket�ж�ȡ��Ӧ�����ݣ������ݷ���ڵ�Ľ��ջ�������
            // Receive
            //
            if (FD_ISSET(hSocket, &fdsetRecv))
            {
                {
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
                }
            }

            // ���ڵ��Ӧ�ķ��ͻ����е����ݷ��ͳ�ȥ
            // Send
            //
            if (FD_ISSET(hSocket, &fdsetSend))
            {

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
        CheckForShutdown(1);
        try
        {
            ThreadOpenConnections2(parg);
        }
        CATCH_PRINT_EXCEPTION("ThreadOpenConnections()")
        vfThreadRunning[1] = false;
        sleep(5000);
    }
}
// ����ÿһ���򿪽ڵ�����ӣ����нڵ�֮����Ϣͨ�ţ���ýڵ��Ӧ��������Ϣ������ڵ��Ӧ��֪����ַ���н�����
void ThreadOpenConnections2(void* parg)
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
        while (vNodes.size() >= nMaxConnections || vNodes.size() >= mapAddresses.size())
        {
            CheckForShutdown(1);
            sleep(50);
        }
        vfThreadRunning[1] = true;
        CheckForShutdown(1);


		// Ip��Ӧ��C���ַ����ͬ��C���ַ����һ��
        fIRCOnly = !fIRCOnly;
        if (mapIRCAddresses.empty())
            fIRCOnly = false;
        else if (nTry++ < 30 && vNodes.size() < nMaxConnections/2)
            fIRCOnly = true;

        // Make a list of unique class C's
        unsigned char pchIPCMask[4] = { 0xff, 0xff, 0xff, 0x00 };
        unsigned int nIPCMask = *(unsigned int*)pchIPCMask;
        vector<unsigned int> vIPC;
        //CRITICAL_BLOCK(cs_mapIRCAddresses)
        //CRITICAL_BLOCK(cs_mapAddresses)
        {
            vIPC.reserve(mapAddresses.size());
            unsigned int nPrev = 0;
			// mapAddress�Ѿ����������ˣ�Ĭ������Ч����
            foreach(const PAIRTYPE(vector<unsigned char>, CAddress)& item, mapAddresses)
            {
                const CAddress& addr = item.second;
                if (!addr.IsIPv4())
                    continue;
                if (fIRCOnly && !mapIRCAddresses.count(item.first))
                    continue;

                // Taking advantage of mapAddresses being in sorted order,
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
            int64 nDelay = ((30 * 60) << vNodes.size());
            if (!fIRCOnly)
            {
                nDelay *= 2;
                if (vNodes.size() >= 3)
                    nDelay *= 4;
                if (!mapIRCAddresses.empty())
                    nDelay *= 100;  
            }
					
            /*
            map::lower_bound(key):����map�е�һ�����ڻ����key�ĵ�����ָ��
            map::upper_bound(key):����map�е�һ������key�ĵ�����ָ��
            */
            for (map<vector<unsigned char>, CAddress>::iterator mi = mapAddresses.lower_bound(CAddress(ipC, 0).GetKey());
                 mi != mapAddresses.upper_bound(CAddress(ipC | ~nIPCMask, 0xffff).GetKey());
                 ++mi)
            {
                const CAddress& addr = (*mi).second;
                if (fIRCOnly && !mapIRCAddresses.count((*mi).first))
                    continue;

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
               // printf("OpenConnection,LocalIP[%s], addrConnect_ip[%s]\n", addrLocalHost.ToStringIP().c_str(), addrConnect.ToStringIP().c_str());
				// ip�����Ǳ���ip���Ҳ����Ƿ�ipV4��ַ����Ӧ��ip��ַ���ڱ��صĽڵ��б���
                CheckForShutdown(1);
                if (addrConnect.ip == addrLocalHost.ip || !addrConnect.IsIPv4() || FindNode(addrConnect.ip))
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
                if (addrLocalHost.IsRoutable())
                {
                    // Advertise our address
                    vector<CAddress> vAddrToSend;
                    vAddrToSend.push_back(addrLocalHost);
                    pnode->PushMessage("addr", vAddrToSend); // ����Ϣ���ͳ�ȥ����vsend�У�����Ϣ�����߳��н��д���
                }

				// �Ӵ����Ľڵ��þ����ܶ�ĵ�ַ��Ϣ��������Ϣ������Ϣ�����߳��н��д���
                // Get as many addresses as we can
                pnode->PushMessage("getaddr");

                ////// should the one on the receiving end do this too?
                // Subscribe our local subscription list
				// �½��Ľڵ�Ҫ�������Ǳ����������ĵĶ�Ӧͨ��
                const unsigned int nHops = 0;
                for (unsigned int nChannel = 0; nChannel < pnodeLocalHost->vfSubscribe.size(); nChannel++)
                    if (pnodeLocalHost->vfSubscribe[nChannel])
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
        vfThreadRunning[2] = true;
        CheckForShutdown(2);
        try
        {
            ThreadMessageHandler2(parg);
        }
        CATCH_PRINT_EXCEPTION("ThreadMessageHandler()")
        vfThreadRunning[2] = false;
        sleep(5000);
    }
}
// ��Ϣ�����߳�
void ThreadMessageHandler2(void* parg)
{
    printf("ThreadMessageHandler started\n");
    //SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    loop
    {
		// ��ѯ���ӵĽڵ�������Ϣ����
        // Poll the connected nodes for messages
        vector<CNode*> vNodesCopy;
        vNodesCopy = vNodes;
		// ��ÿһ���ڵ������Ϣ����������Ϣ�ͽ�����Ϣ
        foreach(CNode* pnode, vNodesCopy)
        {

            pnode->AddRef();

            // Receive messages
            BlockEngine::getInstance()->ProcessMessages(pnode);

            // Send messages
            BlockEngine::getInstance()->SendMessages(pnode);

            pnode->Release();
        }

        // Wait and allow messages to bunch up
        vfThreadRunning[2] = false;
        sleep(10);
        vfThreadRunning[2] = true;
        CheckForShutdown(2);
    }
}



//// todo: start one thread per processor, use getenv("NUMBER_OF_PROCESSORS")
void* ThreadBitcoinMiner(void* parg)
{
    vfThreadRunning[3] = true;
    CheckForShutdown(3);
    try
    {
        bool fRet = BlockEngine::getInstance()->BitcoinMiner();
        printf("BitcoinMiner returned %s\n\n\n", fRet ? "true" : "false");
    }
    CATCH_PRINT_EXCEPTION("BitcoinMiner()")
    vfThreadRunning[3] = false;
}










// �����ڵ�
bool StartNode(string& strError)
{
    strError = "";
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
    addrLocalHost = CAddress(*(long*)(phostent->h_addr_list[0]),
                             DEFAULT_PORT,
                             nLocalServices);

    printf("addrLocalHost = %s\n", addrLocalHost.ToString().c_str());

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
    struct sockaddr_in sockaddr = addrLocalHost.GetSockAddr();
    if (bind(hListenSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
    {
        int nErr = errno;
        if (nErr == EADDRINUSE)
            strError = strprintf("Error: Unable to bind to port %s on this computer. The program is probably already running.", addrLocalHost.ToString().c_str());
        else
            strError = strprintf("Error: Unable to bind to port %s on this computer (bind returned error %d)", addrLocalHost.ToString().c_str(), nErr);
        printf("%s\n", strError.c_str());
        return false;
    }
    printf("bound to addrLocalHost = %s\n\n", addrLocalHost.ToString().c_str());

    // Listen for incoming connections
    if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf("Error: Listening for incoming connections failed (listen returned error %d)", errno);
        printf("%s\n", strError.c_str());
        return false;
    }
    CAddress& addrIncoming = BlockEngine::getInstance()->addrIncoming;
    // Get our external IP address for incoming connections
    if (addrIncoming.ip)
        addrLocalHost.ip = addrIncoming.ip;
    if (GetMyExternalIP(addrLocalHost.ip))
    {
        addrIncoming = addrLocalHost;
        CWalletDB().WriteSetting("addrIncoming", addrIncoming);
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

bool StopNode()
{
    printf("StopNode()\n");
    fShutdown = true;
    BlockEngine::getInstance()->nTransactionsUpdated++;
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

void CheckForShutdown(int n)
{
    if (fShutdown)
    {
        if (n != -1)
            vfThreadRunning[n] = false;
        if (n == 0)
            foreach(CNode* pnode, vNodes)
                close(pnode->hSocket);
//        _endthread();
    }
}

/* EOF */
