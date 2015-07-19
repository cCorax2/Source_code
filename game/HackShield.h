
#ifndef HACK_SHIELD_MANAGER_H_
#define HACK_SHIELD_MANAGER_H_

class CHackShieldImpl;

class CHackShieldManager : public singleton<CHackShieldManager>
{
	public:
		bool Initialize ();
		void Release ();

		bool CreateClientHandle (DWORD dwPlayerID);
		void DeleteClientHandle (DWORD dwPlayerID);

		bool SendCheckPacket (LPCHARACTER ch);
		bool VerifyAck (LPCHARACTER ch, const void* buf);

	private:
		CHackShieldImpl* impl_;
};

#endif /* HACK_SHIELD_MANAGER_H_ */

