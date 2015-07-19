#include "stdafx.h"
#include "../../common/stl.h"
#include "text_file_loader.h"

CDynamicPool<CTextFileLoader::TGroupNode> CTextFileLoader::ms_groupNodePool;

void CTextFileLoader::DestroySystem()
{
	ms_groupNodePool.Clear();
}

CTextFileLoader::CTextFileLoader()
	: m_dwcurLineIndex(0), mc_pData(NULL)
{
	SetTop();

	m_globalNode.strGroupName = "global";
	m_globalNode.pParentNode = NULL;
}

CTextFileLoader::~CTextFileLoader()
{	
}

const char * CTextFileLoader::GetFileName()
{
	return m_strFileName.c_str();
}

bool CTextFileLoader::Load(const char * c_szFileName)
{
	m_strFileName = c_szFileName;

	m_dwcurLineIndex = 0;

	FILE* fp = fopen(c_szFileName, "rb");

	if (NULL == fp)
		return false;

	fseek(fp, 0L, SEEK_END);
	const size_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char * pData = M2_NEW char[fileSize];
	fread(pData, fileSize, 1, fp);
	fclose(fp);

	m_fileLoader.Bind(fileSize, pData);
	M2_DELETE_ARRAY(pData);

	LoadGroup(&m_globalNode);
	return true;
}

bool CTextFileLoader::LoadGroup(TGroupNode * pGroupNode)
{
	TTokenVector stTokenVector;
	for (; m_dwcurLineIndex < m_fileLoader.GetLineCount(); ++m_dwcurLineIndex)
	{
		if (!m_fileLoader.SplitLine(m_dwcurLineIndex, &stTokenVector))
			continue;

		stl_lowers(stTokenVector[0]);

		if ('{' == stTokenVector[0][0])
			continue;

		if ('}' == stTokenVector[0][0])
			break;

		// Group
		if (0 == stTokenVector[0].compare("group"))
		{
			if (2 != stTokenVector.size())
			{
				sys_err("Invalid group syntax token size: %u != 2 (DO NOT SPACE IN NAME)", stTokenVector.size());
				for (unsigned int i = 0; i < stTokenVector.size(); ++i)
					sys_err("  %u %s", i, stTokenVector[i].c_str());
				exit(1);
				continue;
			}

			TGroupNode * pNewNode = ms_groupNodePool.Alloc();
			pNewNode->pParentNode = pGroupNode;
			pNewNode->strGroupName = stTokenVector[1];
			stl_lowers(pNewNode->strGroupName);
			pGroupNode->ChildNodeVector.push_back(pNewNode);

			++m_dwcurLineIndex;

			LoadGroup(pNewNode);
		}
		// List
		else if (0 == stTokenVector[0].compare("list"))
		{
			if (2 != stTokenVector.size())
			{
				assert(!"There is no list name!");
				continue;
			}

			TTokenVector stSubTokenVector;

			stl_lowers(stTokenVector[1]);
			std::string key = stTokenVector[1];
			stTokenVector.clear();

			++m_dwcurLineIndex;
			for (; m_dwcurLineIndex < m_fileLoader.GetLineCount(); ++m_dwcurLineIndex)
			{
				if (!m_fileLoader.SplitLine(m_dwcurLineIndex, &stSubTokenVector))
					continue;

				if ('{' == stSubTokenVector[0][0])
					continue;

				if ('}' == stSubTokenVector[0][0])
					break;

				for (DWORD j = 0; j < stSubTokenVector.size(); ++j)
				{
					stTokenVector.push_back(stSubTokenVector[j]);
				}
			}

			pGroupNode->LocalTokenVectorMap.insert(std::make_pair(key, stTokenVector));
		}
		else
		{
			std::string key = stTokenVector[0];

			if (1 == stTokenVector.size())
			{
				sys_err("CTextFileLoader::LoadGroup : must have a value (filename: %s line: %d key: %s)",
						m_strFileName.c_str(),
						m_dwcurLineIndex,
						key.c_str());
				break;
			}

			stTokenVector.erase(stTokenVector.begin());
			pGroupNode->LocalTokenVectorMap.insert(std::make_pair(key, stTokenVector));
		}
	}

	return true;
}

void CTextFileLoader::SetTop()
{
	m_pcurNode = &m_globalNode;
}

DWORD CTextFileLoader::GetChildNodeCount()
{
	if (!m_pcurNode)
	{
		assert(!"Node to access has not set!");
		return 0;
	}

	return m_pcurNode->ChildNodeVector.size();
}

