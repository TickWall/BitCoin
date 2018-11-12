/*
 * =====================================================================================
 *
 *       Filename:  ZMQNode.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/20/2018 07:52:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <map>
#include "CommonBase/ProtocSerialize.h"
#include "CommonBase/market.h"
#include "BlockEngine/CBlockLocator.h"
#include "BlockEngine/BlockEngine.h"
#include "DAO/DaoServ.h"
#include "WalletService/WalletServ.h"
#include "ZMQNode.h"
#include "NetWorkServ.h"
using namespace std;
using namespace Enze;

ZNode::ZNode(const string& id, void* ctx, const CAddress& addrIn, bool fInboundIn)
: m_bInbound(fInboundIn),
  m_bNetworkNode(false),
  m_bDisconnect(false),
  m_bClient(false),
  m_nRefCount(0),
  m_nVersion(0),
  m_nReleaseTime(0),
  m_nServices(0),
  m_cAddr(addrIn),
  m_Id(id),
  m_ServId(),
  m_cP2PSocket(nullptr),
  m_SendLst(),
  m_RecvLst(),
  m_cSndMtx(),
  m_cRcvMtx()
{
    m_cP2PSocket  = zmq_socket(ctx, ZMQ_DEALER);
    m_EndPoint = "tcp://"+m_cAddr.ToString();
  //  m_Id += "_"+m_EndPoint;
    zmq_setsockopt(m_cP2PSocket, ZMQ_IDENTITY, m_Id.c_str(), m_Id.length());

    printf("Znode Endpoint [%s], id[%s]\n", m_EndPoint.c_str(), m_Id.c_str());
    int rc = zmq_connect(m_cP2PSocket, m_EndPoint.c_str());
    assert(0 == rc);
}

ZNode::ZNode(const string& id, void* ctx, const char* endPoint)
: m_bInbound(false),
  m_bNetworkNode(false),
  m_bDisconnect(false),
  m_bClient(false),
  m_nRefCount(0),
  m_nVersion(0),
  m_nReleaseTime(0),
  m_nServices(0),
  m_cAddr(endPoint+4),
  m_Id(id),
  m_ServId(),
  m_cP2PSocket(nullptr),
  m_SendLst(),
  m_RecvLst(),
  m_cSndMtx(),
  m_cRcvMtx()
{
    assert(endPoint != NULL);
    m_cP2PSocket  = zmq_socket(ctx, ZMQ_DEALER);
    m_EndPoint = endPoint;
    
    zmq_setsockopt(m_cP2PSocket, ZMQ_IDENTITY, m_Id.c_str(), m_Id.length());

    printf("Znode Endpoint [%s], id[%s]\n", m_EndPoint.c_str(), m_Id.c_str());
    int rc = zmq_connect(m_cP2PSocket, m_EndPoint.c_str());
    assert(0 == rc);
}

ZNode::~ZNode()
{
    foreach(PB_MessageData* pRecv, m_RecvLst)
    {
        delete pRecv;
    }
    m_RecvLst.clear();
    foreach(PB_MessageData*pSend, m_SendLst)
    {
        delete pSend;
    }
    m_SendLst.clear();
    zmq_close(m_cP2PSocket);
}

bool ZNode::pingNode()
{
    zmq_pollitem_t items[] = {{m_cP2PSocket, 0, ZMQ_POLLIN, 0}};
    printf("ZNode::pingNode [%s]\n", m_EndPoint.c_str());
    s_sendmore(m_cP2PSocket, "");
    s_send(m_cP2PSocket, "ping");// ping the server 
    //wait the server reply the pong cmd, we will wait no more than 6s
    int rc = zmq_poll(items, 1, 6000*3);
    if (-1 ==rc) return false;
    
    if (rc != 0) {
        char*emp = s_recv(m_cP2PSocket);

        char* rep = s_recv(m_cP2PSocket);
        m_ServId = rep;
        printf("pingNode Has Data[%s]\n", rep);
        free(emp);
        free(rep);
        return true;
    }

    return false;
}


const char* ZNode::getServId()const
{
    return m_ServId.c_str();
}

void ZNode::Disconnect()
{
    printf("disconnecting node %s\n",  m_Id.c_str());

    map<string, CAddress>& mapAddresses = Enze::NetWorkServ::getInstance()->getMapAddr();
    // If outbound and never got version message, mark address as failed
    if (!m_bInbound)
        mapAddresses[m_cAddr.GetKey()].nLastFailed = GetTime();

    zmq_disconnect(m_cP2PSocket, m_EndPoint.c_str());
    m_bDisconnect = true;
    // All of a nodes broadcasts and subscriptions are automatically torn down
    // when it goes down, so a node has to stay up to keep its broadcast going.

  /*  for (std::map<uint256, CProduct>::iterator mi = mapProducts.begin(); mi != mapProducts.end();)
        Enze::NetWorkServ::getInstance()->AdvertRemoveSource(this, MSG_PRODUCT, 0, (*(mi++)).second);
    */
}
// ׼���ͷ�����
bool ZNode::ReadyToDisconnect()
{
    return m_bDisconnect || GetRefCount() <= 0;
}

