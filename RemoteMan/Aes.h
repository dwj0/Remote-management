#ifndef MY_AES_H
#define MY_AES_H

#define AES_KEY		"远程连接管理软件"

/**
 * 参数 p: 明文的字符串数组。
 * 参数 plen: 明文的长度,长度必须为16的倍数。
 * 参数 key: 密钥的字符串数组。
 */
void aes(char *p, int plen, char *key);

/**
 * 参数 c: 密文的字符串数组。
 * 参数 clen: 密文的长度,长度必须为16的倍数。
 * 参数 key: 密钥的字符串数组。
 */
void deAes(char *c, int clen, char *key);


/**********************************************************************************
p：		要加密的数据,不用为16的倍数，因为要做16字节扩展，因此最少要多留16字节的空间
len：	要加密数据的长度
key:	密钥,<=16字节
返回：	加密后的长度，加密的的数据保存在p中
***********************************************************************************/
int AesEnCode(unsigned char *p, int len, char const *key=AES_KEY);

/**********************************************************************************
p：		要加密的数据,不用为16的倍数，因为要做16字节扩展，因此最少要多留16字节的空间
len：	要加密数据的长度
key:	密钥,<=16字节
DstStr:	存放加密后并转换为字符串的数据
返回：	加密后的数据
***********************************************************************************/
char *AesEnCodeToStr(void const *p, int len,  char *DstStr, char const *key=AES_KEY);


/***********************************************************************************
p：		要解密的数据
len：	要解密数据的长度,需要16的倍数
key:	密钥,<=16字节
返回：	解密后的长度，解密的的数据保存在p中
***********************************************************************************/
int AesDeCode(unsigned char *p, int len, char const *key=AES_KEY);

//将ASCII码转换为字符串，方便存储
char *BytesToString(unsigned char const *pData, int len, char *str);
//将字符串还原为ASCII码
int StringToBytes(char const *str, unsigned char *pData);

#endif