BOOL CTextFileLoader::SetChildNode(const char * c_szKey)
{
	if (!m_pcurNode)
	{
		assert(!"Node to access has not set!");
		return FALSE;
	}

	for (DWORD i = 0; i < m_pcurNode->ChildNodeVector.size(); ++i)
	{
		TGroupNode * pGroupNode = m_pcurNode->ChildNodeVector[i];
		if (0 == pGroupNode->strGroupName.compare(c_szKey))
		{
			m_pcurNode = pGroupNode;
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CTextFileLoader::SetChildNode(const std::string & c_rstrKeyHead, DWORD dwIndex)
{
	char szKey[32];
	snprintf(szKey, sizeof(szKey), "%s%02u", c_rstrKeyHead.c_str(), (unsigned int) dwIndex);
	return SetChildNode(szKey);
}

BOOL CTextFileLoader::SetChildNode(DWORD dwIndex)
{
	if (!m_pcurNode)
	{
		assert(!"Node to access has not set!");
		return FALSE;
	}

	if (dwIndex >= m_pcurNode->ChildNodeVector.size())
	{
		assert(!"Node index to set is too large to access!");
		return FALSE;
	}

	m_pcurNode = m_pcurNode->ChildNodeVector[dwIndex];

	return TRUE;
}

BOOL CTextFileLoader::SetParentNode()
{
	if (!m_pcurNode)
	{
		assert(!"Node to access has not set!");
		return FALSE;
	}

	if (NULL == m_pcurNode->pParentNode)
	{
		assert(!"Current group node is already top!");
		return FALSE;
	}

	m_pcurNode = m_pcurNode->pParentNode;

	return TRUE;
}

BOOL CTextFileLoader::GetCurrentNodeName(std::string * pstrName)
{
	if (!m_pcurNode)
		return FALSE;
	if (NULL == m_pcurNode->pParentNode)
		return FALSE;

	*pstrName = m_pcurNode->strGroupName;

	return TRUE;
}

BOOL CTextFileLoader::IsToken(const std::string & c_rstrKey)
{
	if (!m_pcurNode)
	{
		assert(!"Node to access has not set!");
		return FALSE;
	}

	return m_pcurNode->LocalTokenVectorMap.end() != m_pcurNode->LocalTokenVectorMap.find(c_rstrKey);
}

BOOL CTextFileLoader::GetTokenVector(const std::string & c_rstrKey, TTokenVector ** ppTokenVector)
{
	if (!m_pcurNode)
	{
		assert(!"Node to access has not set!");
		return FALSE;
	}

	TTokenVectorMap::iterator it = m_pcurNode->LocalTokenVectorMap.find(c_rstrKey);
	if (m_pcurNode->LocalTokenVectorMap.end() == it)
	{
		sys_log(2, " CTextFileLoader::GetTokenVector - Failed to find the key %s [%s :: %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	*ppTokenVector = &it->second;

	return TRUE;
}

BOOL CTextFileLoader::GetTokenBoolean(const std::string & c_rstrKey, BOOL * pData)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->empty())
	{
		sys_log(2, " CTextFileLoader::GetTokenBoolean - Failed to find the value %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	BOOL out = 0;
	str_to_number(out, pTokenVector->at(0).c_str());
	*pData = out;

	return TRUE;
}

BOOL CTextFileLoader::GetTokenByte(const std::string & c_rstrKey, BYTE * pData)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->empty())
	{
		sys_log(2, " CTextFileLoader::GetTokenByte - Failed to find the value %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	BYTE out = 0;
	str_to_number(out, pTokenVector->at(0).c_str());
	*pData = out;

	return TRUE;
}

BOOL CTextFileLoader::GetTokenWord(const std::string & c_rstrKey, WORD * pData)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->empty())
	{
		sys_log(2, " CTextFileLoader::GetTokenWord - Failed to find the value %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	WORD out = 0;
	str_to_number(out, pTokenVector->at(0).c_str());
	*pData = out;

	return TRUE;
}

BOOL CTextFileLoader::GetTokenInteger(const std::string & c_rstrKey, int * pData)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->empty())
	{
		sys_log(2, " CTextFileLoader::GetTokenInteger - Failed to find the value %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	int out = 0;
	str_to_number(out, pTokenVector->at(0).c_str());
	*pData = out;

	return TRUE;
}

BOOL CTextFileLoader::GetTokenDoubleWord(const std::string & c_rstrKey, DWORD * pData)
{
	return GetTokenInteger(c_rstrKey, (int *) pData);
}

BOOL CTextFileLoader::GetTokenFloat(const std::string & c_rstrKey, float * pData)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->empty())
	{
		sys_log(2, " CTextFileLoader::GetTokenFloat - Failed to find the value %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	*pData = atof(pTokenVector->at(0).c_str());

	return TRUE;
}

BOOL CTextFileLoader::GetTokenVector2(const std::string & c_rstrKey, D3DXVECTOR2 * pVector2)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->size() != 2)
	{
		sys_log(2, " CTextFileLoader::GetTokenVector2 - This key should have 2 values %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	pVector2->x = atof(pTokenVector->at(0).c_str());
	pVector2->y = atof(pTokenVector->at(1).c_str());

	return TRUE;
}

BOOL CTextFileLoader::GetTokenVector3(const std::string & c_rstrKey, D3DXVECTOR3 * pVector3)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->size() != 3)
	{
		sys_log(2, " CTextFileLoader::GetTokenVector3 - This key should have 3 values %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	pVector3->x = atof(pTokenVector->at(0).c_str());
	pVector3->y = atof(pTokenVector->at(1).c_str());
	pVector3->z = atof(pTokenVector->at(2).c_str());

	return TRUE;
}

BOOL CTextFileLoader::GetTokenVector4(const std::string & c_rstrKey, D3DXVECTOR4 * pVector4)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->size() != 4)
	{
		sys_log(2, " CTextFileLoader::GetTokenVector3 - This key should have 3 values %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	pVector4->x = atof(pTokenVector->at(0).c_str());
	pVector4->y = atof(pTokenVector->at(1).c_str());
	pVector4->z = atof(pTokenVector->at(2).c_str());
	pVector4->w = atof(pTokenVector->at(3).c_str());

	return TRUE;
}


BOOL CTextFileLoader::GetTokenPosition(const std::string & c_rstrKey, D3DXVECTOR3 * pVector)
{
	return GetTokenVector3(c_rstrKey, pVector);
}

BOOL CTextFileLoader::GetTokenQuaternion(const std::string & c_rstrKey, D3DXQUATERNION * pQ)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->size() != 4)
	{
		sys_log(2, " CTextFileLoader::GetTokenVector3 - This key should have 3 values %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	pQ->x = atof(pTokenVector->at(0).c_str());
	pQ->y = atof(pTokenVector->at(1).c_str());
	pQ->z = atof(pTokenVector->at(2).c_str());
	pQ->w = atof(pTokenVector->at(3).c_str());
	return TRUE;
}

BOOL CTextFileLoader::GetTokenDirection(const std::string & c_rstrKey, D3DVECTOR * pVector)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->size() != 3)
	{
		sys_log(2, " CTextFileLoader::GetTokenDirection - This key should have 3 values %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	pVector->x = atof(pTokenVector->at(0).c_str());
	pVector->y = atof(pTokenVector->at(1).c_str());
	pVector->z = atof(pTokenVector->at(2).c_str());
	return TRUE;
}

BOOL CTextFileLoader::GetTokenColor(const std::string & c_rstrKey, D3DXCOLOR * pColor)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->size() != 4)
	{
		sys_log(2, " CTextFileLoader::GetTokenColor - This key should have 4 values %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	pColor->r = atof(pTokenVector->at(0).c_str());
	pColor->g = atof(pTokenVector->at(1).c_str());
	pColor->b = atof(pTokenVector->at(2).c_str());
	pColor->a = atof(pTokenVector->at(3).c_str());

	return TRUE;
}

BOOL CTextFileLoader::GetTokenColor(const std::string & c_rstrKey, D3DCOLORVALUE * pColor)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->size() != 4)
	{
		sys_log(2, " CTextFileLoader::GetTokenColor - This key should have 4 values %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	pColor->r = atof(pTokenVector->at(0).c_str());
	pColor->g = atof(pTokenVector->at(1).c_str());
	pColor->b = atof(pTokenVector->at(2).c_str());
	pColor->a = atof(pTokenVector->at(3).c_str());

	return TRUE;
}

BOOL CTextFileLoader::GetTokenString(const std::string & c_rstrKey, std::string * pString)
{
	TTokenVector * pTokenVector;
	if (!GetTokenVector(c_rstrKey, &pTokenVector))
		return FALSE;

	if (pTokenVector->empty())
	{
		sys_log(2, " CTextFileLoader::GetTokenString - Failed to find the value %s [%s : %s]", m_strFileName.c_str(), m_pcurNode->strGroupName.c_str(), c_rstrKey.c_str());
		return FALSE;
	}

	*pString = pTokenVector->at(0);

	return TRUE;
}

