/*
 * =====================================================================================
 *
 *       Filename:  CTransaction.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 10:57:41 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef CXX_BT_CTRANSACTION_H
#define CXX_BT_CTRANSACTION_H
#include "script.h"
#include "BlockEngine.h"
#include "TX/CTxIn.h"
#include "TX/CTxOut.h"
#include "Block/CDiskTxPos.h"

class CTxDB;
class CTxIndex;
//
// The basic transaction that is broadcasted on the network and contained in
// blocks.  A transaction can contain multiple inputs and outputs.
//
class CTransaction
{
public:
    int m_nCurVersion; // ���׵İ汾�ţ���������
    vector<CTxIn> m_vTxIn; // ���׶�Ӧ������
    vector<CTxOut> m_vTxOut; // ���׶�Ӧ�����
    int m_nLockTime; // ���׶�Ӧ������ʱ��


    CTransaction()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->m_nCurVersion);
        nVersion = this->m_nCurVersion;
        READWRITE(m_vTxIn);
        READWRITE(m_vTxOut);
        READWRITE(m_nLockTime);
    )

    void SetNull()
    {
        m_nCurVersion = 1;
        m_vTxIn.clear();
        m_vTxOut.clear();
        m_nLockTime = 0;
    }

    bool IsNull() const
    {
        return (m_vTxIn.empty() && m_vTxOut.empty());
    }

    uint256 GetHash() const
    {
        return SerializeHash(*this);
    }

    // �ж��Ƿ������յĽ���
    bool IsFinal() const
    {
        // �������ʱ�����0��������ʱ��С�������ĳ��ȣ���˵�������յĽ���
        const int& nBestHeight = BlockEngine::getInstance()->nBestHeight;
        if (m_nLockTime == 0 || m_nLockTime < nBestHeight)
            return true;
        foreach(const CTxIn& txin, m_vTxIn)
            if (!txin.IsFinal())
                return false;
        return true;
    }
	// �Ա���ͬ�Ľ�����һ�����µ㣺������ͬ����Ľ����ж���һ������
    bool IsNewerThan(const CTransaction& old) const
    {
        if (m_vTxIn.size() != old.m_vTxIn.size())
            return false;
        for (int i = 0; i < m_vTxIn.size(); i++)
            if (m_vTxIn[i].m_cPrevOut != old.m_vTxIn[i].m_cPrevOut)
                return false;

        bool fNewer = false;
        unsigned int nLowest = UINT_MAX;
        for (int i = 0; i < m_vTxIn.size(); i++)
        {
            if (m_vTxIn[i].m_uSequence != old.m_vTxIn[i].m_uSequence)
            {
                if (m_vTxIn[i].m_uSequence <= nLowest)
                {
                    fNewer = false;
                    nLowest = m_vTxIn[i].m_uSequence;
                }
                if (old.m_vTxIn[i].m_uSequence < nLowest)
                {
                    fNewer = true;
                    nLowest = old.m_vTxIn[i].m_uSequence;
                }
            }
        }
        return fNewer;
    }

	// �жϵ�ǰ�����Ƿ��Ǳһ����ף��һ����׵��ص��ǽ��������СΪ1�����Ƕ�Ӧ����������Ϊ��
    bool IsCoinBase() const
    {
        return (m_vTxIn.size() == 1 && m_vTxIn[0].m_cPrevOut.IsNull());
    }
	/* ����߽��׽��м�飺
	(1)���׶�Ӧ�������������б�����Ϊ��
	(2)���׶�Ӧ���������С��0
	(3)����Ǳһ����ף����Ӧ������ֻ��Ϊ1���Ҷ�Ӧ������ǩ����С���ܴ���100
	(4)����ǷǱһ����ף����Ӧ�Ľ���������������Ϊ��
	*/
    bool CheckTransaction() const
    {
        // Basic checks that don't depend on any context
        if (m_vTxIn.empty() || m_vTxOut.empty())
            return error("CTransaction::CheckTransaction() : m_vTxIn or m_vTxOut empty");

        // Check for negative values
        foreach(const CTxOut& txout, m_vTxOut)
            if (txout.m_nValue < 0)
                return error("CTransaction::CheckTransaction() : txout.nValue negative");

        if (IsCoinBase())
        {
            if (m_vTxIn[0].m_cScriptSig.size() < 2 || m_vTxIn[0].m_cScriptSig.size() > 100)
                return error("CTransaction::CheckTransaction() : coinbase script size");
        }
        else
        {
            foreach(const CTxIn& txin, m_vTxIn)
                if (txin.m_cPrevOut.IsNull())
                    return error("CTransaction::CheckTransaction() : preout is null");
        }

        return true;
    }

	// �жϵ�ǰ�Ľ����Ƿ�����ڵ㱾��Ľ��ף�������б��У�
    bool IsMine() const
    {
        foreach(const CTxOut& txout, m_vTxOut)
            if (txout.IsMine())
                return true;
        return false;
    }

	// ��õ�ǰ�����ܵ����룺�跽
    int64 GetDebit() const
    {
        int64 nDebit = 0;
        foreach(const CTxIn& txin, m_vTxIn)
            nDebit += txin.GetDebit();
        return nDebit;
    }

	// ��õ�ǰ�����ܵĴ��������ڽڵ������
    int64 GetCredit() const
    {
        int64 nCredit = 0;
        foreach(const CTxOut& txout, m_vTxOut)
            nCredit += txout.GetCredit();
        return nCredit;
    }
	// ��ȡ���׶�Ӧ����������֮��
    int64 GetValueOut() const
    {
        int64 nValueOut = 0;
        foreach(const CTxOut& txout, m_vTxOut)
        {
            if (txout.m_nValue < 0)
                throw runtime_error("CTransaction::GetValueOut() : negative value");
            nValueOut += txout.m_nValue;
        }
        return nValueOut;
    }
	// ��ȡ���׶�Ӧ����С���׷ѣ���СС��10000�ֽ����Ӧ����С���׷�Ϊ0������Ϊ���մ�С���м��㽻�׷�
	// Transaction fee requirements, mainly only needed for flood control
	// Under 10K (about 80 inputs) is free for first 100 transactions
	// Base rate is 0.01 per KB
    int64 GetMinFee(bool fDiscount=false) const
    {
        return 0;
        // Base fee is 1 cent per kilobyte
        unsigned int nBytes = ::GetSerializeSize(*this, SER_NETWORK);
        int64 nMinFee = (1 + (int64)nBytes / 1000) * CENT;

        // First 100 transactions in a block are free
        if (fDiscount && nBytes < 10000)
            nMinFee = 0;

        // To limit dust spam, require a 0.01 fee if any output is less than 0.01
        if (nMinFee < CENT)
            foreach(const CTxOut& txout, m_vTxOut)
                if (txout.m_nValue < CENT)
                    nMinFee = CENT;

        return nMinFee;
    }

	// ��Ӳ���н��ж�ȡ
    bool ReadFromDisk(const CDiskTxPos& pos, FILE** pfileRet=NULL)
    {
        CAutoFile filein = BlockEngine::getInstance()->OpenBlockFile(pos.m_nFile, 0, pfileRet ? "rb+" : "rb");
        if (!filein)
            return error("CTransaction::ReadFromDisk() : OpenBlockFile failed");

        // Read transaction
        if (fseek(filein, pos.m_nTxPos, SEEK_SET) != 0)
            return error("CTransaction::ReadFromDisk() : fseek failed");
        filein >> *this;

        // Return file pointer
        if (pfileRet)
        {
            if (fseek(filein, pos.m_nTxPos, SEEK_SET) != 0)
                return error("CTransaction::ReadFromDisk() : second fseek failed");
            *pfileRet = filein.release();
        }
        return true;
    }


    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return (a.m_nCurVersion  == b.m_nCurVersion &&
                a.m_vTxIn       == b.m_vTxIn &&
                a.m_vTxOut      == b.m_vTxOut &&
                a.m_nLockTime == b.m_nLockTime);
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return !(a == b);
    }


    string ToString() const
    {
        string str;
        str += strprintf("CTransaction(hash=%s, ver=%d, m_vTxIn.size=%d, m_vTxOut.size=%d, nLockTime=%d)\n",
            GetHash().ToString().substr(0,6).c_str(),
            m_nCurVersion,
            m_vTxIn.size(),
            m_vTxOut.size(),
            m_nLockTime);
        for (int i = 0; i < m_vTxIn.size(); i++)
            str += "    " + m_vTxIn[i].ToString() + "\n";
        for (int i = 0; i < m_vTxOut.size(); i++)
            str += "    " + m_vTxOut[i].ToString() + "\n";
        return str;
    }

    void print() const
    {
        printf("%s", ToString().c_str());
    }


	// �Ͽ����ӣ��ͷŽ��׶�Ӧ�����ռ�úͽ����״Ӷ�Ӧ�Ľ������������ͷŵ�
    bool DisconnectInputs(CTxDB& txdb);
	// �����������ӣ�����Ӧ�Ľ�������ռ�ö�Ӧ�Ľ�������Ļ��ѱ��
    bool ConnectInputs(CTxDB& txdb, map<uint256, CTxIndex>& mapTestPool, const CDiskTxPos& posThisTx, int m_nCurHeight, int64& nFees, bool fBlock, bool fMiner, int64 nMinFee=0);
	// �ͻ����������룬�Խ��ױ��������֤
	bool ClientConnectInputs();
	// �ж���ʽ����Ƿ�Ӧ�ñ�����
    bool AcceptTransaction(CTxDB& txdb, bool fCheckInputs=true, bool* pfMissingInputs=NULL);

    bool AcceptTransaction(bool fCheckInputs=true, bool* pfMissingInputs=NULL);

protected:
	// ����ǰ�������ӵ��ڴ��mapTransactions,mapNextTx�У����Ҹ��½��׸��µĴ���
    bool AddToMemoryPool();
public:
	// ����ǰ���״��ڴ����mapTransactions��mapNextTx���Ƴ����������ӽ��׶�Ӧ�ĸ��´���
    bool RemoveFromMemoryPool();
};



#endif /* CXX_BT_CTRANSACTION_H */
/* EOF */

