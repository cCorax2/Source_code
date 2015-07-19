#include "stdafx.h"
/*
static unsigned char const k8[16] = { 14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7 };
static unsigned char const k7[16] = { 15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10 };
static unsigned char const k6[16] = { 10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8 };
static unsigned char const k5[16] = {  7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15 };
static unsigned char const k4[16] = {  2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9 };
static unsigned char const k3[16] = { 12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11 };
static unsigned char const k2[16] = {  4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1 };
static unsigned char const k1[16] = { 13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7 };
*/
static unsigned char const k8[16] = { 15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13 };
static unsigned char const k7[16] = {  3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5 };
static unsigned char const k6[16] = { 10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8 };
static unsigned char const k5[16] = { 13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9 };
static unsigned char const k4[16] = { 11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3 };
static unsigned char const k3[16] = {  4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13 };
static unsigned char const k2[16] = {  4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1 };
static unsigned char const k1[16] = {  7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8 };

/* Byte-at-a-time substitution boxes */
static unsigned char k87[256];
static unsigned char k65[256];
static unsigned char k43[256];
static unsigned char k21[256];

void GOST_Init()
{
    int i;

    for (i = 0; i < 256; i++)
    {
	k87[i] = k8[i >> 4] << 4 | k7[i & 15];
	k65[i] = k6[i >> 4] << 4 | k5[i & 15];
	k43[i] = k4[i >> 4] << 4 | k3[i & 15];
	k21[i] = k2[i >> 4] << 4 | k1[i & 15];
    }
}

INLINE static DWORD f(DWORD x)
{
    x = k87[x >> 24 & 255] << 24 | k65[x >> 16 & 255] << 16 | k43[x >> 8 & 255] <<  8 | k21[x & 255];
    return x << 11 | x >> (32 - 11);
}
/*
static void GOST_ECB_Encrypt(DWORD * N1, DWORD * N2, const DWORD * KeyAddress)
{
    register DWORD n1, n2; // As named in the GOST 

    n1 = *N1;
    n2 = *N2;

    // Instead of swapping halves, swap names each round 
    n2 ^= f(n1+KeyAddress[0]);
    n1 ^= f(n2+KeyAddress[1]);
    n2 ^= f(n1+KeyAddress[2]);
    n1 ^= f(n2+KeyAddress[3]);
    n2 ^= f(n1+KeyAddress[4]);
    n1 ^= f(n2+KeyAddress[5]);
    n2 ^= f(n1+KeyAddress[6]);
    n1 ^= f(n2+KeyAddress[7]);

    n2 ^= f(n1+KeyAddress[0]);
    n1 ^= f(n2+KeyAddress[1]);
    n2 ^= f(n1+KeyAddress[2]);
    n1 ^= f(n2+KeyAddress[3]);
    n2 ^= f(n1+KeyAddress[4]);
    n1 ^= f(n2+KeyAddress[5]);
    n2 ^= f(n1+KeyAddress[6]);
    n1 ^= f(n2+KeyAddress[7]);

    n2 ^= f(n1+KeyAddress[0]);
    n1 ^= f(n2+KeyAddress[1]);
    n2 ^= f(n1+KeyAddress[2]);
    n1 ^= f(n2+KeyAddress[3]);
    n2 ^= f(n1+KeyAddress[4]);
    n1 ^= f(n2+KeyAddress[5]);
    n2 ^= f(n1+KeyAddress[6]);
    n1 ^= f(n2+KeyAddress[7]);

    n2 ^= f(n1+KeyAddress[7]);
    n1 ^= f(n2+KeyAddress[6]);
    n2 ^= f(n1+KeyAddress[5]);
    n1 ^= f(n2+KeyAddress[4]);
    n2 ^= f(n1+KeyAddress[3]);
    n1 ^= f(n2+KeyAddress[2]);
    n2 ^= f(n1+KeyAddress[1]);
    n1 ^= f(n2+KeyAddress[0]);

    // There is no swap after the last round 
    *N1 = n2;
    *N2 = n1;
}
*/
int GOST_Encrypt(DWORD * DstBuffer, const DWORD * SrcBuffer, const DWORD * KeyAddress, DWORD Length, DWORD *IVector)
{
    DWORD i;
    DWORD N1,N2;

    N1 = IVector[0];
    N2 = IVector[1];

    for (i = 0; i < (Length >> 2); i = i+2)
    {
	register DWORD n1, n2; // As named in the GOST 

	n1 = N1;
	n2 = N2;

	// Instead of swapping halves, swap names each round 
	n2 ^= f(n1+KeyAddress[0]);
	n1 ^= f(n2+KeyAddress[1]);
	n2 ^= f(n1+KeyAddress[2]);
	n1 ^= f(n2+KeyAddress[3]);
	n2 ^= f(n1+KeyAddress[4]);
	n1 ^= f(n2+KeyAddress[5]);
	n2 ^= f(n1+KeyAddress[6]);
	n1 ^= f(n2+KeyAddress[7]);

	n2 ^= f(n1+KeyAddress[0]);
	n1 ^= f(n2+KeyAddress[1]);
	n2 ^= f(n1+KeyAddress[2]);
	n1 ^= f(n2+KeyAddress[3]);
	n2 ^= f(n1+KeyAddress[4]);
	n1 ^= f(n2+KeyAddress[5]);
	n2 ^= f(n1+KeyAddress[6]);
	n1 ^= f(n2+KeyAddress[7]);

	n2 ^= f(n1+KeyAddress[0]);
	n1 ^= f(n2+KeyAddress[1]);
	n2 ^= f(n1+KeyAddress[2]);
	n1 ^= f(n2+KeyAddress[3]);
	n2 ^= f(n1+KeyAddress[4]);
	n1 ^= f(n2+KeyAddress[5]);
	n2 ^= f(n1+KeyAddress[6]);
	n1 ^= f(n2+KeyAddress[7]);

	n2 ^= f(n1+KeyAddress[7]);
	n1 ^= f(n2+KeyAddress[6]);
	n2 ^= f(n1+KeyAddress[5]);
	n1 ^= f(n2+KeyAddress[4]);
	n2 ^= f(n1+KeyAddress[3]);
	n1 ^= f(n2+KeyAddress[2]);
	n2 ^= f(n1+KeyAddress[1]);
	n1 ^= f(n2+KeyAddress[0]);

    	N1 = n2;
	N2 = n1;
    	//GOST_ECB_Encrypt(&N1, &N2, KeyAddress);
	// XOR plaintext with initial vector, 
	// move rezult to ciphertext and to initial vector
	DstBuffer[i]   = SrcBuffer[i] ^ N1;
	N1             = DstBuffer[i];

	DstBuffer[i+1] = SrcBuffer[i+1] ^ N2;
	N2             = DstBuffer[i+1];
    }

    return Length;
}


