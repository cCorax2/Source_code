#ifndef __INC_METIN_II_GAME_SPEEDSERVER_H__
#define __INC_METIN_II_GAME_SPEEDSERVER_H__

#include "../../common/length.h"
#include <list>

// castle.cpp 에 있는 것을 복붙 하였다
#define EMPIRE_NONE		0	// 아무국가 아님
#define EMPIRE_RED		1	// 신수
#define EMPIRE_YELLOW	2	// 천조
#define EMPIRE_BLUE		3	// 진노

class HME
{
	public :
		int hour;
		int min;
		int exp;

		HME (int h=0, int m=0, int e=0){
			hour =  h; min = m; 
			exp = e;
		}

		HME& operator=(const HME &rhs)
		{
			hour = rhs.hour;
			min = rhs.min;
			exp = rhs.exp;
			return *this;
		}

		bool operator==(const HME &rhs) const
		{
			return hour == rhs.hour
				&& min == rhs.min
				&& exp == rhs.exp;
		}

		bool operator<(const HME &rhs) const
		{
			return (hour<rhs.hour)
				|| (hour==rhs.hour) && (min<rhs.min);
		}
};

class Date
{
	public : 
		int year;
		int mon;
		int day;

		Date (int y = 0, int m = 0, int d = 0)
		{
			year = y; mon = m; day = d;
		}

		bool operator==(const Date &rhs) const
		{
			return year == rhs.year
				&& mon == rhs.mon
				&& day == rhs.day;
		}
		bool operator<(const Date &rhs) const
		{
			return (year<rhs.year)
				|| (year==rhs.year) && (mon<rhs.mon)
				|| (year==rhs.year) && (mon==rhs.mon) && (day<rhs.day);
		}
};

class CSpeedServerEmpireExp
{
	public :
		CSpeedServerEmpireExp();
		~CSpeedServerEmpireExp();

		bool Initialize (BYTE empire);
		
		std::list <HME>& GetWdayExpTable(int wday);
		void SetWdayExpTable(int wday, HME hme);

		std::list <HME>& GetHolidayExpTable(Date date, bool &is_exist);
		void SetHolidayExpTable(Date date, HME hme);

		void InitWdayExpTable(int wday);
		void InitHolidayExpTable(Date date);
		HME GetCurrentExpPriv (int &duration, bool &is_change);

		bool WriteExpTable();

	private :
		bool LoadExpTable ();
		bool LoadWdayExpTable (int wday, char *str);

		BYTE empire;
		char file_name [256];
		HME current_hme;
		std::map <Date, std::list <HME> > holiday_map;
		std::list <HME> wday_exp_table[7];
};

class CSpeedServerManager : public singleton<CSpeedServerManager>
{
	public:
		CSpeedServerManager();
		~CSpeedServerManager();

		bool Initialize ();

		std::list <HME>& GetWdayExpTableOfEmpire (BYTE empire, int wday);
		void SetWdayExpTableOfEmpire (BYTE empire, int wday, HME hme);
		void InitWdayExpTableOfEmpire (BYTE empire, int wday);
		
		std::list <HME>& GetHolidayExpTableOfEmpire (BYTE empire, Date date, bool &is_exist);
		void SetHolidayExpTableOfEmpire (BYTE empire, Date date, HME hme);
		void InitHolidayExpTableOfEmpire (BYTE empire, Date date);

		bool WriteExpTableOfEmpire (BYTE empire);
		
		HME GetCurrentExpPrivOfEmpire (BYTE empire, int &duration, bool &is_change);

	private:
		CSpeedServerEmpireExp Empire[EMPIRE_MAX_NUM];
		
};

#endif
