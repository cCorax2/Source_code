#include "stdafx.h"
#include "file_loader.h"

CMemoryTextFileLoader::CMemoryTextFileLoader()
{
}

CMemoryTextFileLoader::~CMemoryTextFileLoader()
{
}

bool CMemoryTextFileLoader::SplitLine(DWORD dwLine, std::vector<std::string>* pstTokenVector, const char * c_szDelimeter)
{
	pstTokenVector->clear();

	std::string stToken;
	const std::string & c_rstLine = GetLineString(dwLine);

	DWORD basePos = 0;

	do
	{
		int beginPos = c_rstLine.find_first_not_of(c_szDelimeter, basePos);
		if (beginPos < 0)
			return false;

		int endPos;

		if (c_rstLine[beginPos] == '#' && c_rstLine.compare(beginPos, 4, "#--#") != 0)
		{
			return false;
		}
		else if (c_rstLine[beginPos] == '"')
		{
			++beginPos;
			endPos = c_rstLine.find_first_of("\"", beginPos);

			if (endPos < 0)
				return false;

			basePos = endPos + 1;
		}
		else
		{
			endPos = c_rstLine.find_first_of(c_szDelimeter, beginPos);
			basePos = endPos;
		}

		pstTokenVector->push_back(c_rstLine.substr(beginPos, endPos - beginPos));

		// 추가 코드. 맨뒤에 탭이 있는 경우를 체크한다. - [levites]
		if (int(c_rstLine.find_first_not_of(c_szDelimeter, basePos)) < 0)
			break;
	} while (basePos < c_rstLine.length());

	return true;
}

DWORD CMemoryTextFileLoader::GetLineCount()
{
	return m_stLineVector.size();
}

bool CMemoryTextFileLoader::CheckLineIndex(DWORD dwLine)
{
	if (dwLine >= m_stLineVector.size())
		return false;

	return true;
}

const std::string & CMemoryTextFileLoader::GetLineString(DWORD dwLine)
{
	assert(CheckLineIndex(dwLine));
	return m_stLineVector[dwLine];
}

void CMemoryTextFileLoader::Bind(int bufSize, const void* c_pvBuf)
{
	m_stLineVector.clear();

	const char * c_pcBuf = (const char *)c_pvBuf;
	std::string stLine;
	int pos = 0;

	while (pos < bufSize)
	{
		const char c = c_pcBuf[pos++];

		if ('\n' == c || '\r' == c)
		{
			if (pos < bufSize)
				if ('\n' == c_pcBuf[pos] || '\r' == c_pcBuf[pos])
					++pos;

			m_stLineVector.push_back(stLine);
			stLine = "";
		}
		else if (c < 0)
		{
			stLine.append(c_pcBuf + (pos-1), 2);
			++pos;
		}
		else
		{
			stLine += c;
		}
	}

	m_stLineVector.push_back(stLine);
}

