/*
 * =====================================================================================
 *
 *       Filename:  CBlockLocator.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 05:30:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef CXX_BT_CBLOCK_LOCATOR_H
#define CXX_BT_CBLOCK_LOCATOR_H
#include "BlockEngine.h"
//
// Describes a place in the block chain to another node such that if the
// other node doesn't have the same branch, it can find a recent common trunk.
// The further back it is, the further before the fork it may be.
//
class CBlockLocator
{
protected:
    vector<uint256> vHave; // ��������Ӧ��block����
public:

    CBlockLocator()
    {
    }

    explicit CBlockLocator(const CBlockIndex* pindex)
    {
        Set(pindex);
    }

    explicit CBlockLocator(uint256 hashBlock)
    {
        map<uint256, CBlockIndex*>& mapBlockIndex = BlockEngine::getInstance()->mapBlockIndex;
        map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end())
            Set((*mi).second);
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    )

    void Set(const CBlockIndex* pindex)
    {
        vHave.clear();
        int nStep = 1;
        while (pindex)
        {
            vHave.push_back(pindex->GetBlockHash());

			// ָ�����ٻ����㷨��ǰ10�����棬������ָ������һֱ��������ͷ��Ϊֹ
            // Exponentially larger steps back
            for (int i = 0; pindex && i < nStep; i++)
                pindex = pindex->m_pPrevBlkIndex;
            if (vHave.size() > 10)
                nStep *= 2;
        }
        vHave.push_back(BlockEngine::getInstance()->hashGenesisBlock); // Ĭ�Ϸ���һ����������
    }
	// �ҵ������е����������ϵĿ������
    CBlockIndex* GetBlockIndex()
    {
        map<uint256, CBlockIndex*>& mapBlockIndex = BlockEngine::getInstance()->mapBlockIndex;
        // Find the first block the caller has in the main chain
        foreach(const uint256& hash, vHave)
        {
			// �ҵ������е����������ϵ�
            map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
            if (mi != mapBlockIndex.end())
            {
                CBlockIndex* pindex = (*mi).second;
                if (pindex->IsInMainChain())
                    return pindex;
            }
        }
        
        return BlockEngine::getInstance()->pindexGenesisBlock;
    }

    uint256 GetBlockHash()
    {
        // Find the first block the caller has in the main chain
        map<uint256, CBlockIndex*>& mapBlockIndex = BlockEngine::getInstance()->mapBlockIndex;
        foreach(const uint256& hash, vHave)
        {
            map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
            if (mi != mapBlockIndex.end())
            {
                CBlockIndex* pindex = (*mi).second;
                if (pindex->IsInMainChain())
                    return hash;
            }
        }
        return BlockEngine::getInstance()->hashGenesisBlock;
    }

    int GetHeight()
    {
        CBlockIndex* pindex = GetBlockIndex();
        if (!pindex)
            return 0;
        return pindex->m_nCurHeight;
    }
};



#endif /* CXX_BT_CBLOCK_LOCATOR_H */
/* EOF */

