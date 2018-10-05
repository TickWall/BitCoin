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
#include "serialize.h"
#include "main.h"
#include "TX/CTransaction.h"
#include "CommonBase/uint256.h"
#include "Network/CMessageHeader.h"
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

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->m_nCurVersion);
        nVersion = this->m_nCurVersion;
        READWRITE(m_hashPrevBlock);
        READWRITE(m_hashMerkleRoot);
        READWRITE(m_uTime);
        READWRITE(m_uBits);
        READWRITE(m_uNonce);

        // ConnectBlock depends on m_vTrans being last so it can calculate offset
        if (!(nType & (SER_GETHASH|SER_BLOCKHEADERONLY)))
            READWRITE(m_vTrans);
        else if (fRead)
            const_cast<CBlock*>(this)->m_vTrans.clear();
    )

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
    uint256 BuildMerkleTree() const
    {
        m_vMerkleTree.clear();
        foreach(const CTransaction& tx, m_vTrans)
            m_vMerkleTree.push_back(tx.GetHash());
        int j = 0;
        for (int nSize = m_vTrans.size(); nSize > 1; nSize = (nSize + 1) / 2)
        {
            for (int i = 0; i < nSize; i += 2)
            {
                int i2 = min(i+1, nSize-1);
                m_vMerkleTree.push_back(Hash(BEGIN(m_vMerkleTree[j+i]),  END(m_vMerkleTree[j+i]),
                                           BEGIN(m_vMerkleTree[j+i2]), END(m_vMerkleTree[j+i2])));
            }
            j += nSize;
        }
        return (m_vMerkleTree.empty() ? 0 : m_vMerkleTree.back());
    }
	// ���ݽ��׶�Ӧ��������ý��׶�Ӧ��Ĭ�˶���֧
    vector<uint256> GetMerkleBranch(int nIndex) const
    {
        if (m_vMerkleTree.empty())
            BuildMerkleTree();
        vector<uint256> vMerkleBranch;
        int j = 0;
        for (int nSize = m_vTrans.size(); nSize > 1; nSize = (nSize + 1) / 2)
        {
            int i = min(nIndex^1, nSize-1);
            vMerkleBranch.push_back(m_vMerkleTree[j+i]);
            nIndex >>= 1;
            j += nSize;
        }
        return vMerkleBranch;
    }
	// �жϵ�ǰ��Ӧ�Ľ���hash��Ĭ�˶���֧����֤��Ӧ�Ľ����Ƿ��ڶ�Ӧ��blcok��
    static uint256 CheckMerkleBranch(uint256 hash, const vector<uint256>& vMerkleBranch, int nIndex)
    {
        if (nIndex == -1)
            return 0;
        foreach(const uint256& otherside, vMerkleBranch)
        {
            if (nIndex & 1)
                hash = Hash(BEGIN(otherside), END(otherside), BEGIN(hash), END(hash));
            else
                hash = Hash(BEGIN(hash), END(hash), BEGIN(otherside), END(otherside));
            nIndex >>= 1;
        }
        return hash;
    }

	// ��blockд�뵽�ļ���
    bool WriteToDisk(bool fWriteTransactions, unsigned int& nFileRet, unsigned int& nBlockPosRet)
    {
        // Open history file to append
        CAutoFile fileout = AppendBlockFile(nFileRet);
        if (!fileout)
            return error("CBlock::WriteToDisk() : AppendBlockFile failed");
        if (!fWriteTransactions)
            fileout.nType |= SER_BLOCKHEADERONLY;

        // Write index header
        unsigned int nSize = fileout.GetSerializeSize(*this);
		// ������Ϣͷ�Ͷ�Ӧ��Ĵ�Сֵ
        fileout << FLATDATA(pchMessageStart) << nSize;

        // Write block
        nBlockPosRet = ftell(fileout);
        if (nBlockPosRet == -1)
            return error("CBlock::WriteToDisk() : ftell failed");
		// ��block������д�뵽�ļ���
        fileout << *this;

        return true;
    }

	// ���ļ��ж�ȡ����Ϣ
    bool ReadFromDisk(unsigned int nFile, unsigned int nBlockPos, bool fReadTransactions)
    {
        SetNull();

        // Open history file to read
        CAutoFile filein = OpenBlockFile(nFile, nBlockPos, "rb");
        if (!filein)
            return error("CBlock::ReadFromDisk() : OpenBlockFile failed");
        if (!fReadTransactions)
            filein.nType |= SER_BLOCKHEADERONLY;

        // Read block ���ļ��е����ݶ�ȡ������
        filein >> *this;

        // Check the header 1. ������֤���ѶȱȽ� 2. �����hashֵҪС�ڶ�Ӧ�Ĺ������Ѷ�
        if (CBigNum().SetCompact(m_uBits) > bnProofOfWorkLimit)
            return error("CBlock::ReadFromDisk() : m_uBits errors in block header");
        if (GetHash() > CBigNum().SetCompact(m_uBits).getuint256())
            return error("CBlock::ReadFromDisk() : GetHash() errors in block header");

        return true;
    }



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
    bool DisconnectBlock(CTxDB& txdb, CBlockIndex* pindex);
	// �������ӣ�ÿһ���������ӣ����ӵ�������������
    bool ConnectBlock(CTxDB& txdb, CBlockIndex* pindex);
	// �����������������ݿ��ļ��ж�ȡ��Ӧ��������Ϣ
    bool ReadFromDisk(const CBlockIndex* blockindex, bool fReadTransactions);
	// ����ǰ�������ӵ���Ӧ������������
    bool AddToBlockIndex(unsigned int m_nFile, unsigned int m_nBlockPos);
	// ����У��
    bool CheckBlock() const;
	// �жϵ�ǰ�����ܹ�������
    bool AcceptBlock();
};





#endif /* CXX_BT_CBLOCK_H */
/* EOF */

