#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "locale_service.h"
#include "log.h"
#include "db.h"
#include "pcbang.h"

CPCBangManager::CPCBangManager()
{
	m_minSavablePlayTime = 10;
	m_map_ip.clear();
}

void CPCBangManager::InsertIP(const char* c_szID, const char* c_szIP)
{
	PCBang_IP ip = __GetIPFromString(c_szIP);
	PCBang_ID id = __GetIDFromString(c_szID);

	m_map_ip.insert(std::map<PCBang_IP, PCBang_ID>::value_type(ip, id));
}

bool CPCBangManager::Log(const char* c_szIP, DWORD pid, time_t playTime)
{
	if (playTime < m_minSavablePlayTime)
	{
		return false;
	}

	std::map<PCBang_IP, PCBang_ID>::iterator f = m_map_ip.find(__GetIPFromString(c_szIP));

	if (m_map_ip.end() == f)
	{
		return false;
	}

	LogManager::instance().PCBangLoginLog(f->second, c_szIP, pid, playTime); 

	return true;
}

void CPCBangManager::RequestUpdateIPList(PCBang_ID id)
{
	if ( LC_IsYMIR() == true || LC_IsKorea() == true )
	{
		if ( id == 0 )
		{
			DBManager::instance().ReturnQuery(QID_PCBANG_IP_LIST_CHECK, 0, NULL, "show table status");
		}
		else
		{
			DBManager::instance().ReturnQuery(QID_PCBANG_IP_LIST_SELECT, 0, NULL, "SELECT pcbang_id, ip FROM pcbang_ip WHERE pcbang_id=%u", id);
		}
	}
}

bool CPCBangManager::IsPCBangIP(const char* c_szIP)
{
	if (!c_szIP)
		return false;

	PCBang_IP ip = __GetIPFromString(c_szIP);
	std::map<PCBang_IP, PCBang_ID>::iterator f = m_map_ip.find(ip);

	if (m_map_ip.end() == f)
		return false;

	return true;
}

PCBang_ID CPCBangManager::__GetIDFromString(const char* c_szID)
{
	PCBang_ID id = 0;
	str_to_number(id, c_szID);
	return id;
}

PCBang_IP CPCBangManager::__GetIPFromString(const char* c_szIP)
{
	int nums[4];
	sscanf(c_szIP, "%d.%d.%d.%d", nums+0, nums+1, nums+2, nums+3);

	return (nums[0]<<24)|(nums[1]<<16)|(nums[2]<<8)|nums[3];
}

