#include "stdafx.h"
#include "DES_table.h"

#define DES_ECB_ENCRYPT 0
#define DES_ECB_DECRYPT 1

// DES ECB encryption code
extern DWORD SP_boxes[8][64];
//extern DWORD KeyPerm[8][64];

/*
 * Macroses to transform array of 4 bytes to 32-bit dwords
 * (and reverse) without depending on the Little-Endian or 
 * Big-Endian processor's architecture 
 */
#define BYTES_TO_DWORD(b,d) (d  = ((DWORD)(*((b)++))), \
                             d |= ((DWORD)(*((b)++)))<< 8, \
                             d |= ((DWORD)(*((b)++)))<<16, \
                             d |= ((DWORD)(*((b)++)))<<24)

#define DWORD_TO_4BYTES(d,b) (*((b)++)=(BYTE)(((d)   )&0xff), \
                              *((b)++)=(BYTE)(((d)>> 8)&0xff), \
                              *((b)++)=(BYTE)(((d)>>16)&0xff), \
                              *((b)++)=(BYTE)(((d)>>24)&0xff))

/*
 * First of all, take into accounts the bytes and bites ordering 
 * used in DES:
 *                                                                 
 *  DES:    1  2  3  4  5  6  7  8   ....  57 58 59 60 61 62 63 64 
 *  INTEL: 63 62 61 60 59 58 57 56   ....   7  6  5  4  3  2  1  0
 *                                                                
 * According to the DES, every 8-th bits is not used: 
 * for DES the bites  8, 16, 32, ..., 64 are excluded,
 * for INTEL:        56, 48, 40, ..., 0  are excluded
 *                                                                 
 * According to the above rool of numbering, the tables
 * used in DES (for Initial Permutation, Final Permutation,
 * Key Permutation, Compression Permutation, Expansion 
 * Permutation and P-Box permutation) have to be re-written.
 * 
 */

/*
  Some main ideas used to optimize DES software 
  implementation:

  a). Do not make an Expansion Permutation of 32-bit to 
      48 bit (32-bit of data - right half of 64-bit of data), 
      but make a correspondent preparation of the Key. So, 
      the step of Expansion Permutation and XORing 48 bit of 
      Expanded data and 48 bit of Compressed key will be 
      replaced by 2 XOR operations: 32 bit of data XOR first 
      32 bit part of prepeared key, then, the same 32 bit of 
      data XOR second 32-bit part of prepeared key

  b). Combine S-Box Substitution and P-Box Permutation 
      operations. 

  c). For the best performance 56-bit encryption key 
      have to be extendended to 128-byte array, i.e. for each 
      of 16 rounds of DES we prepare a unique pair of two 
      32-bit (4-bytes) words (see 'a)' above).

  d). We can use XOR, SHIFT, AND operations to swap 
      bits between words. For example, we have:

      word A       word B
      0 1 2 3      4 5 6 7

      We want to get:

      word A       word B
      0 4 2 6      1 5 3 7

      First, shift word A to get bites 1, 3 on the "right place"

      word TMP = (word A) >> 1      = 1   2   3   -

      TMP      = TMP ^ B            = 1^4 2^5 3^6 7

      we don't want to change bits 5 and 7 in the B word, so 

      TMP = TMP & MASK = TMP & 1010 = 1^4  -  3^6 -  

      now we can easy get the word B:

      B = B ^ TMP = (4 5 6 7) ^ (1^4  -  3^6 -) = 1 5 3 7 

      if we shift our "masking" TMP word reverse - we get 
      a mask for A word:

      TMP = TMP << 1 =  - 1^4  -  3^6

      now we can easy get the word A:

      A = A ^ TMP = (0 1 2 3) ^ (- 1^4  -  3^6) = 0 4 2 6

      The example above may be used to swap not only single 
      bits, but also bit sequencies. In this case you should 
      use shift on the value, equal to the number of bits 
      in pattern.

      As you see, all this opearations may be written like:
      TMP = ((A >> size) ^ B) & mask;
      B ^ = TMP;
      A ^=  TMP << size;
*/

