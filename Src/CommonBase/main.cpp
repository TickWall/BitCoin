// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "headers.h"
#include "sha.h"
#include "serialize.h"
#include "market.h"
#include "util.h"
#include "CommonBase/CommDataDef.h"
#include "CommonBase/bignum.h"
#include "CommonBase/uint256.h"
#include "Network/net.h"
#include "TX/CTransaction.h"
#include "TX/CMerkleTx.h"
#include "TX/CWalletTx.h"
#include "TX/CTxInPoint.h"
#include "TX/CTxIndex.h"
#include "Block/CBlock.h"
#include "Block/CBlockIndex.h"
#include "Block/CBlockLocator.h"
#include "Db/CTxDB.h"
#include "Db/CWalletDB.h"
#include "Db/CAddrDB.h"



//
// Global state
//

//CCriticalSection cs_main;

map<uint256, CTransaction> mapTransactions;// ������׶�Ӧ�������Ѿ����������У��򽫴��ڴ���ɾ����Щ���������еĽ��ף�Ҳ����˵�������������û�б�����������н���
//CCriticalSection cs_mapTransactions;
unsigned int nTransactionsUpdated = 0; // ÿ�ζ�mapTransactions�н��׽��и��£����Ը��ֶν���++����
map<COutPoint, CInPoint> mapNextTx;// �����Ӧ�������Ѿ����뵽�����У����Ӧ�����齻��Ӧ��Ҫ�ӱ��ڵ㱣��Ľ����ڴ����ɾ��

map<uint256, CBlockIndex*> mapBlockIndex; // ��������Ϣ������key��Ӧ��block��hashֵ
const uint256 hashGenesisBlock("0xdc8be64865ce298ca87ebee785b15fa29240e60db149c2ba85666e702f3eb734");
CBlockIndex* pindexGenesisBlock = NULL; // �������Ӧ��������Ҳ���Ǵ��������Ӧ������
int nBestHeight = -1; // �����Ӧ������������Ӵ������鵽��ǰ�������һ�����飬�м���˶��ٸ�����
uint256 hashBestChain = 0; // ������һ�������Ӧ��hash
CBlockIndex* pindexBest = NULL; // ��¼��ǰ���������Ӧ����������ָ��

map<uint256, CBlock*> mapOrphanBlocks; // �¶���map
multimap<uint256, CBlock*> mapOrphanBlocksByPrev;

map<uint256, CDataStream*> mapOrphanTransactions;// �¶����ף�����key��Ӧ�Ľ���hashֵ
multimap<uint256, CDataStream*> mapOrphanTransactionsByPrev; // ����keyΪvalue���׶�Ӧ����Ľ��׵�hashֵ��valueΪ��ǰ����

map<uint256, CWalletTx> mapWallet; // Ǯ�����׶�Ӧ��map������key��Ӧ��Ǯ�����׵�hashֵ��mapWallet������źͱ��ڵ���صĽ���
vector<pair<uint256, bool> > vWalletUpdated; // ֪ͨUI����Ӧ��hash�����˸ı�

map<vector<unsigned char>, CPrivKey> mapKeys; // ��Կ��˽Կ��Ӧ��ӳ���ϵ������keyΪ��Կ��valueΪ˽Կ
map<uint160, vector<unsigned char> > mapPubKeys; // ��Կ��hashֵ�͹�Կ�Ĺ�ϵ������keyΪ��Կ��hashֵ��valueΪ��Կ
//CCriticalSection cs_mapKeys;
CKey keyUser; // ��ǰ�û���˽Կ����Ϣ

string strSetDataDir;
int nDropMessagesTest = 0; // ��Ϣ�ɼ���Ƶ�ʣ����Ƕ������Ϣ�ɼ�һ�ν��д���

// Settings
int fGenerateBitcoins = 0; // �Ƿ��ڿ󣬲������ر�
int64 nTransactionFee = 0; // ���׷���
CAddress addrIncoming;








//////////////////////////////////////////////////////////////////////////////
//
// mapKeys
//
// ����Ӧkey����Ϣ��ŵ���Ӧ��ȫ�ֱ�����
bool AddKey(const CKey& key)
{
 //   CRITICAL_BLOCK(cs_mapKeys)
    {
        mapKeys[key.GetPubKey()] = key.GetPrivKey();
        mapPubKeys[Hash160(key.GetPubKey())] = key.GetPubKey();
    }
    return CWalletDB().WriteKey(key.GetPubKey(), key.GetPrivKey());
}
// �����µĹ�˽Կ��
vector<unsigned char> GenerateNewKey()
{
    CKey key;
    key.MakeNewKey();
    if (!AddKey(key))
        throw runtime_error("GenerateNewKey() : AddKey failed\n");
    return key.GetPubKey();
}




//////////////////////////////////////////////////////////////////////////////
//
// mapWallet
//
// ����ǰ�������ӵ�Ǯ��mapWallet�У�������룬������£�mapWallet������źͱ��ڵ���صĽ���
bool AddToWallet(const CWalletTx& wtxIn)
{
    uint256 hash = wtxIn.GetHash();
 //   CRITICAL_BLOCK(cs_mapWallet)
    {
        // Inserts only if not already there, returns tx inserted or tx found
        pair<map<uint256, CWalletTx>::iterator, bool> ret = mapWallet.insert(make_pair(hash, wtxIn));
        CWalletTx& wtx = (*ret.first).second;
        bool fInsertedNew = ret.second; // �ж��Ƿ����²���ģ�Ҳ����ԭ����ӦmapWallet��û�У�
        if (fInsertedNew)
            wtx.m_m_uTimeReceived = GetAdjustedTime(); // ���ױ��ڵ���յ�ʱ��

        //// debug print
        printf("AddToWallet %s  %s\n", wtxIn.GetHash().ToString().substr(0,6).c_str(), fInsertedNew ? "new" : "update");

        if (!fInsertedNew)
        {
			// ��ǰ�����Ѿ���mapWallet�д���
            // Merge
            bool fUpdated = false;
            if (wtxIn.m_hashBlock != 0 && wtxIn.m_hashBlock != wtx.m_hashBlock)
            {
                wtx.m_hashBlock = wtxIn.m_hashBlock;
                fUpdated = true;
            }
            if (wtxIn.m_nIndex != -1 && (wtxIn.m_vMerkleBranch != wtx.m_vMerkleBranch || wtxIn.m_nIndex != wtx.m_nIndex))
            {
                wtx.m_vMerkleBranch = wtxIn.m_vMerkleBranch;
                wtx.m_nIndex = wtxIn.m_nIndex;
                fUpdated = true;
            }
            if (wtxIn.m_bFromMe && wtxIn.m_bFromMe != wtx.m_bFromMe)
            {
                wtx.m_bFromMe = wtxIn.m_bFromMe;
                fUpdated = true;
            }
            if (wtxIn.m_bSpent && wtxIn.m_bSpent != wtx.m_bSpent)
            {
                wtx.m_bSpent = wtxIn.m_bSpent;
                fUpdated = true;
            }
            if (!fUpdated)
                return true;
        }

        // Write to disk
        if (!wtx.WriteToDisk())
            return false;

        // Notify UI
        vWalletUpdated.push_back(make_pair(hash, fInsertedNew));
    }

    // Refresh UI
    //MainFrameRepaint();
    return true;
}

// �����ǰ�������ڱ��ڵ㣬�򽫵�ǰ���׼��뵽Ǯ����
bool AddToWalletIfMine(const CTransaction& tx, const CBlock* pblock)
{
    if (tx.IsMine() || mapWallet.count(tx.GetHash()))
    {
        CWalletTx wtx(tx);
        // Get merkle branch if transaction was found in a block
        if (pblock)
            wtx.SetMerkleBranch(pblock);
        return AddToWallet(wtx);
    }
    return true;
}

// �����״�Ǯ��ӳ�����mapWallet���Ƴ���ͬʱ�����״�CWalletDB���Ƴ�
bool EraseFromWallet(uint256 hash)
{
 //   CRITICAL_BLOCK(cs_mapWallet)
    {
        if (mapWallet.erase(hash))
            CWalletDB().EraseTx(hash);
    }
    return true;
}