// ��ȡ��Ӧ��Ӧ�ü���
int ZNode::GetRefCount()
{
    return max(m_nRefCount, 0) + (GetTime() < m_nReleaseTime ? 1 : 0);
}

// ���Ӷ�Ӧ��Ӧ�ü���
void ZNode::AddRef(int64 nTimeout)
{
    if (nTimeout != 0)
        m_nReleaseTime = max(m_nReleaseTime, GetTime() + nTimeout); // �Ƴٽڵ��Ӧ���ͷ�ʱ��
    else
        m_nRefCount++;

}

// �ڵ��ͷŶ�Ӧ�����Ӧ��Ӧ�ü�����1
void ZNode::Release()
{
    --m_nRefCount;
}

void ZNode::SendVersion()
{
    printf("ZNode::SendVersion\n");
    int64 nTime = (m_bInbound ? GetAdjustedTime() : GetTime());
    PB_MessageData *cProtoc = new PB_MessageData();
    cProtoc->set_emsgkind(PB_MessageData_Mesg_Kind_MK_Version);
    Version* pVersion = cProtoc->mutable_cversion();
    pVersion->set_utime(nTime);
    pVersion->set_nversion(VERSION);
    pVersion->set_nservices(NODE_NETWORK);
    ::SeriaAddress(m_cAddr, *pVersion->mutable_addrme());
    
    string strData;
    cProtoc->SerializePartialToString(&strData);
    //s_sendmore(socket, m_EndPoint.c_str());
    s_sendmore(m_cP2PSocket, "");
    s_sendmore(m_cP2PSocket, "data");
    s_send(m_cP2PSocket, (char*)strData.c_str());
    delete cProtoc;

}

void ZNode::SendGetBlocks(const CBlockLocator& local, const uint256& hash)
{
    PB_MessageData *cProtoc = new PB_MessageData();
    cProtoc->set_emsgkind(PB_MessageData_Mesg_Kind_MK_GetBlocks);
    GetBlocks* pGetBlock = cProtoc->mutable_getblocks();
    pGetBlock->set_hashstop(hash.ToString());
    GetBlocks::BlockLocator* pLoc = pGetBlock->mutable_locator();
    foreach(auto it, local.vHave)
    {
        pLoc->add_vhave(it.ToString());
    }
    
    std::lock_guard<std::mutex> guard(m_cSndMtx);
    m_SendLst.push_back(cProtoc);
}

void ZNode::SendAddr(const vector<CAddress>& addr)
{
    if (0 == addr.size()) return; 
    
    PB_MessageData *cProtoc = new PB_MessageData();
    cProtoc->set_emsgkind(PB_MessageData_Mesg_Kind_MK_Addr);
    foreach(auto it, addr) 
    {
        ::SeriaAddress(it, *cProtoc->add_vaddr()); 
    }
    std::lock_guard<std::mutex> guard(m_cSndMtx);
    m_SendLst.push_back(cProtoc);
}

void ZNode::SendTx(const CTransaction& tx)
{
    PB_MessageData *cProtoc = new PB_MessageData();
    cProtoc->set_emsgkind(PB_MessageData_Mesg_Kind_MK_Tx);
    ::SeriaTransaction(tx, *cProtoc->mutable_tx());
    std::lock_guard<std::mutex> guard(m_cSndMtx);
    m_SendLst.push_back(cProtoc);
}
    