#define PERMUTATION(a,b,t,n,m) ((t)=((((a)>>(n))^(b))&(m)),\
	(b)^=(t),\
	(a)^=((t)<<(n)))

#define HPERMUTATION(a,t,n,m) ((t)=((((a)<<(16-(n)))^(a))&(m)),\
	(a)=(a)^(t)^(t>>(16-(n))))

#define D_ENCRYPT(Left, Right, Ks, Num, TmpA, TmpB) \
	TmpA = (Right ^ Ks[Num  ]); \
	TmpB = (Right ^ Ks[Num+1]); \
	TmpB = ((TmpB >> 4) + (TmpB << 28)); \
	Left ^=	SP_boxes[1][(TmpB   )&0x3f]| \
	SP_boxes[3][(TmpB>> 8)&0x3f]| \
	SP_boxes[5][(TmpB>>16)&0x3f]| \
	SP_boxes[7][(TmpB>>24)&0x3f]| \
	SP_boxes[0][(TmpA   )&0x3f]| \
	SP_boxes[2][(TmpA>> 8)&0x3f]| \
	SP_boxes[4][(TmpA>>16)&0x3f]| \
	SP_boxes[6][(TmpA>>24)&0x3f];

void DES_ECB_mode(BYTE * Input,        /* 8 bytes of input data              */
                  BYTE * Output,       /* 8 bytes of output data             */
                  const DWORD * KeySchedule, /* [16][2] array of DWORDs            */ 
                  BYTE Operation)      /* DES_ECB_ENCRYPT or DES_ECB_DECRYPT */
{
    static BYTE * bInp, * bOut;
    static DWORD dwLeft, dwRigh, dwTmp, dwTmp1;

    bInp = Input;
    bOut = Output;

    BYTES_TO_DWORD(bInp, dwLeft);
    BYTES_TO_DWORD(bInp, dwRigh);

    /* Initial Permutation */
    PERMUTATION(dwRigh, dwLeft, dwTmp, 4, 0x0f0f0f0f);
    PERMUTATION(dwLeft, dwRigh, dwTmp,16, 0x0000ffff);
    PERMUTATION(dwRigh, dwLeft, dwTmp, 2, 0x33333333);
    PERMUTATION(dwLeft, dwRigh, dwTmp, 8, 0x00ff00ff);
    PERMUTATION(dwRigh, dwLeft, dwTmp, 1, 0x55555555);

    /* dwRigh and dwLeft has reversed bit orders - itwill be taken 
       into account in the next step */

    /* The initial rotate is done outside the loop.  This required the 
     * SP_boxes values to be rotated 1 bit to the right.
     */
    dwTmp  = (dwRigh<<1) | (dwRigh>>31);
    dwRigh = (dwLeft<<1) | (dwLeft>>31); 
    dwLeft = dwTmp;

    /* clear the top bits on machines with 8byte longs */
    dwLeft &= 0xffffffff;
    dwRigh &= 0xffffffff;

    if (Operation == DES_ECB_ENCRYPT)
    {                                                              /* Key order */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 0,  dwTmp, dwTmp1); /*  1  */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 2,  dwTmp, dwTmp1); /*  2  */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 4,  dwTmp, dwTmp1); /*  3  */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 6,  dwTmp, dwTmp1); /*  4  */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 8,  dwTmp, dwTmp1); /*  5  */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 10, dwTmp, dwTmp1); /*  6  */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 12, dwTmp, dwTmp1); /*  7  */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 14, dwTmp, dwTmp1); /*  8  */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 16, dwTmp, dwTmp1); /*  9  */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 18, dwTmp, dwTmp1); /*  10 */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 20, dwTmp, dwTmp1); /*  11 */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 22, dwTmp, dwTmp1); /*  12 */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 24, dwTmp, dwTmp1); /*  13 */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 26, dwTmp, dwTmp1); /*  14 */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 28, dwTmp, dwTmp1); /*  15 */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 30, dwTmp, dwTmp1); /*  16 */
    }
    else
    {
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 30, dwTmp, dwTmp1); /*  16 */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 28, dwTmp, dwTmp1); /*  15 */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 26, dwTmp, dwTmp1); /*  14 */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 24, dwTmp, dwTmp1); /*  13 */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 22, dwTmp, dwTmp1); /*  12 */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 20, dwTmp, dwTmp1); /*  11 */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 18, dwTmp, dwTmp1); /*  10 */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 16, dwTmp, dwTmp1); /*  9  */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 14, dwTmp, dwTmp1); /*  8  */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 12, dwTmp, dwTmp1); /*  7  */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 10, dwTmp, dwTmp1); /*  6  */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 8,  dwTmp, dwTmp1); /*  5  */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 6,  dwTmp, dwTmp1); /*  4  */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 4,  dwTmp, dwTmp1); /*  3  */
		D_ENCRYPT(dwLeft, dwRigh, KeySchedule, 2,  dwTmp, dwTmp1); /*  2  */
		D_ENCRYPT(dwRigh, dwLeft, KeySchedule, 0,  dwTmp, dwTmp1); /*  1  */
    }
	
    dwLeft = (dwLeft>>1) | (dwLeft<<31);
    dwRigh = (dwRigh>>1) | (dwRigh<<31);
	
    /* clear the top bits on machines with 8byte longs */
    dwLeft &= 0xffffffff;
    dwRigh &= 0xffffffff;
	
    /*
	* Do we need to swap dwLeft and dwRigh?
	* We have not to do the swap 
	* (We remember they are reversed)
	*  So - all we have to do is to make a correspondent Final Permutation 
	*/
	
    PERMUTATION(dwRigh, dwLeft, dwTmp, 1,0x55555555);
    PERMUTATION(dwLeft, dwRigh, dwTmp, 8,0x00ff00ff);
    PERMUTATION(dwRigh, dwLeft, dwTmp, 2,0x33333333);
    PERMUTATION(dwLeft, dwRigh, dwTmp,16,0x0000ffff);
    PERMUTATION(dwRigh, dwLeft, dwTmp, 4,0x0f0f0f0f);
	
    /*  Place our two 32-bits results into 8 bytes of output data */
    DWORD_TO_4BYTES(dwLeft, bOut);
    DWORD_TO_4BYTES(dwRigh, bOut);
}

