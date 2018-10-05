/*
 * =====================================================================================
 *
 *       Filename:  BlockEngine.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/05/2018 11:28:27 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef CXX_BT_BLOCKENGINE_H
#define CXX_BT_BLOCKENGINE_H

#include <pthread.h>
#include "key.h"
#include "CommonBase/CommDataDef.h"
#include "Network/CAddress.h"
class COutPoint;
class CInPoint;
class CDiskTxPos;
class CCoinBase;
class CBlock;
class CBlockIndex;
class CWalletTx;
class CKeyItem;
class CNode;
class CScript;
class CAddress;
class CDataStream;
class CTransaction;
class uint256;
class uint160;
class CTxDB;
class CInv;
class BlockEngine
{
public:
    static BlockEngine* getInstance();
    static void Destory();
public:
    void initiation();
    bool AddKey(const CKey& key);
    vector<unsigned char> GenerateNewKey();
    bool AddToWallet(const CWalletTx& wtxIn);
    bool AddToWalletIfMine(const CTransaction& tx, const CBlock* pblock);
    bool EraseFromWallet(uint256 hash);
    void AddOrphanTx(const CDataStream& vMsg);
    void EraseOrphanTx(uint256 hash);
    void ReacceptWalletTransactions();
    void RelayWalletTransactions();
    uint256 GetOrphanRoot(const CBlock* pblock);
    unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast);
    bool Reorganize(CTxDB& txdb, CBlockIndex* pindexNew);
    bool ProcessBlock(CNode* pfrom, CBlock* pblock);
    template<typename Stream>
    bool ScanMessageStart(Stream& s);
    string GetAppDir();
    bool CheckDiskSpace(int64 nAdditionalBytes=0);
    FILE* OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, const char* pszMode="rb");
    FILE* AppendBlockFile(unsigned int& m_nFileRet);
    void PrintBlockTree();
    bool AlreadyHave(CTxDB& txdb, const CInv& inv);
    bool ProcessMessages(CNode* pfrom);
    bool ProcessMessage(CNode* pfrom, string strCommand, CDataStream& vRecv);
    bool SendMessages(CNode* pto);
    int FormatHashBlocks(void* pbuffer, unsigned int len);
    void BlockSHA256(const void* pin, unsigned int nBlocks, void* pout);
    bool BitcoinMiner();
    int64 GetBalance();
    bool SelectCoins(int64 nTargetValue, set<CWalletTx*>& setCoinsRet);
    bool CreateTransaction(CScript m_cScriptPubKey, int64 nValue, CWalletTx& txNew, int64& nFeeRequiredRet);
    bool CommitTransactionSpent(const CWalletTx& wtxNew);
    bool SendMoney(CScript m_cScriptPubKey, int64 nValue, CWalletTx& wtxNew);

private:
    BlockEngine();
    ~BlockEngine();
    BlockEngine(const BlockEngine&);
    BlockEngine& operator =(const BlockEngine&);
    
    bool LoadBlockIndex(bool fAllowNew=true);
    bool LoadWallet();
    bool LoadAddresses();
public:
    map<uint256, CTransaction> mapTransactions;// ������׶�Ӧ�������Ѿ����������У��򽫴��ڴ���ɾ����Щ���������еĽ��ף�Ҳ����˵�������������û�б�����������н���
    unsigned int nTransactionsUpdated;
    map<COutPoint, CInPoint> mapNextTx;// �����Ӧ�������Ѿ����뵽�����У����Ӧ�����齻��Ӧ��Ҫ�ӱ��ڵ㱣��Ľ����ڴ����ɾ��
    map<uint256, CBlockIndex*> mapBlockIndex;
    const uint256 hashGenesisBlock;
    CBlockIndex* pindexGenesisBlock;
    int nBestHeight;
    uint256 hashBestChain;
    CBlockIndex* pindexBest;
    map<uint256, CBlock*> mapOrphanBlocks; // �¶���map
    multimap<uint256, CBlock*> mapOrphanBlocksByPrev;

    map<uint256, CDataStream*> mapOrphanTransactions;// �¶����ף�����key��Ӧ�Ľ���hashֵ
    multimap<uint256, CDataStream*> mapOrphanTransactionsByPrev; // ����keyΪvalue���׶�Ӧ����Ľ��׵�hashֵ��valueΪ��ǰ����


    map<uint256, CWalletTx> mapWallet; // Ǯ�����׶�Ӧ��map������key��Ӧ��Ǯ�����׵�hashֵ��mapWallet������źͱ��ڵ���صĽ���
    vector<pair<uint256, bool> > vWalletUpdated;
    map<vector<unsigned char>, CPrivKey> mapKeys; // ��Կ��˽Կ��Ӧ��ӳ���ϵ������keyΪ��Կ��valueΪ˽Կ
    map<uint160, vector<unsigned char> > mapPubKeys; // ��Կ��hashֵ�͹�Կ�Ĺ�ϵ������keyΪ��Կ��hashֵ��valueΪ��Կ
    CKey keyUser; // ��ǰ�û���˽Կ����Ϣ
    string strSetDataDir;
    int nDropMessagesTest;
    int fGenerateBitcoins;
    int64 nTransactionFee;
    CAddress addrIncoming;
    map<string, string> mapAddressBook; // ��ַ�����Ƶ�ӳ�䣬����keyΪ��ַ��valueΪ����

private:
    static BlockEngine* m_pInstance;
    static pthread_mutex_t m_mutexLock;
};

#endif/* CXX_BT_BLOCKENGINE_H */
/* EOF */

