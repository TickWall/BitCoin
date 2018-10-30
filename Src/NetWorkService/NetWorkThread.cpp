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
#include "CommonBase/ProtocSerialize.h"
#include "CommonBase/market.h"
#include "ProtocSrc/Message.pb.h"
#include "NetWorkServ.h"
#include "ZMQNode.h"
#include "zhelpers.h"
#include <thread>
using namespace Enze;

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
    
    CAddress& addrIncoming = WalletServ::getInstance()->addrIncoming;
    // Get our external IP address for incoming connections
    if (addrIncoming.ip)
        m_cAddrLocalHost.ip = addrIncoming.ip;
    if (GetMyExternalIP(m_cAddrLocalHost.ip))
    {
        addrIncoming = m_cAddrLocalHost;
        //CWalletDB().WriteSetting("addrIncoming", addrIncoming);
    }

    m_strIdentity = "tcp://"+m_cAddrLocalHost.ToString();
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
     printf("start Node\n");
    
   // std::thread t1(ThreadSocketHandler);
    std::thread t1(&NetWorkServ::SocketHandler, this);
    t1.detach();
    
   // std::thread t2(ThreadMessageHandler);
    std::thread t2(&NetWorkServ::MessageHandler, this);
    t2.detach();
    
    std::thread t3(&NetWorkServ::OpenConnections, this);
    t3.detach();
    
    std::thread t4(&NetWorkServ::MessageRecv, this);
    t4.detach();
    
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
 //   printf("%s---%d\n", __FILE__, __LINE__);
#if 0
    if (fShutdown)
    {
        if (n != -1)
            vfThreadRunning[n] = false;
        if (n == 0)
            foreach(ZNode* pnode, m_cZNodeLst)
                close(pnode->hSocket);
    }
#endif 
}


void NetWorkServ::NodeSyncThread()
{
    printf("%s---%d\n", __FILE__, __LINE__);
    printf("NetWorkServ::NodeSyncThread--start\n");
    void* worker = zmq_socket(m_cZmqCtx, ZMQ_ROUTER);
    zmq_connect(worker, "inproc://backend.ipc");
    while(1) 
    {
        char* brokerId = s_recv(worker);
        char* repId  = s_recv(worker);
        char*emp = s_recv(worker);
        free(emp);
        char*ReqType = s_recv(worker);
        printf("Recv Data, brokerId[%s], repId[%s]\n", brokerId, repId);
        printf("Recv Data, ReqType[%s]\n", ReqType);
        if (0 == strcmp("ping", ReqType)) {;
            s_sendmore(worker, brokerId);
            s_sendmore(worker, repId);
            s_send(worker, (char*)m_strIdentity.c_str()); 
            printf("Recv Data, ReqType[%s]--rep\n", ReqType);
#if 0
            if (!FindNode(repId)) {
                printf("Recv Data, ReqType not Find[%s]\n", repId);
                ConnectNode(repId);
            }
#endif
        }
        else {
            char* pData = s_recv(worker);
            ZNode *pNode = FindNode(repId);
            if (!pNode) {
                pNode = ConnectNode(repId);
            }
            if (!pNode) {
                printf("NetWorkServ::NodeSyncThread--Node[%s] is not alive\n", repId);
                free(pData);
                continue;
            }
            string req = pData;
            PB_MessageData *cProtoc = new PB_MessageData;
            cProtoc->ParsePartialFromString(req);
            pNode->AddRecvMessage(cProtoc);
#if 0
            if (PB_MessageData_Mesg_Kind_MK_Version == cProtoc->emsgkind()) {
                printf("NetWorkServ::MessageRecv--recv Version[%d]\n", cProtoc->cversion().nversion());
            }
            cProtoc->Clear();
            cProtoc->set_emsgkind(PB_MessageData_Mesg_Kind_MK_Reply);
            cProtoc->set_hashreply("jyb");
           // SeriaAddress(m_cAddrLocalHost, cProtoc.mu);
            string strData;
            cProtoc->SerializePartialToString(&strData);
            s_sendmore(worker, brokerId);
            s_sendmore(worker, repId);
            s_sendmore(worker, (char*)m_strIdentity.c_str());
            s_send(worker, (char*)strData.c_str());
#endif
        }
        free(brokerId);
        free(repId);
        //free(ReqType);
        free(ReqType);
        sleep(1);
    }
    

}


void NetWorkServ::MessageRecv()
{
    
    void* backend = zmq_socket(m_cZmqCtx, ZMQ_DEALER);
    zmq_setsockopt(backend, ZMQ_IDENTITY, m_strIdentity.c_str(), m_strIdentity.length());
    zmq_bind(backend, "inproc://backend.ipc");
    printf("NetWorkServ::MessageRecv--start\n");
    void * frontend = zmq_socket(m_cZmqCtx, ZMQ_ROUTER);
   // string ipAddr = "tcp://"+m_cAddrLocalHost.ToString();
    printf("m_strIdentity [%s]\n", m_strIdentity.c_str());
    zmq_setsockopt(frontend, ZMQ_IDENTITY, m_strIdentity.c_str(), m_strIdentity.length());
    zmq_bind(frontend, m_strIdentity.c_str());
   
    std::thread t0(&NetWorkServ::NodeSyncThread, this);
    t0.detach();
    
    std::thread t1(&NetWorkServ::NodeSyncThread, this);
    t1.detach();

    zmq_proxy(frontend, backend, NULL);
}