//************ DES CBC mode encryption **************
int DES_Encrypt(DWORD *DstBuffer, const DWORD * SrcBuffer, const DWORD *KeyAddress, DWORD Length, DWORD *IVector)
{
    DWORD i;
    DWORD buffer[2];

    buffer[0] = IVector[0];
    buffer[1] = IVector[1];

    for (i = 0; i < (Length >> 2); i = i+2)
    { 
	// do EBC encryption of (Initial_Vector XOR Data)
	buffer[0] ^= SrcBuffer[i];
	buffer[1] ^= SrcBuffer[i+1];

	DES_ECB_mode((BYTE *) buffer, (BYTE *) buffer, KeyAddress, DES_ECB_ENCRYPT);

	DstBuffer[i]   = buffer[0];
	DstBuffer[i+1] = buffer[1];
    }

    return Length;
}

//************ DES CBC mode decryption **************
int DES_Decrypt(DWORD *DstBuffer, const DWORD * SrcBuffer, const DWORD *KeyAddress, DWORD Length, DWORD *IVector)
{
    DWORD i;
    DWORD buffer[2], ivectorL, ivectorR, oldSrcL, oldSrcR;

    ivectorL = IVector[0];
    ivectorR = IVector[1];

    for (i = 0; i < (Length >> 2); i = i + 2)
    { 
	buffer[0] = oldSrcL = SrcBuffer[i];
	buffer[1] = oldSrcR = SrcBuffer[i+1];

	// Encrypted Data -> new IV, 
	// then do EBC decryption of Encrypted Data, 
	// then XOR decrypted data with old IV
	DES_ECB_mode((BYTE *)buffer, (BYTE *)buffer, KeyAddress, DES_ECB_DECRYPT);

	DstBuffer[i]   = buffer[0] ^ ivectorL;
	DstBuffer[i+1] = buffer[1] ^ ivectorR;	  

	ivectorL = oldSrcL;
	ivectorR = oldSrcR;
    }

    return Length;
}

