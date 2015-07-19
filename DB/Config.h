#ifndef __INC_CONFIG_H__
#define __INC_CONFIG_H__

typedef std::map<std::string, std::string> TValueMap;

class CConfig : public singleton<CConfig>
{
	public:
		CConfig();
		~CConfig();

		bool LoadFile(const char* filename);
		bool GetValue(const char* key, int* dest);
		bool GetValue(const char* key, float* dest);
		bool GetValue(const char* key, DWORD* dest);
		bool GetValue(const char* key, BYTE* dest);
		bool GetValue(const char* key, char* dest, size_t destSize);
		bool GetWord(FILE* fp, char* dest);
		bool GetLine(FILE* fp, char* dest);
		bool GetTwoValue(const char* key, DWORD * dest1, DWORD *dest2);
		void NextLine(FILE* fp);

	private:
		void Destroy();
		bool GetParam(const char*key,int index, DWORD *Param);

		const char *	Get(const char* key);
		std::string *	Search(const char* key);

	private:
		TValueMap	m_valueMap;
};

#endif
