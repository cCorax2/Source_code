#ifndef PASSPOD_H
#define PASSPOD_H

class CPasspod : public singleton<CPasspod>
{
public:
	CPasspod();
	~CPasspod();

 	int ConfirmPasspod( const char * account,const char* passpod  );
	bool Connect( LPFDWATCH fdw );
	bool Disconnect();


private:
	bool IConv( const char * src, char * desc );


private:

	socket_t 	m_sock;
	LPFDWATCH 	m_lpFDW;
};

extern const char ERR_STRINGS[6][32]; 
extern const char ERR_MESSAGE[6][64]; 

#endif