void ZNode::SendBlock(const CBlock& block)
{
    PB_MessageData *cProtoc = new PB_MessageData();
    cProtoc->set_emsgkind(PB_MessageData_Mesg_Kind_MK_Block);
    ::SeriaBlock(block, *cProtoc->mutable_cblock());
    
    std::lock_guard<std::mutex> guard(m_cSndMtx);
    m_SendLst.push_back(cProtoc);
}

void ZNode::SendGetAddrRequest()
{
    PB_MessageData *cProtoc = new PB_MessageData();
    cProtoc->set_emsgkind(PB_MessageData_Mesg_Kind_MK_GetAddr);
    std::lock_guard<std::mutex> guard(m_cSndMtx);
    m_SendLst.push_back(cProtoc);
}
    
void ZNode::SendInv(const vector<CInv>& vInv, bool bReqInv)
{
    PB_MessageData *cProtoc = new PB_MessageData();
    if (!bReqInv)
        cProtoc->set_emsgkind(PB_MessageData_Mesg_Kind_MK_Inv);
    else 
        cProtoc->set_emsgkind(PB_MessageData_Mesg_Kind_MK_GetData);
    foreach(auto it, vInv) {
        SeriaInv(it, *(cProtoc->add_vinv()));
    }

    printf("ZNode::SendInv,DataSize[%d],%s---%d\n", cProtoc->ByteSizeLong(),__FILE__, __LINE__);
    std::lock_guard<std::mutex> guard(m_cSndMtx);
    m_SendLst.push_back(cProtoc);
}

// ���ӿ��
void ZNode::AddInventoryKnown(const CInv& inv)
{
    m_setInventoryKnown.insert(inv);
}

// ���Ϳ��
void ZNode::PushInventory(const CInv& inv)
{
    if (!m_setInventoryKnown.count(inv))
        m_vInventoryToSend.push_back(inv);
}

void ZNode::AskFor(const CInv& inv)
{
    // We're using mapAskFor as a priority queue, ���ȼ�����
    // the key is the earliest time the request can be sent ��key��Ӧ�����������类���͵�ʱ�䣩
    int64& nRequestTime = Enze::NetWorkServ::getInstance()->getMapAlreadyAskedFor()[inv];
    printf("askfor %s  %I64d\n", inv.ToString().c_str(), nRequestTime);

    // ȷ����Ҫʱ��������������ͬһ��˳��
    // Make sure not to reuse time indexes to keep things in the same order
    int64 nNow = (GetTime() - 1) * 1000000; // ��λ��΢��
    static int64 nLastTime;
    nLastTime = nNow = max(nNow, ++nLastTime);//������úܿ�Ļ������Ա�֤��Ӧ��nlastTime++�ǵĶ�Ӧ��ʱ�䲻һ��

    // Each retry is 2 minutes after the last��û�е�2���ӣ����Ӧ��nRequesttime��Ӧ��ֵ��һ��
    nRequestTime = max(nRequestTime + 2 * 60 * 1000000, nNow);
    m_mapAskFor.insert(make_pair(nRequestTime, inv));
}




void ZNode::AddRecvMessage(PB_MessageData* pRecvData)
{
    std::lock_guard<std::mutex> guard(m_cRcvMtx);
    m_RecvLst.push_back(pRecvData);
}

void ZNode::Recv()
{
    char* req = s_recv(m_cP2PSocket);
    std::lock_guard<std::mutex> guard(m_cRcvMtx);
    PB_MessageData *cProtoc = new PB_MessageData();
    cProtoc->ParsePartialFromString(req);
    free(req);
    m_RecvLst.push_back(cProtoc);
}

void ZNode::Send()
{
    if (!pingNode()) {
        printf("ZNode::Send--Serv[%s] is offline\n", m_ServId.c_str());
        m_bDisconnect = true;
        return;
    }
    if (0 == m_nVersion) {
        SendVersion();
        printf("Do not Get peer Node Version Data ,will not send data to it\n");
        return ;
    }
    std::lock_guard<std::mutex> guard(m_cSndMtx);
    while(!m_SendLst.empty()) {
        PB_MessageData* pData = m_SendLst.front();
        string strData;
        pData->SerializePartialToString(&strData);
        //s_sendmore(socket, m_EndPoint.c_str());
        s_sendmore(m_cP2PSocket, "");
        s_sendmore(m_cP2PSocket, "data");
        s_send(m_cP2PSocket, (char*)strData.c_str());
        m_SendLst.pop_front();
        delete pData;
    }
}

