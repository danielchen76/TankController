/*
 * StateMachine.h
 *
 *  Created on: Jan 16, 2016
 *      Author: daniel
 */

#ifndef INCLUDE_STATEMACHINE_H_
#define INCLUDE_STATEMACHINE_H_

#include <FreeRTOS.h>

typedef void* (*StateMachineFunc)(void);
typedef void (*StateChanged)(StateMachineFunc pOldState);


typedef struct
{
	StateMachineFunc		oldState;
	StateMachineFunc		newState;
	StateChanged			changeState;
} struStateChangeEntry;

typedef struct
{
	const struStateChangeEntry*	Entries;
	uint8_t					size;					// 状态相互切换的处理入口有多少个（Entries）
	StateMachineFunc		currentState;			// 当前状态
} struStateMachine;

// 状态机的处理和切换在这个函数一次完成
void StateMachineSwitch(struStateMachine* pStateMachine, StateMachineFunc newState);
void StateMachineRun(struStateMachine* pStateMachine);


#endif /* INCLUDE_STATEMACHINE_H_ */
