/*
 * StateMachine.c
 *
 *  Created on: Jan 16, 2016
 *      Author: daniel
 */

#include "StateMachine.h"

void StateMachineSwitch(struStateMachine* pStateMachine, StateMachineFunc newState)
{
	// 根据返回值（状态）来判断是否进行状态切换
	// 查表判断两个状态切换是否有对应的处理函数（如果没有，那么就认为状态切换异常）
	for (uint8_t i = 0; i < pStateMachine->size; i++)
	{
		// 如果在查表时，旧状态为空，就表示当前处于任何状态切换到新状态，都是用这个函数
		if (((pStateMachine->Entries[i].oldState == NULL) || (pStateMachine->Entries[i].oldState == pStateMachine->currentState))
				&& (pStateMachine->Entries[i].newState == newState))
		{
			// 调用状态切换处理函数
			pStateMachine->Entries[i].changeState(pStateMachine->currentState);

			pStateMachine->currentState = newState;

			return;
		}
	}

	//TODO: 跑到这里是异常（也有可能是那种不需要做任何处理？？？）
	configASSERT(pdFALSE);

	pStateMachine->currentState = newState;
}

void StateMachineRun(struStateMachine* pStateMachine)
{
	StateMachineFunc		state;

	// 运行当前状态处理函数
	state = (StateMachineFunc)pStateMachine->currentState();

	// 状态处理函数返回为NULL，则表示当前不需要切换状态
	if (state != NULL)
	{
		StateMachineSwitch(pStateMachine, state);
	}
}