bool ZNode::ProcessMsg()
{
    printf("ZNode::ProcessMsg()---start\n");
    PB_MessageData* pData = popRecvMsg();
   
    bool bFinish = (!pData);
    if (!bFinish) {
        printf("ZNode::ProcessMsg()---recv Data\n");
        PB_MessageData_Mesg_Kind eMsgKind = pData->emsgkind();
        static map<unsigned int, vector<unsigned char> > mapReuseKey;
        printf("ZNode::ProcessMsg()---recv Data Kind[%d]\n", eMsgKind);
#if 0
        // ��Ϣ�ɼ�Ƶ�ʽ��д���
        if (nDropMessagesTest > 0 && GetRand(nDropMessagesTest) == 0)
        {
            printf("dropmessages DROPPING RECV MESSAGE\n");
            return true;
        }
#endif 
        switch(eMsgKind) {
            // ��������ǰ汾���ڵ��Ӧ�İ汾
            case PB_MessageData_Mesg_Kind_MK_Version:
                    ProcessVerMsg(pData->cversion());
                    break;

            case PB_MessageData_Mesg_Kind_MK_Addr:
                    foreach(auto it, pData->vaddr()) {
                        ProcessAddrMsg(it); 
                    }
                    break;
                    
            case PB_MessageData_Mesg_Kind_MK_Inv:
                    foreach(auto it, pData->vinv())
                        ProcessInvMsg(it); 
                    break;

            case PB_MessageData_Mesg_Kind_MK_GetData:
                    foreach(auto it, pData->vinv())
                        ProcessGetDataMsg(it);  
                    break;
                    
            case PB_MessageData_Mesg_Kind_MK_GetBlocks:
                    ProcessGetBlockMsg(pData->getblocks());
                    break;
                    
            case PB_MessageData_Mesg_Kind_MK_Tx:
                    ProcessTxMsg(pData->tx());
                    break;
                    
            case PB_MessageData_Mesg_Kind_MK_Review:
                    ProcessReviewMsg(pData->creview());
                    break;
                    
            case PB_MessageData_Mesg_Kind_MK_Block:
                    ProcessBlockMsg(pData->cblock());
                    break;

            case PB_MessageData_Mesg_Kind_MK_GetAddr:
                    ProcessGetAddrMsg(); 
                    break;
                    
            case PB_MessageData_Mesg_Kind_MK_CheckOrder:
                    ProcessCheckOrderMsg(pData->corder(), mapReuseKey);
                    break;
                    
            case PB_MessageData_Mesg_Kind_MK_SubmitOrder:
                    ProcessSubmitOrderMsg(pData->corder(), mapReuseKey);
                    break;
            
            case PB_MessageData_Mesg_Kind_MK_Reply:
                    ProcessHashReplyMsg(pData->hashreply());
                    break;
        
        }

    }   
    printf("ZNode::ProcessMsg()---end\n");
    delete pData;
    pData = NULL;
    return bFinish;
}

