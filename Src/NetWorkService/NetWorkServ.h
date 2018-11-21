/*
 * =====================================================================================
 *
 *       Filename:  NetWorkServ.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/06/2018 10:35:31 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef EZ_BT_NWS_H
#define EZ_BT_NWS_H
#include <mutex>
#include "CAddress.h"
#include "PeerNode.h"
#include "zhelpers.h"
#include "CommonBase/ThreadGuard.h"
namespace Enze
{
    class ServMsg;
    class CInv;
    class NetWorkServ
    {
        public:
            static NetWorkServ* getInstance();
            static void Destory();
        public:
            void initiation();
            bool ConnectSocket(const CAddress& addrConnect, SOCKET& hSocketRet);
            PeerNode* FindNode(unsigned int ip);
            PeerNode* FindNode(const CAddress& addr);
            PeerNode* ConnectNode(const CAddress& addrConnect, int64 nTimeout=0);
            bool StartNode();
            bool StopNode();
    
            bool ProcessMessages(PeerNode* pfrom);
            bool SendMessages(PeerNode* pto);
            
            void CheckForShutdown(int n);
            void AbandonRequests(/*void (*fn)(void*, CDataStream&), void* param1*/);
            bool AnySubscribed(unsigned int nChannel);
            map<string, CAddress>& getMapAddr()
            {
                return m_cMapAddresses; 
            }
            vector<PeerNode*>& getNodeList()
            {
                return m_cPeerNodeLst;
            }
            
            const CAddress& getLocakAddr()
            {
                return m_cAddrLocalHost;
            }
            
            map<CInv, int64>& getMapAlreadyAskedFor()
            {
                return mapAlreadyAskedFor;
            }
            
            inline void removeAlreadyAskedFor(const CInv& inv)
            {
                mapAlreadyAskedFor.erase(inv);
            }

            void MessageHandler();
            void SocketHandler();
            void PeerNodeManagerThread();
            void AddrManagerThread();
            
        private:
            NetWorkServ();
            ~NetWorkServ();
            bool LoadAddresses();
            bool AddUserProviedAddress();
            void AddNewAddrByEndPoint(const char* endPoint);
            bool GetMyExternalIP(unsigned int& ipRet);
            bool GetMyExternalIP2(const CAddress& addrConnect, 
                                    const char* pszGet, const char* pszKeyword, unsigned int& ipRet);
        
            void AddAddress(const ServMsg& cMsg);
        
            map<unsigned int, vector<CAddress> > selectIp(unsigned int ipC);
            vector<unsigned int> getIPCList();
            void AddrConvertPeerNode();
            void DealPeerNodeMsg(void* zmqSock);
            void UpdatePeerNodeStatu();
        private:
            
            int nDropMessagesTest = 0; // ��Ϣ�ɼ���Ƶ�ʣ����Ƕ������Ϣ�ɼ�һ�ν��д���
            int m_iSocketFd = 0;
            bool m_bClient = false;
            uint64 m_nLocalServices = (m_bClient ? 0 : NODE_NETWORK);
            CAddress m_cAddrLocalHost;// = new CAddress(0, DEFAULT_PORT, m_nLocalServices);// ����������ַ
            PeerNode* m_pcNodeLocalHost;// = new PeerNode(INVALID_SOCKET, CAddress("127.0.0.1", m_nLocalServices)); // ���ؽڵ�
            
            vector<PeerNode*> m_cPeerNodeLst; //value is node
            map<string, CAddress> m_cMapAddresses;
            CAddress addrProxy;

            list<ThreadGuard*> m_ThreadList;
            std::mutex m_cAddrMutex;
            deque<pair<int64, CInv> > vRelayExpiration;
            map<CInv, int64> mapAlreadyAskedFor;
            void* m_cZmqCtx; 
            string m_strIdentity;
            static NetWorkServ* m_pInstance;
            static pthread_mutex_t m_NWSLock;
        public:
            // ת�����
            inline void RelayInventory(const CInv& inv)
            {
                printf("%s---%d\n", __FILE__, __LINE__);
                // ���˽ڵ����������нڵ����ת������Ϣ
                // Put on lists to offer to the other nodes
                foreach(auto it, m_cPeerNodeLst) {
                    PeerNode * pnode =  it;
                    pnode->PushInventory(inv);
                }
            }


            template<typename T>
            void RelayMessage(const CInv& inv, const T& a)
            {
                printf("error %s--%d\n", __FILE__, __LINE__);
#if 0
                CDataStream ss(SER_NETWORK);
                ss.reserve(10000);
                ss << a;
                RelayMessage(inv, ss);
#endif
            }
#if 0
            inline void RelayMessage(const CInv& inv, const CDataStream& ss)
            {
                //CRITICAL_BLOCK(cs_mapRelay)
                {
                    // Expire old relay messages
                    while (!vRelayExpiration.empty() && vRelayExpiration.front().first < GetTime())
                    {
                        mapRelay.erase(vRelayExpiration.front().second);
                        vRelayExpiration.pop_front();
                    }

                    // Save original serialized message so newer versions are preserved
                    mapRelay[inv] = ss;
                    vRelayExpiration.push_back(make_pair(GetTime() + 15 * 60, inv));
                }
                // �ڵ���п��ת��
                RelayInventory(inv);
            }
#endif
            


            //
            // Templates for the publish and subscription system.
            // The object being published as T& obj needs to have:
            //   a set<unsigned int> setSources member
            //   specializations of AdvertInsert and AdvertErase
            // Currently implemented for CTable and CProduct.
            //

            template<typename T>
            void AdvertStartPublish(PeerNode* pfrom, unsigned int nChannel, unsigned int nHops, T& obj)
            {
                // Add to sources
                obj.setSources.insert(pfrom->getAddr().ip);

                if (!AdvertInsert(obj))
                    return;
                printf("%s---%d\n", __FILE__, __LINE__);
                // Relay
                //foreach(PeerNode* pnode, m_cPeerNodeLst)
                    //if (pnode != pfrom && (nHops < PUBLISH_HOPS || pnode->IsSubscribed(nChannel)))
                        //pnode->PushMessage("publish", nChannel, nHops, obj);
            }

            template<typename T>
            void AdvertStopPublish(PeerNode* pfrom, unsigned int nChannel, unsigned int nHops, T& obj)
            {
                uint256 hash = obj.GetHash();
                printf("%s---%d\n", __FILE__, __LINE__);
                //foreach(PeerNode* pnode, m_cPeerNodeLst)
                //    if (pnode != pfrom && (nHops < PUBLISH_HOPS || pnode->IsSubscribed(nChannel)))
                //        pnode->PushMessage("pub-cancel", nChannel, nHops, hash);

                AdvertErase(obj);
            }

            template<typename T>
            void AdvertRemoveSource(PeerNode* pfrom, unsigned int nChannel, unsigned int nHops, T& obj)
            {
                // Remove a source
                obj.setSources.erase(pfrom->getAddr().ip);

                // If no longer supported by any sources, cancel it
                if (obj.setSources.empty())
                    AdvertStopPublish(pfrom, nChannel, nHops, obj);
            }

        
    };

}


#endif /* EZ_BT_NWS_H */
/* EOF */

