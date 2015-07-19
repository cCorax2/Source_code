
#include "stdafx.h"
#include "locale_service.h"

typedef std::map< std::string, std::string > LocaleStringMapType;

LocaleStringMapType localeString;

int g_iUseLocale = 0;

void locale_add(const char **strings)
{
	LocaleStringMapType::const_iterator iter = localeString.find( strings[0] );

	if( iter == localeString.end() )
	{
		localeString.insert( std::make_pair( strings[0], strings[1] ) );
	}
}

const char * locale_find(const char *string)
{
	if (0 == g_iUseLocale || LC_IsKorea() || LC_IsWE_Korea())
	{
		return (string);
	}

	LocaleStringMapType::const_iterator iter = localeString.find( string );

	if( iter == localeString.end() )
	{
		static char s_line[1024] = "@0949";
		strlcpy(s_line + 5, string, sizeof(s_line) - 5);

		sys_err("LOCALE_ERROR: \"%s\";", string);
		return s_line;
	}

	return iter->second.c_str();
}

const char *quote_find_end(const char *string)
{
	const char  *tmp = string;
	int         quote = 0;

	while (*tmp)
	{
		if (quote && *tmp == '\\' && *(tmp + 1))
		{
			// \ 다음 문자가 " 면 스킵한다.
			switch (*(tmp + 1))
			{
				case '"':
					tmp += 2;
					continue;
			}
		}
		else if (*tmp == '"')
		{
			quote = !quote;
		}
		else if (!quote && *tmp == ';')
			return (tmp);

		tmp++;
	}

	return (NULL);
}

char *locale_convert(const char *src, int len)
{
	const char	*tmp;
	int		i, j;
	char	*buf, *dest;
	int		start = 0;
	char	last_char = 0;

	if (!len)
		return NULL;

	buf = M2_NEW char[len + 1];

	for (j = i = 0, tmp = src, dest = buf; i < len; i++, tmp++)
	{
		if (*tmp == '"')
		{
			if (last_char != '\\')
				start = !start;
			else
				goto ENCODE;
		}
		else if (*tmp == ';')
		{
			if (last_char != '\\' && !start)
				break;
			else
				goto ENCODE;
		}
		else if (start)
		{
ENCODE:
			if (*tmp == '\\' && *(tmp + 1) == 'n')
			{
				*(dest++) = '\n';
				tmp++;
				last_char = '\n';
			}
			else
			{
				*(dest++) = *tmp;
				last_char = *tmp;
			}

			j++;
		}
	}

	if (!j)
	{
		M2_DELETE_ARRAY(buf);
		return NULL;
	}

	*dest = '\0';
	return (buf);
}

#define NUM_LOCALES 2

void locale_init(const char *filename)
{
	FILE        *fp = fopen(filename, "rb");
	char        *buf;

	if (!fp) return;

	fseek(fp, 0L, SEEK_END);
	int i = ftell(fp); 
	fseek(fp, 0L, SEEK_SET);

	i++;

	buf = M2_NEW char[i];

	memset(buf, 0, i);

	fread(buf, i - 1, sizeof(char), fp);

	fclose(fp);

	const char * tmp;
	const char * end;

	char *	strings[NUM_LOCALES];

	if (!buf)
	{
		sys_err("locale_read: no file %s", filename);
		exit(1);
	}

	tmp = buf;

	do
	{
		for (i = 0; i < NUM_LOCALES; i++)
			strings[i] = NULL;

		if (*tmp == '"')
		{
			for (i = 0; i < NUM_LOCALES; i++)
			{
				if (!(end = quote_find_end(tmp)))
					break;

				strings[i] = locale_convert(tmp, end - tmp);
				tmp = ++end;

				while (*tmp == '\n' || *tmp == '\r' || *tmp == ' ') tmp++;

				if (i + 1 == NUM_LOCALES)
					break;

				if (*tmp != '"')
				{
					sys_err("locale_init: invalid format filename %s", filename);
					break;
				}
			}

			if (strings[0] == NULL || strings[1] == NULL)
				break;

			locale_add((const char**)strings);

			for (i = 0; i < NUM_LOCALES; i++)
				if (strings[i])
					M2_DELETE_ARRAY(strings[i]);
		}
		else
		{
			tmp = strchr(tmp, '\n');

			if (tmp)
				tmp++;
		}
	}
	while (tmp && *tmp);

	M2_DELETE_ARRAY(buf);
}