void ZNode::ProcessVerMsg(const Version& cVer)
{
    // �ڵ��Ӧ�İ汾ֻ�ܸ���һ�Σ���ʼΪ0��������и���
    // Can only do this once
    if (m_nVersion != 0)
    {
        // �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        printf("ProcessVerMsg Can only do this once,: %s---%d\n", __FILE__, __LINE__);
        return ;
    }

    int64 uTime;
    CAddress addrMe; // ��ȡ��Ϣ��Ӧ������
    m_nVersion = cVer.nversion();
    m_nServices = cVer.nservices();
    uTime = cVer.utime();
    ::UnSeriaAddress(cVer.addrme(), addrMe);
    if (m_nVersion == 0) {
        printf("ZNode::ProcessVerMsg Recv Version[%d]\n", m_nVersion);
        return ;
    }
    // ���·��ͺͽ��ջ������еĶ�Ӧ�İ汾
    //pfrom->vSend.SetVersion(min(pfrom->nVersion, VERSION));
    //pfrom->vRecv.SetVersion(min(pfrom->nVersion, VERSION));

    // ����ڵ��Ӧ�ķ��������ǽڵ����磬���Ӧ�ڵ�Ŀͻ��˱�Ǿ���false
    m_bClient = !(m_nServices & NODE_NETWORK);
    if (m_bClient)
    {
        printf("error %s_%d\n", __FILE__, __LINE__);
        // ������ǽڵ����磬���ܽ�����һЩ�ڵ㲻Ҫ�����Ӧ������������Ϣ��������Ҫ�����ͷ������У��Ϳ�����
        //pfrom->vSend.nType |= SER_BLOCKHEADERONLY;
        //pfrom->vRecv.nType |= SER_BLOCKHEADERONLY;
    }
    // ����ʱ���������ݣ�û��ʲô�ô��������������
    AddTimeData(m_cAddr.ip, uTime);

    // �Ե�һ�������Ľڵ�����block��Ϣ
    // Ask the first connected node for block updates
    static bool fAskedForBlocks;
    if (!fAskedForBlocks && m_bClient)
    {
        fAskedForBlocks = true;
        SendGetBlocks(CBlockLocator(BlockEngine::getInstance()->getBestIndex()), uint256(0));
    }

    printf("version message: %s has version %d, addrMe=%s\n", m_cAddr.ToString().c_str(), m_nVersion, addrMe.ToString().c_str());

}

void ZNode::ProcessAddrMsg(const Address& pbAddr)
{
    if (m_nVersion == 0)
    {
        // �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        printf("Must have a version message before anything: %s---%d\n", __FILE__, __LINE__);
        return ;
    }
    
    CAddress addr;
    ::UnSeriaAddress(pbAddr, addr);
    // Store the new addresses
    // ����ַ���ӵ����ݿ���
    if (DaoServ::getInstance()->WriteAddress(addr))
    {
        // Put on lists to send to other nodes
        m_setAddrKnown.insert(addr); // ����Ӧ�ĵ�ַ���뵽��֪��ַ������
        map<string, ZNode*>& cZNodeLst = NetWorkServ::getInstance()->getNodeList();
        foreach(auto it, cZNodeLst) 
        {
            ZNode* pnode = it.second;
            if (!pnode->isExsitAddr(addr))
                pnode->AddAddr2Send(addr);// ��ַ�Ĺ㲥
        }
    }

}

void ZNode::ProcessInvMsg(const Inventory& pbInv)
{
    printf("ZNode::ProcessInvMsg()---start\n");
    if (m_nVersion == 0)
    {
        // �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        printf("Must have a version message before anything: %s---%d\n", __FILE__, __LINE__);
        return ;
    }
    
    CInv inv;
    UnSeriaInv(pbInv, inv);
    AddInventoryKnown(inv); // ����Ӧ�Ŀ�淢����Ϣ���ӵ���淢����֪��

    bool fAlreadyHave = WalletServ::getInstance()->AlreadyHave(inv);
    printf("  got inventory: %s  %s\n", inv.ToString().c_str(), fAlreadyHave ? "have" : "new");

    if (!fAlreadyHave)
        AskFor(inv);// ��������ڣ���������ѯ����������߳��з���getdata��Ϣ
    else if (inv.type == MSG_BLOCK && BlockEngine::getInstance()->mapOrphanBlocks.count(inv.hash))
        SendGetBlocks(CBlockLocator(BlockEngine::getInstance()->getBestIndex()), BlockEngine::getInstance()->GetOrphanRoot(BlockEngine::getInstance()->mapOrphanBlocks[inv.hash]));

}

void ZNode::ProcessGetDataMsg(const Inventory& pbInv)
{
    if (m_nVersion == 0)
    {
        // �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        printf("Must have a version message before anything: %s---%d\n", __FILE__, __LINE__);
        return ;
    }
    CInv inv;
    UnSeriaInv(pbInv, inv);

    printf("received getdata for: %s\n", inv.ToString().c_str());

    if (inv.type == MSG_BLOCK)
    {
        // Send block from disk
        map<uint256, CBlockIndex*>::const_iterator mi = BlockEngine::getInstance()->getMapBlockIndex().find(inv.hash);
        if (mi != BlockEngine::getInstance()->getMapBlockIndex().end())
        {
            //// could optimize this to send header straight from blockindex for client
            CBlock block;
            block.ReadFromDisk((*mi).second, !m_bClient);
            SendBlock(block);// ��ȡ���ݶ�Ӧ��������block�����Ͷ�Ӧ�Ŀ���Ϣ
        }
    }
    else if (inv.IsKnownType())
    {
        printf("error %s---%d\n", __FILE__, __LINE__);
        // Send stream from relay memory
#if 0
        map<CInv, CDataStream>::iterator mi = mapRelay.find(inv); // ����ת��������
        if (mi != mapRelay.end())
            pfrom->PushMessage(inv.GetCommand(), (*mi).second);
#endif
    }

}

