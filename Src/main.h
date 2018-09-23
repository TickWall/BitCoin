// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef CXX_BT_MAIN_H
#define CXX_BT_MAIN_H
#include "headers.h"
#include "bignum.h"
#include "uint256.h"
#include "script.h"
#include "net.h"
class COutPoint;
class CInPoint;
class CDiskTxPos;
class CCoinBase;
class CTxIn;
class CTxOut;
class CTransaction;
class CBlock;
class CBlockIndex;
class CWalletTx;
class CKeyItem;
class CNode;
static const unsigned int MAX_SIZE = 0x02000000;
// COIN ��ʾ����һ�����رң�����100000000���Ǳ�ʾһ�����رң����ر���С��λΪС�����8λ
static const int64 COIN = 100000000;
static const int64 CENT = 1000000;
static const int COINBASE_MATURITY = 100;// �һ������
// ������֤�����Ѷ�
static const CBigNum bnProofOfWorkLimit(~uint256(0) >> 32);






//extern CCriticalSection cs_main;
extern map<uint256, CBlockIndex*> mapBlockIndex;
extern const uint256 hashGenesisBlock;
extern CBlockIndex* pindexGenesisBlock;
extern int nBestHeight;
extern uint256 hashBestChain;
extern CBlockIndex* pindexBest;
extern unsigned int nTransactionsUpdated;
extern string strSetDataDir;
extern int nDropMessagesTest;

// Settings
extern int fGenerateBitcoins;
extern int64 nTransactionFee;
extern CAddress addrIncoming;







string GetAppDir();
bool CheckDiskSpace(int64 nAdditionalBytes=0);
FILE* OpenBlockFile(unsigned int m_nFile, unsigned int m_nBlockPos, const char* pszMode="rb");
FILE* AppendBlockFile(unsigned int& m_nFileRet);
bool AddKey(const CKey& key);
vector<unsigned char> GenerateNewKey();
bool AddToWallet(const CWalletTx& wtxIn);
void ReacceptWalletTransactions();
void RelayWalletTransactions();
bool LoadBlockIndex(bool fAllowNew=true);
void PrintBlockTree();
bool BitcoinMiner();
bool ProcessMessages(CNode* pfrom);
bool ProcessMessage(CNode* pfrom, string strCommand, CDataStream& vRecv);
bool SendMessages(CNode* pto);
int64 GetBalance();
bool CreateTransaction(CScript m_cScriptPubKey, int64 nValue, CWalletTx& txNew, int64& nFeeRequiredRet);
bool CommitTransactionSpent(const CWalletTx& wtxNew);
bool SendMoney(CScript m_cScriptPubKey, int64 nValue, CWalletTx& wtxNew);











class CDiskTxPos
{
public:
    unsigned int m_nFile; // �������ļ�����Ϣ�����ҿ��ļ�������һ����blk${nFile}.dat
    unsigned int m_nBlockPos; // ��ǰ���ڶ�Ӧ���ļ��е�ƫ��
    unsigned int m_nTxPos; // �����ڶ�Ӧ���е�ƫ��

    CDiskTxPos()
    {
        SetNull();
    }

    CDiskTxPos(unsigned int nFileIn, unsigned int nBlockPosIn, unsigned int nTxPosIn)
    {
        m_nFile = nFileIn;
        m_nBlockPos = nBlockPosIn;
        m_nTxPos = nTxPosIn;
    }

    IMPLEMENT_SERIALIZE( READWRITE(FLATDATA(*this)); )
    void SetNull() { m_nFile = -1; m_nBlockPos = 0; m_nTxPos = 0; }
    bool IsNull() const { return (m_nFile == -1); }

    friend bool operator==(const CDiskTxPos& a, const CDiskTxPos& b)
    {
        return (a.m_nFile     == b.m_nFile &&
                a.m_nBlockPos == b.m_nBlockPos &&
                a.m_nTxPos    == b.m_nTxPos);
    }

    friend bool operator!=(const CDiskTxPos& a, const CDiskTxPos& b)
    {
        return !(a == b);
    }

    string ToString() const
    {
        if (IsNull())
            return strprintf("null");
        else
            return strprintf("(m_nFile=%d, m_nBlockPos=%d, m_nTxPos=%d)", m_nFile, m_nBlockPos, m_nTxPos);
    }

