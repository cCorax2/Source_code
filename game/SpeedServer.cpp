#include "stdafx.h"
#include <time.h>
#include "SpeedServer.h"
#include "locale_service.h"

// 쾌도 서버 보너스 경험치 시스템
// by rtsummit

CSpeedServerManager::CSpeedServerManager()
{
}

CSpeedServerManager::~CSpeedServerManager()
{
}

CSpeedServerEmpireExp::CSpeedServerEmpireExp()
{
}

CSpeedServerEmpireExp::~CSpeedServerEmpireExp()
{
}

bool CSpeedServerManager::Initialize()
{
	for (int i = 1; i < EMPIRE_MAX_NUM; i++)
	{
		sys_log (0,"speed manager init");
		if(!Empire[i].Initialize (i))
		{
			sys_err ("EMPIRE %d Exp Bonus Manager Init fail",i);
			return false;
		}
	}
	return true;
}

bool CSpeedServerEmpireExp::Initialize (BYTE e)
{
	empire = e;
	sys_log (0, "empire exp init %d", empire);
	snprintf (file_name, sizeof(file_name), "%s/exp_bonus_table_%d.txt", LocaleService_GetBasePath().c_str(), empire);

	for (int i = 1; i < 6; i++)
	{
		wday_exp_table[i].push_back (HME (18, 0, 50));
		wday_exp_table[i].push_back (HME (24, 0, 100));
	}

	wday_exp_table[0].push_back (HME (18, 0, 100));
	wday_exp_table[0].push_back (HME (24, 0, 150));
	wday_exp_table[6].push_back (HME (18, 0, 100));
	wday_exp_table[6].push_back (HME (24, 0, 150));

	LoadExpTable();
	return true;
}

bool CSpeedServerEmpireExp::LoadWdayExpTable(int wday, char *str)
{
	std::list <HME> &lst = wday_exp_table[wday];
	lst.clear();
	char *p, *n;
	const char	*delim = " \t\r\n";
	char *t;
	char *h, *m, *e;
	int hour, min, exp;
	sys_log (0, "str %s", str);
	strtok (str, delim);
	p = strtok (NULL, ";");
	n = strtok (NULL, ";");
	while (p != NULL)
	{
		t = strtok (p, delim);
		e = strtok (NULL, delim);
		h = strtok (t, ":");
		m = strtok (NULL, delim);
		if (!str_to_number (hour, h) || !str_to_number (min, m) || !str_to_number (exp, e))
		{
			sys_log (0, "h m e : %s %s %s",h, m, e);
			sys_err ("Invalid argument. Please insert hh:mm exp");
			return false;
		}
			sys_log (0, "h m e : %s %s %s",h, m, e);
		
		lst.push_back (HME (hour, min, exp));
		p = strtok (n, ";");
		n = strtok (NULL, ";");
	}
	return true;
}

bool CSpeedServerManager::WriteExpTableOfEmpire(BYTE empire)
{
	return Empire[empire].WriteExpTable();
}

bool CSpeedServerEmpireExp::WriteExpTable()
{
	FILE *fp;

	sys_log (0, "write");
	if (0==file_name || 0==file_name[0])
		return false;

	if ((fp = fopen(file_name, "w"))==0)
	{
		return false;
	}

	char wday_name[7][4] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	for (int i = 0; i < 7; i++)
	{
		fprintf (fp, "%s", wday_name[i]);
		for (std::list <HME>::iterator it = wday_exp_table[i].begin(); it != wday_exp_table[i].end(); it++)
		{
			fprintf (fp, " %d:%d %d;", it->hour, it->min, it->exp);
		}
		fprintf(fp, "\n");
	}
	
	for (std::map <Date, std::list <HME> >::iterator holi_it = holiday_map.begin(); holi_it != holiday_map.end(); holi_it++)
	{
		fprintf (fp, "HOLIDAY %d.%d.%d", holi_it->first.year + 1900, holi_it->first.mon + 1, holi_it->first.day);
		for (std::list <HME>::iterator it = holi_it->second.begin(); it != holi_it->second.end(); it++)
		{
			fprintf (fp, " %d:%d %d;", it->hour, it->min, it->exp);
		}
		fprintf(fp, "\n");
	}
	fclose (fp);
	return true;
}

