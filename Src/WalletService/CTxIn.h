/*
 * =====================================================================================
 *
 *       Filename:  CTxIn.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 11:12:25 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef EZ_BT_CWalletServiceIN_H
#define EZ_BT_CWalletServiceIN_H

#include "script.h"
#include "WalletService/CTxOutPoint.h"

namespace Enze
{

//
// An input of a transaction.  It contains the location of the previous
// transaction's output that it claims and a signature that matches the
// output's public key.
//
class CTxIn
{
public:
    COutPoint m_cPrevOut; // ǰһ�����׶�Ӧ���������һ�����׶�Ӧ��hashֵ�Ͷ�Ӧ�ĵڼ��������
    CScript m_cScriptSig; // ����ű���Ӧ��ǩ��
    unsigned int m_uSequence;// ��Ҫ�������ж���ͬ����Ľ�����һ�����£�ֵԽ��Խ��

    CTxIn()
    {
        m_uSequence = UINT_MAX;
    }

    explicit CTxIn(COutPoint preoutIn, CScript scriptSigIn=CScript(), unsigned int nSequenceIn=UINT_MAX)
    {
        m_cPrevOut = preoutIn;
        m_cScriptSig = scriptSigIn;
        m_uSequence = nSequenceIn;
    }

    CTxIn(uint256 hashPrevTx, unsigned int nOut, CScript scriptSigIn=CScript(), unsigned int nSequenceIn=UINT_MAX)
    {
        m_cPrevOut = COutPoint(hashPrevTx, nOut);
        m_cScriptSig = scriptSigIn;
        m_uSequence = nSequenceIn;
    }

    // ���׶�ӦnSequence������Ѿ��������ˣ������յ�
    bool IsFinal() const
    {
        return (m_uSequence == UINT_MAX);
    }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.m_cPrevOut   == b.m_cPrevOut &&
                a.m_cScriptSig == b.m_cScriptSig &&
                a.m_uSequence == b.m_uSequence);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    string ToString() const
    {
        string str;
        str += strprintf("CTxIn(");
        str += m_cPrevOut.ToString();
        if (m_cPrevOut.IsNull())
            str += strprintf(", coinbase %s", HexStr(m_cScriptSig.begin(), m_cScriptSig.end(), false).c_str());
        else
            str += strprintf(", scriptSig=%s", m_cScriptSig.ToString().substr(0,24).c_str());
        if (m_uSequence != UINT_MAX)
            str += strprintf(", nSequence=%u", m_uSequence);
        str += ")";
        return str;
    }

    void print() const
    {
        printf("%s\n", ToString().c_str());
    }

	// �жϵ�ǰ����Ľ����Ƿ����ڱ��ڵ㣬���Ƕ�Ӧ�Ľű�ǩ���Ƿ��ڱ����ܹ��ҵ�
    bool IsMine() const;
	// ��ö�Ӧ���׵Ľ跽�������Ӧ�������Ǳ��ڵ���˺ţ���跽�����ǽ���������
    int64 GetDebit() const;
};

} //end namespace 
#endif /* EZ_BT_CTXIN_H */
/* EOF */