    void print() const
    {
        printf("%s", ToString().c_str());
    }
};




class CInPoint
{
public:
    CTransaction* m_pcTrans; // ����ָ��
    unsigned int m_nIndex; // ��Ӧ���׵�ǰ�ĵڼ�������

    CInPoint() { SetNull(); }
    CInPoint(CTransaction* ptxIn, unsigned int nIn) { m_pcTrans = ptxIn; m_nIndex = nIn; }
    void SetNull() { m_pcTrans = NULL; m_nIndex = -1; }
    bool IsNull() const { return (m_pcTrans == NULL && m_nIndex == -1); }
};




class COutPoint
{
public:
    uint256 m_u256Hash; // ���׶�Ӧ��hash
    unsigned int m_nIndex; // ���׶�Ӧ�ĵڼ������

    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, unsigned int nIn) { m_u256Hash = hashIn; m_nIndex = nIn; }
    IMPLEMENT_SERIALIZE( READWRITE(FLATDATA(*this)); )
    void SetNull() { m_u256Hash = 0; m_nIndex = -1; }
    bool IsNull() const { return (m_u256Hash == 0 && m_nIndex == -1); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        return (a.m_u256Hash < b.m_u256Hash || (a.m_u256Hash == b.m_u256Hash && a.m_nIndex < b.m_nIndex));
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.m_u256Hash == b.m_u256Hash && a.m_nIndex == b.m_nIndex);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    string ToString() const
    {
        return strprintf("COutPoint(%s, %d)", m_u256Hash.ToString().substr(0,6).c_str(), m_nIndex);
    }

    void print() const
    {
        printf("%s\n", ToString().c_str());
    }
};




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

    IMPLEMENT_SERIALIZE
    (
        READWRITE(m_cPrevOut);
        READWRITE(m_cScriptSig);
        READWRITE(m_uSequence);
    )
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

    IMPLEMENT_SERIALIZE
    (
        READWRITE(m_nValue);
        READWRITE(m_cScriptPubKey);
    )

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
    bool ReadFromDisk(CDiskTxPos pos, FILE** pfileRet=NULL)
    {
        CAutoFile filein = OpenBlockFile(pos.m_nFile, 0, pfileRet ? "rb+" : "rb");
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
    bool ConnectInputs(CTxDB& txdb, map<uint256, CTxIndex>& mapTestPool, CDiskTxPos posThisTx, int m_nCurHeight, int64& nFees, bool fBlock, bool fMiner, int64 nMinFee=0);
	// �ͻ����������룬�Խ��ױ��������֤
	bool ClientConnectInputs();
	// �ж���ʽ����Ƿ�Ӧ�ñ�����
    bool AcceptTransaction(CTxDB& txdb, bool fCheckInputs=true, bool* pfMissingInputs=NULL);

    bool AcceptTransaction(bool fCheckInputs=true, bool* pfMissingInputs=NULL)
    {
        CTxDB txdb("r");
        return AcceptTransaction(txdb, fCheckInputs, pfMissingInputs);
    }

protected:
	// ����ǰ�������ӵ��ڴ��mapTransactions,mapNextTx�У����Ҹ��½��׸��µĴ���
    bool AddToMemoryPool();
public:
	// ����ǰ���״��ڴ����mapTransactions��mapNextTx���Ƴ����������ӽ��׶�Ӧ�ĸ��´���
    bool RemoveFromMemoryPool();
};





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
        // Must wait until coinbase is safely deep enough in the chain before valuing it
        if (IsCoinBase() && GetBlocksToMaturity() > 0)
            return 0;
        return CTransaction::GetCredit();
    }

    IMPLEMENT_SERIALIZE
    (
        nSerSize += SerReadWrite(s, *(CTransaction*)this, nType, m_nCurVersion, ser_action);
        nVersion = this->m_nCurVersion;
        READWRITE(m_hashBlock);
        READWRITE(m_vMerkleBranch);
        READWRITE(m_nIndex);
    )

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
    bool AcceptTransaction(CTxDB& txdb, bool fCheckInputs=true);
    bool AcceptTransaction() { CTxDB txdb("r"); return AcceptTransaction(txdb); }
};




