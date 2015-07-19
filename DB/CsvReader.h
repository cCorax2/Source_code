#ifndef __CSVFILE_H__
#define __CSVFILE_H__

#include <string>
#include <vector>

#if _MSC_VER
    #include <hash_map>
#else
    #include <map>
#endif

////////////////////////////////////////////////////////////////////////////////
/// \class cCsvAlias
/// \brief CSV 파일을 수정했을 때 발생하는 인덱스 문제를 줄이기 위한 
/// 별명 객체.
///
/// 예를 들어 0번 컬럼이 A에 관한 내용을 포함하고, 1번 컬럼이 B에 관한 내용을 
/// 포함하고 있었는데...
///
/// <pre>
/// int a = row.AsInt(0);
/// int b = row.AsInt(1);
/// </pre>
///
/// 그 사이에 C에 관한 내용을 포함하는 컬럼이 끼어든 경우, 하드코딩되어 있는 
/// 1번을 찾아서 고쳐야 하는데, 상당히 에러가 발생하기 쉬운 작업이다. 
///
/// <pre>
/// int a = row.AsInt(0);
/// int c = row.AsInt(1);
/// int b = row.AsInt(2); <-- 이 부분을 일일이 신경써야 한다.
/// </pre>
/// 
/// 이 부분을 문자열로 처리하면 유지보수에 들어가는 수고를 약간이나마 줄일 수 
/// 있다.
////////////////////////////////////////////////////////////////////////////////

class cCsvAlias
{
private:
#if _MSC_VER
    typedef stdext::hash_map<std::string, size_t> NAME2INDEX_MAP;
    typedef stdext::hash_map<size_t, std::string> INDEX2NAME_MAP;
#else
    typedef std::map<std::string, size_t> NAME2INDEX_MAP;
    typedef std::map<size_t, std::string> INDEX2NAME_MAP;
#endif

    NAME2INDEX_MAP m_Name2Index;  ///< 셀 인덱스 대신으로 사용하기 위한 이름들
    INDEX2NAME_MAP m_Index2Name;  ///< 잘못된 alias를 검사하기 위한 추가적인 맵


public:
    /// \brief 생성자
    cCsvAlias() {} 

    /// \brief 소멸자
    virtual ~cCsvAlias() {}


public:
    /// \brief 셀을 액세스할 때, 숫자 대신 사용할 이름을 등록한다.
    void AddAlias(const char* name, size_t index);

    /// \brief 모든 데이터를 삭제한다.
    void Destroy();

    /// \brief 숫자 인덱스를 이름으로 변환한다.
    const char* operator [] (size_t index) const;

    /// \brief 이름을 숫자 인덱스로 변환한다.
    size_t operator [] (const char* name) const;


private:
    /// \brief 복사 생성자 금지
    cCsvAlias(const cCsvAlias&) {}

    /// \brief 대입 연산자 금지
    const cCsvAlias& operator = (const cCsvAlias&) { return *this; }
};


////////////////////////////////////////////////////////////////////////////////
/// \class cCsvRow 
/// \brief CSV 파일의 한 행을 캡슐화한 클래스
///
/// CSV의 기본 포맷은 엑셀에서 보이는 하나의 셀을 ',' 문자로 구분한 것이다.
/// 하지만, 셀 안에 특수 문자로 쓰이는 ',' 문자나 '"' 문자가 들어갈 경우, 
/// 모양이 약간 이상하게 변한다. 다음은 그 변화의 예이다.
/// 
/// <pre>
/// 엑셀에서 보이는 모양 | 실제 CSV 파일에 들어가있는 모양
/// ---------------------+----------------------------------------------------
/// ItemPrice            | ItemPrice
/// Item,Price           | "Item,Price"
/// Item"Price           | "Item""Price"
/// "ItemPrice"          | """ItemPrice"""
/// "Item,Price"         | """Item,Price"""
/// Item",Price          | "Item"",Price"
/// </pre>
/// 
/// 이 예로서 다음과 같은 사항을 알 수 있다.
/// - 셀 내부에 ',' 또는 '"' 문자가 들어갈 경우, 셀 좌우에 '"' 문자가 생긴다.
/// - 셀 내부의 '"' 문자는 2개로 치환된다.
///
/// \sa cCsvFile
////////////////////////////////////////////////////////////////////////////////

