/*
 * =====================================================================================
 *
 *       Filename:  CTxOut.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 11:13:56 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef EZ_BT_TXOUT_H
#define EZ_BT_TXOUT_H
#include "CommonBase/script.h"
#include "CommonBase/CommDataDef.h"
#include "CommonBase/CommFunctionDef.h"

namespace Enze
{

//
// An output of a transaction.  It contains the public key that the next input
// must be able to sign with to claim it.
//
class CTxOut
{
public:
    int64 m_nValue; // ���������Ӧ�Ľ��
    CScript m_cScriptPubKey; // ���׶�Ӧ�Ĺ�Կ

public:
    CTxOut()
    {
        SetNull();
    }

    CTxOut(int64 nValueIn, CScript cScriptPubKeyIn)
    {
        m_nValue = nValueIn;
        m_cScriptPubKey = cScriptPubKeyIn;
    }


    void SetNull()
    {
        m_nValue = -1;
        m_cScriptPubKey.clear();
    }

    bool IsNull()
    {
        return (m_nValue == -1);
    }

    uint256 GetHash() const
    {
        return SerializeHash(*this);
    }

	// �жϽ��׵�����Ƿ��ǽڵ��Լ������Ӧ�Ľ��ף�Ҳ���ڵ�ǰ�и��ݹ�Կ�ܹ��ҵ���Ӧ��˽Կ
    bool IsMine() const
    {
        return ::IsMine(m_cScriptPubKey);
    }

	// ��ȡ��ǰ�������ݴ���������ǽڵ㱾��Ľ����򷵻ض�Ӧ����Ľ�����Խڵ���˵���Ӧ�Ĵ������Ϊ0
    int64 GetCredit() const
    {
        if (IsMine())
            return m_nValue;
        return 0;
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.m_nValue       == b.m_nValue &&
                a.m_cScriptPubKey == b.m_cScriptPubKey);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    string ToString() const
    {
        if (m_cScriptPubKey.size() < 6)
            return "CTxOut(error)";
        return strprintf("CTxOut(nValue=%I64d.%08I64d, m_cScriptPubKey=%s)", m_nValue / COIN, m_nValue % COIN, m_cScriptPubKey.ToString().substr(0,24).c_str());
    }

    void print() const
    {
        printf("%s\n", ToString().c_str());
    }
};


}//end namespace 

#endif /* EZ_BT_TXOUT_H */
/* EOF */

