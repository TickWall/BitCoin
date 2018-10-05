/*
 * =====================================================================================
 *
 *       Filename:  CMerkleTx.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 03:37:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "BlockEngine.h"
#include "TX/CMerkleTx.h"
#include "TX/CTxIndex.h"
#include "TX/CWalletTx.h"
#include "Block/CBlock.h"
#include "Block/CBlockIndex.h"
#include "Db/CTxDB.h"
#include "Network/CInv.h"
#include "Network/net.h"

// ��������ڶ�Ӧ�������У������ý��׶�Ӧ��Ĭ�˶�����֧
int CMerkleTx::SetMerkleBranch(const CBlock* pblock)
{
    if (fClient)
    {
        if (m_hashBlock == 0)
            return 0;
    }
    else
    {
        CBlock blockTmp;
        if (pblock == NULL)
        {
            // Load the block this tx is in
            CTxIndex txindex;
			// ���ݵ�ǰ���׵�hash�����ݿ��в��Ҷ�Ӧ�Ľ�������
            if (!CTxDB("r").ReadTxIndex(GetHash(), txindex))
                return 0;
			// ���ݽ�����������Ϣ�����ݿ��в�ѯ��Ӧ��block��Ϣ
            if (!blockTmp.ReadFromDisk(txindex.m_cDiskPos.m_nFile, txindex.m_cDiskPos.m_nBlockPos, true))
                return 0;
            pblock = &blockTmp;
        }

		// ���ݽ��׶�Ӧ��block��hashֵ
        // Update the tx's hashBlock
        m_hashBlock = pblock->GetHash();

		// ��λ��ǰ������block��Ӧ�Ľ����б��е�����
        // Locate the transaction
        for (m_nIndex = 0; m_nIndex < pblock->m_vTrans.size(); m_nIndex++)
            if (pblock->m_vTrans[m_nIndex] == *(CTransaction*)this)
                break;
        if (m_nIndex == pblock->m_vTrans.size())
        {
            m_vMerkleBranch.clear();
            m_nIndex = -1;
            printf("ERROR: SetMerkleBranch() : couldn't find tx in block\n");
            return 0;
        }

        // Fill in merkle branch
        m_vMerkleBranch = pblock->GetMerkleBranch(m_nIndex);
    }

    map<uint256, CBlockIndex*>& mapBlockIndex = BlockEngine::getInstance()->mapBlockIndex;
    // Is the tx in a block that's in the main chain
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(m_hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !pindex->IsInMainChain())
        return 0;

	// ���ص�ǰ�����������еĸ߶ȣ����ǵ�ǰblock���������ĩβ֮���м���˶��ٸ�block��
    return BlockEngine::getInstance()->pindexBest->m_nCurHeight - pindex->m_nCurHeight + 1;
}


// ��ȡĬ�˶������������е����--��ǰ��������ĩβ�м���˶��ٸ�block
int CMerkleTx::GetDepthInMainChain() const
{
	// ���׵ĳ�ʼ������û�б�����block��
    if (m_hashBlock == 0 || m_nIndex == -1)
        return 0;

	// ��ȡ��ǰ�������ڵ�block�����ڴ����mapBlockIndex�л�ȡ
    // Find the block it claims to be in
    map<uint256, CBlockIndex*>& mapBlockIndex = BlockEngine::getInstance()->mapBlockIndex;
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(m_hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !pindex->IsInMainChain())
        return 0;

	// ���Ĭ�˶������Ƿ��Ѿ�У�飬���û��У�������У�飬У��֮�����ֵ��Ϊtrue
    // Make sure the merkle branch connects to this block
    if (!m_bMerkleVerified)
    {
		// �жϽ����Ƿ���block��Ӧ��Ĭ�˶�����
        if (CBlock::CheckMerkleBranch(GetHash(), m_vMerkleBranch, m_nIndex) != pindex->m_hashMerkleRoot)
            return 0;
        m_bMerkleVerified = true;
    }

    return BlockEngine::getInstance()->pindexBest->m_nCurHeight - pindex->m_nCurHeight + 1;
}


// �ж϶�Ӧ�Ŀ��Ƿ���죬���Ǳ��������������Ͽɣ�����ǷǱһ����׶�Ӧ��Ϊ������Ϊ0������Ҫ���м���
// �����ԽСԽ�ã�˵����ǰ���ױ��ϿɵĶ�Խ��
int CMerkleTx::GetBlocksToMaturity() const
{
    if (!IsCoinBase())
        return 0;
    int iMax = max(0, (COINBASE_MATURITY+20) - GetDepthInMainChain());
    printf("CMerkleTx::GetBlocksToMaturity--[%d]\n", iMax);
    return iMax;
}


bool CMerkleTx::AcceptTransaction(CTxDB& txdb, bool fCheckInputs)
{
    if (fClient)
    {
        if (!IsInMainChain() && !ClientConnectInputs())
            return false;
        return CTransaction::AcceptTransaction(txdb, false);
    }
    else
    {
        return CTransaction::AcceptTransaction(txdb, fCheckInputs);
    }
}
bool CMerkleTx::AcceptTransaction()
{
    CTxDB txdb("r");
    return AcceptTransaction(txdb);
}

/* EOF */

