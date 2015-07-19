#ifndef __INC_LIBTHECORE_HANGUL_H__
#define __INC_LIBTHECORE_HANGUL_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifdef __WIN32__
#define isdigit iswdigit
#define isspace iswspace
#endif

#define ishan(ch)       (((ch) & 0xE0) > 0x90)
#define ishanasc(ch)    (isascii(ch) || ishan(ch))
#define ishanalp(ch)    (isalpha(ch) || ishan(ch))
#define isnhdigit(ch)   (!ishan(ch) && isdigit(ch))
#define isnhspace(ch)   (!ishan(ch) && isspace(ch))

    extern const char *	first_han(const BYTE * str);	// 첫번째 두 글자의 모음(ㄱㄴㄷ)을 뽑아 가/나/다/..를 리턴한다.
    extern int		check_han(const char * str);	// 한글이면 true 스트링 전부 체크
    extern int		is_hangul(const BYTE * str);	// 한글이면 true (2바이트만 체크)
    extern int		under_han(const void * orig);	// 받침이 있으면 true

#define UNDER(str)	under_han(str)

#ifdef __cplusplus
};
#endif

#endif
