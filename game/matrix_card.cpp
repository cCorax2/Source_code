//#define __MATRIX_MAIN_ENABLE__ // define 되어 있으면 main 함수가 포함된다. Unit test 시에 사용
#ifndef __MATRIX_MAIN_ENABLE__
#include "stdafx.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define thecore_random random
#define sys_err printf
#define sys_log printf

#endif

#include "matrix_card.h"

#define ROW(rows, i) ((rows >> ((4 - i - 1) * 8)) & 0x000000FF)
#define COL(cols, i) ((cols >> ((4 - i - 1) * 8)) & 0x000000FF)

const static int MAX_ROWS = 6;
const static int MAX_COLS = 8;
const static unsigned int MAX_LENS = (MAX_ROWS * MAX_COLS * 2);
const static unsigned int MAX_LENE = (MAX_LENS * 2);
const static int ASLENGTH = 8;

bool EncodeMatrix(const char* szsrc, const char* szpwd, char* lpdes, const unsigned int usize)
{
	int nlen = strlen(szsrc);
	int i;

	if (strlen(szsrc) != MAX_LENS || strlen(szpwd) != MAX_LENS || lpdes == NULL || usize < (MAX_LENE + 1))
		return false;

	memset(lpdes, 0, usize);

	for (i = 0; i < nlen; i++)
	{
		char sc = szsrc[i];
		char pc = szpwd[i];
		char dc = sc ^ pc;
		char szhex[3];
		snprintf(szhex, sizeof(szhex), "%02X", dc);
		strlcat(lpdes, szhex, usize);
	}

	return (i == nlen) ? true : false;
}

bool DecodeMatrix(const char* szsrc, const char* szpwd, char* lpdes, const unsigned int usize)
{
	int nlen = strlen(szpwd);
	int i;

	if (strlen(szsrc) != MAX_LENE || strlen(szpwd) != MAX_LENS || lpdes == NULL || usize < (MAX_LENS + 1))
		return false;

	memset(lpdes, 0, usize);

	char szhex[] = { "0123456789ABCDEF" };

	for (i = 0; i < nlen; i++)
	{
		char sc1 = szsrc[i*2];
		char sc2 = szsrc[i*2+1];
		char pc = szpwd[i];
		char sn1 = (char)(strchr(szhex, sc1) - szhex); 
		char sn2 = (char)(strchr(szhex, sc2) - szhex);
		char dc = (sn1 * 16 + sn2) ^ pc;
		lpdes[i] = dc;
	}

	return (i == nlen) ? true : false;
}

void MatrixCardRndCoordinate(unsigned long & rows, unsigned long & cols)
{
	for (register unsigned long i = 0; i < (ASLENGTH >> 1); i++)
	{
		rows |= ((thecore_random() % MAX_ROWS) & 0x000000FF) << ((4 - i - 1) * 8);
		cols |= ((thecore_random() % MAX_COLS) & 0x000000FF) << ((4 - i - 1) * 8);
	}
}

bool ChkCoordinate(const unsigned long rows, const unsigned long cols, const char* matrix, const char* answer)
{
	unsigned int max_lens = strlen(matrix);
	int answer_lens = strlen(answer);

	if (max_lens != MAX_LENS || answer_lens != ASLENGTH)
	{
		sys_err("MATRIX_CARD: length error matrix %d answer %d", max_lens, answer_lens);
		return false;
	}

	bool fResult = true;

	unsigned short * pmatrix = (unsigned short *)matrix;
	unsigned short * panswer = (unsigned short *)answer;

	for (register unsigned long i = 0; i < (ASLENGTH >> 1); i++)
	{
		if (*(pmatrix + (ROW(rows, i) * MAX_COLS + COL(cols, i))) != *(panswer + i))
		{
			fResult = false;
			break;
		}
	}

	return fResult;
}

