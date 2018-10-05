/*
 * =====================================================================================
 *
 *       Filename:  CBlock.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 07:24:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "Block/CBlock.h"
#include "Block/CBlockIndex.h"
#include "Block/CDiskBlockIndex.h"
#include "Block/CDiskTxPos.h"
#include "Db/CTxDB.h"
#include "Network/net.h"
#include "TX/CTxIndex.h"

// �������ӣ�ÿһ���������ӣ����ӵ�������������
bool CBlock::ConnectBlock(CTxDB& txdb, CBlockIndex* pindex)
{
    //// issue here: it doesn't know the version
    unsigned int nTxPos = pindex->m_nBlockPos + ::GetSerializeSize(CBlock(), SER_DISK) - 1 + GetSizeOfCompactSize(m_vTrans.size());

    map<uint256, CTxIndex> mapUnused;
    int64 nFees = 0;
    foreach(CTransaction& tx, m_vTrans)
    {
        CDiskTxPos posThisTx(pindex->m_nFile, pindex->m_nBlockPos, nTxPos);
        nTxPos += ::GetSerializeSize(tx, SER_DISK);
		// ��ÿһ�����׽������������ж�
        if (!tx.ConnectInputs(txdb, mapUnused, posThisTx, pindex->m_nCurHeight, nFees, true, false))
            return false;
    }
	// �һ������ж�Ӧ��������ܴ���������Ӧ�Ľ���+����������
    if (m_vTrans[0].GetValueOut() > GetBlockValue(nFees))
        return false;

    // Update block index on disk without changing it in memory.
    // The memory index structure will be changed after the db commits.
    if (pindex->m_pPrevBlkIndex)
    {
		// ����ǰ�������� ���� ǰһ����������֮��
        CDiskBlockIndex blockindexPrev(pindex->m_pPrevBlkIndex);
        blockindexPrev.m_NextHash = pindex->GetBlockHash();
        txdb.WriteBlockIndex(blockindexPrev);
    }

	// ������block����Щ
    // Watch for transactions paying to me
    foreach(CTransaction& tx, m_vTrans)
        AddToWalletIfMine(tx, this);
    return true;
}

//////////////////////////////////////////////////////////////////////////////
//
// CBlock and CBlockIndex
//
// �����������������ݿ��ļ��ж�ȡ��Ӧ��������Ϣ
bool CBlock::ReadFromDisk(const CBlockIndex* pblockindex, bool fReadTransactions)
{
    return ReadFromDisk(pblockindex->m_nFile, pblockindex->m_nBlockPos, fReadTransactions);
}

// ��ȡ��������Ӧ�ļ�ֵ������+���������ѣ�
int64 CBlock::GetBlockValue(int64 nFees) const
{
	// ����;��������ʼ������50�����ر�
    int64 nSubsidy = 50 * COIN;

	// ������ÿ4���һ�룬������2100��
	// nBestHeight �����������ÿ����210000��block���Ӧ�Ľ������룬������һ��block��Ҫ10����
	// �����210000��block��Ҫ��ʱ���� 210000*10/(60*24*360)=4.0509...���꣩ ������ÿ4���һ��
    // Subsidy is cut in half every 4 years
    nSubsidy >>= (nBestHeight / 210000);

    return nSubsidy + nFees;
}

// ��һ������block�Ͽ����ӣ������ͷ������Ӧ����Ϣ��ͬʱ�ͷ������Ӧ������������
bool CBlock::DisconnectBlock(CTxDB& txdb, CBlockIndex* pindex)
{
	// �����ͷŽ��׵�����
    // Disconnect in reverse order
    for (int i = m_vTrans.size()-1; i >= 0; i--)
        if (!m_vTrans[i].DisconnectInputs(txdb))
            return false;

	// ������������
    // Update block index on disk without changing it in memory.
    // The memory index structure will be changed after the db commits.
    if (pindex->m_pPrevBlkIndex)
    {
		// ����ǰ����������Ӧ��ǰһ������������hashNextֵΪ0����ʾ����ǰ����������ǰһ����������������ȥ��
        CDiskBlockIndex blockindexPrev(pindex->m_pPrevBlkIndex);
        blockindexPrev.m_NextHash = 0;
        txdb.WriteBlockIndex(blockindexPrev);
    }

    return true;
}

// ����ǰ�������ӵ���Ӧ��������������mapBlockIndex
bool CBlock::AddToBlockIndex(unsigned int nFile, unsigned int nBlockPos)
{
    // Check for duplicate
    uint256 hash = GetHash();
    if (mapBlockIndex.count(hash))
        return error("AddToBlockIndex() : %s already exists", hash.ToString().substr(0,14).c_str());

    // Construct new block index object
    CBlockIndex* pindexNew = new CBlockIndex(nFile, nBlockPos, *this);
    if (!pindexNew)
        return error("AddToBlockIndex() : new CBlockIndex failed");
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
    pindexNew->m_pHashBlock = &((*mi).first);
    map<uint256, CBlockIndex*>::iterator miPrev = mapBlockIndex.find(m_hashPrevBlock);
    if (miPrev != mapBlockIndex.end())
    {
        pindexNew->m_pPrevBlkIndex = (*miPrev).second;
		// ����ǰһ������������Ӧ�ĸ߶�
        pindexNew->m_nCurHeight = pindexNew->m_pPrevBlkIndex->m_nCurHeight + 1;
    }

    CTxDB txdb;
    txdb.TxnBegin();
    txdb.WriteBlockIndex(CDiskBlockIndex(pindexNew));

	// ���������Ӧ��ָ��
    // New best
	// �����ĸ߶��Ѿ����������ˣ�������������������ĳ��� ���� ���ڵ���Ϊ���������������ĳ���
    if (pindexNew->m_nCurHeight > nBestHeight)
    {
		// �ж��Ƿ��Ǵ�������
        if (pindexGenesisBlock == NULL && hash == hashGenesisBlock)
        {
            pindexGenesisBlock = pindexNew;
            txdb.WriteHashBestChain(hash);
        }
        else if (m_hashPrevBlock == hashBestChain)
        {
			// �����ǰ���Ӧ��ǰһ�����������
            // Adding to current best branch
            if (!ConnectBlock(txdb, pindexNew) || !txdb.WriteHashBestChain(hash))
            {
                txdb.TxnAbort();
                pindexNew->EraseBlockFromDisk();
                mapBlockIndex.erase(pindexNew->GetBlockHash());
                delete pindexNew;
                return error("AddToBlockIndex() : ConnectBlock failed");
            }
            txdb.TxnCommit();
			// ���������У������ö�Ӧ����������m_pNextBlkIndex�ֶΣ�����ǰ��������������ǰһ�����������ĺ���
            pindexNew->m_pPrevBlkIndex->m_pNextBlkIndex = pindexNew;

			// �����Ӧ�������Ѿ����뵽�����У����Ӧ�����齻��Ӧ��Ҫ�ӱ��ڵ㱣��Ľ����ڴ����ɾ��
            // Delete redundant memory transactions
            foreach(CTransaction& tx, m_vTrans)
                tx.RemoveFromMemoryPool();
        }
        else
        {
			// ��ǰ����Ȳ��Ǵ������飬�ҵ�ǰ�����Ӧ��ǰһ������Ҳ����������ϵ����
			// �ټ����������������ĳ��ȴ��ڱ��ڵ���Ϊ�����ĳ��ȣ����н����зֲ洦��
            // New best branch
            if (!Reorganize(txdb, pindexNew))
            {
                txdb.TxnAbort();
                return error("AddToBlockIndex() : Reorganize failed");
            }
        }

        // New best link
        hashBestChain = hash;
        pindexBest = pindexNew;
        nBestHeight = pindexBest->m_nCurHeight;
        nTransactionsUpdated++;
        printf("AddToBlockIndex: new best=%s  height=%d\n", hashBestChain.ToString().substr(0,14).c_str(), nBestHeight);
    }

    txdb.TxnCommit();
    txdb.Close();

	// ת����Щ��ĿǰΪֹ��û�н���block�е�Ǯ������
    // Relay wallet transactions that haven't gotten in yet
    if (pindexNew == pindexBest)
        RelayWalletTransactions();// �ڽڵ�֮�����ת��

  //  MainFrameRepaint();
    return true;
}



// ����У��
bool CBlock::CheckBlock() const
{
    // These are checks that are independent of context
    // that can be verified before saving an orphan �¶� block.

    // Size limits
    if (m_vTrans.empty() || m_vTrans.size() > MAX_SIZE || ::GetSerializeSize(*this, SER_DISK) > MAX_SIZE)
        return error("CheckBlock() : size limits failed");

	// block�Ĵ���ʱ�� ����ڵ�ǰʱ�� ����2��Сʱ
    // Check timestamp
    if (m_uTime > GetAdjustedTime() + 2 * 60 * 60)
        return error("CheckBlock() : block timestamp too far in the future");

	// �ڿ��бһ�����һ��Ҫ���ڣ����ҽ���ֻ�ܴ���һ��
    // First transaction must be coinbase, the rest must not be
    if (m_vTrans.empty() || !m_vTrans[0].IsCoinBase())
        return error("CheckBlock() : first tx is not coinbase");
    for (int i = 1; i < m_vTrans.size(); i++)
        if (m_vTrans[i].IsCoinBase())
            return error("CheckBlock() : more than one coinbase");

	// �Կ��еĽ��׽���У��
    // Check transactions
    foreach(const CTransaction& tx, m_vTrans)
        if (!tx.CheckTransaction())
            return error("CheckBlock() : CheckTransaction failed");

	// �Թ������Ѷ�ָ�����У��
    // Check proof of work matches claimed amount
    if (CBigNum().SetCompact(m_uBits) > bnProofOfWorkLimit)
        return error("CheckBlock() : m_uBits below minimum work");
	// ���㵱ǰ���hash�Ƿ������Ӧ�������Ѷ�ָ��
    if (GetHash() > CBigNum().SetCompact(m_uBits).getuint256())
        return error("CheckBlock() : hash doesn't match m_uBits");

	// ��Ĭ�˶�����Ӧ�ĸ�����У��
    // Check merkleroot
    if (m_hashMerkleRoot != BuildMerkleTree())
        return error("CheckBlock() : hashMerkleRoot mismatch");

    return true;
}
// �жϵ�ǰ�����ܹ�������
bool CBlock::AcceptBlock()
{
    // Check for duplicate
    uint256 hash = GetHash();
    if (mapBlockIndex.count(hash)) 
        return error("AcceptBlock() : block already in mapBlockIndex");

    // Get prev block index
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(m_hashPrevBlock);
    if (mi == mapBlockIndex.end())
        return error("AcceptBlock() : prev block not found");
    CBlockIndex* pindexPrev = (*mi).second;

	// ��ǰ�鴴����ʱ��Ҫ����ǰһ�����Ӧ����λ��ʱ��
    // Check timestamp against prev
    if (m_uTime <= pindexPrev->GetMedianTimePast())
        return error("AcceptBlock() : block's timestamp is too early");

	//������֤��У�飺ÿһ���ڵ��Լ������Ӧ�Ĺ������Ѷ�
    // Check proof of work
    if (m_uBits != GetNextWorkRequired(pindexPrev))
        return error("AcceptBlock() : incorrect proof of work");

    // Write block to history file
    if (!CheckDiskSpace(::GetSerializeSize(*this, SER_DISK)))
        return error("AcceptBlock() : out of disk space");
    unsigned int nFile;
    unsigned int nBlockPos;
	// ������Ϣд���ļ���
    if (!WriteToDisk(!fClient, nFile, nBlockPos))
        return error("AcceptBlock() : WriteToDisk failed");
	// ���ӿ��Ӧ�Ŀ�������Ϣ
    if (!AddToBlockIndex(nFile, nBlockPos))
        return error("AcceptBlock() : AddToBlockIndex failed");

    if (hashBestChain == hash)
        RelayInventory(CInv(MSG_BLOCK, hash));

    // // Add atoms to user reviews for coins created
    // vector<unsigned char> vchPubKey;
    // if (ExtractPubKey(vtx[0].vout[0].m_cScriptPubKey, false, vchPubKey))
    // {
    //     unsigned short nAtom = GetRand(USHRT_MAX - 100) + 100;
    //     vector<unsigned short> vAtoms(1, nAtom);
    //     AddAtomsAndPropagate(Hash(vchPubKey.begin(), vchPubKey.end()), vAtoms, true);
    // }

    return true;
}


/* EOF */