//////////////////////////////////////////////////////////////////////////////
//
// mapOrphanTransactions
//
// ���ӹ¶�����
void AddOrphanTx(const CDataStream& vMsg)
{
    CTransaction tx;
    CDataStream(vMsg) >> tx;
    uint256 hash = tx.GetHash();
    if (mapOrphanTransactions.count(hash))
        return;
    CDataStream* pvMsg = mapOrphanTransactions[hash] = new CDataStream(vMsg);
    // ��ǰ���׶�Ӧ�������Ӧ�Ľ���hash
    foreach(const CTxIn& txin, tx.m_vTxIn)
        mapOrphanTransactionsByPrev.insert(make_pair(txin.m_cPrevOut.m_u256Hash, pvMsg));
}
// ɾ����Ӧ�Ĺ¶�����
void EraseOrphanTx(uint256 hash)
{
    if (!mapOrphanTransactions.count(hash))
        return;
    const CDataStream* pvMsg = mapOrphanTransactions[hash];
    CTransaction tx;
    CDataStream(*pvMsg) >> tx;
    foreach(const CTxIn& txin, tx.m_vTxIn)
    {
        for (multimap<uint256, CDataStream*>::iterator mi = mapOrphanTransactionsByPrev.lower_bound(txin.m_cPrevOut.m_u256Hash);
             mi != mapOrphanTransactionsByPrev.upper_bound(txin.m_cPrevOut.m_u256Hash);)
        {
            if ((*mi).second == pvMsg)
                mapOrphanTransactionsByPrev.erase(mi++);
            else
                mi++;
        }
    }
    delete pvMsg;
    mapOrphanTransactions.erase(hash);
}



void ReacceptWalletTransactions()
{
    // Reaccept any txes of ours that aren't already in a block
    CTxDB txdb("r");
 //   CRITICAL_BLOCK(cs_mapWallet)
    {
        foreach(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;
            if (!wtx.IsCoinBase() && !txdb.ContainsTx(wtx.GetHash()))
                wtx.AcceptWalletTransaction(txdb, false);
        }
    }
}


// �������Ľڵ�֮��ת����Щ��ĿǰΪֹ��û�н���block�е�Ǯ������
void RelayWalletTransactions()
{
    static int64 nLastTime;
	// ת��Ǯ������ʱ��ļ����10���ӣ�С��10�����򲻽���ת��
    if (GetTime() - nLastTime < 10 * 60)
        return;
    nLastTime = GetTime();

    // Rebroadcast any of our txes that aren't in a block yet
    printf("RelayWalletTransactions()\n");
    CTxDB txdb("r");
//    CRITICAL_BLOCK(cs_mapWallet)
    {
		// ����ʱ�䣨����ǰ�ڵ���յ�ʱ�䣩˳���Ǯ���еĽ��׽�������
        // Sort them in chronological order
        multimap<unsigned int, CWalletTx*> mapSorted;// Ĭ���ǰ���unsigned int��Ӧ��ֵ�������У�����Խ��ʱ��Խ��ǰ
        foreach(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;
            mapSorted.insert(make_pair(wtx.m_m_uTimeReceived, &wtx));
        }
        foreach(PAIRTYPE(const unsigned int, CWalletTx*)& item, mapSorted)
        {
            CWalletTx& wtx = *item.second;
			// Ǯ�����׽���ת��
            wtx.RelayWalletTransaction(txdb);
        }
    }
}




// ��ȡ�¶����Ӧ�ĸ�
uint256 GetOrphanRoot(const CBlock* pblock)
{
    // Work back to the first block in the orphan chain
    while (mapOrphanBlocks.count(pblock->m_hashPrevBlock))
        pblock = mapOrphanBlocks[pblock->m_hashPrevBlock];
    return pblock->GetHash();
}


// ����ǰһ��block��Ӧ�Ĺ�������ȡ��һ��block��ȡ��Ҫ�Ĺ�����
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast)
{
    const unsigned int nTargetTimespan = 14 * 24 * 60 * 60; // two weeks
    const unsigned int nTargetSpacing = 10 * 60; // 10���Ӳ���һ��block
	// ÿ��2016�����Ӧ�Ĺ������ѶȾ���Ҫ���¼���һ��
    const unsigned int nInterval = nTargetTimespan / nTargetSpacing; // �м���˶��ٸ�block 2016����
 //   printf("GetNextWorkRequired---bnProofOfWorkLimit[%0x]\n", bnProofOfWorkLimit.GetCompact());
	// ˵����ǰ����һ���������飬��Ϊ��ǰ���Ӧ��ǰһ������Ϊ��
    // Genesis block
    if (pindexLast == NULL)
        return bnProofOfWorkLimit.GetCompact();

	// ���������0�����й������Ѷȸı�
    // Only change once per interval
    if ((pindexLast->m_nCurHeight+1) % nInterval != 0)
        return pindexLast->m_uBits;

	// ��ǰ��2016������
    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < nInterval-1; i++)
        pindexFirst = pindexFirst->m_pPrevBlkIndex;
    assert(pindexFirst);

	// ��ǰ�����ǰһ�����鴴��ʱ�� ��ȥ �ӵ�ǰ������ǰ��2016������õ����鴴��ʱ��
    // Limit adjustment step
    unsigned int nActualTimespan = pindexLast->m_uTime - pindexFirst->m_uTime;
    printf("  nActualTimespan = %d  before bounds\n", nActualTimespan);
    // ����Ŀ���Ѷȵ����Ŀ�Ȳ���̫��
	if (nActualTimespan < nTargetTimespan/4)
        nActualTimespan = nTargetTimespan/4;
    if (nActualTimespan > nTargetTimespan*4)
        nActualTimespan = nTargetTimespan*4;

	// ����Ŀ������Ѷȣ���ǰ�����Ӧ��ǰһ�������Ӧ��Ŀ���Ѷ� * ʵ��2016�����Ӧ�Ĵ���ʱ���� / Ŀ��ʱ����14��
    // Retarget
    CBigNum bnNew;
    bnNew.SetCompact(pindexLast->m_uBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

	// �������Ĺ������Ѷȣ�ֵԽ���Ӧ�Ĺ����Ѷ�ԽС��С�ڵ�ǰ��Ӧ�Ĺ������Ѷ�
    if (bnNew > bnProofOfWorkLimit)
        bnNew = bnProofOfWorkLimit;

    /// debug print
    printf("\n\n\nGetNextWorkRequired RETARGET *****\n");
    printf("nTargetTimespan = %d    nActualTimespan = %d\n", nTargetTimespan, nActualTimespan);
    printf("Before: %08x  %s\n", pindexLast->m_uBits, CBigNum().SetCompact(pindexLast->m_uBits).getuint256().ToString().c_str());
    printf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString().c_str());

    return bnNew.GetCompact();
}





// ������֯�������������Ϊ��ʱ�Ѿ������������ֲ�
bool Reorganize(CTxDB& txdb, CBlockIndex* pindexNew)
{
    printf("*** REORGANIZE ***\n");

	// �ҵ�����ֲ��
    // Find the fork
    CBlockIndex* pfork = pindexBest;
    CBlockIndex* plonger = pindexNew;
	// �ҵ������ͷֲ�����Ӧ�Ľ����
    while (pfork != plonger)
    {
        if (!(pfork = pfork->m_pPrevBlkIndex))
            return error("Reorganize() : pfork->m_pPrevBlkIndex is null");
        while (plonger->m_nCurHeight > pfork->m_nCurHeight)
            if (!(plonger = plonger->m_pPrevBlkIndex))
                return error("Reorganize() : plonger->m_pPrevBlkIndex is null");
    }

	// �оٳ���ǰ�ڵ���Ϊ������У��ӵ�ǰ���������㣩ʧȥ���ӵĿ�
    // List of what to disconnect
    vector<CBlockIndex*> vDisconnect;
    for (CBlockIndex* pindex = pindexBest; pindex != pfork; pindex = pindex->m_pPrevBlkIndex)
        vDisconnect.push_back(pindex);

	// ��ȡ��Ҫ���ӵĿ飬��Ϊ�Լ���Ϊ�����ʵ���ϲ������
    // List of what to connect
    vector<CBlockIndex*> vConnect;
    for (CBlockIndex* pindex = pindexNew; pindex != pfork; pindex = pindex->m_pPrevBlkIndex)
        vConnect.push_back(pindex);
	// ��Ϊ��������ʱ���ǵ��ŷŵģ����������ڽ�������򣬵õ������
    reverse(vConnect.begin(), vConnect.end());

	// �ͷŶ����������ͷŶ�Ӧ��block������Ӧ��block��������û���ͷţ�
    // Disconnect shorter branch
    vector<CTransaction> vResurrect;
    foreach(CBlockIndex* pindex, vDisconnect)
    {
        CBlock block;
        if (!block.ReadFromDisk(pindex->m_nFile, pindex->m_nBlockPos, true))
            return error("Reorganize() : ReadFromDisk for disconnect failed");
        if (!block.DisconnectBlock(txdb, pindex))
            return error("Reorganize() : DisconnectBlock failed");

		// ���ͷſ��еĽ��׷���vResurrect���ȴ�����
        // Queue memory transactions to resurrect
        foreach(const CTransaction& tx, block.m_vTrans)
            if (!tx.IsCoinBase())
                vResurrect.push_back(tx);
    }

	// ������ķ�֧
    // Connect longer branch
    vector<CTransaction> vDelete;
    for (int i = 0; i < vConnect.size(); i++)
    {
        CBlockIndex* pindex = vConnect[i];
        CBlock block;
        if (!block.ReadFromDisk(pindex->m_nFile, pindex->m_nBlockPos, true))
            return error("Reorganize() : ReadFromDisk for connect failed");
        if (!block.ConnectBlock(txdb, pindex))
        {
			// ���block����ʧ��֮��˵�����block��Ч����ɾ�����֮��ķ�֧
            // Invalid block, delete the rest of this branch
            txdb.TxnAbort();
            for (int j = i; j < vConnect.size(); j++)
            {
                CBlockIndex* pindex = vConnect[j];
                pindex->EraseBlockFromDisk();
                txdb.EraseBlockIndex(pindex->GetBlockHash());
                mapBlockIndex.erase(pindex->GetBlockHash());
                delete pindex;
            }
            return error("Reorganize() : ConnectBlock failed");
        }
		// �������������Ŀ��еĽ��״Ӷ�Ӧ���ڴ���ɾ��
        // Queue memory transactions to delete
        foreach(const CTransaction& tx, block.m_vTrans)
            vDelete.push_back(tx);
    }
	// д�����
    if (!txdb.WriteHashBestChain(pindexNew->GetBlockHash()))
        return error("Reorganize() : WriteHashBestChain failed");

    // Commit now because resurrecting ����could take some time
    txdb.TxnCommit();

	// �ͷŶ�Ӧ�Ŀ�������
    // Disconnect shorter branch
    foreach(CBlockIndex* pindex, vDisconnect)
        if (pindex->m_pPrevBlkIndex)
            pindex->m_pPrevBlkIndex->m_pNextBlkIndex = NULL; // ��ʾ��Щ��û����������

	// �γ�һ�������Ŀ�������
    // Connect longer branch
    foreach(CBlockIndex* pindex, vConnect)
        if (pindex->m_pPrevBlkIndex)
            pindex->m_pPrevBlkIndex->m_pNextBlkIndex = pindex;

	// ���ͷ����ӵķ�֧�л�ȡ��Ӧ�Ľ��ף�����Щ���׷����Ӧ��ȫ�ֱ����еõ�����
    // Resurrect memory transactions that were in the disconnected branch
    foreach(CTransaction& tx, vResurrect)
        tx.AcceptTransaction(txdb, false);

	// ��ȫ�ֱ�����ɾ����Щ�Ѿ��������еĽ���
    // Delete redundant memory transactions that are in the connected branch
    foreach(CTransaction& tx, vDelete)
        tx.RemoveFromMemoryPool();

    return true;
}