void ZNode::ProcessGetBlockMsg(const GetBlocks& cBks)
{
    if (m_nVersion == 0)
    {
        // �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        printf("Must have a version message before anything: %s---%d\n", __FILE__, __LINE__);
        return ;
    }
    
    CBlockLocator locator;
    UnSeriaLoctor(cBks.locator(), locator);
    uint256 hashStop(cBks.hashstop());

    //�ҵ������е����������ϵ�
    // Find the first block the caller has in the main chain
    CBlockIndex* pindex = const_cast<CBlockIndex*>(locator.GetBlockIndex());

    // ��ƥ��õ��Ŀ�����֮��������������ϵĿ鷢�ͳ�ȥ
    // Send the rest of the chain
    if (pindex)
        pindex = pindex->m_pNextBlkIndex;
    printf("getblocks %d to %s\n", (pindex ? pindex->m_nCurHeight : -1), hashStop.ToString().substr(0,14).c_str());
    for (; pindex; pindex = pindex->m_pNextBlkIndex)
    {
        if (pindex->GetBlockHash() == hashStop)
        {
            printf("  getblocks stopping at %d %s\n", pindex->m_nCurHeight, pindex->GetBlockHash().ToString().substr(0,14).c_str());
            break;
        }

        // Bypass setInventoryKnown in case an inventory message got lost
        CInv inv(MSG_BLOCK, pindex->GetBlockHash());
        // �ж�����֪���2���Ƿ����
        // returns true if wasn't already contained in the set
        if (m_setInventoryKnown2.insert(inv).second)
        {
            m_setInventoryKnown.erase(inv);
            m_vInventoryToSend.push_back(inv);// �����Ӧ�Ŀ�淢�ͼ�����׼�����ͣ�����һ���߳��н��з��ͣ����͵���ϢΪinv
        }
    }

}

void ZNode::ProcessTxMsg(const Transaction& pbTx)
{
    if (m_nVersion == 0)
    {
        // �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        printf("Must have a version message before anything: %s---%d\n", __FILE__, __LINE__);
        return ;
    }
    vector<uint256> vWorkQueue;
    CTransaction tx;
    UnSeriaTransaction(pbTx, tx);

    CInv inv(MSG_TX, tx.GetHash());
    AddInventoryKnown(inv);// ��������Ϣ���뵽��Ӧ����֪�����

    bool fMissingInputs = false;
    // ��������ܹ�������
    if (tx.AcceptTransaction(true, &fMissingInputs))
    {
        WalletServ::getInstance()->AddToWalletIfMine(tx, NULL);
        // RelayMessage(inv, vMsg);// ת����Ϣ need define the function
        NetWorkServ::getInstance()->removeAlreadyAskedFor(inv);
        vWorkQueue.push_back(inv.hash);

        // �ݹ鴦����������������׶�Ӧ�Ĺ¶�����
        // Recursively process any orphan transactions that depended on this one
        for (int i = 0; i < vWorkQueue.size(); i++)
        {
            printf("error:%s---%d\n", __FILE__, __LINE__);
            uint256 hashPrev = vWorkQueue[i];
            for (multimap<uint256, CTransaction*>::iterator mi = WalletServ::getInstance()->mapOrphanTransactionsByPrev.lower_bound(hashPrev);
                 mi != WalletServ::getInstance()->mapOrphanTransactionsByPrev.upper_bound(hashPrev);
                 ++mi)
            {
                CTransaction tx = *((*mi).second);
                CInv inv(MSG_TX, tx.GetHash());

                if (tx.AcceptTransaction(true))
                {
                    printf("   accepted orphan tx %s\n", inv.hash.ToString().substr(0,6).c_str());
                    WalletServ::getInstance()->AddToWalletIfMine(tx, NULL);
                //    RelayMessage(inv, vMsg);
                    NetWorkServ::getInstance()->removeAlreadyAskedFor(inv);
                    vWorkQueue.push_back(inv.hash);
                }
            }
        }

        foreach(uint256 hash, vWorkQueue)
            WalletServ::getInstance()->EraseOrphanTx(hash);
    }
    else if (fMissingInputs)
    {
        printf("storing orphan tx %s\n", inv.hash.ToString().substr(0,6).c_str());
        WalletServ::getInstance()->AddOrphanTx(pbTx); // ������׵�ǰ�����������Ӧ�Ĺ¶�����
    }

}

