// Local Includes

#include <cassert>
#include <cstdlib>

#include "FSM.h"

// Constructor
CFSM::CFSM()
{
	// Initialize States
	m_stateInitial.Set(this, &CFSM::BeginStateInitial, &CFSM::StateInitial, &CFSM::EndStateInitial);

	// Initialize State Machine
	m_pCurrentState = static_cast<CState *>(&m_stateInitial);
	m_pNewState = NULL;
}

//======================================================================================================
// Global Functions

// Update
void CFSM::Update()
{
	// Check New State
	if (m_pNewState)
	{
		if (NULL != m_pCurrentState)
		{
			m_pCurrentState->ExecuteEndState();
		}

		// Set New State
		m_pCurrentState = m_pNewState;
		m_pNewState = 0;

		// Execute Begin State
		m_pCurrentState->ExecuteBeginState();
	}

	// Execute State
	m_pCurrentState->ExecuteState();
}

//======================================================================================================
// State Functions

// Is State
bool CFSM::IsState(CState & State) const
{
	return (m_pCurrentState == &State);
}

// Goto State
bool CFSM::GotoState(CState & NewState)
{
	if (IsState(NewState) && m_pNewState == &NewState)
		return true;

	// Set New State
	m_pNewState = &NewState;
	return true;
}

