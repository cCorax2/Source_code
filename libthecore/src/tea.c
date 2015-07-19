/*
 *    Filename: tea.c
 * Description: TEA 암호화 모듈
 *
 *      Author: 김한주 (aka. 비엽, Cronan), 송영진 (aka. myevan, 빗자루)
 */
#include "stdafx.h"

/*
 * TEA Encryption Module Instruction
 *					Edited by 김한주 aka. 비엽, Cronan
 *
 * void tea_code(const DWORD sz, const DWORD sy, const DWORD *key, DWORD *dest)
 * void tea_decode(const DWORD sz, const DWORD sy, const DWORD *key, DWORD *dest)
 *   8바이트를 암호/복호화 할때 사용된다. key 는 16 바이트여야 한다.
 *   sz, sy 는 8바이트의 역순으로 대입한다. 
 * 
 * int tea_decrypt(DWORD *dest, const DWORD *src, const DWORD *key, int size);
 * int tea_encrypt(DWORD *dest, const DWORD *src, const DWORD *key, int size);
 *   한꺼번에 8 바이트 이상을 암호/복호화 할때 사용한다. 만약 size 가
 *   8의 배수가 아니면 8의 배수로 크기를 "늘려서" 암호화 한다. 
 *
 * ex. tea_code(pdwSrc[1], pdwSrc[0], pdwKey, pdwDest);
 *     tea_decrypt(pdwDest, pdwSrc, pdwKey, nSize);
 */

#define TEA_ROUND		32		// 32 를 권장하며, 높을 수록 결과가 난해해 진다.
#define DELTA			0x9E3779B9	// DELTA 값 바꾸지 말것.

char tea_nilbuf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

INLINE void tea_code(const DWORD sz, const DWORD sy, const DWORD *key, DWORD *dest)
{
    register DWORD	y = sy, z = sz, sum = 0;

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 1
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 2
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 3
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 4
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 5
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 6
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 7
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 8
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 9
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 10
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 11
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 12
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 13
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 14
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 15
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 16
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 17
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 18
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 19
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 20
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 21
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 22
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 23
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 24
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 25
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 26
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 27
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 28
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 29
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 30
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 31
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 32
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    *(dest++)	= y;
    *dest	= z;
}

INLINE void tea_decode(const DWORD sz, const DWORD sy, const DWORD *key, DWORD *dest)
{
    register DWORD y = sy, z = sz, sum = DELTA * TEA_ROUND;

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 1
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 2
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 3
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 4
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 5
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 6
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 7
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 8
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 9
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 10
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 11
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 12
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 13
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 14
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 15
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 16
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 17
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 18
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 19
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 20
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 21
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 22
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 23
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 24
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 25
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 26
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 27
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 28
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 29
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 30
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 31
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 32
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    *(dest++)	= y;
    *dest	= z;
}

int TEA_Encrypt(DWORD *dest, const DWORD *src, const DWORD * key, int size)
{
    int		i;
    int		resize;

    if (size % 8 != 0)
    {
	resize = size + 8 - (size % 8);
	memset((char *) src + size, 0, resize - size);
    }
    else
	resize = size;

    for (i = 0; i < resize >> 3; i++, dest += 2, src += 2)
	tea_code(*(src + 1), *src, key, dest);

    return (resize);
}

int TEA_Decrypt(DWORD *dest, const DWORD *src, const DWORD * key, int size)
{
    int		i;
    int		resize;

    if (size % 8 != 0)
	resize = size + 8 - (size % 8);
    else
	resize = size;

    for (i = 0; i < resize >> 3; i++, dest += 2, src += 2)
	tea_decode(*(src + 1), *src, key, dest);

    return (resize);
}

