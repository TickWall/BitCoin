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
#include "WalletService/CMerkleTx.h"
#include "WalletService/CTxIndex.h"
#include "WalletService/CWalletTx.h"
#include "BlockEngine/BlockEngine.h"
#include "BlockEngine/CBlock.h"
#include "BlockEngine/CBlockIndex.h"
#include "DAO/DaoServ.h"
#include "NetWorkService/CInv.h"
#include "NetWorkService/NetWorkServ.h"
using namespace Enze;
extern bool fClient ;
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
            if (!DaoServ::getInstance()->ReadTxIndex(GetHash(), txindex))
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

    const map<uint256, CBlockIndex*>& mapBlockIndex = BlockEngine::getInstance()->getMapBlockIndex();
    // Is the tx in a block that's in the main chain
    map<uint256, CBlockIndex*>::const_iterator mi = mapBlockIndex.find(m_hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !pindex->IsInMainChain())
        return 0;

	// ���ص�ǰ�����������еĸ߶ȣ����ǵ�ǰblock���������ĩβ֮���м���˶��ٸ�block��
    return BlockEngine::getInstance()->getBestIndex()->m_nCurHeight - pindex->m_nCurHeight + 1;
}


// ��ȡĬ�˶������������е����--��ǰ��������ĩβ�м���˶��ٸ�block
int CMerkleTx::GetDepthInMainChain() const
{
	// ���׵ĳ�ʼ������û�б�����block��
    if (m_hashBlock == 0 || m_nIndex == -1)
        return 0;

	// ��ȡ��ǰ�������ڵ�block�����ڴ����mapBlockIndex�л�ȡ
    // Find the block it claims to be in
    const map<uint256, CBlockIndex*>& mapBlockIndex = BlockEngine::getInstance()->getMapBlockIndex();
    map<uint256, CBlockIndex*>::const_iterator mi = mapBlockIndex.find(m_hashBlock);
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

    return BlockEngine::getInstance()->getBestIndex()->m_nCurHeight - pindex->m_nCurHeight + 1;
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


bool CMerkleTx::AcceptTransaction(bool fCheckInputs)
{
    if (fClient)
    {
        if (!IsInMainChain() && !ClientConnectInputs())
            return false;
        return CTransaction::AcceptTransaction(false);
    }
    else
    {
        return CTransaction::AcceptTransaction(fCheckInputs);
    }
}


/* EOF */