class cCsvRow : public std::vector<std::string>
{
public:
    /// \brief 기본 생성자
    cCsvRow() {}

    /// \brief 소멸자
    ~cCsvRow() {}


public:
    /// \brief 해당 셀의 데이터를 int 형으로 반환한다.
    int AsInt(size_t index) const { return atoi(at(index).c_str()); }

    /// \brief 해당 셀의 데이터를 double 형으로 반환한다.
    double AsDouble(size_t index) const { return atof(at(index).c_str()); }

    /// \brief 해당 셀의 데이터를 문자열로 반환한다.
    const char* AsString(size_t index) const { return at(index).c_str(); }

    /// \brief 해당하는 이름의 셀 데이터를 int 형으로 반환한다.
    int AsInt(const char* name, const cCsvAlias& alias) const {
        return atoi( at(alias[name]).c_str() ); 
    }

    /// \brief 해당하는 이름의 셀 데이터를 int 형으로 반환한다.
    double AsDouble(const char* name, const cCsvAlias& alias) const {
        return atof( at(alias[name]).c_str() ); 
    }

    /// \brief 해당하는 이름의 셀 데이터를 문자열로 반환한다.
    const char* AsString(const char* name, const cCsvAlias& alias) const { 
        return at(alias[name]).c_str(); 
    }


private:
    /// \brief 복사 생성자 금지
    cCsvRow(const cCsvRow&) {}

    /// \brief 대입 연산자 금지
    const cCsvRow& operator = (const cCsvRow&) { return *this; }
};


////////////////////////////////////////////////////////////////////////////////
/// \class cCsvFile
/// \brief CSV(Comma Seperated Values) 파일을 read/write하기 위한 클래스
///
/// <b>sample</b>
/// <pre>
/// cCsvFile file;
///
/// cCsvRow row1, row2, row3;
/// row1.push_back("ItemPrice");
/// row1.push_back("Item,Price");
/// row1.push_back("Item\"Price");
///
/// row2.reserve(3);
/// row2[0] = "\"ItemPrice\"";
/// row2[1] = "\"Item,Price\"";
/// row2[2] = "Item\",Price\"";
///
/// row3 = "\"ItemPrice\"\"Item,Price\"Item\",Price\"";
///
/// file.add(row1);
/// file.add(row2);
/// file.add(row3);
/// file.save("test.csv", false);
/// </pre>
///
/// \todo 파일에서만 읽어들일 것이 아니라, 메모리 소스로부터 읽는 함수도 
/// 있어야 할 듯 하다.
////////////////////////////////////////////////////////////////////////////////

class cCsvFile
{
private:
    typedef std::vector<cCsvRow*> ROWS;

    ROWS m_Rows; ///< 행 컬렉션


public:
    /// \brief 생성자
    cCsvFile() {}

    /// \brief 소멸자
    virtual ~cCsvFile() { Destroy(); }


public:
    /// \brief 지정된 이름의 CSV 파일을 로드한다.
    bool Load(const char* fileName, const char seperator=',', const char quote='"');

    /// \brief 가지고 있는 내용을 CSV 파일에다 저장한다.
    bool Save(const char* fileName, bool append=false, char seperator=',', char quote='"') const;

    /// \brief 모든 데이터를 메모리에서 삭제한다.
    void Destroy();

    /// \brief 해당하는 인덱스의 행을 반환한다.
    cCsvRow* operator [] (size_t index);

    /// \brief 해당하는 인덱스의 행을 반환한다.
    const cCsvRow* operator [] (size_t index) const;

