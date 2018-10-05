/*
 * =====================================================================================
 *
 *       Filename:  Net_DataDef.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 08:27:04 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef CXX_BT_NET_DATA_DEF_H
#define CXX_BT_NET_DATA_DEF_H

#include <arpa/inet.h>
#include "serialize.h"
#include "util.h"

typedef int SOCKET;
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;
// Ĭ�϶˿ں�
const unsigned short DEFAULT_PORT = htons(8333);
const unsigned int PUBLISH_HOPS = 5;

const unsigned char pchIPv4[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };

extern bool fClient;
extern uint64 nLocalServices;
extern bool fShutdown;
extern boost::array<bool, 10> vfThreadRunning;

enum
{
    NODE_NETWORK = (1 << 0),
};
// ��Ϣ����
enum
{
    MSG_TX = 1, // ������Ϣ
    MSG_BLOCK, // ����Ϣ
    MSG_REVIEW, //
    MSG_PRODUCT, // ��Ʒ��Ϣ
    MSG_TABLE,// ��
};

static const char* ppszTypeName[] =
{
    "ERROR",
    "tx",
    "block",
    "review",
    "product",
    "table",
};



#endif /* CXX_BT_NET_DATA_DEF_H */
/* EOF */

