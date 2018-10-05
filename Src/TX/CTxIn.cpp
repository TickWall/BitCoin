/*
 * =====================================================================================
 *
 *       Filename:  CTxIn.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 03:17:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "util.h"
#include "main.h"
#include "TX/CTxIn.h"
#include "TX/CWalletTx.h"
// �жϵ�ǰ�������Ƿ��Ӧ���ڵ�Ľ���
bool CTxIn::IsMine() const
{
//    CRITICAL_BLOCK(cs_mapWallet)
    {
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(m_cPrevOut.m_u256Hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (m_cPrevOut.m_nIndex < prev.m_vTxOut.size())
                if (prev.m_vTxOut[m_cPrevOut.m_nIndex].IsMine())
                    return true;
        }
    }
    return false;
}

// ��ȡ��ǰ�ڵ���ڴ˱ʽ��׶�Ӧ���������������Ӧ�Ĳ��ǵ�ǰ�ڵ����Ӧ�Ľ跽���Ϊ0
int64 CTxIn::GetDebit() const
{
    //CRITICAL_BLOCK(cs_mapWallet)
    {
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(m_cPrevOut.m_u256Hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (m_cPrevOut.m_nIndex < prev.m_vTxOut.size())
                if (prev.m_vTxOut[m_cPrevOut.m_nIndex].IsMine())
                    return prev.m_vTxOut[m_cPrevOut.m_nIndex].m_nValue;
        }
    }
    return 0;
}


/* EOF */
