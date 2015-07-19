#ifndef __INC_METIN_II_COMMON_FILE_LOADER_H__
#define __INC_METIN_II_COMMON_FILE_LOADER_H__

typedef std::vector< std::string > TTokenVector;

class CMemoryTextFileLoader
{
	public:
		CMemoryTextFileLoader();
		virtual ~CMemoryTextFileLoader();

		void			Bind(int bufSize, const void* c_pvBuf);
		DWORD			GetLineCount();
		bool			CheckLineIndex(DWORD dwLine);
		bool			SplitLine(DWORD dwLine, std::vector<std::string> * pstTokenVector, const char * c_szDelimeter = " \t");
		const std::string &	GetLineString(DWORD dwLine);

	protected:
		std::vector<std::string> m_stLineVector;
};

#endif /* __INC_METIN_II_COMMON_FILE_LOADER_H__ */