// socket ����parag��Ӧ���Ǳ��ؽڵ㿪���ļ���socket
void NetWorkServ::SocketHandler()
{
    printf("ThreadSocketHandler started\n");
    list<ZNode*> cZNodeLstDisconnected;
    loop
    {
        // Disconnect duplicate connections �ͷ�ͬһ��ip�ظ����Ӷ�Ӧ�Ľڵ㣬�����ǲ�ͬ�˿�
        map<unsigned int, ZNode*> mapFirst;
        foreach(auto it, m_cZNodeLst)
        {
            ZNode* pnode= it.second;
            if (pnode->isDistconnect())
                continue;
            unsigned int ip = pnode->getAddr().ip;
            // ��������ip��ַ��Ӧ����0���������е�ip��ַ��Ӧ�ô������ip
            if (mapFirst.count(ip) && m_cAddrLocalHost.ip < ip)
            {
                // In case two nodes connect to each other at once,
                // the lower ip disconnects its outbound connection
                ZNode* pnodeExtra = mapFirst[ip];

                if (pnodeExtra->GetRefCount() > (pnodeExtra->isNetworkNode() ? 1 : 0))
                    std::swap(pnodeExtra, pnode);

                if (pnodeExtra->GetRefCount() <= (pnodeExtra->isNetworkNode() ? 1 : 0))
                {
                    printf("(%d nodes) disconnecting duplicate: %s\n", m_cZNodeLst.size(), pnodeExtra->getAddr().ToString().c_str());
                    if (pnodeExtra->isNetworkNode() && !pnode->isNetworkNode())
                    {
                        pnode->AddRef();
                        //printf("error %s_%d\n", __FILE__, __LINE__);
                        bool bNode = pnode->isNetworkNode();
                        bool bExtNode = pnodeExtra->isNetworkNode();
                        pnode->setNetworkState(bExtNode);
                        pnodeExtra->setNetworkState(bNode);
                        pnodeExtra->Release();
                    }
                    pnodeExtra->setNetworkState(true);
                }
            }
            mapFirst[ip] = pnode;
        }
        // �Ͽ���ʹ�õĽڵ�
        // Disconnect unused nodes
        map<string, ZNode*> cZNodeLstCopy = m_cZNodeLst;
        foreach(auto it, cZNodeLstCopy)
        {
            ZNode* pnode= it.second;
            // �ڵ�׼���ͷ����ӣ����Ҷ�Ӧ�Ľ��պͷ��ͻ��������ǿ�
            if (pnode->ReadyToDisconnect() /*&& pnode->vRecv.empty() && pnode->vSend.empty()*/)
            {
                // �ӽڵ��б����Ƴ�
                // remove from m_cZNodeLst
                //m_cZNodeLst.erase(remove(m_cZNodeLst.begin(), m_cZNodeLst.end(), it), m_cZNodeLst.end());
                printf("Remove Node[%s] From List\n",it.first.c_str());
                m_cZNodeLst.erase(it.first);
                pnode->Disconnect();

                // ����Ӧ׼���ͷŵĽڵ���ڶ�Ӧ�Ľڵ��ͷ����ӳ��У��ȴ���Ӧ�ڵ�����������ͷ�
                // hold in disconnected pool until all refs are released
                pnode->setReleaseTime(max(pnode->getReleaseTime(), GetTime() + 5 * 60)); // ����Ƴ�5����
                if (pnode->isNetworkNode())
                    pnode->Release();
                cZNodeLstDisconnected.push_back(pnode);
            }
        }

        // ɾ���˿ڵ����ӵĽڵ㣺ɾ���������Ƕ�Ӧ�ڵ������С�ڵ���0
        // Delete disconnected nodes
        // map<string, ZNode*> cZNodeLstDisconnectedCopy = cZNodeLstDisconnected;
        foreach(ZNode* pnode, cZNodeLstDisconnected)
        {
            // wait until threads are done using it
            if (pnode->GetRefCount() <= 0)
            {
                cZNodeLstDisconnected.remove(pnode);
                delete pnode;
            }
        }

        if (m_cZNodeLst.size() == 0)
        {
           sleep(10);
           continue;
        }

        zmq_pollitem_t *items = new zmq_pollitem_t[m_cZNodeLst.size()];
        int iCnt = 0;
        foreach(auto it, m_cZNodeLst) {
            items[iCnt].socket = it.second->getPeerSock();
            items[iCnt].fd = 0;
            items[iCnt].events = ZMQ_POLLIN;
            items[iCnt].revents = 0;
        }

        // �ҳ���һ��socket������Ҫ����
        // Find which sockets have data to receive
        //
        long timeout = 1000; //1s frequency to poll pnode->vSend ��ѯ�ڵ��Ƿ�������Ҫ���͵�Ƶ��,
	    
        int retCnt = zmq_poll(items, m_cZNodeLst.size(), timeout);
        if ( -1 == retCnt) { 
            printf("%s---%d\n", __FILE__, __LINE__);
            break;
        }
        
		// ����������ӣ����ܼ���
        RandAddSeed();
        // ��ÿһ��socket���з�����
        // Service each socket
        //
        if (0 != retCnt) {
            for (int i = 0; i < m_cZNodeLst.size(); ++i) {
                if (items[i].revents & ZMQ_POLLIN) {
                    //zmq::socket_t* pSock = items[i].socket;
                  //  string emp = s_recv(items[i].socket);
                    string identity = s_recv(items[i].socket);
                    auto it = m_cZNodeLst.find(identity);
                    if (it != m_cZNodeLst.end()) {
                        printf("[%s] Start recv\n", identity.c_str());
                        it->second->Recv();
                    }else {
                       printf("%s---%d---%s,error[Node %s was not find]\n", __FILE__, __LINE__, __func__, identity.c_str());
                       foreach (auto it, m_cZNodeLst) {
                           printf("Node id [%s]\n", it.first.c_str());
                       }
                    }
                }
            }
        }
        else {
            // No Data Come in ,we will send all data to peer node
            // ���ڵ��Ӧ�ķ��ͻ����е����ݷ��ͳ�ȥ
            // Send
            // deal send data
            printf("NetWorkServ::SocketHandler start send\n");

            foreach (auto it, m_cZNodeLst) {
                it.second->Send();
            }
        }
        delete[] items;
        sleep(10);
    }
}