//
// A transaction with a bunch of additional info that only the owner cares
// about.  It includes any unrecorded transactions needed to link it back
// to the block chain.
//
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
    unsigned int m_m_uTimeReceived;  // time received by this node ���ױ�����ڵ���յ�ʱ��
    /*  
    char m_bFromMe;
    char m_bSpent; // �Ƿ񻨷ѽ��ױ��
    */
    bool m_bFromMe;
    char m_bSpent; // �Ƿ񻨷ѽ��ױ��
    //// probably need to sign the order info so know it came from payer

    // memory only
    mutable unsigned int m_m_uTimeDisplayed;


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
        m_m_uTimeReceived = 0;
        m_bFromMe = false;
        m_bSpent = false;
        m_m_uTimeDisplayed = 0;
    }

    IMPLEMENT_SERIALIZE
    (
        nSerSize += SerReadWrite(s, *(CMerkleTx*)this, nType, m_nCurVersion, ser_action);
        nVersion = this->m_nCurVersion;
        READWRITE(m_vPrevTx);
        READWRITE(m_mapValue);
        READWRITE(m_vOrderForm);
        READWRITE(m_bTimeReceivedIsTxTime);
        READWRITE(m_m_uTimeReceived);
        READWRITE(m_bFromMe);
        READWRITE(m_bSpent);
    )

    bool WriteToDisk()
    {
        return CWalletDB().WriteTx(GetHash(), *this);
    }

	// ��ȡ����ʱ��
    int64 GetTxTime() const;
	// ����֧�ֵĽ���
    void AddSupportingTransactions(CTxDB& txdb);
	// �жϵ�ǰ�����ܹ�������
    bool AcceptWalletTransaction(CTxDB& txdb, bool fCheckInputs=true);
    bool AcceptWalletTransaction() { CTxDB txdb("r"); return AcceptWalletTransaction(txdb); }
	// ת��Ǯ������
    void RelayWalletTransaction(CTxDB& txdb);
    void RelayWalletTransaction() { CTxDB txdb("r"); RelayWalletTransaction(txdb); }
};




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






//
// The block chain is a tree shaped structure starting with the
// genesis block at the root, with each block potentially having multiple
// candidates to be the next block.  m_pPrevBlkIndex and m_pNextBlkIndex link a path through the
// main/longest chain.  A blockindex may have multiple m_pPrevBlkIndex pointing back
// to it, but m_pNextBlkIndex will only point forward to the longest branch, or will
// be null if the block is not part of the longest chain.
//
// �����������Ӧ��pNext��Ϊ�գ������������һ����Ӧ��������
// ������
class CBlockIndex
{
public:
    const uint256* m_pHashBlock; // ��Ӧ��hashֵָ��
    CBlockIndex* m_pPrevBlkIndex; // ָ��ǰһ��blockIndex
    CBlockIndex* m_pNextBlkIndex; // ָ��ǰ������������һ����ֻ�е�ǰ���������������ϵ�ʱ�����ֵ���Ƿǿ�
	// �������ļ��е���Ϣ
    unsigned int m_nFile; 
    unsigned int m_nBlockPos;
    int m_nCurHeight; // ���������������ȣ������м���˶��ٸ�block�����ǴӴ������鵽��ǰ�����м���˶��ٸ�����

    // block header ���ͷ����Ϣ
    int m_nCurVersion;
    uint256 m_hashMerkleRoot;
	// ȡǰ11�������Ӧ�Ĵ���ʱ��ƽ��ֵ
    unsigned int m_uTime;// �鴴��ʱ�䣬ȡ������ʱ����λ��
    unsigned int m_uBits;
    unsigned int m_uNonce;


    CBlockIndex()
    {
        m_pHashBlock = NULL;
        m_pPrevBlkIndex = NULL;
        m_pNextBlkIndex = NULL;
        m_nFile = 0;
        m_nBlockPos = 0;
        m_nCurHeight = 0;

        m_nCurVersion       = 0;
        m_hashMerkleRoot = 0;
        m_uTime          = 0;
        m_uBits          = 0;
        m_uNonce         = 0;
    }

