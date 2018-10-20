/*
 * =====================================================================================
 *
 *       Filename:  CMerkleTx.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 03:38:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef EZ_BT_CMERKLEWalletService_H
#define EZ_BT_CMERKLEWalletService_H

#include "WalletService/CTransaction.h"

namespace Enze
{

class CBlock;
class CTxDB;
//
// A transaction with a merkle branch linking it to the block chain
//
class CMerkleTx : public CTransaction
{
public:
    uint256 m_hashBlock;// ��������block��Ӧ��hashֵ����Ϊblock���ж�Ӧ�������׵�Ĭ�˶������������ܸ��ݷ�֧��У�鵱ǰ�����Ƿ���block��
    vector<uint256> m_vMerkleBranch; // ��ǰ���׶�Ӧ��Ĭ�˶���֧
    int m_nIndex;// ��ǰ�����ڶ�Ӧ��block��Ӧ������m_vTrans�б��е�������CMerkleTx���Ǹ�������������������׶�Ӧ��Ĭ�˶�����֧��

    // memory only
    mutable bool m_bMerkleVerified;// ���Ĭ�˶������Ƿ��Ѿ�У�飬���û��У�������У�飬У��֮�����ֵ��Ϊtrue


    CMerkleTx()
    {
        Init();
    }

    CMerkleTx(const CTransaction& txIn) : CTransaction(txIn)
    {
        Init();
    }

    void Init()
    {
        m_hashBlock = 0;
        m_nIndex = -1;
        m_bMerkleVerified = false;
    }

	// ��ȡĬ�˶�����Ӧ�Ĵ�������ʱ�򣬶��ڱһ����ף�һ��Ҫ�ȶ�Ӧ��block�㹻�����˲���ʹ��
    int64 GetCredit() const
    {
//        // Must wait until coinbase is safely deep enough in the chain before valuing it
//        if (IsCoinBase() && GetBlocksToMaturity() > 0)
//            return 0;
        return CTransaction::GetCredit();
    }

    // ��������ڶ�Ӧ�������У������ý��׶�Ӧ��Ĭ�˶�����֧
    int SetMerkleBranch(const CBlock* pblock=NULL);
	//��ȡĬ�˶������������е����--��ǰ��������ĩβ�м���˶��ٸ�block
    int GetDepthInMainChain() const;
	// �жϵ�ǰ�����Ƿ���������
    bool IsInMainChain() const { return GetDepthInMainChain() > 0; }
	// �ж϶�Ӧ�Ŀ��Ƿ���죬���Ǳ��������������Ͽɣ�����ǷǱһ����׶�Ӧ��Ϊ������Ϊ0������Ҫ���м���
    // �����ԽСԽ�ã�˵����ǰ���ױ��ϿɵĶ�Խ��
    int GetBlocksToMaturity() const;
	// �ж���߽����ܲ��ܱ����ܣ�����ܽ��ܽ���Ӧ�Ľ��׷���ȫ�ֱ�����mapTransactions��mapNextTx��
    bool AcceptTransaction(bool fCheckInputs=true);
};



} //end namespace 
#endif /* EZ_BT_CMERKLETX_H */
/* EOF */