// ����ÿһ���򿪽ڵ�����ӣ����нڵ�֮����Ϣͨ�ţ���ýڵ��Ӧ��������Ϣ������ڵ��Ӧ��֪����ַ���н�����
void NetWorkServ::OpenConnections()
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
        while (m_cZNodeLst.size() >= nMaxConnections || m_cZNodeLst.size() >= m_cMapAddresses.size())
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
        else if (nTry++ < 30 && m_cZNodeLst.size() < nMaxConnections/2)
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
            int64 nDelay = ((30 * 60) << m_cZNodeLst.size());
            if (!fIRCOnly)
            {
                nDelay *= 2;
                if (m_cZNodeLst.size() >= 3)
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
            boost::iterators::advance_adl_barrier::advance(mi, GetRand(mapIP.size())); // ��ָ�붨λ�����λ��
            //advance(mi, GetRand(mapIP.size())); // ��ָ�붨λ�����λ��

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
                ZNode* pnode = ConnectNode(addrConnect);
                vfThreadRunning[1] = true;
                CheckForShutdown(1);
                if (!pnode)
                    continue;
                pnode->setNetworkState(true);

				// �������������ַ�ܹ�����·�ɣ�����Ҫ�㲥���ǵĵ�ַ
                if (m_cAddrLocalHost.IsRoutable())
                {
                    // Advertise our address
                    vector<CAddress> vAddrToSend;
                    vAddrToSend.push_back(m_cAddrLocalHost);
                    pnode->SendAddr(vAddrToSend);// ����Ϣ���ͳ�ȥ����vsend�У�����Ϣ�����߳��н��д���
                }

				// �Ӵ����Ľڵ��þ����ܶ�ĵ�ַ��Ϣ��������Ϣ������Ϣ�����߳��н��д���
                // Get as many addresses as we can
                pnode->SendGetAddrRequest();

                ////// should the one on the receiving end do this too?
                // Subscribe our local subscription list
				// �½��Ľڵ�Ҫ�������Ǳ����������ĵĶ�Ӧͨ��
#if 0
                const unsigned int nHops = 0;
                for (unsigned int nChannel = 0; nChannel < m_pcNodeLocalHost->vfSubscribe.size(); nChannel++)
                    if (m_pcNodeLocalHost->vfSubscribe[nChannel])
                        pnode->PushMessage("subscribe", nChannel, nHops);
#endif 
                fSuccess = true;
                break;
            }
        }
    }
}


// ��Ϣ�����߳�
void NetWorkServ::MessageHandler()
{
    printf("ThreadMessageHandler started\n");
    //SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    loop
    {
		// ��ѯ���ӵĽڵ�������Ϣ����
        // Poll the connected nodes for messages
        map<string, ZNode*>cNodeLstCopy = m_cZNodeLst;
		// ��ÿһ���ڵ������Ϣ����������Ϣ�ͽ�����Ϣ
        foreach(auto it, cNodeLstCopy)
        {
            ZNode* pnode = it.second;
            pnode->AddRef();

            // Receive messages
            ProcessMessages(pnode);

            // Send messages
            SendMessages(pnode);

            pnode->Release();
        }

        // Wait and allow messages to bunch up
        sleep(10);
    }
}



/* EOF */