// �������飬�����ǽ��յ��Ļ����Լ��ڿ�õ���
bool ProcessBlock(CNode* pfrom, CBlock* pblock)
{
    // Check for duplicate
    uint256 hash = pblock->GetHash();
    if (mapBlockIndex.count(hash))
        return error("ProcessBlock() : already have block %d %s", mapBlockIndex[hash]->m_nCurHeight, hash.ToString().substr(0,14).c_str());
    if (mapOrphanBlocks.count(hash))
        return error("ProcessBlock() : already have block (orphan) %s", hash.ToString().substr(0,14).c_str());

    // Preliminary checks ��������
    if (!pblock->CheckBlock())
    {
        delete pblock;
        return error("ProcessBlock() : CheckBlock FAILED");
    }

    // If don't already have its previous block, shunt it off to holding area until we get it
    if (!mapBlockIndex.count(pblock->m_hashPrevBlock))
    {
        printf("ProcessBlock: ORPHAN BLOCK, prev=%s\n", pblock->m_hashPrevBlock.ToString().substr(0,14).c_str());
        mapOrphanBlocks.insert(make_pair(hash, pblock));
        mapOrphanBlocksByPrev.insert(make_pair(pblock->m_hashPrevBlock, pblock));

        // Ask this guy to fill in what we're missing
        if (pfrom)
            pfrom->PushMessage("getblocks", CBlockLocator(pindexBest), GetOrphanRoot(pblock));
        return true;
    }

    // Store to disk
    if (!pblock->AcceptBlock())
    {
        delete pblock;
        return error("ProcessBlock() : AcceptBlock FAILED");
    }
    delete pblock;

    // Recursively process any orphan blocks that depended on this one
    vector<uint256> vWorkQueue;
    vWorkQueue.push_back(hash);
    for (int i = 0; i < vWorkQueue.size(); i++)
    {
        uint256 hashPrev = vWorkQueue[i];
        for (multimap<uint256, CBlock*>::iterator mi = mapOrphanBlocksByPrev.lower_bound(hashPrev);
             mi != mapOrphanBlocksByPrev.upper_bound(hashPrev);
             ++mi)
        {
            CBlock* pblockOrphan = (*mi).second;
            if (pblockOrphan->AcceptBlock())
                vWorkQueue.push_back(pblockOrphan->GetHash());
            mapOrphanBlocks.erase(pblockOrphan->GetHash());
            delete pblockOrphan;
        }
        mapOrphanBlocksByPrev.erase(hashPrev);
    }

    printf("ProcessBlock: ACCEPTED\n");
    return true;
}








template<typename Stream>
bool ScanMessageStart(Stream& s)
{
    // Scan ahead to the next pchMessageStart, which should normally be immediately
    // at the file pointer.  Leaves file pointer at end of pchMessageStart.
    s.clear(0);
    short prevmask = s.exceptions(0);
    const char* p = BEGIN(pchMessageStart);
    try
    {
        loop
        {
            char c;
            s.read(&c, 1);
            if (s.fail())
            {
                s.clear(0);
                s.exceptions(prevmask);
                return false;
            }
            if (*p != c)
                p = BEGIN(pchMessageStart);
            if (*p == c)
            {
                if (++p == END(pchMessageStart))
                {
                    s.clear(0);
                    s.exceptions(prevmask);
                    return true;
                }
            }
        }
    }
    catch (...)
    {
        s.clear(0);
        s.exceptions(prevmask);
        return false;
    }
}

string GetAppDir()
{
    string strDir;
    if (!strSetDataDir.empty())
    {
        strDir = strSetDataDir;
    }
    else if (getenv("APPDATA"))
    {
        //strDir = strprintf("%s\\Bitcoin", getenv("APPDATA"));
		strDir = strprintf("%s\\Bitcoin-debug", getenv("APPDATA"));
    }
    else if (getenv("USERPROFILE"))
    {
        string strAppData = strprintf("%s\\Application Data", getenv("USERPROFILE"));
        static bool fMkdirDone;
        if (!fMkdirDone)
        {
            fMkdirDone = true;
            mkdir(strAppData.c_str(), 775);
        }
       // strDir = strprintf("%s\\Bitcoin", strAppData.c_str());
		strDir = strprintf("%s\\Bitcoin-debug", strAppData.c_str());
    }
    else
    {
        return ".";
    }
    static bool fMkdirDone;
    if (!fMkdirDone)
    {
        fMkdirDone = true;
        mkdir(strDir.c_str(), 775);
    }
    return strDir;
}

bool CheckDiskSpace(int64 nAdditionalBytes)
{
#if 0
    uint64 nFreeBytesAvailable = 0;     // bytes available to caller
    uint64 nTotalNumberOfBytes = 0;     // bytes on disk
    uint64 nTotalNumberOfFreeBytes = 0; // free bytes on disk

    if (!GetDiskFreeSpaceEx(GetAppDir().c_str(),
            (PULARGE_INTEGER)&nFreeBytesAvailable,
            (PULARGE_INTEGER)&nTotalNumberOfBytes,
            (PULARGE_INTEGER)&nTotalNumberOfFreeBytes))
    {
        printf("ERROR: GetDiskFreeSpaceEx() failed\n");
        return true;
    }

    // Check for 15MB because database could create another 10MB log file at any time
    if ((int64)nFreeBytesAvailable < 15000000 + nAdditionalBytes)
    {
        fShutdown = true;
        //wxMessageBox("Warning: Your disk space is low  ", "Bitcoin", wxICON_EXCLAMATION);
        _beginthread(Shutdown, 0, NULL);
        return false;
    }
#endif
    return true;
}

// �򿪿��ļ�
// ֪�����ļ���ӦnFileֵ�Ϳ���֪�����Ӧ���ļ�����blk${nFile}.dat
FILE* OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, const char* pszMode)
{
    if (nFile == -1)
        return NULL;
    FILE* file = fopen(strprintf("%s\/blk%04d.dat", GetAppDir().c_str(), nFile).c_str(), pszMode);
    if (!file)
        return NULL;
    if (nBlockPos != 0 && !strchr(pszMode, 'a') && !strchr(pszMode, 'w'))
    {
		// ���ļ��и��ݿ��ƫ�ƽ��ж�λ�ļ�ָ��
        if (fseek(file, nBlockPos, SEEK_SET) != 0)
        {
            fclose(file);
            return NULL;
        }
    }
    return file;
}

// ȫ�־�̬���������ƶ�Ӧ�ĵ�ǰblock��Ӧ���ļ���ţ�Ҳ�����ļ�����
// ÿһ��block��Ӧһ���������ļ�
static unsigned int nCurrentBlockFile = 1;

