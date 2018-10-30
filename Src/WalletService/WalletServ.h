/*
 * =====================================================================================
 *
 *       Filename:  WalletServ.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/14/2018 03:45:06 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef EZ_BT_WALLET_SERV_H
#define EZ_BT_WALLET_SERV_H
#include <pthread.h>
#include "key.h"
#include "CommonBase/CommDataDef.h"
#include "NetWorkService/CAddress.h"
#include "WalletService/CWalletTx.h"

namespace Enze
{
    class CDataStream;
    class COutPoint;
    class CInPoint;
    class CDiskTxPos;
    class CCoinBase;
    class CBlock;
    class CBlockIndex;
    class CKeyItem;
    class CNode;
    class CScript;
    class CAddress;
    class CTransaction;
    class uint256;
    class uint160;
    class CInv;
    class Transaction;
    class WalletServ
    {
        public:
            static WalletServ* getInstance();
            static void Destory();
            
            void initiation();
            bool AddKey(const CKey& key);
            vector<unsigned char> GenerateNewKey();
            bool AddToWallet(const CWalletTx& wtxIn);
            bool AddToWalletIfMine(const CTransaction& tx, const CBlock* pblock);
            bool EraseFromWallet(uint256 hash);
            void AddOrphanTx(const Transaction& pbTx);
            void EraseOrphanTx(uint256 hash);
            void ReacceptWalletTransactions();
            void RelayWalletTransactions();
            bool AlreadyHave(const CInv& inv);
            int64 GetBalance();
            bool SelectCoins(int64 nTargetValue, set<CWalletTx*>& setCoinsRet);
            bool CreateTransaction(CScript m_cScriptPubKey, int64 nValue, CWalletTx& txNew, int64& nFeeRequiredRet);
            bool CommitTransactionSpent(const CWalletTx& wtxNew);
            bool SendMoney(CScript m_cScriptPubKey, int64 nValue, CWalletTx& wtxNew);    
            
        public:
            map<uint256, CTransaction> mapTransactions;// ������׶�Ӧ�������Ѿ����������У��򽫴��ڴ���ɾ����Щ���������еĽ��ף�Ҳ����˵�������������û�б�����������н���
            unsigned int nTransactionsUpdated;
            map<COutPoint, CInPoint> mapNextTx;// �����Ӧ�������Ѿ����뵽�����У����Ӧ�����齻��Ӧ��Ҫ�ӱ��ڵ㱣��Ľ����ڴ����ɾ��
            map<uint256, CWalletTx> mapWallet; // Ǯ�����׶�Ӧ��map������key��Ӧ��Ǯ�����׵�hashֵ��mapWallet������źͱ��ڵ���صĽ���
            vector<pair<uint256, bool> > vWalletUpdated;
            map<vector<unsigned char>, CPrivKey> mapKeys; // ��Կ��˽Կ��Ӧ��ӳ���ϵ������keyΪ��Կ��valueΪ˽Կ
            map<uint160, vector<unsigned char> > mapPubKeys; // ��Կ��hashֵ�͹�Կ�Ĺ�ϵ������keyΪ��Կ��hashֵ��valueΪ��Կ
            CKey keyUser; // ��ǰ�û���˽Կ����Ϣ
            int64 nTransactionFee;
            CAddress addrIncoming;
            map<string, string> mapAddressBook; // ��ַ�����Ƶ�ӳ�䣬����keyΪ��ַ��valueΪ����
            map<uint256, CTransaction*> mapOrphanTransactions;// �¶����ף�����key��Ӧ�Ľ���hashֵ
            multimap<uint256, CTransaction*> mapOrphanTransactionsByPrev; // ����keyΪvalue���׶�Ӧ����Ľ��׵�hashֵ��valueΪ��ǰ����
            
        private:
            WalletServ();
            ~WalletServ();
            bool LoadWallet();
        private:
            static WalletServ* m_pInstance;
            static pthread_mutex_t m_mutexLock;
    
    };

}// end namespace

#endif /* EZ_BT_WALLET_SERV_H */
/* EOF */