void ZNode::ProcessReviewMsg(const Review& cRev)
{
    if (m_nVersion == 0)
    {
        // �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        printf("Must have a version message before anything: %s---%d\n", __FILE__, __LINE__);
        return ;
    }
    CReview review;
    UnSeriaReview(cRev, review);

    CInv inv(MSG_REVIEW, review.GetHash());
    AddInventoryKnown(inv);

    if (review.AcceptReview())
    {
        // Relay the original message as-is in case it's a higher version than we know how to parse
        printf("%s---%d\n", __FILE__, __LINE__);
        //RelayMessage(inv, vMsg);
        NetWorkServ::getInstance()->removeAlreadyAskedFor(inv);
    }

}


void ZNode::ProcessBlockMsg(const Block& cbk)
{
    if (m_nVersion == 0)
    {
        // �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        printf("Must have a version message before anything: %s---%d\n", __FILE__, __LINE__);
        return ;
    }
    
    auto_ptr<CBlock> pblock(new CBlock);
    UnSeriaBlock(cbk, *pblock);

    //// debug print
    printf("received block:\n"); pblock->print();

    CInv inv(MSG_BLOCK, pblock->GetHash());
    AddInventoryKnown(inv);// ���ӿ��

    if (BlockEngine::getInstance()->ProcessBlock(this, pblock.release()))
        NetWorkServ::getInstance()->removeAlreadyAskedFor(inv);

}

void ZNode::ProcessHashReplyMsg(const string& strReply)
{
    if (m_nVersion == 0)
    {
        // �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        printf("Must have a version message before anything: %s---%d\n", __FILE__, __LINE__);
        return ;
    }
    
    uint256 hashReply(strReply);

    CRequestTracker tracker;
    map<uint256, CRequestTracker>::iterator mi = m_mapRequests.find(hashReply);
    if (mi != m_mapRequests.end())
    {
        tracker = (*mi).second;
        m_mapRequests.erase(mi);
    }
    if (!tracker.IsNull()) {
        printf("%s---%d\n", __FILE__, __LINE__);
        //tracker.fn(tracker.param1, vRecv);
    
    }

}

void ZNode::ProcessGetAddrMsg()
{

    m_vAddrToSend.clear();
    int64 nSince = GetAdjustedTime() - 5 * 24 * 60 * 60; // in the last 5 days
    map<string, CAddress>& cMapAddr = NetWorkServ::getInstance()->getMapAddr();
    unsigned int nSize = cMapAddr.size();
    foreach(const PAIRTYPE(string, CAddress)& item, cMapAddr)
    {
        const CAddress& addr = item.second;
        //// will need this if we lose IRC
        //if (addr.nTime > nSince || (rand() % nSize) < 500)
        if (addr.nTime > nSince)
            m_vAddrToSend.push_back(addr);
    }
}


void ZNode::ProcessSubmitOrderMsg(const Order& cOrd, map<unsigned int, vector<unsigned char> >& mapReuseKey)
{
    if (m_nVersion == 0)
    {
        // �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        printf("Must have a version message before anything: %s---%d\n", __FILE__, __LINE__);
        return ;
    }
    uint256 hashReply(cOrd.hashreply());
    CWalletTx wtxNew;
    UnSeriaWalletTx(cOrd.txorder(), wtxNew);
   // vRecv >> hashReply >> wtxNew;

    // Broadcast
    if (!wtxNew.AcceptWalletTransaction())
    {
        printf("%s---%d\n", __FILE__, __LINE__);
        //PushMessage("reply", hashReply, (int)1);
        error("submitorder AcceptWalletTransaction() failed, returning error 1");
        return ;
    }
    wtxNew.m_bTimeReceivedIsTxTime = true;
    WalletServ::getInstance()->AddToWallet(wtxNew);
    wtxNew.RelayWalletTransaction();
    mapReuseKey.erase(m_cAddr.ip);

    // Send back confirmation
    printf("%s---%d\n", __FILE__, __LINE__);
    //PushMessage("reply", hashReply, (int)0);

}

