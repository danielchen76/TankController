/*
 * const.h
 *
 *  Created on: Oct 24, 2015
 *      Author: daniel
 */

#ifndef INCLUDE_CONST_H_
#define INCLUDE_CONST_H_

#define MAIN_TANK_HEIGHT		650
#define MAIN_TANK_LENGTH		(600 - 24)			// 主缸玻璃厚度12mm
#define MAIN_TANK_WIDTH			(600 - 24)

#define SUB_TANK_HEIGHT			402
#define SUB_TANK_LENGTH			(520 - 16)			// 底缸玻璃厚度8mm（TODO:需要再量底缸实际宽度和长度）
#define SUB_TANK_WIDTH			(520 - 16)

#define RO_TANK_HEIGHT			288
#define RO_TANK_LENGTH			(350 - 10)			// RO缸厚度5mm有机玻璃
#define RO_TANK_WIDTH			(200 - 10)

typedef enum
{
	GLOBAL_STATE_RUNNING,				// 正常运行
	GLOBAL_STATE_STOP,					// 停机
	GLOBAL_STATE_CHANGEWATER,			// 换水
	GLOBAL_STATE_FEED,					// 喂食模式
	GLOBAL_STATE_NIGHTMODE,				// 夜间模式
} enumGlobalState;


#endif /* INCLUDE_CONST_H_ */