    /// \brief 행의 갯수를 반환한다.
    size_t GetRowCount() const { return m_Rows.size(); }


private:
    /// \brief 복사 생성자 금지
    cCsvFile(const cCsvFile&) {}

    /// \brief 대입 연산자 금지
    const cCsvFile& operator = (const cCsvFile&) { return *this; }
};


////////////////////////////////////////////////////////////////////////////////
/// \class cCsvTable
/// \brief CSV 파일을 이용해 테이블 데이터를 로드하는 경우가 많은데, 이 클래스는 
/// 그 작업을 좀 더 쉽게 하기 위해 만든 유틸리티 클래스다.
///
/// CSV 파일을 로드하는 경우, 숫자를 이용해 셀을 액세스해야 하는데, CSV 
/// 파일의 포맷이 바뀌는 경우, 이 숫자들을 변경해줘야한다. 이 작업이 꽤 
/// 신경 집중을 요구하는 데다가, 에러가 발생하기 쉽다. 그러므로 숫자로 
/// 액세스하기보다는 문자열로 액세스하는 것이 약간 느리지만 낫다고 할 수 있다.
///
/// <b>sample</b>
/// <pre>
/// cCsvTable table;
///
/// table.alias(0, "ItemClass");
/// table.alias(1, "ItemType");
///
/// if (table.load("test.csv"))
/// {
///     while (table.next())
///     {
///         std::string item_class = table.AsString("ItemClass");
///         int         item_type  = table.AsInt("ItemType"); 
///     }
/// }
/// </pre>
////////////////////////////////////////////////////////////////////////////////

class cCsvTable
{
public :
    cCsvFile  m_File;   ///< CSV 파일 객체
private:
    cCsvAlias m_Alias;  ///< 문자열을 셀 인덱스로 변환하기 위한 객체
    int       m_CurRow; ///< 현재 횡단 중인 행 번호


public:
    /// \brief 생성자
    cCsvTable();

    /// \brief 소멸자
    virtual ~cCsvTable();


public:
    /// \brief 지정된 이름의 CSV 파일을 로드한다.
    bool Load(const char* fileName, const char seperator=',', const char quote='"');

    /// \brief 셀을 액세스할 때, 숫자 대신 사용할 이름을 등록한다.
    void AddAlias(const char* name, size_t index) { m_Alias.AddAlias(name, index); }

    /// \brief 다음 행으로 넘어간다.
    bool Next();

    /// \brief 현재 행의 셀 숫자를 반환한다.
    size_t ColCount() const;

    /// \brief 인덱스를 이용해 int 형으로 셀값을 반환한다.
    int AsInt(size_t index) const;

    /// \brief 인덱스를 이용해 double 형으로 셀값을 반환한다.
    double AsDouble(size_t index) const;

    /// \brief 인덱스를 이용해 std::string 형으로 셀값을 반환한다.
    const char* AsStringByIndex(size_t index) const;

    /// \brief 셀 이름을 이용해 int 형으로 셀값을 반환한다.
    int AsInt(const char* name) const { return AsInt(m_Alias[name]); }

    /// \brief 셀 이름을 이용해 double 형으로 셀값을 반환한다.
    double AsDouble(const char* name) const { return AsDouble(m_Alias[name]); }

    /// \brief 셀 이름을 이용해 std::string 형으로 셀값을 반환한다.
    const char* AsString(const char* name) const { return AsStringByIndex(m_Alias[name]); }

    /// \brief alias를 포함해 모든 데이터를 삭제한다.
    void Destroy();


private:
    /// \brief 현재 행을 반환한다.
    const cCsvRow* const CurRow() const;

    /// \brief 복사 생성자 금지
    cCsvTable(const cCsvTable&) {}

    /// \brief 대입 연산자 금지
    const cCsvTable& operator = (const cCsvTable&) { return *this; }
};

#endif //__CSVFILE_H__
