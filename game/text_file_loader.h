#ifndef __INC_METIN_II_TEXTFILELOADER_H__
#define __INC_METIN_II_TEXTFILELOADER_H__

#include "../../common/d3dtype.h"
#include "../../common/pool.h"
#include "file_loader.h"

typedef std::map<std::string, TTokenVector>	TTokenVectorMap;

class CTextFileLoader
{
	public:
		typedef struct SGroupNode
		{
			std::string strGroupName;

			TTokenVectorMap LocalTokenVectorMap;

			SGroupNode * pParentNode;
			std::vector<SGroupNode*> ChildNodeVector;
		} TGroupNode;

		typedef std::vector<TGroupNode *> TGroupNodeVector;

	public:
		static void DestroySystem();

	public:
		CTextFileLoader();
		virtual ~CTextFileLoader();

		bool Load(const char * c_szFileName);
		const char * GetFileName();

		void SetTop();
		DWORD GetChildNodeCount();
		BOOL SetChildNode(const char * c_szKey);
		BOOL SetChildNode(const std::string & c_rstrKeyHead, DWORD dwIndex);
		BOOL SetChildNode(DWORD dwIndex);
		BOOL SetParentNode();
		BOOL GetCurrentNodeName(std::string * pstrName);

		BOOL IsToken(const std::string & c_rstrKey);
		BOOL GetTokenVector(const std::string & c_rstrKey, TTokenVector ** ppTokenVector);
		BOOL GetTokenBoolean(const std::string & c_rstrKey, BOOL * pData);
		BOOL GetTokenByte(const std::string & c_rstrKey, BYTE * pData);
		BOOL GetTokenWord(const std::string & c_rstrKey, WORD * pData);
		BOOL GetTokenInteger(const std::string & c_rstrKey, int * pData);
		BOOL GetTokenDoubleWord(const std::string & c_rstrKey, DWORD * pData);
		BOOL GetTokenFloat(const std::string & c_rstrKey, float * pData);

		BOOL GetTokenVector2(const std::string & c_rstrKey, D3DXVECTOR2 * pVector2);
		BOOL GetTokenVector3(const std::string & c_rstrKey, D3DXVECTOR3 * pVector3);
		BOOL GetTokenVector4(const std::string & c_rstrKey, D3DXVECTOR4 * pVector4);

		BOOL GetTokenPosition(const std::string & c_rstrKey, D3DXVECTOR3 * pVector);
		BOOL GetTokenQuaternion(const std::string & c_rstrKey, D3DXQUATERNION * pQ);
		BOOL GetTokenDirection(const std::string & c_rstrKey, D3DVECTOR * pVector);
		BOOL GetTokenColor(const std::string & c_rstrKey, D3DXCOLOR * pColor);
		BOOL GetTokenColor(const std::string & c_rstrKey, D3DCOLORVALUE * pColor);
		BOOL GetTokenString(const std::string & c_rstrKey, std::string * pString);

	protected:
		bool LoadGroup(TGroupNode * pGroupNode);

	protected:
		std::string					m_strFileName;
		DWORD						m_dwcurLineIndex;
		const void *					mc_pData;

		CMemoryTextFileLoader				m_fileLoader;

		TGroupNode					m_globalNode;
		TGroupNode *					m_pcurNode;

	private:
		static CDynamicPool<TGroupNode>			ms_groupNodePool;
};

#endif

