/*
 * =====================================================================================
 *
 *       Filename:  CWalletTx.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 05:36:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef EZ_BT_CWALLETWalletService_H
#define EZ_BT_CWALLETWalletService_H
#include "WalletService/CMerkleTx.h"
//
// A transaction with a bunch of additional info that only the owner cares
// about.  It includes any unrecorded transactions needed to link it back
// to the block chain.
//
namespace Enze
{

class CWalletTx : public CMerkleTx
{
public:
    vector<CMerkleTx> m_vPrevTx; // ��ǰ����A��Ӧ�������Ӧ�Ľ���B�����B����block�����ĩβ�ĳ���С��3���򽫴ν��׷���
    /*
	��Ҫ���ڴ��һ���Զ����ֵ
	wtx.mapValue["to"] = strAddress;
	wtx.mapValue["from"] = m_textCtrlFrom->GetValue();
	wtx.mapValue["message"] = m_textCtrlMessage->GetValue();
	*/
	map<string, string> m_mapValue;
	// ���ؼ���Ϣ
    vector<pair<string, string> > m_vOrderForm;
    //unsigned int m_bTimeReceivedIsTxTime;// ����ʱ���Ƿ��ǽ���ʱ����
    bool m_bTimeReceivedIsTxTime;// ����ʱ���Ƿ��ǽ���ʱ����
    unsigned int m_uTimeReceived;  // time received by this node ���ױ�����ڵ���յ�ʱ��
    /*  
    char m_bFromMe;
    char m_bSpent; // �Ƿ񻨷ѽ��ױ��
    */
    bool m_bFromMe;
    char m_bSpent; // �Ƿ񻨷ѽ��ױ��
    //// probably need to sign the order info so know it came from payer

    // memory only
    mutable unsigned int m_uTimeDisplayed;


    CWalletTx()
    {
        Init();
    }

    CWalletTx(const CMerkleTx& txIn) : CMerkleTx(txIn)
    {
        Init();
    }

    CWalletTx(const CTransaction& txIn) : CMerkleTx(txIn)
    {
        Init();
    }

    void Init()
    {
        m_bTimeReceivedIsTxTime = false;
        m_uTimeReceived = 0;
        m_bFromMe = false;
        m_bSpent = false;
        m_uTimeDisplayed = 0;
    }

    bool WriteToDisk();

	// ��ȡ����ʱ��
    int64 GetTxTime() const;
	// ����֧�ֵĽ���
    void AddSupportingTransactions();
	// �жϵ�ǰ�����ܹ�������
    bool AcceptWalletTransaction(bool fCheckInputs=true);
	// ת��Ǯ������
    void RelayWalletTransaction() ;
};



} //end namespace


#endif /* EZ_BT_CWALLETTX_H */
/* EOF */

