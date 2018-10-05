/*
 * =====================================================================================
 *
 *       Filename:  CommDataDef.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/02/2018 03:52:55 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  enze (), 767201935@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef CXX_BT_COMMON_DATA_DEFINE_H
#define CXX_BT_COMMON_DATA_DEFINE_H


#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64  int64;
typedef unsigned __int64  uint64;
#else
typedef long long  int64;
typedef unsigned long long  uint64;
#endif
#if defined(_MSC_VER) && _MSC_VER < 1300
#define for  if (false) ; else for
#endif

const unsigned int MAX_SIZE = 0x02000000;
// COIN ��ʾ����һ�����رң�����100000000���Ǳ�ʾһ�����رң����ر���С��λΪС�����8λ
const int64 COIN = 100000000;
const int64 CENT = 1000000;
const int COINBASE_MATURITY = 2;// �һ������
// ������֤�����Ѷ�
const int VERSION = 105;
#endif /* CXX_BT_COMMON_DATA_DEFINE_H */
/* EOF */

