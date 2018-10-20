/*
 * =====================================================================================
 *
 *       Filename:  CBlock.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 05:25:07 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef CXX_BT_CBLOCK_H
#define CXX_BT_CBLOCK_H
#include "headers.h"
#include "CommonBase/uint256.h"
#include "WalletService/CTransaction.h"
#include "NetWorkService/CMessageHeader.h"

namespace Enze
{

    class CBlockIndex;
//
// Nodes collect new transactions into a block, hash them into a hash tree,
// and scan through nonce values to make the block's hash satisfy proof-of-work
// requirements.  When they solve the proof-of-work, they broadcast the block
// to everyone and the block is added to the block chain.  The first transaction
// in the block is a special one that creates a new coin owned by the creator
// of the block.
//
// Blocks are appended to blk0001.dat files on disk.  Their location on disk
// is indexed by CBlockIndex objects in memory.
//
// �鶨��
class CBlock
{
public:
    // header
    int m_nCurVersion; // ��İ汾����ҪΪ�˺���������ʹ��
    uint256 m_hashPrevBlock; // ǰһ�����Ӧ��hash
    uint256 m_hashMerkleRoot; // Ĭ�˶���Ӧ�ĸ�
	// ȡǰ11�������Ӧ�Ĵ���ʱ��ƽ��ֵ
    unsigned int m_uTime; // ��λΪ�룬ȡ�������ж�Ӧ��ǰ���ٸ������Ӧʱ�����λ�������������ǰһ����ȥ��ǰʱ��
    unsigned int m_uBits; // ��¼�������Ѷ�
    unsigned int m_uNonce; // ������֤���������������������������㵱ǰ�ڿ��Ӧ���Ѷ�

    // network and disk
    vector<CTransaction> m_vTrans; // ���н����б�

    // memory only
    mutable vector<uint256> m_vMerkleTree; // �������׶�Ӧ��Ĭ�˶����б�


    CBlock()
    {
        SetNull();
    }


    void SetNull()
    {
        m_nCurVersion = 1;
        m_hashPrevBlock = 0;
        m_hashMerkleRoot = 0;
        m_uTime = 0;
        m_uBits = 0;
        m_uNonce = 0;
        m_vTrans.clear();
        m_vMerkleTree.clear();
    }

    bool IsNull() const
    {
        return (m_uBits == 0);
    }

	// ��hashֵ����������m_nCurVersion �� m_uNonce ��ֵ
    uint256 GetHash() const
    {
        return Hash(BEGIN(m_nCurVersion), END(m_uNonce));
    }

	// ���ݽ��׹�����Ӧ��Ĭ�˶���
    uint256 BuildMerkleTree() const;
	// ���ݽ��׶�Ӧ��������ý��׶�Ӧ��Ĭ�˶���֧
    vector<uint256> GetMerkleBranch(int nIndex) const;
	// �жϵ�ǰ��Ӧ�Ľ���hash��Ĭ�˶���֧����֤��Ӧ�Ľ����Ƿ��ڶ�Ӧ��blcok��
    static uint256 CheckMerkleBranch(uint256 hash, const vector<uint256>& vMerkleBranch, int nIndex);

	// ��blockд�뵽�ļ���
    bool WriteToDisk(bool fWriteTransactions, unsigned int& nFileRet, unsigned int& nBlockPosRet);

	// ���ļ��ж�ȡ����Ϣ
    bool ReadFromDisk(unsigned int nFile, unsigned int nBlockPos, bool fReadTransactions);

    void print() const
    {
        printf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, m_uTime=%u, m_uBits=%08x, m_uNonce=%u, m_vTrans=%d)\n",
            GetHash().ToString().substr(0,14).c_str(),
            m_nCurVersion,
            m_hashPrevBlock.ToString().substr(0,14).c_str(),
            m_hashMerkleRoot.ToString().substr(0,6).c_str(),
            m_uTime, m_uBits, m_uNonce,
            m_vTrans.size());
        for (int i = 0; i < m_vTrans.size(); i++)
        {
            printf("  ");
            m_vTrans[i].print();
        }
        printf("  m_vMerkleTree: ");
        for (int i = 0; i < m_vMerkleTree.size(); i++)
            printf("%s ", m_vMerkleTree[i].ToString().substr(0,6).c_str());
        printf("\n");
    }

	// ��ȡ��������Ӧ�ļ�ֵ������+���������ѣ�
    int64 GetBlockValue(int64 nFees) const;
	// ��һ������block�Ͽ����ӣ������ͷ������Ӧ����Ϣ��ͬʱ�ͷ������Ӧ������������
    bool DisconnectBlock(CBlockIndex* pindex);
	// �������ӣ�ÿһ���������ӣ����ӵ�������������
    bool ConnectBlock(CBlockIndex* pindex);
	// �����������������ݿ��ļ��ж�ȡ��Ӧ��������Ϣ
    bool ReadFromDisk(const CBlockIndex* blockindex, bool fReadTransactions);
	// ����ǰ�������ӵ���Ӧ������������
    bool AddToBlockIndex(unsigned int m_nFile, unsigned int m_nBlockPos);
	// ����У��
    bool CheckBlock() const;
	// �жϵ�ǰ�����ܹ�������
    bool AcceptBlock();
};




}

#endif /* CXX_BT_CBLOCK_H */
/* EOF */

