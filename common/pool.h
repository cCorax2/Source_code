#ifndef __INC_METIN_II_COMMON_POOL_H__
#define __INC_METIN_II_COMMON_POOL_H__

#include <assert.h>
#include <string>

template<typename T>
class CPoolNode : public T
{
	public:
		CPoolNode()
		{
			m_pNext = NULL;
			m_pPrev = NULL;
		}

		virtual ~CPoolNode()
		{
		}

	public:
		CPoolNode<T> * m_pNext;	
		CPoolNode<T> * m_pPrev;
};

template<typename T>
class CDynamicPool
{	
	public:
		typedef CPoolNode<T> TNode;

	public:
		CDynamicPool()
		{
			Initialize();
		}

		virtual ~CDynamicPool()
		{
			assert(m_pFreeList==NULL && "CDynamicPool::~CDynamicPool() - NOT Clear");
			assert(m_pUsedList==NULL && "CDynamicPool::~CDynamicPool() - NOT Clear");
			Clear();
		}

		void Initialize()
		{
			m_nodes = NULL;
			m_nodeCount = 0;

			m_pFreeList = NULL;
			m_pUsedList = NULL;
		}

		void SetName(const char* c_szName)
		{
			m_stName = c_szName;
		}

		DWORD GetCapacity()
		{
			return m_nodeCount;
		}

		T* Alloc()
		{
			TNode* pnewNode;

			if (m_pFreeList) 
			{
				pnewNode = m_pFreeList;
				m_pFreeList = m_pFreeList->m_pNext;
			}
			else
			{
				pnewNode = AllocNode();
			}

			if (!pnewNode)
				return NULL;		

			if (!m_pUsedList)
			{
				m_pUsedList = pnewNode;
				m_pUsedList->m_pPrev = m_pUsedList->m_pNext = NULL;
			}
			else
			{
				m_pUsedList->m_pPrev = pnewNode;
				pnewNode->m_pNext = m_pUsedList;
				pnewNode->m_pPrev = NULL;
				m_pUsedList = pnewNode;
			}
			//Tracef("%s Pool Alloc %p\n", m_stName.c_str(), pnewNode);
			return (T*) pnewNode;
		}

		void Free(T * pdata)
		{
			TNode* pfreeNode = (TNode*) pdata;

			if (pfreeNode == m_pUsedList)
			{
				if (NULL != (m_pUsedList = m_pUsedList->m_pNext))
					m_pUsedList->m_pPrev = NULL;
			}
			else
			{
				if (pfreeNode->m_pNext)
					pfreeNode->m_pNext->m_pPrev = pfreeNode->m_pPrev;

				if (pfreeNode->m_pPrev)
					pfreeNode->m_pPrev->m_pNext = pfreeNode->m_pNext;
			}

			pfreeNode->m_pPrev = NULL;
			pfreeNode->m_pNext = m_pFreeList;
			m_pFreeList = pfreeNode;
			//Tracef("%s Pool Free\n", m_stName.c_str());
		}

		void FreeAll()
		{
			TNode * pcurNode;
			TNode * pnextNode;

			pcurNode = m_pUsedList;

			while (pcurNode)
			{
				pnextNode = pcurNode->m_pNext;
				Free(pcurNode);
				pcurNode = pnextNode;
			}
		}

		void Clear()
		{
			TNode* pcurNode;
			TNode* pnextNode;

			DWORD count = 0;

			pcurNode = m_pFreeList;
			while (pcurNode)
			{
				pnextNode = pcurNode->m_pNext;
				delete pcurNode;
				pcurNode = pnextNode;
				++count;
			}
			m_pFreeList = NULL;

			pcurNode = m_pUsedList;
			while (pcurNode)
			{
				pnextNode = pcurNode->m_pNext;
				delete pcurNode;
				pcurNode = pnextNode;
				++count;
			}

			m_pUsedList = NULL;

			assert(count==m_nodeCount && "CDynamicPool::Clear()");

			m_nodeCount=0;
		}

	protected:
		TNode* AllocNode()
		{
			++m_nodeCount;
			return new TNode;
		}

	protected:
		TNode *		m_nodes;
		TNode *		m_pFreeList;
		TNode *		m_pUsedList;

		DWORD		m_nodeCount;
		std::string	m_stName;
};

#endif
