#include "stdafx.h"
#include "CsvReader.h"
#include <fstream>
#include <algorithm>

#ifndef Assert
    #include <assert.h>
    #define Assert assert
    #define LogToFile (void)(0);
#endif

namespace
{
    /// 파싱용 state 열거값
    enum ParseState
    {
        STATE_NORMAL = 0, ///< 일반 상태
        STATE_QUOTE       ///< 따옴표 뒤의 상태
    };

    /// 문자열 좌우의 공백을 제거해서 반환한다.
    std::string Trim(std::string str)
    {
        str = str.erase(str.find_last_not_of(" \t\r\n") + 1);
        str = str.erase(0, str.find_first_not_of(" \t\r\n"));
        return str;
    }

    /// \brief 주어진 문장에 있는 알파벳을 모두 소문자로 바꾼다.
    std::string Lower(std::string original)
    {
        std::transform(original.begin(), original.end(), original.begin(), tolower);
        return original;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 셀을 액세스할 때, 숫자 대신 사용할 이름을 등록한다.
/// \param name 셀 이름
/// \param index 셀 인덱스
////////////////////////////////////////////////////////////////////////////////
void cCsvAlias::AddAlias(const char* name, size_t index)
{
    std::string converted(Lower(name));

    Assert(m_Name2Index.find(converted) == m_Name2Index.end());
    Assert(m_Index2Name.find(index) == m_Index2Name.end());

    m_Name2Index.insert(NAME2INDEX_MAP::value_type(converted, index));
    m_Index2Name.insert(INDEX2NAME_MAP::value_type(index, name));
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 모든 데이터를 삭제한다.
////////////////////////////////////////////////////////////////////////////////
void cCsvAlias::Destroy()
{
    m_Name2Index.clear();
    m_Index2Name.clear();
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 숫자 인덱스를 이름으로 변환한다.
/// \param index 숫자 인덱스
/// \return const char* 이름
////////////////////////////////////////////////////////////////////////////////
const char* cCsvAlias::operator [] (size_t index) const
{
    INDEX2NAME_MAP::const_iterator itr(m_Index2Name.find(index));
    if (itr == m_Index2Name.end())
    {
        LogToFile(NULL, "cannot find suitable conversion for %d", index);
        Assert(false && "cannot find suitable conversion");
        return NULL;
    }

    return itr->second.c_str();
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 이름을 숫자 인덱스로 변환한다.
/// \param name 이름
/// \return size_t 숫자 인덱스
////////////////////////////////////////////////////////////////////////////////
size_t cCsvAlias::operator [] (const char* name) const
{
    NAME2INDEX_MAP::const_iterator itr(m_Name2Index.find(Lower(name)));
    if (itr == m_Name2Index.end())
    {
        LogToFile(NULL, "cannot find suitable conversion for %s", name);
        Assert(false && "cannot find suitable conversion");
        return 0;
    }

    return itr->second;
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 지정된 이름의 CSV 파일을 로드한다.
/// \param fileName CSV 파일 이름
/// \param seperator 필드 분리자로 사용할 글자. 기본값은 ','이다.
/// \param quote 따옴표로 사용할 글자. 기본값은 '"'이다.
/// \return bool 무사히 로드했다면 true, 아니라면 false
////////////////////////////////////////////////////////////////////////////////
bool cCsvFile::Load(const char* fileName, const char seperator, const char quote)
{
    Assert(seperator != quote);

    std::ifstream file(fileName, std::ios::in);
    if (!file) return false;

    Destroy(); // 기존의 데이터를 삭제

    cCsvRow* row = NULL;
    ParseState state = STATE_NORMAL;
    std::string token = "";
    char buf[2048+1] = {0,};

    while (file.good())
    {
        file.getline(buf, 2048);
        buf[sizeof(buf)-1] = 0;

        std::string line(Trim(buf));
        if (line.empty() || (state == STATE_NORMAL && line[0] == '#')) continue;
        
        std::string text  = std::string(line) + "  "; // 파싱 lookahead 때문에 붙여준다.
        size_t cur = 0;

        while (cur < text.size())
        {
            // 현재 모드가 QUOTE 모드일 때,
            if (state == STATE_QUOTE)
            {
                // '"' 문자의 종류는 두 가지이다.
                // 1. 셀 내부에 특수 문자가 있을 경우 이를 알리는 셀 좌우의 것
                // 2. 셀 내부의 '"' 문자가 '"' 2개로 치환된 것
                // 이 중 첫번째 경우의 좌측에 있는 것은 CSV 파일이 정상적이라면, 
                // 무조건 STATE_NORMAL에 걸리게 되어있다.
                // 그러므로 여기서 걸리는 것은 1번의 우측 경우나, 2번 경우 뿐이다.
                // 2번의 경우에는 무조건 '"' 문자가 2개씩 나타난다. 하지만 1번의
                // 우측 경우에는 아니다. 이를 바탕으로 해서 코드를 짜면...
                if (text[cur] == quote)
                {
                    // 다음 문자가 '"' 문자라면, 즉 연속된 '"' 문자라면 
                    // 이는 셀 내부의 '"' 문자가 치환된 것이다.
                    if (text[cur+1] == quote)
                    {
                        token += quote;
                        ++cur;
                    }
                    // 다음 문자가 '"' 문자가 아니라면 
                    // 현재의 '"'문자는 셀의 끝을 알리는 문자라고 할 수 있다.
                    else
                    {
                        state = STATE_NORMAL;
                    }
                }
                else
                {
                    token += text[cur];
                }
            }
            // 현재 모드가 NORMAL 모드일 때,
            else if (state == STATE_NORMAL)
            {
                if (row == NULL)
                    row = new cCsvRow();

                // ',' 문자를 만났다면 셀의 끝의 의미한다.
                // 토큰으로서 셀 리스트에다가 집어넣고, 토큰을 초기화한다.
                if (text[cur] == seperator)
                {
                    row->push_back(token);
                    token.clear();
                }
                // '"' 문자를 만났다면, QUOTE 모드로 전환한다.
                else if (text[cur] == quote)
                {
                    state = STATE_QUOTE;
                }
                // 다른 일반 문자라면 현재 토큰에다가 덧붙인다.
                else
                {
                    token += text[cur];
                }
            }

            ++cur;
        }

        // 마지막 셀은 끝에 ',' 문자가 없기 때문에 여기서 추가해줘야한다.
        // 단, 처음에 파싱 lookahead 때문에 붙인 스페이스 문자 두 개를 뗀다.
        if (state == STATE_NORMAL)
        {
            Assert(row != NULL);
            row->push_back(token.substr(0, token.size()-2));
            m_Rows.push_back(row);
            token.clear();
            row = NULL;
        }
        else
        {
            token = token.substr(0, token.size()-2) + "\r\n";
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 가지고 있는 내용을 CSV 파일에다 저장한다.
/// \param fileName CSV 파일 이름
/// \param append true일 경우, 기존의 파일에다 덧붙인다. false인 경우에는 
/// 기존의 파일 내용을 삭제하고, 새로 쓴다.
/// \param seperator 필드 분리자로 사용할 글자. 기본값은 ','이다.
/// \param quote 따옴표로 사용할 글자. 기본값은 '"'이다.
/// \return bool 무사히 저장했다면 true, 에러가 생긴 경우에는 false
////////////////////////////////////////////////////////////////////////////////
bool cCsvFile::Save(const char* fileName, bool append, char seperator, char quote) const
{
    Assert(seperator != quote);

    // 출력 모드에 따라 파일을 적당한 플래그로 생성한다.
    std::ofstream file;
    if (append) { file.open(fileName, std::ios::out | std::ios::app); }
    else { file.open(fileName, std::ios::out | std::ios::trunc); }

    // 파일을 열지 못했다면, false를 리턴한다.
    if (!file) return false;

    char special_chars[5] = { seperator, quote, '\r', '\n', 0 };
    char quote_escape_string[3] = { quote, quote, 0 };

    // 모든 행을 횡단하면서...
    for (size_t i=0; i<m_Rows.size(); i++)
    {
        const cCsvRow& row = *((*this)[i]);

        std::string line;

        // 행 안의 모든 토큰을 횡단하면서...
        for (size_t j=0; j<row.size(); j++)
        {
            const std::string& token = row[j];

            // 일반적인('"' 또는 ','를 포함하지 않은) 
            // 토큰이라면 그냥 저장하면 된다.
            if (token.find_first_of(special_chars) == std::string::npos)
            {
                line += token;
            }
            // 특수문자를 포함한 토큰이라면 문자열 좌우에 '"'를 붙여주고,
            // 문자열 내부의 '"'를 두 개로 만들어줘야한다.
            else
            {
                line += quote;

                for (size_t k=0; k<token.size(); k++)
                {
                    if (token[k] == quote) line += quote_escape_string;
                    else line += token[k];
                }

                line += quote;
            }

            // 마지막 셀이 아니라면 ','를 토큰의 뒤에다 붙여줘야한다.
            if (j != row.size() - 1) { line += seperator; }
        }

        // 라인을 출력한다.
        file << line << std::endl;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 모든 데이터를 메모리에서 삭제한다.
////////////////////////////////////////////////////////////////////////////////
void cCsvFile::Destroy()
{
    for (ROWS::iterator itr(m_Rows.begin()); itr != m_Rows.end(); ++itr)
        delete *itr;

    m_Rows.clear();
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 해당하는 인덱스의 행을 반환한다.
/// \param index 인덱스
/// \return cCsvRow* 해당 행
////////////////////////////////////////////////////////////////////////////////
cCsvRow* cCsvFile::operator [] (size_t index)
{
    Assert(index < m_Rows.size());
    return m_Rows[index];
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 해당하는 인덱스의 행을 반환한다.
/// \param index 인덱스
/// \return const cCsvRow* 해당 행
////////////////////////////////////////////////////////////////////////////////
const cCsvRow* cCsvFile::operator [] (size_t index) const
{
    Assert(index < m_Rows.size());
    return m_Rows[index];
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 생성자
////////////////////////////////////////////////////////////////////////////////
cCsvTable::cCsvTable()
: m_CurRow(-1)
{
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 소멸자
////////////////////////////////////////////////////////////////////////////////
cCsvTable::~cCsvTable()
{
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 지정된 이름의 CSV 파일을 로드한다.
/// \param fileName CSV 파일 이름
/// \param seperator 필드 분리자로 사용할 글자. 기본값은 ','이다.
/// \param quote 따옴표로 사용할 글자. 기본값은 '"'이다.
/// \return bool 무사히 로드했다면 true, 아니라면 false
////////////////////////////////////////////////////////////////////////////////
bool cCsvTable::Load(const char* fileName, const char seperator, const char quote)
{
    Destroy();
    return m_File.Load(fileName, seperator, quote);
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 다음 행으로 넘어간다.
/// \return bool 다음 행으로 무사히 넘어간 경우 true를 반환하고, 더 이상
/// 넘어갈 행이 존재하지 않는 경우에는 false를 반환한다.
////////////////////////////////////////////////////////////////////////////////
bool cCsvTable::Next()
{
    // 20억번 정도 호출하면 오버플로가 일어날텐데...괜찮겠지?
    return ++m_CurRow < (int)m_File.GetRowCount() ? true : false;
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 현재 행의 셀 숫자를 반환한다.
/// \return size_t 현재 행의 셀 숫자
////////////////////////////////////////////////////////////////////////////////
size_t cCsvTable::ColCount() const
{
    return CurRow()->size();
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 인덱스를 이용해 int 형으로 셀 값을 반환한다.
/// \param index 셀 인덱스
/// \return int 셀 값
////////////////////////////////////////////////////////////////////////////////
int cCsvTable::AsInt(size_t index) const
{
    const cCsvRow* const row = CurRow();
    Assert(row);
    Assert(index < row->size());
    return row->AsInt(index);
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 인덱스를 이용해 double 형으로 셀 값을 반환한다.
/// \param index 셀 인덱스
/// \return double 셀 값
////////////////////////////////////////////////////////////////////////////////
double cCsvTable::AsDouble(size_t index) const
{
    const cCsvRow* const row = CurRow();
    Assert(row);
    Assert(index < row->size());
    return row->AsDouble(index);
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 인덱스를 이용해 std::string 형으로 셀 값을 반환한다.
/// \param index 셀 인덱스
/// \return const char* 셀 값
////////////////////////////////////////////////////////////////////////////////
const char* cCsvTable::AsStringByIndex(size_t index) const
{
    const cCsvRow* const row = CurRow();
    Assert(row);
    Assert(index < row->size());
    return row->AsString(index);
}

////////////////////////////////////////////////////////////////////////////////
/// \brief alias를 포함해 모든 데이터를 삭제한다.
////////////////////////////////////////////////////////////////////////////////
void cCsvTable::Destroy()
{
    m_File.Destroy();
    m_Alias.Destroy();
    m_CurRow = -1;
}

////////////////////////////////////////////////////////////////////////////////
/// \brief 현재 행을 반환한다.
/// \return const cCsvRow* 액세스가 가능한 현재 행이 존재하는 경우에는 그 행의
/// 포인터를 반환하고, 더 이상 액세스 가능한 행이 없는 경우에는 NULL을 
/// 반환한다.
////////////////////////////////////////////////////////////////////////////////
const cCsvRow* const cCsvTable::CurRow() const
{
    if (m_CurRow < 0)
    {
        Assert(false && "call Next() first!");
        return NULL;
    }
    else if (m_CurRow >= (int)m_File.GetRowCount())
    {
        Assert(false && "no more rows!");
        return NULL;
    }

    return m_File[m_CurRow];
}