// ���ص�ǰblockӦ���ڵ��ļ�ָ��
FILE* AppendBlockFile(unsigned int& nFileRet)
{
    nFileRet = 0;
    loop
    {
        FILE* file = OpenBlockFile(nCurrentBlockFile, 0, "ab");
        if (!file)
            return NULL;
        if (fseek(file, 0, SEEK_END) != 0)
            return NULL;
        // FAT32 filesize max 4GB, fseek and ftell max 2GB, so we must stay under 2GB
        if (ftell(file) < 0x7F000000 - MAX_SIZE)
        {
            nFileRet = nCurrentBlockFile;
            return file;
        }
        fclose(file);
        nCurrentBlockFile++;
    }
}

bool LoadBlockIndex(bool fAllowNew)
{
    //
    // Load block index
    //
    CTxDB txdb("cr");
    if (!txdb.LoadBlockIndex())
        return false;
    txdb.Close();

    //
    // Init with genesis block
    //
    if (mapBlockIndex.empty())
    {
        if (!fAllowNew)
            return false;


        // Genesis Block:
        // GetHash()      = 0xdc8be64865ce298ca87ebee785b15fa29240e60db149c2ba85666e702f3eb734
        // hashMerkleRoot = 0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b
        // txNew.vin[0].scriptSig     = 486604799 4 0x736B6E616220726F662074756F6C69616220646E6F63657320666F206B6E697262206E6F20726F6C6C65636E61684320393030322F6E614A2F33302073656D695420656854
        // txNew.vout[0].m_nValue       = 5000000000
        // txNew.vout[0].m_cScriptPubKey = 0x5F1DF16B2B704C8A578D0BBAF74D385CDE12C11EE50455F3C438EF4C3FBCF649B6DE611FEAE06279A60939E028A8D65C10B73071A6F16719274855FEB0FD8A6704 OP_CHECKSIG
        // block.nVersion = 1
        // block.m_uTime    = 1231006505
        // block.m_uBits    = 0x1d00ffff
        // block.m_uNonce   = 2083236893
        // CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, m_uTime=1231006505, m_uBits=1d00ffff, m_uNonce=2083236893, vtx=1)
        //   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
        //     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
        //     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
        //   vMerkleTree: 4a5e1e

        // Genesis block
        char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
        CTransaction txNew;
        txNew.m_vTxIn.resize(1);
        txNew.m_vTxOut.resize(1);
        txNew.m_vTxIn[0].m_cScriptSig     = CScript() << 486604799 << CBigNum(4) << vector<unsigned char>((unsigned char*)pszTimestamp, (unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.m_vTxOut[0].m_nValue       = 50 * COIN;
        txNew.m_vTxOut[0].m_cScriptPubKey = CScript() << CBigNum("0x5F1DF16B2B704C8A578D0BBAF74D385CDE12C11EE50455F3C438EF4C3FBCF649B6DE611FEAE06279A60939E028A8D65C10B73071A6F16719274855FEB0FD8A6704") << OP_CHECKSIG;
        CBlock block;
        block.m_vTrans.push_back(txNew);
        block.m_hashPrevBlock = 0;
        block.m_hashMerkleRoot = block.BuildMerkleTree();
        block.m_nCurVersion = 1;
        block.m_uTime    = 1231006505;
        block.m_uBits    = bnProofOfWorkLimit.GetCompact();//0x1d00ffff;
        block.m_uNonce   = 2083236893;

            //// debug print, delete this later
            printf("%s\n", block.GetHash().ToString().c_str());
            printf("%s\n", block.m_hashMerkleRoot.ToString().c_str());
            printf("%s\n", hashGenesisBlock.ToString().c_str());
            txNew.m_vTxOut[0].m_cScriptPubKey.print();
            block.print();
            assert(block.m_hashMerkleRoot == uint256("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        assert(block.GetHash() == hashGenesisBlock);

        // Start new block file
        unsigned int nFile;
        unsigned int nBlockPos;
        if (!block.WriteToDisk(!fClient, nFile, nBlockPos))
            return error("LoadBlockIndex() : writing genesis block to disk failed");
        if (!block.AddToBlockIndex(nFile, nBlockPos))
            return error("LoadBlockIndex() : genesis block not accepted");
    }

    return true;
}



void PrintBlockTree()
{
    // precompute tree structure
    map<CBlockIndex*, vector<CBlockIndex*> > mapNext;
    for (map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi)
    {
        CBlockIndex* pindex = (*mi).second;
        mapNext[pindex->m_pPrevBlkIndex].push_back(pindex);
        // test
        //while (rand() % 3 == 0)
        //    mapNext[pindex->m_pPrevBlkIndex].push_back(pindex);
    }

    vector<pair<int, CBlockIndex*> > vStack;
    vStack.push_back(make_pair(0, pindexGenesisBlock));

    int nPrevCol = 0;
    while (!vStack.empty())
    {
        int nCol = vStack.back().first;
        CBlockIndex* pindex = vStack.back().second;
        vStack.pop_back();

        // print split or gap
        if (nCol > nPrevCol)
        {
            for (int i = 0; i < nCol-1; i++)
                printf("| ");
            printf("|\\\n");
        }
        else if (nCol < nPrevCol)
        {
            for (int i = 0; i < nCol; i++)
                printf("| ");
            printf("|\n");
        }
        nPrevCol = nCol;

        // print columns
        for (int i = 0; i < nCol; i++)
            printf("| ");

        // print item
        CBlock block;
        block.ReadFromDisk(pindex, true);
#if 0
        printf("%d (%u,%u) %s  %s  tx %d",
            pindex->m_nCurHeight,
            pindex->nFile,
            pindex->nBlockPos,
            block.GetHash().ToString().substr(0,14).c_str(),
            DateTimeStr(block.m_uTime).c_str(),
            block.m_vTrans.size());
#endif
        //CRITICAL_BLOCK(cs_mapWallet)
        {
            if (mapWallet.count(block.m_vTrans[0].GetHash()))
            {
                CWalletTx& wtx = mapWallet[block.m_vTrans[0].GetHash()];
                printf("    mine:  %d  %d  %d", wtx.GetDepthInMainChain(), wtx.GetBlocksToMaturity(), wtx.GetCredit());
            }
        }
        printf("\n");


        // put the main timechain first
        vector<CBlockIndex*>& vNext = mapNext[pindex];
        for (int i = 0; i < vNext.size(); i++)
        {
            if (vNext[i]->m_pNextBlkIndex)
            {
                swap(vNext[0], vNext[i]);
                break;
            }
        }

        // iterate children
        for (int i = 0; i < vNext.size(); i++)
            vStack.push_back(make_pair(nCol+i, vNext[i]));
    }
}










//////////////////////////////////////////////////////////////////////////////
//
// Messages
//

// �ж϶�Ӧ��������Ϣ�Ƿ��Ѿ�����
bool AlreadyHave(CTxDB& txdb, const CInv& inv)
{
    switch (inv.type)
    {
    case MSG_TX:        return mapTransactions.count(inv.hash) || txdb.ContainsTx(inv.hash);
    case MSG_BLOCK:     return mapBlockIndex.count(inv.hash) || mapOrphanBlocks.count(inv.hash);
    case MSG_REVIEW:    return true;
    case MSG_PRODUCT:   return mapProducts.count(inv.hash);
    }
    // Don't know what it is, just say we already got one
    return true;
}






// �������ڵ��Ӧ����Ϣ�������ڵ���յ�����Ϣ���д���
bool ProcessMessages(CNode* pfrom)
{
    CDataStream& vRecv = pfrom->vRecv;
    if (vRecv.empty())
        return true;
    printf("ProcessMessages(%d bytes)\n", vRecv.size());

    // ͬһ������Ϣ��ʽ
    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (x) data
    //
	// ��Ϣͷ������message start;command;size;

    loop
    {
        // Scan for message start
        CDataStream::iterator pstart = search(vRecv.begin(), vRecv.end(), BEGIN(pchMessageStart), END(pchMessageStart));
        // ɾ����Ч����Ϣ�� �����ڶ�Ӧ����Ϣ��ʼǰ�滹��һЩ��Ϣ
	    if (vRecv.end() - pstart < sizeof(CMessageHeader))
        {
            if (vRecv.size() > sizeof(CMessageHeader))
            {
                printf("\n\nPROCESSMESSAGE MESSAGESTART NOT FOUND\n\n");
                vRecv.erase(vRecv.begin(), vRecv.end() - sizeof(CMessageHeader));
            }
            break;
        }
        if (pstart - vRecv.begin() > 0)
            printf("\n\nPROCESSMESSAGE SKIPPED %d BYTES\n\n", pstart - vRecv.begin());
        vRecv.erase(vRecv.begin(), pstart); // �Ƴ���Ϣ��ʼ��Ϣ�ͽ��ջ�������ͷ֮��

		// ��ȡ��Ϣͷ
        // Read header
        CMessageHeader hdr;
        vRecv >> hdr; // ָ���Ѿ�ƫ����
        if (!hdr.IsValid())
        {
            printf("\n\nPROCESSMESSAGE: ERRORS IN HEADER %s\n\n\n", hdr.GetCommand().c_str());
            continue;
        }
        string strCommand = hdr.GetCommand();

        // Message size
        unsigned int nMessageSize = hdr.m_nMessageSize;
        if (nMessageSize > vRecv.size())
        {
            // Rewind and wait for rest of message
            ///// need a mechanism to give up waiting for overlong message size error
            printf("MESSAGE-BREAK 2\n");
            vRecv.insert(vRecv.begin(), BEGIN(hdr), END(hdr));
            sleep(100);
            break;
        }

        // Copy message to its own buffer
        CDataStream vMsg(vRecv.begin(), vRecv.begin() + nMessageSize, vRecv.nType, vRecv.nVersion);
        vRecv.ignore(nMessageSize);

        // Process message
        bool fRet = false;
        try
        {
            CheckForShutdown(2);
            //CRITICAL_BLOCK(cs_main)
                fRet = ProcessMessage(pfrom, strCommand, vMsg);
            CheckForShutdown(2);
        }
        CATCH_PRINT_EXCEPTION("ProcessMessage()")
        if (!fRet)
            printf("ProcessMessage(%s, %d bytes) from %s to %s FAILED\n", strCommand.c_str(), nMessageSize, pfrom->addr.ToString().c_str(), addrLocalHost.ToString().c_str());
    }

    vRecv.Compact();
    return true;
}



// �Խڵ�pFrom��������strCommand��Ӧ����Ϣ����ΪvRecv
bool ProcessMessage(CNode* pfrom, string strCommand, CDataStream& vRecv)
{
    static map<unsigned int, vector<unsigned char> > mapReuseKey;
    printf("received: %-12s (%d bytes)  ", strCommand.c_str(), vRecv.size());
    for (int i = 0; i < min(vRecv.size(), (unsigned int)20); i++)
        printf("%02x ", vRecv[i] & 0xff);
    printf("\n");
	// ��Ϣ�ɼ�Ƶ�ʽ��д���
    if (nDropMessagesTest > 0 && GetRand(nDropMessagesTest) == 0)
    {
        printf("dropmessages DROPPING RECV MESSAGE\n");
        return true;
    }

	// ��������ǰ汾���ڵ��Ӧ�İ汾
    if (strCommand == "version")
    {
		// �ڵ��Ӧ�İ汾ֻ�ܸ���һ�Σ���ʼΪ0��������и���
        // Can only do this once
        if (pfrom->nVersion != 0)
            return false;

        int64 m_uTime;
        CAddress addrMe; // ��ȡ��Ϣ��Ӧ������
        vRecv >> pfrom->nVersion >> pfrom->m_nServices >> m_uTime >> addrMe;
        if (pfrom->nVersion == 0)
            return false;
		// ���·��ͺͽ��ջ������еĶ�Ӧ�İ汾
        pfrom->vSend.SetVersion(min(pfrom->nVersion, VERSION));
        pfrom->vRecv.SetVersion(min(pfrom->nVersion, VERSION));

		// ����ڵ��Ӧ�ķ��������ǽڵ����磬���Ӧ�ڵ�Ŀͻ��˱�Ǿ���false
        pfrom->fClient = !(pfrom->m_nServices & NODE_NETWORK);
        if (pfrom->fClient)
        {
			// ������ǽڵ����磬���ܽ�����һЩ�ڵ㲻Ҫ�����Ӧ������������Ϣ��������Ҫ�����ͷ������У��Ϳ�����
            pfrom->vSend.nType |= SER_BLOCKHEADERONLY;
            pfrom->vRecv.nType |= SER_BLOCKHEADERONLY;
        }
		// ����ʱ���������ݣ�û��ʲô�ô��������������
        AddTimeData(pfrom->addr.ip, m_uTime);

		// �Ե�һ�������Ľڵ�����block��Ϣ
        // Ask the first connected node for block updates
        static bool fAskedForBlocks;
        if (!fAskedForBlocks && !pfrom->fClient)
        {
            fAskedForBlocks = true;
            pfrom->PushMessage("getblocks", CBlockLocator(pindexBest), uint256(0));
        }

        printf("version message: %s has version %d, addrMe=%s\n", pfrom->addr.ToString().c_str(), pfrom->nVersion, addrMe.ToString().c_str());
    }


    else if (pfrom->nVersion == 0)
    {
		// �ڵ��ڴ����κ���Ϣ֮ǰһ����һ���汾��Ϣ
        // Must have a version message before anything else
        return false;
    }

	// ��ַ��Ϣ
    else if (strCommand == "addr")
    {
        vector<CAddress> vAddr;
        vRecv >> vAddr;

        // Store the new addresses
        CAddrDB addrdb;
        foreach(const CAddress& addr, vAddr)
        {
            if (fShutdown)
                return true;
			// ����ַ���ӵ����ݿ���
            if (AddAddress(addrdb, addr))
            {
                // Put on lists to send to other nodes
                pfrom->setAddrKnown.insert(addr); // ����Ӧ�ĵ�ַ���뵽��֪��ַ������
                //CRITICAL_BLOCK(cs_vNodes)
                    foreach(CNode* pnode, vNodes)
                        if (!pnode->setAddrKnown.count(addr))
                            pnode->vAddrToSend.push_back(addr);// ��ַ�Ĺ㲥
            }
        }
    }

	// �����Ϣ
    else if (strCommand == "inv")
    {
        vector<CInv> vInv;
        vRecv >> vInv;

        CTxDB txdb("r");
        foreach(const CInv& inv, vInv)
        {
            if (fShutdown)
                return true;
            pfrom->AddInventoryKnown(inv); // ����Ӧ�Ŀ�淢����Ϣ���ӵ���淢����֪��

            bool fAlreadyHave = AlreadyHave(txdb, inv);
            printf("  got inventory: %s  %s\n", inv.ToString().c_str(), fAlreadyHave ? "have" : "new");

            if (!fAlreadyHave)
                pfrom->AskFor(inv);// ��������ڣ���������ѯ����������߳��з���getdata��Ϣ
            else if (inv.type == MSG_BLOCK && mapOrphanBlocks.count(inv.hash))
                pfrom->PushMessage("getblocks", CBlockLocator(pindexBest), GetOrphanRoot(mapOrphanBlocks[inv.hash]));
        }
    }

	// ��ȡ����
    else if (strCommand == "getdata")
    {
        vector<CInv> vInv;
        vRecv >> vInv;

        foreach(const CInv& inv, vInv)
        {
            if (fShutdown)
                return true;
            printf("received getdata for: %s\n", inv.ToString().c_str());

            if (inv.type == MSG_BLOCK)
            {
                // Send block from disk
                map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(inv.hash);
                if (mi != mapBlockIndex.end())
                {
                    //// could optimize this to send header straight from blockindex for client
                    CBlock block;
                    block.ReadFromDisk((*mi).second, !pfrom->fClient);
                    pfrom->PushMessage("block", block);// ��ȡ���ݶ�Ӧ��������block�����Ͷ�Ӧ�Ŀ���Ϣ
                }
            }
            else if (inv.IsKnownType())
            {
                // Send stream from relay memory
                //CRITICAL_BLOCK(cs_mapRelay)
                {
                    map<CInv, CDataStream>::iterator mi = mapRelay.find(inv); // ����ת��������
                    if (mi != mapRelay.end())
                        pfrom->PushMessage(inv.GetCommand(), (*mi).second);
                }
            }
        }
    }


    else if (strCommand == "getblocks")
    {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

		//�ҵ������е����������ϵ�
        // Find the first block the caller has in the main chain
        CBlockIndex* pindex = locator.GetBlockIndex();

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
            //CRITICAL_BLOCK(pfrom->cs_inventory)
            {
                CInv inv(MSG_BLOCK, pindex->GetBlockHash());
				// �ж�����֪���2���Ƿ����
                // returns true if wasn't already contained in the set
                if (pfrom->setInventoryKnown2.insert(inv).second)
                {
                    pfrom->setInventoryKnown.erase(inv);
                    pfrom->vInventoryToSend.push_back(inv);// �����Ӧ�Ŀ�淢�ͼ�����׼�����ͣ�����һ���߳��н��з��ͣ����͵���ϢΪinv
                }
            }
        }
    }

	// ��������
    else if (strCommand == "tx")
    {
        vector<uint256> vWorkQueue;
        CDataStream vMsg(vRecv);
        CTransaction tx;
        vRecv >> tx;

        CInv inv(MSG_TX, tx.GetHash());
        pfrom->AddInventoryKnown(inv);// ��������Ϣ���뵽��Ӧ����֪�����

        bool fMissingInputs = false;
		// ��������ܹ�������
        if (tx.AcceptTransaction(true, &fMissingInputs))
        {
            AddToWalletIfMine(tx, NULL);
            RelayMessage(inv, vMsg);// ת����Ϣ
            mapAlreadyAskedFor.erase(inv);
            vWorkQueue.push_back(inv.hash);

			// �ݹ鴦����������������׶�Ӧ�Ĺ¶�����
            // Recursively process any orphan transactions that depended on this one
            for (int i = 0; i < vWorkQueue.size(); i++)
            {
                uint256 hashPrev = vWorkQueue[i];
                for (multimap<uint256, CDataStream*>::iterator mi = mapOrphanTransactionsByPrev.lower_bound(hashPrev);
                     mi != mapOrphanTransactionsByPrev.upper_bound(hashPrev);
                     ++mi)
                {
                    const CDataStream& vMsg = *((*mi).second);
                    CTransaction tx;
                    CDataStream(vMsg) >> tx;
                    CInv inv(MSG_TX, tx.GetHash());

                    if (tx.AcceptTransaction(true))
                    {
                        printf("   accepted orphan tx %s\n", inv.hash.ToString().substr(0,6).c_str());
                        AddToWalletIfMine(tx, NULL);
                        RelayMessage(inv, vMsg);
                        mapAlreadyAskedFor.erase(inv);
                        vWorkQueue.push_back(inv.hash);
                    }
                }
            }

            foreach(uint256 hash, vWorkQueue)
                EraseOrphanTx(hash);
        }
        else if (fMissingInputs)
        {
            printf("storing orphan tx %s\n", inv.hash.ToString().substr(0,6).c_str());
            AddOrphanTx(vMsg); // ������׵�ǰ�����������Ӧ�Ĺ¶�����
        }
    }


    else if (strCommand == "review")
    {
        CDataStream vMsg(vRecv);
        CReview review;
        vRecv >> review;

        CInv inv(MSG_REVIEW, review.GetHash());
        pfrom->AddInventoryKnown(inv);

        if (review.AcceptReview())
        {
            // Relay the original message as-is in case it's a higher version than we know how to parse
            RelayMessage(inv, vMsg);
            mapAlreadyAskedFor.erase(inv);
        }
    }


    else if (strCommand == "block")
    {
        auto_ptr<CBlock> pblock(new CBlock);
        vRecv >> *pblock;

        //// debug print
        printf("received block:\n"); pblock->print();

        CInv inv(MSG_BLOCK, pblock->GetHash());
        pfrom->AddInventoryKnown(inv);// ���ӿ��

        if (ProcessBlock(pfrom, pblock.release()))
            mapAlreadyAskedFor.erase(inv);
    }


    else if (strCommand == "getaddr")
    {
        pfrom->vAddrToSend.clear();
        int64 nSince = GetAdjustedTime() - 5 * 24 * 60 * 60; // in the last 5 days
        //CRITICAL_BLOCK(cs_mapAddresses)
        {
            unsigned int nSize = mapAddresses.size();
            foreach(const PAIRTYPE(vector<unsigned char>, CAddress)& item, mapAddresses)
            {
                if (fShutdown)
                    return true;
                const CAddress& addr = item.second;
                //// will need this if we lose IRC
                //if (addr.nTime > nSince || (rand() % nSize) < 500)
                if (addr.nTime > nSince)
                    pfrom->vAddrToSend.push_back(addr);
            }
        }
    }


    else if (strCommand == "checkorder")
    {
        uint256 hashReply;
        CWalletTx order;
        vRecv >> hashReply >> order;

        /// we have a chance to check the order here

        // Keep giving the same key to the same ip until they use it
        if (!mapReuseKey.count(pfrom->addr.ip))
            mapReuseKey[pfrom->addr.ip] = GenerateNewKey();

        // Send back approval of order and pubkey to use
        CScript scriptPubKey;
        scriptPubKey << mapReuseKey[pfrom->addr.ip] << OP_CHECKSIG;
        pfrom->PushMessage("reply", hashReply, (int)0, scriptPubKey);
    }


    else if (strCommand == "submitorder")
    {
        uint256 hashReply;
        CWalletTx wtxNew;
        vRecv >> hashReply >> wtxNew;

        // Broadcast
        if (!wtxNew.AcceptWalletTransaction())
        {
            pfrom->PushMessage("reply", hashReply, (int)1);
            return error("submitorder AcceptWalletTransaction() failed, returning error 1");
        }
        wtxNew.m_bTimeReceivedIsTxTime = true;
        AddToWallet(wtxNew);
        wtxNew.RelayWalletTransaction();
        mapReuseKey.erase(pfrom->addr.ip);

        // Send back confirmation
        pfrom->PushMessage("reply", hashReply, (int)0);
    }


    else if (strCommand == "reply")
    {
        uint256 hashReply;
        vRecv >> hashReply;

        CRequestTracker tracker;
     //   CRITICAL_BLOCK(pfrom->cs_mapRequests)
        {
            map<uint256, CRequestTracker>::iterator mi = pfrom->mapRequests.find(hashReply);
            if (mi != pfrom->mapRequests.end())
            {
                tracker = (*mi).second;
                pfrom->mapRequests.erase(mi);
            }
        }
        if (!tracker.IsNull())
            tracker.fn(tracker.param1, vRecv);
    }


    else
    {
        // Ignore unknown commands for extensibility
        printf("ProcessMessage(%s) : Ignored unknown message\n", strCommand.c_str());
    }


    if (!vRecv.empty())
        printf("ProcessMessage(%s) : %d extra bytes\n", strCommand.c_str(), vRecv.size());

    return true;
}








// ����ڵ��Ӧ����Ϣ����
bool SendMessages(CNode* pto)
{
    CheckForShutdown(2);
    //CRITICAL_BLOCK(cs_main)
    {
        // Don't send anything until we get their version message
        if (pto->nVersion == 0)
            return true;


        // ��Ϣ���͵ĵ�ַ
        // Message: addr
        //
        vector<CAddress> vAddrToSend;
        vAddrToSend.reserve(pto->vAddrToSend.size());
		// ������͵ĵ�ַ������֪��ַ�ļ����У����������ʱ��ַ����������
        foreach(const CAddress& addr, pto->vAddrToSend)
            if (!pto->setAddrKnown.count(addr))
                vAddrToSend.push_back(addr);
		// ��յ�ַ���͵�����
        pto->vAddrToSend.clear();
		// �����ʱ��ַ�������鲻Ϊ�գ�����е�ַ����Ϣ�ķ���
        if (!vAddrToSend.empty())
            pto->PushMessage("addr", vAddrToSend);


        // �����Ϣ����
        // Message: inventory
        //
        vector<CInv> vInventoryToSend;
        //CRITICAL_BLOCK(pto->cs_inventory)
        {
            vInventoryToSend.reserve(pto->vInventoryToSend.size());
            foreach(const CInv& inv, pto->vInventoryToSend)
            {
                // returns true if wasn't already contained in the set
                if (pto->setInventoryKnown.insert(inv).second)
                    vInventoryToSend.push_back(inv);
            }
            pto->vInventoryToSend.clear();
            pto->setInventoryKnown2.clear();
        }
		// �����Ϣ����
        if (!vInventoryToSend.empty())
            pto->PushMessage("inv", vInventoryToSend);


        // getdata��Ϣ����
        // Message: getdata
        //
        vector<CInv> vAskFor;
        int64 nNow = GetTime() * 1000000;
        CTxDB txdb("r");
		// �жϽڵ��Ӧ��������Ϣmap�Ƿ�Ϊ�գ��Ҷ�Ӧ������map�е���Ϣ��Ӧ����������ʱ���Ƿ�С�ڵ�ǰʱ��
        while (!pto->mapAskFor.empty() && (*pto->mapAskFor.begin()).first <= nNow)
        {
            const CInv& inv = (*pto->mapAskFor.begin()).second;
            printf("sending getdata: %s\n", inv.ToString().c_str());
            if (!AlreadyHave(txdb, inv))
                vAskFor.push_back(inv);// �����ڲ���Ҫ������Ϣ����
            pto->mapAskFor.erase(pto->mapAskFor.begin());// ������Ϣ������һ����ɾ��һ��
        }
        if (!vAskFor.empty())
            pto->PushMessage("getdata", vAskFor);

    }
    return true;
}














//////////////////////////////////////////////////////////////////////////////
//
// BitcoinMiner
//

int FormatHashBlocks(void* pbuffer, unsigned int len)
{
    unsigned char* pdata = (unsigned char*)pbuffer;
    unsigned int blocks = 1 + ((len + 8) / 64);
    unsigned char* pend = pdata + 64 * blocks;
    memset(pdata + len, 0, 64 * blocks - len);
    pdata[len] = 0x80;
    unsigned int bits = len * 8;
    pend[-1] = (bits >> 0) & 0xff;
    pend[-2] = (bits >> 8) & 0xff;
    pend[-3] = (bits >> 16) & 0xff;
    pend[-4] = (bits >> 24) & 0xff;
    return blocks;
}

using CryptoPP::ByteReverse;
static int detectlittleendian = 1;

// ����hash
void BlockSHA256(const void* pin, unsigned int nBlocks, void* pout)
{
    unsigned int* pinput = (unsigned int*)pin;
    unsigned int* pstate = (unsigned int*)pout;

    CryptoPP::SHA256::InitState(pstate);

    // ����Ǵ�˻���С��
    if (*(char*)&detectlittleendian != 0)
    {
        for (int n = 0; n < nBlocks; n++)
        {
            unsigned int pbuf[16];
            // ��С�˵����⽫�ֽڷ�ת
            for (int i = 0; i < 16; i++)
                pbuf[i] = ByteReverse(pinput[n * 16 + i]);
            CryptoPP::SHA256::Transform(pstate, pbuf);
        }
        for (int i = 0; i < 8; i++)
            pstate[i] = ByteReverse(pstate[i]);
    }
    else
    {
        for (int n = 0; n < nBlocks; n++)
            CryptoPP::SHA256::Transform(pstate, pinput + n * 16);
    }
}

// �ڵ��ڿ�
bool BitcoinMiner()
{
    printf("BitcoinMiner started\n");
    ////SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    CKey key;
    key.MakeNewKey(); // ʹ����Բ�����㷨���һ�Թ�Կ��˽Կ
	// �������0��ʼ
    CBigNum bnExtraNonce = 0;
    while (fGenerateBitcoins)
    {
        sleep(50);
        CheckForShutdown(3);
        while (vNodes.empty())
        {
            sleep(30);
            CheckForShutdown(3);
        }

        unsigned int nTransactionsUpdatedLast = nTransactionsUpdated;
        CBlockIndex* pindexPrev = pindexBest;
		// ��ȡ�ڿ��Ѷ�
        unsigned int m_uBits = GetNextWorkRequired(pindexPrev);
        printf("BitcoinMiner---[%0x]\n", m_uBits);

        // �����һ�����
        // Create coinbase tx
        //
        CTransaction txNew;
        txNew.m_vTxIn.resize(1);
        txNew.m_vTxIn[0].m_cPrevOut.SetNull();
        txNew.m_vTxIn[0].m_cScriptSig << m_uBits << ++bnExtraNonce;
        txNew.m_vTxOut.resize(1);
        txNew.m_vTxOut[0].m_cScriptPubKey << key.GetPubKey() << OP_CHECKSIG;


        // �����µ�����
        // Create new block
        //
        auto_ptr<CBlock> pblock(new CBlock());
        if (!pblock.get())
            return false;

		// ���ӱһ�������������ĵ�һ������
        // Add our coinbase tx as first transaction
        pblock->m_vTrans.push_back(txNew);

		// �ռ����µĽ��׷���������
        // Collect the latest transactions into the block
        int64 nFees = 0;
        //CRITICAL_BLOCK(cs_main)
        //CRITICAL_BLOCK(cs_mapTransactions)
        {
            CTxDB txdb("r");
            map<uint256, CTxIndex> mapTestPool;
            vector<char> vfAlreadyAdded(mapTransactions.size());
            bool fFoundSomething = true;
            unsigned int nBlockSize = 0;
            // ���ѭ������Ϊ�Ƕ��̣߳����ܸտ�ʼ��Ӧ�Ľ���û����ô�࣬���ڵȴ����ף����д����ֻ�ȴ�һ�֣����mapTransactions�кཻܶ����һ����
            while (fFoundSomething && nBlockSize < MAX_SIZE/2)
            {
                fFoundSomething = false;
                unsigned int n = 0;
                for (map<uint256, CTransaction>::iterator mi = mapTransactions.begin(); mi != mapTransactions.end(); ++mi, ++n)
                {
                    if (vfAlreadyAdded[n])
                        continue;
                    CTransaction& tx = (*mi).second;
                    if (tx.IsCoinBase() || !tx.IsFinal())
                        continue;

                    // Transaction fee requirements, mainly only needed for flood control
                    // Under 10K (about 80 inputs) is free for first 100 transactions
                    // Base rate is 0.01 per KB
                    // ���ݷ������ж�ÿһ��������Ҫ�����ٷ���
                    int64 nMinFee = tx.GetMinFee(pblock->m_vTrans.size() < 100);

                    map<uint256, CTxIndex> mapTestPoolTmp(mapTestPool);
                    // �жϵ�ǰ�����Ƿ������Ӧ����ͷ���Ҫ�󣬶�Ӧ��nFees��ConnectInputs�ǽ����ۼӵ�
                    if (!tx.ConnectInputs(txdb, mapTestPoolTmp, CDiskTxPos(1,1,1), 0, nFees, false, true, nMinFee))
                        continue;
                    swap(mapTestPool, mapTestPoolTmp);

                    pblock->m_vTrans.push_back(tx);
                    nBlockSize += ::GetSerializeSize(tx, SER_NETWORK); // ����ǰ�����Ľ��״�С�����Ӧ�Ŀ��С��
                    vfAlreadyAdded[n] = true;
                    fFoundSomething = true;
                }
            }
        }
        pblock->m_uBits = m_uBits; // ���ö�Ӧ���ڿ��Ѷ�ֵ
        pblock->m_vTrans[0].m_vTxOut[0].m_nValue = pblock->GetBlockValue(nFees); // ���ö�Ӧ�Ŀ��һ�����׶�Ӧ�������Ӧ��ֵ=���� + ���׷���
        printf("\n\nRunning BitcoinMiner with %d transactions in block, uBit[%0x]\n", pblock->m_vTrans.size(), m_uBits);


        //
        // Prebuild hash buffer
        //
        struct unnamed1
        {
            struct unnamed2
            {
                int nVersion;
                uint256 hashPrevBlock;
                uint256 hashMerkleRoot;
                unsigned int m_uTime;
                unsigned int m_uBits;
                unsigned int m_uNonce;
            }
            block;
            unsigned char pchPadding0[64];
            uint256 hash1;
            unsigned char pchPadding1[64];
        }
        tmp;

        tmp.block.nVersion       = pblock->m_nCurVersion;
        tmp.block.hashPrevBlock  = pblock->m_hashPrevBlock  = (pindexPrev ? pindexPrev->GetBlockHash() : 0);
        tmp.block.hashMerkleRoot = pblock->m_hashMerkleRoot = pblock->BuildMerkleTree();
        // ȡǰ11�������Ӧ�Ĵ���ʱ���Ӧ����λ��
        tmp.block.m_uTime          = pblock->m_uTime          = max((pindexPrev ? pindexPrev->GetMedianTimePast()+1 : 0), GetAdjustedTime());
        tmp.block.m_uBits          = pblock->m_uBits          = m_uBits;
        tmp.block.m_uNonce         = pblock->m_uNonce         = 1; // �������1��ʼ

        unsigned int nBlocks0 = FormatHashBlocks(&tmp.block, sizeof(tmp.block));
        unsigned int nBlocks1 = FormatHashBlocks(&tmp.hash1, sizeof(tmp.hash1));


        //
        // Search
        //
        unsigned int nStart = GetTime();
        uint256 hashTarget = CBigNum().SetCompact(pblock->m_uBits).getuint256(); // �����Ѷ�ϵ��ֵ��ȡ��Ӧ��hashĿ��ֵ
        printf("BitcoinMiner: hashTarget[%s]--nBlocks0[%u]--nBlocks1[%u]\n", hashTarget.ToString().c_str(), nBlocks0, nBlocks1);
        uint256 hash;
        loop
        {
            BlockSHA256(&tmp.block, nBlocks0, &tmp.hash1);
            BlockSHA256(&tmp.hash1, nBlocks1, &hash);


            // �ڿ�ɹ�
            if (hash <= hashTarget)
            {
                pblock->m_uNonce = tmp.block.m_uNonce;
                assert(hash == pblock->GetHash());

                //// debug print
                printf("BitcoinMiner:\n");
                printf("proof-of-work found  \n  hash: %s  \ntarget: %s\n", hash.GetHex().c_str(), hashTarget.GetHex().c_str());
                pblock->print();

                // Save key
                if (!AddKey(key))
                    return false;
                key.MakeNewKey();

                // Process this block the same as if we had received it from another node
                if (!ProcessBlock(NULL, pblock.release()))
                    printf("ERROR in BitcoinMiner, ProcessBlock, block not accepted\n");

                sleep(500);
                break;
            }

            // �������鴴��ʱ�䣬���������ڿ�
            // Update m_uTime every few seconds
            if ((++tmp.block.m_uNonce & 0x3ffff) == 0)
            {
                CheckForShutdown(3);
                if (tmp.block.m_uNonce == 0)
                    break;
                if (pindexPrev != pindexBest)
                    break;
                if (nTransactionsUpdated != nTransactionsUpdatedLast && GetTime() - nStart > 60)
                    break;
                if (!fGenerateBitcoins)
                    break;
                tmp.block.m_uTime = pblock->m_uTime = max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());
            }
        }
    }

    return true;
}



//////////////////////////////////////////////////////////////////////////////
//
// Actions
//


int64 GetBalance()
{
    int64 nStart, nEnd;
  //  QueryPerformanceCounter((LARGE_INTEGER*)&nStart);

    int64 nTotal = COIN*100;
    //CRITICAL_BLOCK(cs_mapWallet)
    {
        for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            CWalletTx* pcoin = &(*it).second;
            if (!pcoin->IsFinal() || pcoin->m_bSpent)
                continue;
            nTotal += pcoin->GetCredit();
        }
    }

//    QueryPerformanceCounter((LARGE_INTEGER*)&nEnd);
    ///printf(" GetBalance() time = %16I64d\n", nEnd - nStart);
    return nTotal;
}



bool SelectCoins(int64 nTargetValue, set<CWalletTx*>& setCoinsRet)
{
    setCoinsRet.clear();

    // List of values less than target
    int64 nLowestLarger = _I64_MAX;
    CWalletTx* pcoinLowestLarger = NULL;
    vector<pair<int64, CWalletTx*> > vValue;
    int64 nTotalLower = 0;
    printf("SelectCoins() nTargetValue:[%lu] --WalletSize[%d]\n", nTargetValue, mapWallet.size());
    //CRITICAL_BLOCK(cs_mapWallet)
    {
        for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            CWalletTx* pcoin = &(*it).second;
            if (!pcoin->IsFinal() || pcoin->m_bSpent)
                continue;
            int64 n = pcoin->GetCredit();
            if (n <= 0)
                continue;
            if (n < nTargetValue)
            {
                vValue.push_back(make_pair(n, pcoin));
                nTotalLower += n;
            }
            else if (n == nTargetValue)
            {
                setCoinsRet.insert(pcoin);
                return true;
            }
            else if (n < nLowestLarger)
            {
                nLowestLarger = n;
                pcoinLowestLarger = pcoin;
            }
        }
    }
    printf("SelectCoins() nTotalLower:[%lu] \n", nTotalLower);
    if (nTotalLower < nTargetValue)
    {
        if (pcoinLowestLarger == NULL)
            return false;
        setCoinsRet.insert(pcoinLowestLarger);
        return true;
    }

    // Solve subset sum by stochastic approximation
    sort(vValue.rbegin(), vValue.rend());
    vector<char> vfIncluded;
    vector<char> vfBest(vValue.size(), true);
    int64 nBest = nTotalLower;

    for (int nRep = 0; nRep < 1000 && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(vValue.size(), false);
        int64 nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (int i = 0; i < vValue.size(); i++)
            {
                if (nPass == 0 ? rand() % 2 : !vfIncluded[i])
                {
                    nTotal += vValue[i].first;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= vValue[i].first;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }

    // If the next larger is still closer, return it
    if (pcoinLowestLarger && nLowestLarger - nTargetValue <= nBest - nTargetValue)
        setCoinsRet.insert(pcoinLowestLarger);
    else
    {
        for (int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
                setCoinsRet.insert(vValue[i].second);

        //// debug print
        printf("SelectCoins() best subset: ");
        for (int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
                printf("%s ", FormatMoney(vValue[i].first).c_str());
        printf("total %s\n", FormatMoney(nBest).c_str());
    }
    printf("SelectCoins Return true\n");
    return true;
}




bool CreateTransaction(CScript scriptPubKey, int64 nValue, CWalletTx& wtxNew, int64& nFeeRequiredRet)
{
    nFeeRequiredRet = 0;
    {
        // txdb must be opened before the mapWallet lock
        CTxDB txdb("r");
        //CRITICAL_BLOCK(cs_mapWallet)
        {
            int64 nFee = nTransactionFee;
            loop
            {
                wtxNew.m_vTxIn.clear();
                wtxNew.m_vTxOut.clear();
                if (nValue < 0)
                    return false;
                int64 nValueOut = nValue;
                nValue += nFee;

                // Choose coins to use
                set<CWalletTx*> setCoins;
                if (!SelectCoins(nValue, setCoins)) 
                {
                     printf("CreateTransaction--SelectCoins Fail\n");
                    return false;
                
                }
                printf("CreateTransaction--SelectCoins Ok\n");
                int64 nValueIn = 0;
                foreach(CWalletTx* pcoin, setCoins)
                    nValueIn += pcoin->GetCredit();

                printf("CreateTransaction--nValueOut[%lu]--nValue[%lu]--nValueIn[%lu]\n", nValueOut, nValue, nValueIn);
                printf("CreateTransaction--scriptPubKey[%s]\n", scriptPubKey.ToString().c_str());
                // Fill vout[0] to the payee
                wtxNew.m_vTxOut.push_back(CTxOut(nValueOut, scriptPubKey));

                // Fill vout[1] back to self with any change
                if (nValueIn > nValue)
                {
                    /// todo: for privacy, should randomize the order of outputs,
                    //        would also have to use a new key for the change.
                    // Use the same key as one of the coins
                    vector<unsigned char> vchPubKey;
                    CTransaction& txFirst = *(*setCoins.begin());
                    foreach(const CTxOut& txout, txFirst.m_vTxOut)
                        if (txout.IsMine())
                            if (ExtractPubKey(txout.m_cScriptPubKey, true, vchPubKey))
                                break;
                    if (vchPubKey.empty())
                        return false;

                    // Fill vout[1] to ourself
                    CScript scriptPubKey;
                    scriptPubKey << vchPubKey << OP_CHECKSIG;
                    wtxNew.m_vTxOut.push_back(CTxOut(nValueIn - nValue, scriptPubKey));
                }

                // Fill vin
                foreach(CWalletTx* pcoin, setCoins)
                    for (int nOut = 0; nOut < pcoin->m_vTxOut.size(); nOut++)
                        if (pcoin->m_vTxOut[nOut].IsMine())
                            wtxNew.m_vTxIn.push_back(CTxIn(pcoin->GetHash(), nOut));

                // Sign
                int nIn = 0;
                foreach(CWalletTx* pcoin, setCoins)
                    for (int nOut = 0; nOut < pcoin->m_vTxOut.size(); nOut++)
                        if (pcoin->m_vTxOut[nOut].IsMine())
                            SignSignature(*pcoin, wtxNew, nIn++);
                // Check that enough fee is included
                if (nFee < wtxNew.GetMinFee(true))
                {
                    nFee = nFeeRequiredRet = wtxNew.GetMinFee(true);
                    continue;
                }
                // Fill vtxPrev by copying from previous transactions vtxPrev
                wtxNew.AddSupportingTransactions(txdb);
                wtxNew.m_bTimeReceivedIsTxTime = true;

                break;
            }
        }
    }
    return true;
}

// Call after CreateTransaction unless you want to abort
bool CommitTransactionSpent(const CWalletTx& wtxNew)
{
    //// todo: make this transactional, never want to add a transaction
    ////  without marking spent transactions
    printf("CommitTransactionSpent---1\n");
    // Add tx to wallet, because if it has change it's also ours,
    // otherwise just for transaction history.
    AddToWallet(wtxNew);

    // Mark old coins as spent
    set<CWalletTx*> setCoins;
    foreach(const CTxIn& txin, wtxNew.m_vTxIn)
        setCoins.insert(&mapWallet[txin.m_cPrevOut.m_u256Hash]);
    foreach(CWalletTx* pcoin, setCoins)
    {
        pcoin->m_bSpent = true;
        pcoin->WriteToDisk();
        vWalletUpdated.push_back(make_pair(pcoin->GetHash(), false));
    }
    return true;
}




bool SendMoney(CScript scriptPubKey, int64 nValue, CWalletTx& wtxNew)
{
//CRITICAL_BLOCK(cs_main)
    {
        int64 nFeeRequired;
        if (!CreateTransaction(scriptPubKey, nValue, wtxNew, nFeeRequired))
        {
            string strError;
            if (nValue + nFeeRequired > GetBalance())
                strError = strprintf("Error: This is an oversized transaction that requires a transaction fee of %s  ", FormatMoney(nFeeRequired).c_str());
            else
                strError = "Error: Transaction creation failed  ";
            //wxMessageBox(strError, "Sending...");
            return error("SendMoney() : %s\n", strError.c_str());
        }
        if (!CommitTransactionSpent(wtxNew))
        {
            //wxMessageBox("Error finalizing transaction  ", "Sending...");
            return error("SendMoney() : Error finalizing transaction");
        }

        printf("SendMoney: %s\n", wtxNew.GetHash().ToString().substr(0,6).c_str());

        // Broadcast
        if (!wtxNew.AcceptTransaction())
        {
            // This must not fail. The transaction has already been signed and recorded.
            throw runtime_error("SendMoney() : wtxNew.AcceptTransaction() failed\n");
            //wxMessageBox("Error: Transaction not valid  ", "Sending...");
            return error("SendMoney() : Error: Transaction not valid");
        }
        wtxNew.RelayWalletTransaction();
    }
    //MainFrameRepaint();
    return true;
}

/* EOF */
