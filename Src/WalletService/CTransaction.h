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
#ifndef EZ_BT_CTRANSACTION_H
#define EZ_BT_CTRANSACTION_H
#include "script.h"
#include "BlockEngine/CDiskTxPos.h"
#include "WalletService/CTxIn.h"
#include "WalletService/CTxOut.h"

namespace Enze
{

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


    void SetNull();

    bool IsNull() const;

    uint256 GetHash() const;

    // �ж��Ƿ������յĽ���
    bool IsFinal() const;
	// �Ա���ͬ�Ľ�����һ�����µ㣺������ͬ����Ľ����ж���һ������
    bool IsNewerThan(const CTransaction& old) const;

	// �жϵ�ǰ�����Ƿ��Ǳһ����ף��һ����׵��ص��ǽ��������СΪ1�����Ƕ�Ӧ����������Ϊ��
    bool IsCoinBase() const;
	/* ����߽��׽��м�飺
	(1)���׶�Ӧ�������������б�����Ϊ��
	(2)���׶�Ӧ���������С��0
	(3)����Ǳһ����ף����Ӧ������ֻ��Ϊ1���Ҷ�Ӧ������ǩ����С���ܴ���100
	(4)����ǷǱһ����ף����Ӧ�Ľ���������������Ϊ��
	*/
    bool CheckTransaction() const;
	// �жϵ�ǰ�Ľ����Ƿ�����ڵ㱾��Ľ��ף�������б��У�
    bool IsMine() const;

	// ��õ�ǰ�����ܵ����룺�跽
    int64 GetDebit() const;

	// ��õ�ǰ�����ܵĴ��������ڽڵ������
    int64 GetCredit() const;
	// ��ȡ���׶�Ӧ����������֮��
    int64 GetValueOut() const;
	// ��ȡ���׶�Ӧ����С���׷ѣ���СС��10000�ֽ����Ӧ����С���׷�Ϊ0������Ϊ���մ�С���м��㽻�׷�
	// Transaction fee requirements, mainly only needed for flood control
	// Under 10K (about 80 inputs) is free for first 100 transactions
	// Base rate is 0.01 per KB
    int64 GetMinFee(bool fDiscount=false) const;

	// ��Ӳ���н��ж�ȡ
    bool ReadFromDisk(const CDiskTxPos& pos, FILE** pfileRet=NULL);


    friend bool operator==(const CTransaction& a, const CTransaction& b);

    friend bool operator!=(const CTransaction& a, const CTransaction& b);

    string ToString() const;

    void print() const;

	// �Ͽ����ӣ��ͷŽ��׶�Ӧ�����ռ�úͽ����״Ӷ�Ӧ�Ľ������������ͷŵ�
    bool DisconnectInputs();
	// �����������ӣ�����Ӧ�Ľ�������ռ�ö�Ӧ�Ľ�������Ļ��ѱ��
    bool ConnectInputs(map<uint256, CTxIndex>& mapTestPool, const CDiskTxPos& posThisTx, int m_nCurHeight, int64& nFees, bool fBlock, bool fMiner, int64 nMinFee=0);
	// �ͻ����������룬�Խ��ױ��������֤
	bool ClientConnectInputs();
	// �ж���ʽ����Ƿ�Ӧ�ñ�����

    bool AcceptTransaction(bool fCheckInputs=true, bool* pfMissingInputs=NULL);

protected:
	// ����ǰ�������ӵ��ڴ��mapTransactions,mapNextTx�У����Ҹ��½��׸��µĴ���
    bool AddToMemoryPool();
public:
	// ����ǰ���״��ڴ����mapTransactions��mapNextTx���Ƴ����������ӽ��׶�Ӧ�ĸ��´���
    bool RemoveFromMemoryPool();
};


} // end namespace

#endif /* EZ_BT_CTRANSACTION_H */
/* EOF */