    CBlockIndex(unsigned int nFileIn, unsigned int nBlockPosIn, CBlock& block)
    {
        m_pHashBlock = NULL;
        m_pPrevBlkIndex = NULL;
        m_pNextBlkIndex = NULL;
        m_nFile = nFileIn;
        m_nBlockPos = nBlockPosIn;
        m_nCurHeight = 0;

        m_nCurVersion       = block.m_nCurVersion;
        m_hashMerkleRoot = block.m_hashMerkleRoot;
        m_uTime          = block.m_uTime;
        m_uBits          = block.m_uBits;
        m_uNonce         = block.m_uNonce;
    }

    uint256 GetBlockHash() const
    {
        return *m_pHashBlock;
    }

	// �ж��Ƿ���������ͨ��m_pNextBlkIndex�Ƿ�Ϊ�պ͵�ǰ������ָ���Ƿ�������ָ��
    bool IsInMainChain() const
    {
        return (m_pNextBlkIndex || this == pindexBest);
    }

	// ���ļ����Ƴ���Ӧ�Ŀ�
    bool EraseBlockFromDisk()
    {
        // Open history file
        CAutoFile fileout = OpenBlockFile(m_nFile, m_nBlockPos, "rb+");
        if (!fileout)
            return false;

		// ���ļ���Ӧ��λ������дһ���տ飬�������൱�ڴ��ļ���ɾ����Ӧ�ĵ��ڿ�
        // Overwrite with empty null block
        CBlock block;
        block.SetNull();
        fileout << block;

        return true;
    }

	// ȡǰ11�������Ӧ�Ĵ���ʱ��ƽ��ֵ
    enum { nMedianTimeSpan=11 };

	// �ӵ�ǰ����ǰ�ƣ���ȡ��ȥ��Ӧ����λ��ʱ�䣬�ڶ�Ӧ���������л�ȡÿһ�������Ӧ��ʱ�䣬Ȼ��ȡ��λ��
    int64 GetMedianTimePast() const
    {
        unsigned int pmedian[nMedianTimeSpan];
        unsigned int* pbegin = &pmedian[nMedianTimeSpan];
        unsigned int* pend = &pmedian[nMedianTimeSpan];

        const CBlockIndex* pindex = this;
        for (int i = 0; i < nMedianTimeSpan && pindex; i++, pindex = pindex->m_pPrevBlkIndex)
            *(--pbegin) = pindex->m_uTime;

        sort(pbegin, pend);
        return pbegin[(pend - pbegin)/2];
    }
	// �ӵ�ǰ�������ƣ�ȡ��λ��ʱ��
    int64 GetMedianTime() const
    {
        const CBlockIndex* pindex = this;
        for (int i = 0; i < nMedianTimeSpan/2; i++)
        {
            if (!pindex->m_pNextBlkIndex)
                return m_uTime;
            pindex = pindex->m_pNextBlkIndex;
        }
        return pindex->GetMedianTimePast();
    }



    string ToString() const
    {
        return strprintf("CBlockIndex(nprev=%08x, m_pNextBlkIndex=%08x, m_nFile=%d, m_nBlockPos=%-6d m_nCurHeight=%d, merkle=%s, hashBlock=%s)",
            m_pPrevBlkIndex, m_pNextBlkIndex, m_nFile, m_nBlockPos, m_nCurHeight,
            m_hashMerkleRoot.ToString().substr(0,6).c_str(),
            GetBlockHash().ToString().substr(0,14).c_str());
    }

    void print() const
    {
        printf("%s\n", ToString().c_str());
    }
};



//
// Used to marshal pointers into hashes for db storage.
// ���ڽ�ָ���滻��hashֵ�������ݿ�洢
//
class CDiskBlockIndex : public CBlockIndex
{
public:
    uint256 m_PrevHash; // block��Ӧ��hashֵ����ָ���ɶ�Ӧ��hash
    uint256 m_NextHash;

    CDiskBlockIndex()
    {
        m_PrevHash = 0;
        m_NextHash = 0;
    }