bool MatrixCardCheck(const char * src, const char * answer, unsigned long rows, unsigned cols)
{
	const char * szpasswd = "xEwx3Lb5fH2mnPaMh215cHTbCrFCSmh9yQ3FrybwPnD89QkNX4UTA8UdH41LnU4P94UnaeXDTk17dY5DLaSDPAwvEpMUNTxV";

	char decode_result[MAX_LENS+1];
	DecodeMatrix(src, szpasswd, decode_result, sizeof(decode_result));

	sys_log(0, "MatrixCardCheck %c%d %c%d %c%d %c%d answer %s", 
			MATRIX_CARD_ROW(rows, 0) + 'A',
			MATRIX_CARD_COL(cols, 0) + 1,
			MATRIX_CARD_ROW(rows, 1) + 'A',
			MATRIX_CARD_COL(cols, 1) + 1,
			MATRIX_CARD_ROW(rows, 2) + 'A',
			MATRIX_CARD_COL(cols, 2) + 1,
			MATRIX_CARD_ROW(rows, 3) + 'A',
			MATRIX_CARD_COL(cols, 3) + 1,
			answer);

	return ChkCoordinate(rows, cols, decode_result, answer);
}

#ifdef __MATRIX_MAIN_ENABLE__

void GetRightAnswer(const unsigned long rows, const unsigned long cols, const char* matrix, char* answer, const unsigned int nsize)
{
	if (strlen(matrix) != MAX_LENS || answer == NULL || nsize < (ASLENGTH + 1))
		return;

	unsigned short * pmatrix = (unsigned short *)matrix;
	unsigned short * panswer = (unsigned short *)answer;

	memset(answer, 0, nsize);

	for (register unsigned long i = 0; i < (ASLENGTH >> 1); i++)
	{
		char sztemp[3] = { 0, 0, 0 };
		memcpy(sztemp, (char*)(pmatrix + (ROW(rows, i) * MAX_COLS + COL(cols, i))), 2);
		strlcat(answer, sztemp, nsize);
	}
}

int main(int argc, char* argv[])
{
	srandomdev();

	char* szmatrix = "9Vnppuvv6D8uDmKV9Lbn3ntav6Y86tbMLre7w3DmFc4mTNYEm2UtrppuC9LX6yhYShYTSTCLNC1GwCEV717hTaVYCftMK2xS";
	char* szpasswd = "xEwx3Lb5fH2mnPaMh215cHTbCrFCSmh9yQ3FrybwPnD89QkNX4UTA8UdH41LnU4P94UnaeXDTk17dY5DLaSDPAwvEpMUNTxV";

	// matrix encode and decode test
	char encode_result[MAX_LENE + 1], decode_result[MAX_LENS+1];
	EncodeMatrix(szmatrix, szpasswd, encode_result, sizeof(encode_result));
	printf("Encode result: %s\r\n", encode_result);

	DecodeMatrix(encode_result, szpasswd, decode_result, sizeof(decode_result));
	printf("Decode result: %s\r\n", decode_result);

	// matrix rand password test
	unsigned long rand_rows = 0, rand_cols = 0;
	MatrixCardRndCoordinate(rand_rows, rand_cols); // get rand position of matrix

	// display rand position
	printf("%c%d %c%d %c%d %c%d\r\n",
			ROW(rand_rows, 0) + 'A', COL(rand_cols, 0) + 1, \
			ROW(rand_rows, 1) + 'A', COL(rand_cols, 1) + 1, \
			ROW(rand_rows, 2) + 'A', COL(rand_cols, 2) + 1, \
			ROW(rand_rows, 3) + 'A', COL(rand_cols, 3) + 1);

	char answer[9];
	GetRightAnswer(rand_rows, rand_cols, szmatrix, answer, sizeof(answer));	// get right answer for test, release not need.
	printf("Answer: %s\r\n", answer);

	bool f = ChkCoordinate(rand_rows, rand_cols, szmatrix, answer);	// check answer
	printf("Result: %s\n", (f) ? "true" : "false");
	return 1;
}

#endif
