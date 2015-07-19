#ifndef __INC_SPAM_MANAGER_H__
#define __INC_SPAM_MANAGER_H__

#include <string.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <utility>

#include "../../common/singleton.h"
#include "utils.h"

class SpamManager : public singleton<SpamManager>
{
	public:
		inline const char * GetSpamScore(const char * src, size_t len, unsigned int & score)
		{
			const char * word = NULL;
			score = 0;

			std::string strOrig(src);
			strOrig.erase( remove_if(strOrig.begin(), strOrig.end(), isspace), strOrig.end() );

			for (size_t i = 0; i < m_vec_word.size(); ++i)
			{
				std::pair<std::string, unsigned int> & r = m_vec_word[i];

				if (true == WildCaseCmp(r.first.c_str(), strOrig.c_str()))
				{
					word = r.first.c_str();
					score += r.second;
				}
			}

			return word;
		}

		inline void Clear()
		{
			m_vec_word.clear();
		}

		inline void Insert(const char * str, unsigned int score = 10)
		{
			m_vec_word.push_back(std::make_pair(str, score));
			sys_log(0, "SPAM: %2d %s", score, str);
		}

	private:
		std::vector< std::pair<std::string, unsigned int> > m_vec_word;
};

#endif