void ZNode::ProcessCheckOrderMsg(const Order& cOrd, map<unsigned int, vector<unsigned char> >& mapReuseKey)
{
    if (m_nVersion == 0)
    {
        // �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        printf("Must have a version message before anything: %s---%d\n", __FILE__, __LINE__);
        return ;
    }
    uint256 hashReply(cOrd.hashreply());
    CWalletTx order;
    UnSeriaWalletTx(cOrd.txorder(), order);

    /// we have a chance to check the order here

    // Keep giving the same key to the same ip until they use it
    if (!mapReuseKey.count(m_cAddr.ip))
        mapReuseKey[m_cAddr.ip] = WalletServ::getInstance()->GenerateNewKey();

    // Send back approval of order and pubkey to use
    CScript scriptPubKey;
    scriptPubKey << mapReuseKey[m_cAddr.ip] << OP_CHECKSIG;
    printf("%s---%d\n", __FILE__, __LINE__);
    //PushMessage("reply", hashReply, (int)0, scriptPubKey);

}

PB_MessageData* ZNode::popRecvMsg()
{
    PB_MessageData* pData = NULL;
    std::lock_guard<std::mutex> guard(m_cRcvMtx);
    if (!m_RecvLst.empty()) {
        pData = m_RecvLst.front();
        m_RecvLst.pop_front();
    }
    
    return pData;
}

void ZNode::SendSelfAddr()
{
    // ��Ϣ���͵ĵ�ַ
    // Message: addr
    //
    vector<CAddress> vAddrToSend;
    vAddrToSend.reserve(m_vAddrToSend.size());
    // ������͵ĵ�ַ������֪��ַ�ļ����У����������ʱ��ַ����������
    foreach(const CAddress& addr, m_vAddrToSend)
        if (!m_setAddrKnown.count(addr))
            vAddrToSend.push_back(addr);
    // ��յ�ַ���͵�����
    m_vAddrToSend.clear();
    // �����ʱ��ַ�������鲻Ϊ�գ�����е�ַ����Ϣ�ķ���
    if (!vAddrToSend.empty())
        SendAddr(vAddrToSend);


}

void ZNode::SendSelfInv()
{
    // �����Ϣ����
    // Message: inventory
    //
    vector<CInv> vInventoryToSend;
    vInventoryToSend.reserve(m_vInventoryToSend.size());
    foreach(const CInv& inv, m_vInventoryToSend)
    {
        // returns true if wasn't already contained in the set
        if (m_setInventoryKnown.insert(inv).second)
            vInventoryToSend.push_back(inv);
    }
    m_vInventoryToSend.clear();
    m_setInventoryKnown2.clear();
    // �����Ϣ����
    if (!vInventoryToSend.empty())
        SendInv(vInventoryToSend, false);

}

void ZNode::SendGetDataReq()
{
    // getdata��Ϣ����
    // Message: getdata
    //
    vector<CInv> vAskFor;
    int64 nNow = GetTime() * 1000000;
    // �жϽڵ��Ӧ��������Ϣmap�Ƿ�Ϊ�գ��Ҷ�Ӧ������map�е���Ϣ��Ӧ����������ʱ���Ƿ�С�ڵ�ǰʱ��
    while (!m_mapAskFor.empty() && (*m_mapAskFor.begin()).first <= nNow)
    {
        const CInv& inv = (*m_mapAskFor.begin()).second;
        printf("sending getdata: %s\n", inv.ToString().c_str());
        if (!WalletServ::getInstance()->AlreadyHave(inv))
            vAskFor.push_back(inv);// �����ڲ���Ҫ������Ϣ����
        m_mapAskFor.erase(m_mapAskFor.begin());// ������Ϣ������һ����ɾ��һ��
    }
    if (!vAskFor.empty())
        SendInv(vAskFor, true); //send get Data

}
/* EOF */

