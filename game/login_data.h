#ifndef __INC_METIN_II_LOGIN_DATA_H__
#define __INC_METIN_II_LOGIN_DATA_H__

class CLoginData
{
	public:
		CLoginData();

		void            SetClientKey(const DWORD * c_pdwClientKey);

		const DWORD *   GetClientKey();
		void            SetKey(DWORD dwKey);
		DWORD           GetKey();

		void		SetLogin(const char * c_pszLogin);
		const char *	GetLogin();

		void            SetBillType(BYTE bType);
		DWORD           GetBillID();

		void            SetBillID(DWORD dwID);
		BYTE            GetBillType();

		void            SetConnectedPeerHandle(DWORD dwHandle);
		DWORD		GetConnectedPeerHandle();

		void            SetLogonTime();
		DWORD		GetLogonTime();

		void		SetIP(const char * c_pszIP);
		const char *	GetIP();

		void		SetRemainSecs(long l);
		long		GetRemainSecs();

		void		SetBilling(bool bOn);
		bool		IsBilling();

		void		SetDeleted(bool bSet);
		bool		IsDeleted();

		void		SetPremium(int * paiPremiumTimes);
		int		GetPremium(BYTE type);
		int *		GetPremiumPtr();


	private:
		DWORD           m_dwKey;
		DWORD           m_adwClientKey[4];
		BYTE            m_bBillType;
		DWORD           m_dwBillID;
		DWORD           m_dwConnectedPeerHandle;
		DWORD           m_dwLogonTime;
		long		m_lRemainSecs;
		char		m_szIP[MAX_HOST_LENGTH+1];
		bool		m_bBilling;
		bool		m_bDeleted;
		std::string	m_stLogin;
		int		m_aiPremiumTimes[PREMIUM_MAX_NUM];

		
		
};

#endif