bool CSpeedServerEmpireExp::LoadExpTable()
{
	FILE	*fp;
	char	one_line[256];
	char	temp[256];
	const char	*delim = " \t\r\n";

	sys_log (0, "load");
	if (0==file_name || 0==file_name[0])
		return false;

	if ((fp = fopen(file_name, "r"))==0)
		return false;

	while (fgets(one_line, 256, fp))
	{
		if (one_line[0]=='#')
			continue;

		strcpy(temp, one_line);

		const char* token_string = strtok(one_line, delim);

		if (NULL==token_string)
			continue;

		TOKEN("SUN")
		{
			LoadWdayExpTable (0, temp);
		}
		else TOKEN("MON")
		{
			LoadWdayExpTable (1, temp);
		}
		else TOKEN("TUE")
		{
			LoadWdayExpTable (2, temp);
		}
		else TOKEN("WED")
		{
			LoadWdayExpTable (3, temp);
		}
		else TOKEN("THU")
		{
			LoadWdayExpTable (4, temp);
		}
		else TOKEN("FRI")
		{
			LoadWdayExpTable (5, temp);
		}
		else TOKEN("SAT")
		{
			LoadWdayExpTable (6, temp);
		}
		else TOKEN("HOLIDAY")
		{
			std::list <HME> lst;
			lst.clear();
			char *p, *n;
			char *t, *v;
			char *h, *m, *e;
			int hour, min, exp;

			v = strtok (temp, delim);
			v = strtok (NULL, delim);
			sys_log (0, "holiday %s", v);

			p = strtok (NULL, ";");
			n = strtok (NULL, ";");
			while (p != NULL)
			{
				t = strtok (p, delim);
				e = strtok (NULL, delim);
				h = strtok (t, ":");
				m = strtok (NULL, delim);
				if (!str_to_number (hour, h) || !str_to_number (min, m) || !str_to_number (exp, e))
				{
					sys_log (0, "h m e : %s %s %s",h, m, e);
					sys_err ("Invalid argument. Please insert hh:mm exp");
					return false;
				}
				sys_log (0, "h m e : %s %s %s",h, m, e);

				lst.push_back (HME (hour, min, exp));
				p = strtok (n, ";");
				n = strtok (NULL, ";");
			}
			int year, mon, day;
			if (!str_to_number (year, strtok (v, "."))
					|| !str_to_number ( mon, strtok (NULL, "."))
					|| !str_to_number ( day, strtok (NULL, ".")))
			{
				sys_err ("Invalid Date");
				return false;
			}

			sys_log (0, "y m d %d %d %d",year, mon, day);

			holiday_map.insert (std::pair <Date, std::list <HME> > (Date (year - 1900, mon - 1, day), lst));
		}
	}

	fclose(fp);

	return true;
}

std::list <HME>& CSpeedServerManager::GetWdayExpTableOfEmpire(BYTE empire, int wday)
{
	return Empire[empire].GetWdayExpTable(wday);
}

std::list <HME>& CSpeedServerEmpireExp::GetWdayExpTable(int wday)
{
	return wday_exp_table[wday];
}

void CSpeedServerManager::SetWdayExpTableOfEmpire (BYTE empire, int wday, HME hme)
{
	Empire[empire].SetWdayExpTable (wday, hme);
}

void CSpeedServerEmpireExp::SetWdayExpTable (int wday, HME hme)
{
	wday_exp_table[wday].push_back (hme);
	WriteExpTable();
}

