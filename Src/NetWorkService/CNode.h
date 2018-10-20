/*
 * =====================================================================================
 *
 *       Filename:  CNode.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 08:19:29 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef EZ_BT_CNODE_H
#define EZ_BT_CNODE_H
#include "NetWorkService/Net_DataDef.h"
#include "NetWorkService/CAddress.h"
#include "NetWorkService/CInv.h"
#include "NetWorkService/CMessageHeader.h"
#include "NetWorkService/CRequestTracker.h"
#include "ProtocSrc/Message.pb.h"
namespace Enze
{
class CBlockLocator;
class CTransaction;
class CBlock;
// �ڵ㶨��
class CNode
{

public:
    CNode(SOCKET hSocketIn, const CAddress& addrIn, bool fInboundIn=false);
    ~CNode();

    bool IsSubscribed(unsigned int nChannel);
    void Subscribe(unsigned int nChannel, unsigned int nHops=0);
    void CancelSubscribe(unsigned int nChannel);
    void Disconnect();
    void AskFor(const CInv& inv);

    // ׼���ͷ�����
    bool ReadyToDisconnect();
    // ��ȡ��Ӧ��Ӧ�ü���
    int GetRefCount();
    // ���Ӷ�Ӧ��Ӧ�ü���
    void AddRef(int64 nTimeout=0);
    // �ڵ��ͷŶ�Ӧ�����Ӧ��Ӧ�ü�����1
    void Release();

    // ���ӿ��
    void AddInventoryKnown(const CInv& inv);

    // ���Ϳ��
    void PushInventory(const CInv& inv);


    void BeginMessage(const char* pszCommand);

    void AbortMessage();
	// �޸���Ϣͷ�ж�Ӧ����Ϣ��С�ֶ�
    void EndMessage();

    void EndMessageAbortIfEmpty();

    const char* GetMessageCommand() const;

    void PushMessage(const char* pszCommand);

    void SendVersion(int64 nTime);
	
    void SendGetBlocks(const CBlockLocator& local, const uint256& hash);
   
    void SendAddr(const vector<CAddress>& addr);
    void SendTx(const CTransaction& tx);
    void SendBlock(const CBlock& block);
    
    // ����Ϣ���Ͷ�Ӧ�ڵ��vsend������
    template<typename T1>
    void PushMessage(const char* pszCommand, const T1& a1)
    {
        printf("error %s--%d\n", __FILE__, __LINE__);
        try
        {
            BeginMessage(pszCommand);
        //    vSend << a1;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2)
    {
        printf("error %s--%d\n", __FILE__, __LINE__);
        try
        {
            BeginMessage(pszCommand);
        //    vSend << a1 << a2;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3)
    {
        printf("error %s--%d\n", __FILE__, __LINE__);
        try
        {
            BeginMessage(pszCommand);
        //    vSend << a1 << a2 << a3;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4)
    {
        printf("error %s--%d\n", __FILE__, __LINE__);
        try
        {
            BeginMessage(pszCommand);
        //    vSend << a1 << a2 << a3 << a4;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }


    void PushRequest(const char* pszCommand,
                     void (*fn)(void*, CDataStream&), void* param1)
    {
        uint256 hashReply;
        RAND_bytes((unsigned char*)&hashReply, sizeof(hashReply));

       // CRITICAL_BLOCK(cs_mapRequests)
        mapRequests[hashReply] = CRequestTracker(fn, param1);

        PushMessage(pszCommand, hashReply);
    }

    template<typename T1>
    void PushRequest(const char* pszCommand, const T1& a1,
                     void (*fn)(void*, CDataStream&), void* param1)
    {
        uint256 hashReply;
        RAND_bytes((unsigned char*)&hashReply, sizeof(hashReply));

       // CRITICAL_BLOCK(cs_mapRequests)
        mapRequests[hashReply] = CRequestTracker(fn, param1);

        PushMessage(pszCommand, hashReply, a1);
    }

    template<typename T1, typename T2>
    void PushRequest(const char* pszCommand, const T1& a1, const T2& a2,
                     void (*fn)(void*, CDataStream&), void* param1)
    {
        uint256 hashReply;
        RAND_bytes((unsigned char*)&hashReply, sizeof(hashReply));

       // CRITICAL_BLOCK(cs_mapRequests)
            mapRequests[hashReply] = CRequestTracker(fn, param1);

        PushMessage(pszCommand, hashReply, a1, a2);
    }



protected:
    int nRefCount; // ʹ�ü�����

public:
    // socket
    uint64 m_nServices;
    SOCKET hSocket;
    list<PB_MessageData> vSend; // ���ͻ�����
    list<PB_MessageData> vRecv; // ���ջ�����
    unsigned int nPushPos;// ָ���������Ѿ����͵�λ��
    CAddress addr;
    int nVersion; // �ڵ��Ӧ�İ汾������ڵ�汾Ϊ0������Ϣ���Ͳ���ȥ
    bool fClient;// �Ƚ��Ƿ��ǿͻ��ˣ�����ǿͻ�������Ҫ�����ͷ������У��Ϳ�����,����Ҫ�����������������
    bool fInbound;
    bool fNetworkNode; // ���ö�Ӧ�Ľڵ�Ϊ����ڵ㣬����Ϊ�Ӷ�Ӧ�ı��ؽڵ��б���û�в�ѯ��
    bool fDisconnect; // �˿����ӵı��
    int64 nReleaseTime; // �ڵ��ͷŵ�ʱ��
    map<uint256, CRequestTracker> mapRequests;
   // CCriticalSection cs_mapRequests;

    // flood �鷺����ַ��Ϣ������Ϊaddr
    vector<CAddress> vAddrToSend; // ��Ϣ��Ҫ���Ͷ�Ӧ�ĵ�ַ������Ҫ���͵ĵ�ַ������֪��ַ�ļ��Ϲ���֮���ٷ���
    set<CAddress> setAddrKnown; // ��֪��ַ�ļ���

    // inventory based relay  ����ת���Ŀ�棺�����Ϣ������Ϊinv
    set<CInv> setInventoryKnown; // ��֪���ļ���
    set<CInv> setInventoryKnown2;
    vector<CInv> vInventoryToSend; //���׼�����͵ļ��ϣ��Կ��׼�����͵ļ��ϸ�����֪���ļ��Ͻ��й���֮���ڷ���
   // CCriticalSection cs_inventory;
    multimap<int64, CInv> mapAskFor; // ��ѯ����ӳ�䣬keyΪʱ�䣨��λ��΢�룩

    // publish and subscription
    vector<char> vfSubscribe;
private:
    CNode(const CNode&);
    void operator=(const CNode&);
};

} //end namespace

#endif /* EZ_BT_CNODE_H */

/* EOF */

