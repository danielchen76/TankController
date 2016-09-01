/*
 * DisplayButtonTask.c
 * 数码管显示、LED、按键、蜂鸣器
 *
 *  Created on: May 7, 2016
 *      Author: daniel
 */

#include <DisplayButtonTask.h>

#include "Msg.h"
#include <queue.h>
#include <task.h>

#include "stm32f10x.h"

#include "tc_gpio.h"
#include "logTask.h"
#include "setting.h"
#include "TimerQueue.h"

#include <TM1637.h>

// 板载LED在PB9上
#define LED_INTERNAL_GPIO           GPIOB
#define LED_INTERNAL_GPIO_CLK       RCC_APB2Periph_GPIOB
#define LED_INTERNAL_Pin            GPIO_Pin_9

#define INTERNAL_LED_OFF			GPIO_ResetBits(LED_INTERNAL_GPIO, LED_INTERNAL_Pin)
#define INTERNAL_LED_ON				GPIO_SetBits(LED_INTERNAL_GPIO, LED_INTERNAL_Pin)

static BaseType_t					s_bError = pdFALSE;

// 单位：ms
#define LED_NORMAL_ON_TIME			50
#define LED_NORMAL_OFF_TIME			1500

#define LED_ERROR_ON_TIME			300
#define LED_ERROR_OFF_TIME			300

// Arduino Micro Pro按键板的串口
#define USARTif                   UART4
#define USARTif_GPIO              GPIOC
#define USARTif_CLK               RCC_APB1Periph_UART4
#define USARTif_GPIO_CLK          RCC_APB2Periph_GPIOC
#define USARTif_RxPin             GPIO_Pin_11
#define USARTif_TxPin             GPIO_Pin_10

#define ROTARY_FWD					0x01
#define ROTARY_REV					0x02
#define ROTARY_DOWN					0x04
#define ROTARY_UP

void LEDBlinkCallback(void* pvParameters);


void InitLEDButton( void )
{
	// 初始化LED引脚
	GPIO_InitTypeDef 	GPIO_InitStructure;
	USART_InitTypeDef 	USART_InitStructure;
	NVIC_InitTypeDef 	NVIC_InitStructure;

	// Enable GPIO Peripheral clock
	RCC_APB2PeriphClockCmd(LED_INTERNAL_GPIO_CLK, ENABLE);

	// Configure pin in output push/pull mode
	GPIO_InitStructure.GPIO_Pin = LED_INTERNAL_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(LED_INTERNAL_GPIO, &GPIO_InitStructure);

	// 数码管初始化
	tm1637Init();

    // Optionally set brightness. 0 is off. By default, initialized to full brightness.
    tm1637SetBrightness(3);

    // Display the value "1234" and turn on the `:` that is between digits 2 and 3.
    tm1637DisplayDecimal(0, 0);		// LED full on

    // 初始化USART，用于接收Arduino Micro Pro通过串口发送的按键状态，以及通过串口接收控制部分LED显示
	/* Enable USART Clock */
	RCC_APB1PeriphClockCmd(USARTif_CLK, ENABLE);
	RCC_APB2PeriphClockCmd(USARTif_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);

	/*!< GPIO configuration */
	//GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);

	/* Configure USART Rx as input floating */
	GPIO_InitStructure.GPIO_Pin = USARTif_RxPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(USARTif_GPIO, &GPIO_InitStructure);

	/* Configure USART Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = USARTif_TxPin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(USARTif_GPIO, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	/* Configure USART */
	USART_Init(USARTif, &USART_InitStructure);

	// 中断初始化
	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable the USART */
	USART_Cmd(USARTif, ENABLE);

}

// 定义队列大小（温度控制用）
QueueHandle_t		LEDButton_queue;
const UBaseType_t 	uxLEDButtonQueueSize = 5;

// 初始化消息队列
void InitLEDButtonMsgQueue( void )
{
	LEDButton_queue = xQueueCreate(uxLEDButtonQueueSize, sizeof(Msg*));
}

void MsgButtonDown( Msg* msg)
{
}

void MsgButtonUp( Msg* msg )
{
	// 判断按下的时间长度，执行不同的操作
}

void MsgButtonFwd( Msg* msg )
{
}

void MsgButtonRev( Msg* msg )
{
}

static struEntry	Entries[] =
{
	{MSG_BUTTON_DOWN,		MsgButtonDown},
	{MSG_BUTTON_UP,			MsgButtonUp},
	{MSG_BUTTON_FWD,		MsgButtonFwd},
	{MSG_BUTTON_REV,		MsgButtonFwd},
};