void CSpeedServerManager::InitWdayExpTableOfEmpire (BYTE empire, int wday)
{
	if (empire > EMPIRE_MAX_NUM)
	{
		sys_err ("invalid empire");
		return;
	}

	Empire[empire].InitWdayExpTable (wday);
}

void CSpeedServerEmpireExp::InitWdayExpTable(int wday)
{
	wday_exp_table[wday].clear();
}


std::list <HME>& CSpeedServerManager::GetHolidayExpTableOfEmpire(BYTE empire, Date date, bool &is_exist)
{
	return Empire[empire].GetHolidayExpTable(date, is_exist);
}

std::list <HME>& CSpeedServerEmpireExp::GetHolidayExpTable(Date date, bool &is_exist)
{
	std::map <Date, std::list <HME> >::iterator it = holiday_map.find(date);
	if (it != holiday_map.end())
	{
		is_exist = true;
		return it->second;
	}
	else
	{
		is_exist = false;
		sys_err ("Cannot find Holiday %d %d %d",date.year, date.mon, date.day);
	}
	return it->second;
}

void CSpeedServerManager::SetHolidayExpTableOfEmpire (BYTE empire, Date date, HME hme)
{
	Empire[empire].SetHolidayExpTable (date, hme);
}

void CSpeedServerEmpireExp::SetHolidayExpTable (Date date, HME hme)
{
	std::map <Date, std::list <HME> >::iterator it = holiday_map.find(date);
	if (it != holiday_map.end())
	{
		it->second.push_back (hme);
	}
	WriteExpTable();
}

void CSpeedServerManager::InitHolidayExpTableOfEmpire (BYTE empire, Date date)
{
	if (empire > EMPIRE_MAX_NUM)
	{
		sys_err ("invalid empire");
		return;
	}

	Empire[empire].InitHolidayExpTable (date);
}

void CSpeedServerEmpireExp::InitHolidayExpTable(Date date)
{
	sys_log (0, "init holiday");
	std::map <Date, std::list <HME> >::iterator it = holiday_map.find(date);
	if (it == holiday_map.end())
	{
		std::list <HME> lst;
		holiday_map.insert (std::pair <Date, std::list <HME> > (date, lst));
	}
	else
	{
		it->second.clear();
	}
}

HME CSpeedServerManager::GetCurrentExpPrivOfEmpire (BYTE empire, int &duration, bool &is_change)
{
	return Empire[empire].GetCurrentExpPriv (duration, is_change);
}

HME CSpeedServerEmpireExp::GetCurrentExpPriv(int &duration, bool &is_change)
{
	struct tm* datetime;
	time_t t;
	t = time(NULL);
	datetime = localtime(&t);

	Date date (datetime -> tm_year, datetime -> tm_mon, 
			datetime -> tm_mday);

	std::map <Date, std::list <HME> >::iterator holi_it = holiday_map.find(date);

	int total_sec = datetime->tm_hour * 3600 + datetime->tm_min * 60 + datetime->tm_sec;

	HME hme;

	// 현재 날짜가 holiday이면 holiday bonus를 도입한다.
	if (holi_it != holiday_map.end())
	{
		for (std::list <HME>::iterator it = holi_it->second.begin();
				it != wday_exp_table[datetime->tm_wday].end(); it++)
		{
			// 현재 시각이 시간 구간 안에 포함되면,
			if (total_sec < (it->hour * 3600 + it->min * 60 ))
			{
				hme = *it;
				break;
			}
		}
	}
	else
	{
		for (std::list <HME>::iterator it =  wday_exp_table[datetime->tm_wday].begin();
				it != wday_exp_table[datetime->tm_wday].end(); it++)
		{
			// 현재 시각이 시간 구간 안에 포함되면,
			if (total_sec < (it->hour * 3600 + it->min * 60 ))
			{
				hme = *it;
				break;
			}
		}
	}

	duration = hme.hour * 3600 + hme.min * 60 - total_sec;
	
	is_change = !(hme == current_hme);
	current_hme = hme;

	return hme;

}