// ************ GOST CBC decryption **************
int GOST_Decrypt(DWORD * DstBuffer, const DWORD * SrcBuffer, const DWORD * KeyAddress, DWORD Length, DWORD *IVector)
{   
    DWORD i;
    DWORD N1, N2, dwTmp;

    N1 = IVector[0];
    N2 = IVector[1];

    for (i = 0; i < (Length >> 2); i = i + 2)
    {
	register DWORD n1, n2; // As named in the GOST 

	n1 = N1;
	n2 = N2;

	// Instead of swapping halves, swap names each round 
	n2 ^= f(n1+KeyAddress[0]);
	n1 ^= f(n2+KeyAddress[1]);
	n2 ^= f(n1+KeyAddress[2]);
	n1 ^= f(n2+KeyAddress[3]);
	n2 ^= f(n1+KeyAddress[4]);
	n1 ^= f(n2+KeyAddress[5]);
	n2 ^= f(n1+KeyAddress[6]);
	n1 ^= f(n2+KeyAddress[7]);

	n2 ^= f(n1+KeyAddress[0]);
	n1 ^= f(n2+KeyAddress[1]);
	n2 ^= f(n1+KeyAddress[2]);
	n1 ^= f(n2+KeyAddress[3]);
	n2 ^= f(n1+KeyAddress[4]);
	n1 ^= f(n2+KeyAddress[5]);
	n2 ^= f(n1+KeyAddress[6]);
	n1 ^= f(n2+KeyAddress[7]);

	n2 ^= f(n1+KeyAddress[0]);
	n1 ^= f(n2+KeyAddress[1]);
	n2 ^= f(n1+KeyAddress[2]);
	n1 ^= f(n2+KeyAddress[3]);
	n2 ^= f(n1+KeyAddress[4]);
	n1 ^= f(n2+KeyAddress[5]);
	n2 ^= f(n1+KeyAddress[6]);
	n1 ^= f(n2+KeyAddress[7]);

	n2 ^= f(n1+KeyAddress[7]);
	n1 ^= f(n2+KeyAddress[6]);
	n2 ^= f(n1+KeyAddress[5]);
	n1 ^= f(n2+KeyAddress[4]);
	n2 ^= f(n1+KeyAddress[3]);
	n1 ^= f(n2+KeyAddress[2]);
	n2 ^= f(n1+KeyAddress[1]);
	n1 ^= f(n2+KeyAddress[0]);

	// There is no swap after the last round 
	N1 = n2;
	N2 = n1;
	//GOST_ECB_Encrypt(&N1, &N2, KeyAddress);
	// XOR  encrypted text with encrypted initial vector (we get rezult - decrypted text), 
	// move encrypted text to new initial vector.
	// We need dwTmp because SrcBuffer may be the same as DstBuffer
	dwTmp        = SrcBuffer[i] ^ N1;
	N1           = SrcBuffer[i];
	DstBuffer[i] = dwTmp;

	dwTmp          = SrcBuffer[i+1] ^ N2;
	N2             = SrcBuffer[i+1];
	DstBuffer[i+1] = dwTmp;
    }

    return Length;
}