    explicit CDiskBlockIndex(CBlockIndex* pindex) : CBlockIndex(*pindex)
    {
        m_PrevHash = (m_pPrevBlkIndex ? m_pPrevBlkIndex->GetBlockHash() : 0);
        m_NextHash = (m_pNextBlkIndex ? m_pNextBlkIndex->GetBlockHash() : 0);
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(m_nCurVersion);

        READWRITE(m_NextHash);
        READWRITE(m_nFile);
        READWRITE(m_nBlockPos);
        READWRITE(m_nCurHeight);

        // block header
        READWRITE(this->m_nCurVersion);
        READWRITE(m_PrevHash);
    char fFromMe;
    char fSpent; // �Ƿ񻨷ѽ��ױ��
        READWRITE(m_hashMerkleRoot);
        READWRITE(m_uTime);
        READWRITE(m_uBits);
        READWRITE(m_uNonce);
    )

    uint256 GetBlockHash() const
    {
        CBlock block;
        block.m_nCurVersion        = m_nCurVersion;
        block.m_hashPrevBlock   = m_PrevHash;
        block.m_hashMerkleRoot  = m_hashMerkleRoot;
        block.m_uTime           = m_uTime;
        block.m_uBits           = m_uBits;
        block.m_uNonce          = m_uNonce;
        return block.GetHash(); // ���hash��������������Щ����
    }


    string ToString() const
    {
        string str = "CDiskBlockIndex(";
        str += CBlockIndex::ToString();
        str += strprintf("\n                hashBlock=%s, hashPrev=%s, hashNext=%s)",
            GetBlockHash().ToString().c_str(),
            m_PrevHash.ToString().substr(0,14).c_str(),
            m_NextHash.ToString().substr(0,14).c_str());
        return str;
    }

    void print() const
    {
        printf("%s\n", ToString().c_str());
    }
};








//
// Describes a place in the block chain to another node such that if the
// other node doesn't have the same branch, it can find a recent common trunk.
// The further back it is, the further before the fork it may be.
//
class CBlockLocator
{
protected:
    vector<uint256> vHave; // ��������Ӧ��block����
public:

    CBlockLocator()
    {
    }

    explicit CBlockLocator(const CBlockIndex* pindex)
    {
        Set(pindex);
    }

    explicit CBlockLocator(uint256 hashBlock)
    {
        map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end())
            Set((*mi).second);
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    )

    void Set(const CBlockIndex* pindex)
    {
        vHave.clear();
        int nStep = 1;
        while (pindex)
        {
            vHave.push_back(pindex->GetBlockHash());

			// ָ�����ٻ����㷨��ǰ10�����棬������ָ������һֱ��������ͷ��Ϊֹ
            // Exponentially larger steps back
            for (int i = 0; pindex && i < nStep; i++)
                pindex = pindex->m_pPrevBlkIndex;
            if (vHave.size() > 10)
                nStep *= 2;
        }
        vHave.push_back(hashGenesisBlock); // Ĭ�Ϸ���һ����������
    }
	// �ҵ������е����������ϵĿ������
    CBlockIndex* GetBlockIndex()
    {
        // Find the first block the caller has in the main chain
        foreach(const uint256& hash, vHave)
        {
			// �ҵ������е����������ϵ�
            map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
            if (mi != mapBlockIndex.end())
            {
                CBlockIndex* pindex = (*mi).second;
                if (pindex->IsInMainChain())
                    return pindex;
            }
        }
        return pindexGenesisBlock;
    }

    uint256 GetBlockHash()
    {
        // Find the first block the caller has in the main chain
        foreach(const uint256& hash, vHave)
        {
            map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
            if (mi != mapBlockIndex.end())
            {
                CBlockIndex* pindex = (*mi).second;
                if (pindex->IsInMainChain())
                    return hash;
            }
        }
        return hashGenesisBlock;
    }

    int GetHeight()
    {
        CBlockIndex* pindex = GetBlockIndex();
        if (!pindex)
            return 0;
        return pindex->m_nCurHeight;
    }
};












extern map<uint256, CTransaction> mapTransactions;
extern map<uint256, CWalletTx> mapWallet;
extern vector<pair<uint256, bool> > vWalletUpdated;
//extern CCriticalSection cs_mapWallet;
extern map<vector<unsigned char>, CPrivKey> mapKeys;
extern map<uint160, vector<unsigned char> > mapPubKeys;
//extern CCriticalSection cs_mapKeys;
extern CKey keyUser;
#endif 
/* EOF */
