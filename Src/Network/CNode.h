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

#ifndef CXX_BT_CNODE_H
#define CXX_BT_CNODE_H
#include "BlockEngine.h"
#include "Network/Net_DataDef.h"
#include "Network/CAddress.h"
#include "Network/CInv.h"
#include "Network/CMessageHeader.h"
#include "Network/CRequestTracker.h"
extern map<CInv, int64> mapAlreadyAskedFor;

// �ڵ㶨��
class CNode
{
public:
    // socket
    uint64 m_nServices;
    SOCKET hSocket;
    CDataStream vSend; // ���ͻ�����
    CDataStream vRecv; // ���ջ�����
    unsigned int nPushPos;// ָ���������Ѿ����͵�λ��
    CAddress addr;
    int nVersion; // �ڵ��Ӧ�İ汾������ڵ�汾Ϊ0������Ϣ���Ͳ���ȥ
    bool fClient;// �Ƚ��Ƿ��ǿͻ��ˣ�����ǿͻ�������Ҫ�����ͷ������У��Ϳ�����,����Ҫ�����������������
    bool fInbound;
    bool fNetworkNode; // ���ö�Ӧ�Ľڵ�Ϊ����ڵ㣬����Ϊ�Ӷ�Ӧ�ı��ؽڵ��б���û�в�ѯ��
    bool fDisconnect; // �˿����ӵı��
protected:
    int nRefCount; // ʹ�ü�����
public:
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


    CNode(SOCKET hSocketIn, const CAddress& addrIn, bool fInboundIn=false)
    {
        m_nServices = 0;
        hSocket = hSocketIn;
        vSend.SetType(SER_NETWORK);
        vRecv.SetType(SER_NETWORK);
        nPushPos = -1;
        addr = addrIn;
        nVersion = 0;
        fClient = false; // set by version message
        fInbound = fInboundIn;
        fNetworkNode = false;
        fDisconnect = false;
        nRefCount = 0;
        nReleaseTime = 0;
        vfSubscribe.assign(256, false);

        // Push a version message
        /// when NTP implemented, change to just nTime = GetAdjustedTime()
        int64 nTime = (fInbound ? GetAdjustedTime() : GetTime());
		// �����ڵ��ʱ��ᷢ�ͽڵ�汾����Ϣ����Ϣ����Ϊversion,��������Ϣ���͵�����
        PushMessage("version", VERSION, nLocalServices, nTime, addr);
    }

    ~CNode()
    {
        if (hSocket != INVALID_SOCKET)
            close(hSocket);
    }

private:
    CNode(const CNode&);
    void operator=(const CNode&);
public:

    // ׼���ͷ�����
    bool ReadyToDisconnect()
    {
        return fDisconnect || GetRefCount() <= 0;
    }
    // ��ȡ��Ӧ��Ӧ�ü���
    int GetRefCount()
    {
        return max(nRefCount, 0) + (GetTime() < nReleaseTime ? 1 : 0);
    }
    // ���Ӷ�Ӧ��Ӧ�ü���
    void AddRef(int64 nTimeout=0)
    {
        if (nTimeout != 0)
            nReleaseTime = max(nReleaseTime, GetTime() + nTimeout); // �Ƴٽڵ��Ӧ���ͷ�ʱ��
        else
            nRefCount++;
    }
    // �ڵ��ͷŶ�Ӧ�����Ӧ��Ӧ�ü�����1
    void Release()
    {
        nRefCount--;
    }


    // ���ӿ��
    void AddInventoryKnown(const CInv& inv)
    {
            setInventoryKnown.insert(inv);
    }

    // ���Ϳ��
    void PushInventory(const CInv& inv)
    {
            if (!setInventoryKnown.count(inv))
                vInventoryToSend.push_back(inv);
    }

