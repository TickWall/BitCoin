/*
 * =====================================================================================
 *
 *       Filename:  CTransaction.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 11:04:33 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "TX/CTransaction.h"
#include "TX/CTxIndex.h"
#include "TX/CTxOutPoint.h"
#include "TX/CTxInPoint.h"
#include "Block/CBlockIndex.h"
#include "Db/CTxDB.h"

//////////////////////////////////////////////////////////////////////////////
//
// CTransaction
//

// �ж���߽����ܲ��ܱ����ܣ�����ܽ��ܽ���Ӧ�Ľ��׷���ȫ�ֱ�����mapTransactions��mapNextTx��
bool CTransaction::AcceptTransaction(CTxDB& txdb, bool fCheckInputs, bool* pfMissingInputs)
{
    if (pfMissingInputs)
        *pfMissingInputs = false;

	// �һ����׽����ڿ�����Ч���һ����ײ�����Ϊһ�������Ľ���
    // Coinbase is only valid in a block, not as a loose transaction
    if (IsCoinBase())
        return error("AcceptTransaction() : coinbase as individual tx");

    if (!CheckTransaction())
        return error("AcceptTransaction() : CheckTransaction failed");

	// �жϵ�ǰ�����Ƿ������Ѿ����յ�����
    // Do we already have it?
    uint256 hash = GetHash();
//    CRITICAL_BLOCK(cs_mapTransactions)
        if (mapTransactions.count(hash)) // �ж��ڴ����map���Ƿ��Ѿ�����
            return false;
    if (fCheckInputs)
        if (txdb.ContainsTx(hash)) // �жϽ���db���Ƿ��Ѿ�����
            return false;

	// �жϵ�ǰ���׶����Ƿ���ڴ��еĽ��׶����б��ͻ
    // Check for conflicts with in-memory transactions
    CTransaction* ptxOld = NULL;
    for (int i = 0; i < m_vTxIn.size(); i++)
    {
        COutPoint outpoint = m_vTxIn[i].m_cPrevOut;
		// ���ݵ�ǰ���׶�Ӧ�����뽻�ף���ö�Ӧ���뽻�׶�Ӧ���������
        if (mapNextTx.count(outpoint))
        {
            // Allow replacing with a newer version of the same transaction
			// i ==0 Ϊcoinbase��Ҳ����coinbase�����滻
            if (i != 0)
                return false;
			// ����ڵ�ǰ���׸��ϵĽ���
            ptxOld = mapNextTx[outpoint].m_pcTrans;
            if (!IsNewerThan(*ptxOld)) // �ж��Ƿ��ԭ�����׸��£�ͨ��nSequences�ж�
                return false;
            for (int i = 0; i < m_vTxIn.size(); i++)
            {
                COutPoint outpoint = m_vTxIn[i].m_cPrevOut;
				// ��ǰ���׵��������ڴ����mapNextTx��Ӧ�������������ڣ��Ҷ�ָ��ԭ���ϵĽ��ף�����մ˽���
                if (!mapNextTx.count(outpoint) || mapNextTx[outpoint].m_pcTrans != ptxOld)
                    return false;
            }
            break;
        }
    }

	// ��ǰ���׽���У�������ǰ���׶�Ӧ�����Ϊ���ѱ��
    // Check against previous transactions
    map<uint256, CTxIndex> mapUnused;
    int64 nFees = 0;
    if (fCheckInputs && !ConnectInputs(txdb, mapUnused, CDiskTxPos(1,1,1), 0, nFees, false, false))
    {
        if (pfMissingInputs)
            *pfMissingInputs = true;
        return error("AcceptTransaction() : ConnectInputs failed %s", hash.ToString().substr(0,6).c_str());
    }

	// ����ǰ���״洢���ڴ棬����ϵĽ��״��ڣ�����ڴ��н���Ӧ�Ľ����Ƴ�
    // Store transaction in memory
 //   CRITICAL_BLOCK(cs_mapTransactions)
    {
        if (ptxOld)
        {
            printf("mapTransaction.erase(%s) replacing with new version\n", ptxOld->GetHash().ToString().c_str());
            mapTransactions.erase(ptxOld->GetHash());
        }
		// ����ǰ���״洢���ڴ������
        AddToMemoryPool();
    }

	// ����ϵĽ��״��ڣ����Ǯ���н��ϵĽ����Ƴ�
    ///// are we sure this is ok when loading transactions or restoring block txes
    // If updated, erase old tx from wallet
    if (ptxOld)
		// �����״�Ǯ��ӳ�����mapWallet���Ƴ���ͬʱ�����״�CWalletDB���Ƴ�
        EraseFromWallet(ptxOld->GetHash());

    printf("AcceptTransaction(): accepted %s\n", hash.ToString().substr(0,6).c_str());
    return true;
}

// ����ǰ�������ӵ��ڴ��mapTransactions,mapNextTx�У����Ҹ��½��׸��µĴ���
bool CTransaction::AddToMemoryPool()
{
    // Add to memory pool without checking anything.  Don't call this directly,
    // call AcceptTransaction to properly check the transaction first.
 //   CRITICAL_BLOCK(cs_mapTransactions)
    {
        uint256 hash = GetHash();
        mapTransactions[hash] = *this; // ����ǰ���׷��뵽�ڴ����mapTransactions��
		// ���»������ö�Ӧ��mapNextTx �ǵĽ��׶�Ӧ������������Ӧ���Ǳ�����
        for (int i = 0; i < m_vTxIn.size(); i++)
            mapNextTx[m_vTxIn[i].m_cPrevOut] = CInPoint(&mapTransactions[hash], i);

		// ��¼���ױ����µĴ���
        nTransactionsUpdated++;
    }
    return true;
}

// ����ǰ���״��ڴ����mapTransactions��mapNextTx���Ƴ����������ӽ��׶�Ӧ�ĸ��´���
bool CTransaction::RemoveFromMemoryPool()
{
    // Remove transaction from memory pool
//    CRITICAL_BLOCK(cs_mapTransactions)
    {
        foreach(const CTxIn& txin, m_vTxIn)
            mapNextTx.erase(txin.m_cPrevOut);
        mapTransactions.erase(GetHash());
        nTransactionsUpdated++;
    }
    return true;
}

// �Ͽ��������룬�����ͷŽ��׶�Ӧ�������ռ�ã������ͷŽ��������Ӧ�Ľ��������ı��ռ��
bool CTransaction::DisconnectInputs(CTxDB& txdb)
{
	// ���������ó�ǰһ�����׶�Ӧ�Ļ��ѱ��ָ��
    // Relinquish previous transactions' spent pointers
    if (!IsCoinBase()) // �һ�
    {
        foreach(const CTxIn& txin, m_vTxIn)
        {
            COutPoint prevout = txin.m_cPrevOut;

            // Get prev txindex from disk
            CTxIndex txindex;
			// �����ݿ��ж�ȡ��Ӧ�Ľ��׵�����
            if (!txdb.ReadTxIndex(prevout.m_u256Hash, txindex))
                return error("DisconnectInputs() : ReadTxIndex failed");

            if (prevout.m_nIndex >= txindex.m_vSpent.size())
                return error("DisconnectInputs() : prevout.m_nIndex out of range");

            // Mark outpoint as not spent
            txindex.m_vSpent[prevout.m_nIndex].SetNull();

            // Write back
            txdb.UpdateTxIndex(prevout.m_u256Hash, txindex);
        }
    }

	// ����ǰ���״ӽ������������Ƴ�
    // Remove transaction from index
    if (!txdb.EraseTxIndex(*this))
        return error("DisconnectInputs() : EraseTxPos failed");

    return true;
}

// �����������ӣ�����Ӧ�Ľ�������ռ�ö�Ӧ�Ľ�������Ļ��ѱ��
bool CTransaction::ConnectInputs(CTxDB& txdb, map<uint256, CTxIndex>& mapTestPool, const CDiskTxPos& posThisTx, int m_nCurHeight, int64& nFees, bool fBlock, bool fMiner, int64 nMinFee)
{
	// ռ��ǰһ�����׶�Ӧ�Ļ���ָ��
    // Take over previous transactions' spent pointers
    if (!IsCoinBase())
    {
        int64 nValueIn = 0;
        for (int i = 0; i < m_vTxIn.size(); i++)
        {
            COutPoint prevout = m_vTxIn[i].m_cPrevOut;

            // Read txindex
            CTxIndex txindex;
            bool fFound = true;
            if (fMiner && mapTestPool.count(prevout.m_u256Hash))
            {
                // Get txindex from current proposed changes
                txindex = mapTestPool[prevout.m_u256Hash];
            }
            else
            {
                // Read txindex from txdb
                fFound = txdb.ReadTxIndex(prevout.m_u256Hash, txindex);
            }
            if (!fFound && (fBlock || fMiner))
                return fMiner ? false : error("ConnectInputs() : %s prev tx %s index entry not found", GetHash().ToString().substr(0,6).c_str(),  prevout.m_u256Hash.ToString().substr(0,6).c_str());

            // Read txPrev
            CTransaction txPrev;
            if (!fFound || txindex.m_cDiskPos == CDiskTxPos(1,1,1))
            {
                // Get prev tx from single transactions in memory
  //              CRITICAL_BLOCK(cs_mapTransactions)
                {
                    if (!mapTransactions.count(prevout.m_u256Hash))
                        return error("ConnectInputs() : %s mapTransactions prev not found %s", GetHash().ToString().substr(0,6).c_str(),  prevout.m_u256Hash.ToString().substr(0,6).c_str());
                    txPrev = mapTransactions[prevout.m_u256Hash];
                }
                if (!fFound)
                    txindex.m_vSpent.resize(txPrev.m_vTxOut.size());
            }
            else
            {
                // Get prev tx from disk
                if (!txPrev.ReadFromDisk(txindex.m_cDiskPos))
                    return error("ConnectInputs() : %s ReadFromDisk prev tx %s failed", GetHash().ToString().substr(0,6).c_str(),  prevout.m_u256Hash.ToString().substr(0,6).c_str());
            }

            if (prevout.m_nIndex >= txPrev.m_vTxOut.size() || prevout.m_nIndex >= txindex.m_vSpent.size())
                return error("ConnectInputs() : %s prevout.m_nIndex out of range %d %d %d", GetHash().ToString().substr(0,6).c_str(), prevout.m_nIndex, txPrev.m_vTxOut.size(), txindex.m_vSpent.size());

            // If prev is coinbase, check that it's matured
            if (txPrev.IsCoinBase())
                for (CBlockIndex* pindex = pindexBest; pindex && nBestHeight - pindex->m_nCurHeight < COINBASE_MATURITY-1; pindex = pindex->m_pPrevBlkIndex)
                    if (pindex->m_nBlockPos == txindex.m_cDiskPos.m_nBlockPos && pindex->m_nFile == txindex.m_cDiskPos.m_nFile)
                        return error("ConnectInputs() : tried to spend coinbase at depth %d", nBestHeight - pindex->m_nCurHeight);

            // Verify signature
            if (!VerifySignature(txPrev, *this, i))
                return error("ConnectInputs() : %s VerifySignature failed", GetHash().ToString().substr(0,6).c_str());

            // Check for conflicts
            if (!txindex.m_vSpent[prevout.m_nIndex].IsNull())
                return fMiner ? false : error("ConnectInputs() : %s prev tx already used at %s", GetHash().ToString().substr(0,6).c_str(), txindex.m_vSpent[prevout.m_nIndex].ToString().c_str());

			// ���ǰһ�����׶�Ӧ�Ľ���������Ӧ�Ļ��ѱ��
            // Mark outpoints as spent
            txindex.m_vSpent[prevout.m_nIndex] = posThisTx;

            // Write back
            if (fBlock)
                txdb.UpdateTxIndex(prevout.m_u256Hash, txindex);
            else if (fMiner)
                mapTestPool[prevout.m_u256Hash] = txindex;

            nValueIn += txPrev.m_vTxOut[prevout.m_nIndex].m_nValue;
        }

        // Tally transaction fees
        int64 nTxFee = nValueIn - GetValueOut();
        if (nTxFee < 0)
            return error("ConnectInputs() : %s nTxFee < 0", GetHash().ToString().substr(0,6).c_str());
        if (nTxFee < nMinFee)
            return false;
        nFees += nTxFee;
    }

    if (fBlock)
    {
        // Add transaction to disk index
        if (!txdb.AddTxIndex(*this, posThisTx, m_nCurHeight))
            return error("ConnectInputs() : AddTxPos failed");
    }
    else if (fMiner)
    {
		// ����ǿ󹤣�����Ӧ�Ľ��׷����Ӧ�Ľ��ײ��Գ���
        // Add transaction to test pool
        mapTestPool[GetHash()] = CTxIndex(CDiskTxPos(1,1,1), m_vTxOut.size());
    }

    return true;
}

// �ͻ����������룬�Խ��ױ��������֤
bool CTransaction::ClientConnectInputs()
{
    if (IsCoinBase())
        return false;

	// ռ��ǰһ�����׶�Ӧ�Ļ��ѱ��
    // Take over previous transactions' spent pointers
//    CRITICAL_BLOCK(cs_mapTransactions)
    {
        int64 nValueIn = 0;
        for (int i = 0; i < m_vTxIn.size(); i++)
        {
            // Get prev tx from single transactions in memory
            COutPoint prevout = m_vTxIn[i].m_cPrevOut;
            if (!mapTransactions.count(prevout.m_u256Hash))
                return false;
            CTransaction& txPrev = mapTransactions[prevout.m_u256Hash];

            if (prevout.m_nIndex >= txPrev.m_vTxOut.size())
                return false;

            // Verify signature
            if (!VerifySignature(txPrev, *this, i))
                return error("ConnectInputs() : VerifySignature failed");

            ///// this is redundant with the mapNextTx stuff, not sure which I want to get rid of
            ///// this has to go away now that posNext is gone
            // // Check for conflicts
            // if (!txPrev.vout[prevout.m_nIndex].posNext.IsNull())
            //     return error("ConnectInputs() : prev tx already used");
            //
            // // Flag outpoints as used
            // txPrev.vout[prevout.m_nIndex].posNext = posThisTx;

            nValueIn += txPrev.m_vTxOut[prevout.m_nIndex].m_nValue;
        }
        if (GetValueOut() > nValueIn)
            return false;
    }

    return true;
}


bool CTransaction::AcceptTransaction(bool fCheckInputs, bool* pfMissingInputs)
{
    CTxDB txdb("r");
    return AcceptTransaction(txdb, fCheckInputs, pfMissingInputs);
}

/* EOF */

