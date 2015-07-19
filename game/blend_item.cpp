/*********************************************************************
 * date        : 2007.02.24
 * file        : blend_item.cpp
 * author      : mhh
 * description : 
 */

#define _blend_item_cpp_

#include "stdafx.h"
#include "constants.h"
#include "log.h"
#include "dev_log.h"
#include "locale_service.h"
#include "item.h"
#include "blend_item.h"

#define DO_ALL_BLEND_INFO(iter)	for (iter=s_blend_info.begin(); iter!=s_blend_info.end(); ++iter)


struct BLEND_ITEM_INFO
{
	DWORD	item_vnum;
	int		apply_type;
	int		apply_value[MAX_BLEND_ITEM_VALUE];
	int		apply_duration[MAX_BLEND_ITEM_VALUE];
};


typedef std::vector<BLEND_ITEM_INFO*>	T_BLEND_ITEM_INFO;
T_BLEND_ITEM_INFO	s_blend_info;

bool	Blend_Item_init()
{
	BLEND_ITEM_INFO	*blend_item_info = NULL;
	T_BLEND_ITEM_INFO::iterator			iter;
	char	file_name[256];
	snprintf (file_name, sizeof(file_name), "%s/blend.txt", LocaleService_GetBasePath().c_str());

	sys_log(0, "Blend_Item_init %s ", file_name);

	DO_ALL_BLEND_INFO(iter)
	{
		blend_item_info = *iter;
		M2_DELETE(blend_item_info);
	}
	s_blend_info.clear();

	if (false==Blend_Item_load(file_name))
	{
		sys_err("<Blend_Item_init> fail");
		return false;
	}
	return true;
}

bool	Blend_Item_load(char *file)
{
	FILE	*fp;
	char	one_line[256];
	const char	*delim = " \t\r\n";
	char	*v;

	BLEND_ITEM_INFO	*blend_item_info;

	if (0==file || 0==file[0])
		return false;

	if ((fp = fopen(file, "r"))==0)
		return false;

	while (fgets(one_line, 256, fp))
	{
		if (one_line[0]=='#')
			continue;

		const char* token_string = strtok(one_line, delim);

		if (NULL==token_string)
			continue;

		TOKEN("section")
		{
			blend_item_info = M2_NEW BLEND_ITEM_INFO;
			memset(blend_item_info, 0x00, sizeof(BLEND_ITEM_INFO));
		}
		else TOKEN("item_vnum")
		{
			v = strtok(NULL, delim);

			if (NULL==v)
			{
				fclose(fp);
				return false;
			}

			str_to_number(blend_item_info->item_vnum, v);
		}
		else TOKEN("apply_type")
		{
			v = strtok(NULL, delim);

			if (NULL==v)
			{
				fclose(fp);
				return false;
			}

			if (0 == (blend_item_info->apply_type = FN_get_apply_type(v)))
			{
				sys_err ("Invalid apply_type(%s)", v);
				return false;
			}
		}
		else TOKEN("apply_value")
		{
			for (int i=0; i<MAX_BLEND_ITEM_VALUE; ++i)
			{
				v = strtok(NULL, delim);

				if (NULL==v)
				{
					fclose(fp);
					return false;
				}

				str_to_number(blend_item_info->apply_value[i], v); 
			}
		}
		else TOKEN("apply_duration")
		{
			for (int i=0; i<MAX_BLEND_ITEM_VALUE; ++i)
			{
				v = strtok(NULL, delim);

				if (NULL==v)
				{
					fclose(fp);
					return false;
				}

				str_to_number(blend_item_info->apply_duration[i], v);
			}
		}
		else TOKEN("end")
		{
			s_blend_info.push_back(blend_item_info);
		}
	}

	fclose(fp);

	return true;
}

static int FN_random_index()
{
	int	percent = number(1,100);

	if (percent<=10)			// level 1 :10%
		return 0;
	else if (percent<=30)		// level 2 : 20%
		return 1;
	else if (percent<=70)		// level 3 : 40%
		return 2;
	else if (percent<=90)		// level 4 : 20%
		return 3;
	else
		return 4;				// level 5 : 10%

	return 0;
}

// 충기환의 확률 테이블
// blend.txt에서 확률도 받도록 고치면 깔끔하겠지만
// 각 나라별로 item proto 등을 따로 관리하므로,
// 혼란이 올 수 있어 이렇게 추가한다.
// by rtsummit

static int FN_ECS_random_index()
{
	int	percent = number(1,100);

	if (percent<=5)			// level 1 : 5%
		return 0;
	else if (percent<=15)		// level 2 : 10%
		return 1;
	else if (percent<=60)		// level 3 : 45%
		return 2;
	else if (percent<=85)		// level 4 : 25%
		return 3;
	else
		return 4;				// level 5 : 15%

	return 0;
}


bool	Blend_Item_set_value(LPITEM item)
{
	BLEND_ITEM_INFO	*blend_info;
	T_BLEND_ITEM_INFO::iterator	iter;

	DO_ALL_BLEND_INFO(iter)
	{
		blend_info = *iter;
		if (blend_info->item_vnum == item->GetVnum())
		{
			int	apply_type;
			int	apply_value;
			int	apply_duration;
	
			if (item->GetVnum() == 51002)
			{
				apply_type		= blend_info->apply_type;
				apply_value		= blend_info->apply_value		[FN_ECS_random_index()];
				apply_duration	= blend_info->apply_duration	[FN_ECS_random_index()];
			}
			else
			{
				apply_type		= blend_info->apply_type;
				apply_value		= blend_info->apply_value		[FN_random_index()];
				apply_duration	= blend_info->apply_duration	[FN_random_index()];
			}
			sys_log (0, "blend_item : type : %d, value : %d, du : %d", apply_type, apply_value, apply_duration);
			item->SetSocket(0, apply_type);
			item->SetSocket(1, apply_value);
			item->SetSocket(2, apply_duration);
			return true;
		}

	}
	return false;
}

bool	Blend_Item_find(DWORD item_vnum)
{
	BLEND_ITEM_INFO	*blend_info;
	T_BLEND_ITEM_INFO::iterator	iter;

	DO_ALL_BLEND_INFO(iter)
	{
		blend_info = *iter;
		if (blend_info->item_vnum == item_vnum)
			return true;
	}
	return false;
}