    void AskFor(const CInv& inv)
    {
        // We're using mapAskFor as a priority queue, ���ȼ�����
        // the key is the earliest time the request can be sent ��key��Ӧ�����������类���͵�ʱ�䣩
        int64& nRequestTime = mapAlreadyAskedFor[inv];
        printf("askfor %s  %I64d\n", inv.ToString().c_str(), nRequestTime);

		// ȷ����Ҫʱ��������������ͬһ��˳��
        // Make sure not to reuse time indexes to keep things in the same order
        int64 nNow = (GetTime() - 1) * 1000000; // ��λ��΢��
        static int64 nLastTime;
        nLastTime = nNow = max(nNow, ++nLastTime);//������úܿ�Ļ������Ա�֤��Ӧ��nlastTime++�ǵĶ�Ӧ��ʱ�䲻һ��

        // Each retry is 2 minutes after the last��û�е�2���ӣ����Ӧ��nRequesttime��Ӧ��ֵ��һ��
        nRequestTime = max(nRequestTime + 2 * 60 * 1000000, nNow);
        mapAskFor.insert(make_pair(nRequestTime, inv));
    }



    void BeginMessage(const char* pszCommand)
    {
        //EnterCriticalSection(&cs_vSend);
        if (nPushPos != -1)
            AbortMessage();
        nPushPos = vSend.size();
        vSend << CMessageHeader(pszCommand, 0);
        printf("sending: %-12s ", pszCommand);
    }

    void AbortMessage()
    {
        if (nPushPos == -1)
            return;
        vSend.resize(nPushPos);
        nPushPos = -1;
       // LeaveCriticalSection(&cs_vSend);
        printf("(aborted)\n");
    }
	// �޸���Ϣͷ�ж�Ӧ����Ϣ��С�ֶ�
    void EndMessage()
    {
        int nDropMessagesTest = BlockEngine::getInstance()->nDropMessagesTest;
        if (nDropMessagesTest > 0 && GetRand(nDropMessagesTest) == 0)
        {
            printf("dropmessages DROPPING SEND MESSAGE\n");
            AbortMessage();
            return;
        }

        if (nPushPos == -1)
            return;

		// �޸���Ϣͷ�ж�Ӧ����Ϣ��С
        // Patch in the size
        unsigned int nSize = vSend.size() - nPushPos - sizeof(CMessageHeader);
        memcpy((char*)&vSend[nPushPos] + offsetof(CMessageHeader, m_nMessageSize), &nSize, sizeof(nSize));

        printf("(%d bytes)  ", nSize);
        //for (int i = nPushPos+sizeof(CMessageHeader); i < min(vSend.size(), nPushPos+sizeof(CMessageHeader)+20U); i++)
        //    printf("%02x ", vSend[i] & 0xff);
        printf("\n");

        nPushPos = -1;
    }

    void EndMessageAbortIfEmpty()
    {
        if (nPushPos == -1)
            return;
        int nSize = vSend.size() - nPushPos - sizeof(CMessageHeader);
        if (nSize > 0)
            EndMessage();
        else
            AbortMessage();
    }

    const char* GetMessageCommand() const
    {
        if (nPushPos == -1)
            return "";
        return &vSend[nPushPos] + offsetof(CMessageHeader, m_pchCommand);
    }




    void PushMessage(const char* pszCommand)
    {
        try
        {
            BeginMessage(pszCommand);
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

	// ����Ϣ���Ͷ�Ӧ�ڵ��vsend������
    template<typename T1>
    void PushMessage(const char* pszCommand, const T1& a1)
    {
        try
        {
            BeginMessage(pszCommand);
            vSend << a1;
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
        try
        {
            BeginMessage(pszCommand);
            vSend << a1 << a2;
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
        try
        {
            BeginMessage(pszCommand);
            vSend << a1 << a2 << a3;
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
        try
        {
            BeginMessage(pszCommand);
            vSend << a1 << a2 << a3 << a4;
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



    bool IsSubscribed(unsigned int nChannel);
    void Subscribe(unsigned int nChannel, unsigned int nHops=0);
    void CancelSubscribe(unsigned int nChannel);
    void Disconnect();
};


#endif /* CXX_BT_CNODE_H */

/* EOF */

