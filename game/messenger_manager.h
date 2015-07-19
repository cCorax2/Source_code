#ifndef __INC_MESSENGER_MANAGER_H
#define __INC_MESSENGER_MANAGER_H

#include "db.h"

class MessengerManager : public singleton<MessengerManager>
{
	public:
		typedef std::string keyT;
		typedef const std::string & keyA;

		MessengerManager();
		virtual ~MessengerManager();

	public:
		void	P2PLogin(keyA account);
		void	P2PLogout(keyA account);

		void	Login(keyA account);
		void	Logout(keyA account);

		void	RequestToAdd(LPCHARACTER ch, LPCHARACTER target);
		void	AuthToAdd(keyA account, keyA companion, bool bDeny);

		void	__AddToList(keyA account, keyA companion);	// 실제 m_Relation, m_InverseRelation 수정하는 메소드
		void	AddToList(keyA account, keyA companion);

		void	__RemoveFromList(keyA account, keyA companion); // 실제 m_Relation, m_InverseRelation 수정하는 메소드
		void	RemoveFromList(keyA account, keyA companion);	

		void	RemoveAllList(keyA account);

		void	Initialize();

	private:
		void	SendList(keyA account);
		void	SendLogin(keyA account, keyA companion);
		void	SendLogout(keyA account, keyA companion);

		void	LoadList(SQLMsg * pmsg);

		void	Destroy();

		std::set<keyT>			m_set_loginAccount;
		std::map<keyT, std::set<keyT> >	m_Relation;
		std::map<keyT, std::set<keyT> >	m_InverseRelation;
		std::set<DWORD>			m_set_requestToAdd;
};

#endif
