/*
 *    Filename: buffer.h
 * Description: Buffer 처리 모듈
 *
 *      Author: 김한주 (aka. 비엽, Cronan), 송영진 (aka. myevan, 빗자루)
 */
#ifndef __INC_LIBTHECORE_BUFFER_H__
#define __INC_LIBTHECORE_BUFFER_H__

#define SAFE_BUFFER_DELETE(buf)		{ if(buf != NULL) { buffer_delete(buf); buf = NULL; } }

    typedef struct buffer	BUFFER;
    typedef struct buffer *	LPBUFFER;

    struct buffer
    {
	struct buffer * next;

	char *          write_point;
	int             write_point_pos;

	const char *    read_point;
	int             length;

	char *          mem_data;
	int             mem_size;

	long            flag;
    };

	extern LPBUFFER	buffer_new(int size);				// 새 버퍼 생성
    extern void		buffer_delete(LPBUFFER buffer);					// 버퍼 삭제
    extern void		buffer_reset(LPBUFFER buffer);					// 버퍼 길이들을 초기화

    extern DWORD	buffer_size(LPBUFFER buffer);					// 버퍼에 남은 길이
    extern int		buffer_has_space(LPBUFFER buffer);				// 쓸 수 있는 길이를 리턴

    extern void		buffer_write (LPBUFFER& buffer, const void* src, int length);	// 버퍼에 쓴다.
    extern void		buffer_read(LPBUFFER buffer, void * buf, int bytes);		// 버퍼에서 읽는다.
    extern BYTE		buffer_get_byte(LPBUFFER buffer);
    extern WORD		buffer_get_word(LPBUFFER buffer);
    extern DWORD	buffer_get_dword(LPBUFFER buffer);

    // buffer_proceed 함수는 buffer_peek으로 읽기용 포인터를 리턴 받아서 쓸 필요가
    // 있을 때 처리가 끝나면 얼마나 처리가 끝났다고 통보해야 할 때 쓴다. 
    // (buffer_read, buffer_get_* 시리즈의 경우에는 알아서 처리되지만 peek으로 처리했을
    //  때는 그렇게 될 수가 없으므로)
    extern const void *	buffer_read_peek(LPBUFFER buffer);				// 읽는 위치를 리턴
    extern void		buffer_read_proceed(LPBUFFER buffer, int length);		// length만큼의 처리가 끝남

    // 마찬가지로 write_peek으로 쓰기 위치를 얻어온 다음 얼마나 썼나 통보할 때
    // buffer_write_proceed를 사용한다.
    extern void *	buffer_write_peek(LPBUFFER buffer);				// 쓰는 위치를 리턴
    extern void		buffer_write_proceed(LPBUFFER buffer, int length);		// length만 증가 시킨다.

    extern void		buffer_adjust_size(LPBUFFER & buffer, int add_size);		// add_size만큼 추가할 크기를 확보
#endif
