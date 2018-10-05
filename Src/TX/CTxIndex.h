/*
 * =====================================================================================
 *
 *       Filename:  CTxIndex.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 05:33:35 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef CXX_BT_CTXINDEX_H
#define CXX_BT_CTXINDEX_H
//
// A txdb record that contains the disk location of a transaction and the
// locations of transactions that spend its outputs.  vSpent is really only
// used as a flag, but ham_vTxIng the location is very helpful for debugging.
//
// ��������---ÿһ�����׶�Ӧһ������
class CTxIndex
{
public:
    CDiskTxPos m_cDiskPos; // ���׶�Ӧ����Ӳ�����ļ���λ��
    vector<CDiskTxPos> m_vSpent; // ��ǽ��׵�����Ƿ��Ѿ��������ˣ������±�����Ƕ�Ӧ����ָ��λ�õ�����Ƿ��Ѿ���������

    CTxIndex()
    {
        SetNull();
    }

    CTxIndex(const CDiskTxPos& posIn, unsigned int nOutputs)
    {
        m_cDiskPos = posIn;
        m_vSpent.resize(nOutputs);
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(m_cDiskPos);
        READWRITE(m_vSpent);
    )

    void SetNull()
    {
        m_cDiskPos.SetNull();
        m_vSpent.clear();
    }

    bool IsNull()
    {
        return m_cDiskPos.IsNull();
    }

    friend bool operator==(const CTxIndex& a, const CTxIndex& b)
    {
        if (a.m_cDiskPos != b.m_cDiskPos || a.m_vSpent.size() != b.m_vSpent.size())
            return false;
        for (int i = 0; i < a.m_vSpent.size(); i++)
            if (a.m_vSpent[i] != b.m_vSpent[i])
                return false;
        return true;
    }

    friend bool operator!=(const CTxIndex& a, const CTxIndex& b)
    {
        return !(a == b);
    }
};





#endif /* CXX_BT_CTXINDEX_H */
/* EOF */