// 串口中断，用于接收Arduino Micro Pro通过串口发送的按键状态（旋转编码器）
void UART4_IRQHandler( void )
{
	uint8_t		data;
	uint16_t 	sr = USARTif->SR;
	Msg*		msg;

	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	if (sr & (USART_FLAG_RXNE | USART_FLAG_ORE))
	{
		data = USART_ReceiveData(USARTif);

		// TODO:data要进行switch分析，可能会出现多个消息（需要注意处理）

		// 将数据解析为按键消息，发送到LEDTask队列
		msg = MallocMsgFromISR();
		if (msg)		// 消息分配不到，忽略此次按键通知
		{
			msg->Id = MSG_BUTTON_FWD;
			xQueueSendFromISR(LEDButton_queue, &msg, &xHigherPriorityTaskWoken);
		}

		portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
	}
}

static void MsgProcess_entry( void )
{
	Msg* msg;
	unsigned int i;
	BaseType_t ret;

	ret = xQueueReceive(LEDButton_queue, &msg, pdMS_TO_TICKS(50));

	if (ret == pdPASS)
	{
		// 对消息进行处理
		for (i = 0; i < (sizeof(Entries) / sizeof(Entries[0])); i++)
		{
			if (Entries[i].MsgId == ((Msg*) msg)->Id)
			{
				Entries[i].Entry((Msg*) msg);
				break;
			}
		}

		// 完成处理后，将Msg设置为空闲
		FreeMsg(msg);
	}
}

// 显示和按键任务使用的定时器
static struMyTimer			s_LEDButtonTimerArray[5];
static struMyTimerQueue		s_LEDButtonTimerQueue = {s_LEDButtonTimerArray, sizeof(s_LEDButtonTimerArray) / sizeof(s_LEDButtonTimerArray[0])};

//static const TickType_t	c_LEDCheckInterval		= pdMS_TO_TICKS(100);		// 实时时钟，每秒检查一次分钟是否有变化，有变化更新时间显示
static int16_t	s_BlinkIndex;

static TickType_t			s_NowTick;

void InitDisplayButtonTask( void )
{
	InitLEDButton();

	InitLEDButtonMsgQueue();

	// 初始化Timer队列
	InitTimerQueue(&s_LEDButtonTimerQueue);

	s_BlinkIndex = AddTimer(&s_LEDButtonTimerQueue, xTaskGetTickCount(), LED_NORMAL_ON_TIME, pdTRUE, LEDBlinkCallback, (void*)pdTRUE);
	LogOutput(LOG_INFO, "Start internal LED blink timer.");
}

void DisplayButtonTask( void * pvParameters)
{
	(void)pvParameters;

	while (1)
	{
		// 处理消息
		MsgProcess_entry();

		s_NowTick = xTaskGetTickCount();

		// 定时器队列检查
		CheckTimerQueue(&s_LEDButtonTimerQueue, s_NowTick);
	}
}


void SetErrorLED(BaseType_t bError)
{
	// 设置LED灯告警状态（正常：慢闪；异常/有错误：快闪）
	s_bError = bError;
}

void SetDebugLED(BaseType_t bOn)
{
	bOn ? INTERNAL_LED_ON : INTERNAL_LED_OFF;
}

// Blink定时器回调，用于控制板载LED的闪烁频率
void LEDBlinkCallback(void* pvParameters)
{
	BaseType_t	bOn = (BaseType_t)pvParameters;
	uint16_t	usON = LED_NORMAL_ON_TIME;
	uint16_t	usOFF = LED_NORMAL_OFF_TIME;

	// 是否处于错误状态
	if (s_bError)
	{
		usON = LED_ERROR_ON_TIME;
		usOFF = LED_ERROR_OFF_TIME;
	}
	else
	{
		usON = LED_NORMAL_ON_TIME;
		usOFF = LED_NORMAL_OFF_TIME;
	}

	if (bOn)
	{
		INTERNAL_LED_OFF;
		// 修改定时器时间
		UpdateTimer(&s_LEDButtonTimerQueue, s_BlinkIndex, usOFF, pdTRUE, (void*)pdFALSE);
	}
	else
	{
		INTERNAL_LED_ON;
		// 修改定时器时间和参数
		UpdateTimer(&s_LEDButtonTimerQueue, s_BlinkIndex, usON, pdTRUE, (void*)pdTRUE);
	}
}

// -------------------------------Command line--------------------------------------------
// 手动清除内部LED的告警状态
static BaseType_t cmd_clearErr( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	(void)pcCommandString;

	SetErrorLED(pdFALSE);
	snprintf(pcWriteBuffer, xWriteBufferLen, "Clear LED error state.\r\n");

	return pdFALSE;
}


const CLI_Command_Definition_t cmd_def_clearErr =
{
	"clearErr",
	"\r\nclearErr\r\n Clear internal LED error state.\r\n",
	cmd_clearErr, /* The function to run. */
	0
};

