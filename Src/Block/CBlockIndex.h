/*
 * =====================================================================================
 *
 *       Filename:  CBlockIndex.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 05:27:02 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef CXX_BT_CBLOCK_INDEX_H
#define CXX_BT_CBLOCK_INDEX_H

#include "Block/CBlock.h"
//
// The block chain is a tree shaped structure starting with the
// genesis block at the root, with each block potentially having multiple
// candidates to be the next block.  m_pPrevBlkIndex and m_pNextBlkIndex link a path through the
// main/longest chain.  A blockindex may have multiple m_pPrevBlkIndex pointing back
// to it, but m_pNextBlkIndex will only point forward to the longest branch, or will
// be null if the block is not part of the longest chain.
//
// �����������Ӧ��pNext��Ϊ�գ������������һ����Ӧ��������
// ������
class CBlockIndex
{
public:
    const uint256* m_pHashBlock; // ��Ӧ��hashֵָ��
    CBlockIndex* m_pPrevBlkIndex; // ָ��ǰһ��blockIndex
    CBlockIndex* m_pNextBlkIndex; // ָ��ǰ������������һ����ֻ�е�ǰ���������������ϵ�ʱ�����ֵ���Ƿǿ�
	// �������ļ��е���Ϣ
    unsigned int m_nFile; 
    unsigned int m_nBlockPos;
    int m_nCurHeight; // ���������������ȣ������м���˶��ٸ�block�����ǴӴ������鵽��ǰ�����м���˶��ٸ�����

    // block header ���ͷ����Ϣ
    int m_nCurVersion;
    uint256 m_hashMerkleRoot;
	// ȡǰ11�������Ӧ�Ĵ���ʱ��ƽ��ֵ
    unsigned int m_uTime;// �鴴��ʱ�䣬ȡ������ʱ����λ��
    unsigned int m_uBits;
    unsigned int m_uNonce;


    CBlockIndex()
    {
        m_pHashBlock = NULL;
        m_pPrevBlkIndex = NULL;
        m_pNextBlkIndex = NULL;
        m_nFile = 0;
        m_nBlockPos = 0;
        m_nCurHeight = 0;

        m_nCurVersion       = 0;
        m_hashMerkleRoot = 0;
        m_uTime          = 0;
        m_uBits          = 0;
        m_uNonce         = 0;
    }

    CBlockIndex(unsigned int nFileIn, unsigned int nBlockPosIn, CBlock& block)
    {
        m_pHashBlock = NULL;
        m_pPrevBlkIndex = NULL;
        m_pNextBlkIndex = NULL;
        m_nFile = nFileIn;
        m_nBlockPos = nBlockPosIn;
        m_nCurHeight = 0;

        m_nCurVersion       = block.m_nCurVersion;
        m_hashMerkleRoot = block.m_hashMerkleRoot;
        m_uTime          = block.m_uTime;
        m_uBits          = block.m_uBits;
        m_uNonce         = block.m_uNonce;
    }

    uint256 GetBlockHash() const
    {
        return *m_pHashBlock;
    }

	// �ж��Ƿ���������ͨ��m_pNextBlkIndex�Ƿ�Ϊ�պ͵�ǰ������ָ���Ƿ�������ָ��
    bool IsInMainChain() const
    {
        return (m_pNextBlkIndex || this == pindexBest);
    }

	// ���ļ����Ƴ���Ӧ�Ŀ�
    bool EraseBlockFromDisk()
    {
        // Open history file
        CAutoFile fileout = OpenBlockFile(m_nFile, m_nBlockPos, "rb+");
        if (!fileout)
            return false;

		// ���ļ���Ӧ��λ������дһ���տ飬�������൱�ڴ��ļ���ɾ����Ӧ�ĵ��ڿ�
        // Overwrite with empty null block
        CBlock block;
        block.SetNull();
        fileout << block;

        return true;
    }

	// ȡǰ11�������Ӧ�Ĵ���ʱ��ƽ��ֵ
    enum { nMedianTimeSpan=11 };

	// �ӵ�ǰ����ǰ�ƣ���ȡ��ȥ��Ӧ����λ��ʱ�䣬�ڶ�Ӧ���������л�ȡÿһ�������Ӧ��ʱ�䣬Ȼ��ȡ��λ��
    int64 GetMedianTimePast() const
    {
        unsigned int pmedian[nMedianTimeSpan];
        unsigned int* pbegin = &pmedian[nMedianTimeSpan];
        unsigned int* pend = &pmedian[nMedianTimeSpan];

        const CBlockIndex* pindex = this;
        for (int i = 0; i < nMedianTimeSpan && pindex; i++, pindex = pindex->m_pPrevBlkIndex)
            *(--pbegin) = pindex->m_uTime;

        sort(pbegin, pend);
        return pbegin[(pend - pbegin)/2];
    }
	// �ӵ�ǰ�������ƣ�ȡ��λ��ʱ��
    int64 GetMedianTime() const
    {
        const CBlockIndex* pindex = this;
        for (int i = 0; i < nMedianTimeSpan/2; i++)
        {
            if (!pindex->m_pNextBlkIndex)
                return m_uTime;
            pindex = pindex->m_pNextBlkIndex;
        }
        return pindex->GetMedianTimePast();
    }



    string ToString() const
    {
        return strprintf("CBlockIndex(nprev=%08x, m_pNextBlkIndex=%08x, m_nFile=%d, m_nBlockPos=%-6d m_nCurHeight=%d, merkle=%s, hashBlock=%s)",
            m_pPrevBlkIndex, m_pNextBlkIndex, m_nFile, m_nBlockPos, m_nCurHeight,
            m_hashMerkleRoot.ToString().substr(0,6).c_str(),
            GetBlockHash().ToString().substr(0,14).c_str());
    }

    void print() const
    {
        printf("%s\n", ToString().c_str());
    }
};


#endif /* CXX_BT_CBLOCK_INDEX_H */
/* EOF */

