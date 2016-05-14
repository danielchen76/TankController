/*
 * TM1637.c
 *
 *  Created on: May 7, 2016
 *      Author: daniel
 */

#include <TM1637.h>

void TM1637Start(void);
void TM1637Stop(void);
void TM1637ReadResult(void);
void TM1637WriteByte(uint8_t b);
void TM1637DelayUsec(uint16_t i);

inline void TM1637ClkHigh(void);
inline void TM1637ClkLow(void);
inline void TM1637DioHigh(void);
inline void TM1637DioLow(void);

// PIN define
#define TM1637_CLK_GPIO           GPIOF
#define TM1637_CLK_GPIO_CLK       RCC_APB2Periph_GPIOF
#define TM1637_CLK_Pin            GPIO_Pin_12

#define TM1637_DIO_GPIO           GPIOF
#define TM1637_DIO_GPIO_CLK       RCC_APB2Periph_GPIOF
#define TM1637_DIO_Pin            GPIO_Pin_11

//
//      A
//     ---
//  F |   | B
//     -G-
//  E |   | C
//     ---
//      D

const uint8_t segmentMap[] =
{
	0b00111111,	   // 0
	0b00000110,    // 1
	0b01011011,    // 2
	0b01001111,    // 3
	0b01100110,    // 4
	0b01101101,    // 5
	0b01111101,    // 6
	0b00000111,    // 7
	0b01111111,    // 8
	0b01101111,    // 9
	0b01110111,    // A
	0b01111100,    // b
	0b00111001,    // C
	0b01011110,    // d
	0b01111001,    // E
	0b01110001,    // F
	0b00000000
};

void tm1637Init(void)
{
	// 初始化引脚
	GPIO_InitTypeDef 	GPIO_InitStructure;

	// Enable GPIO Peripheral clock
	RCC_APB2PeriphClockCmd(TM1637_CLK_GPIO_CLK, ENABLE);

	// Configure pin in output push/pull mode
	GPIO_InitStructure.GPIO_Pin = TM1637_CLK_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(TM1637_CLK_GPIO, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = TM1637_DIO_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(TM1637_DIO_GPIO, &GPIO_InitStructure);

	tm1637SetBrightness(8);
}

void tm1637DisplayDecimal(int v, int displaySeparator)
{
	unsigned char digitArr[4];
	for (int i = 0; i < 4; ++i)
	{
		digitArr[i] = segmentMap[v % 10];
		if (i == 2 && displaySeparator)
		{
			digitArr[i] |= 1 << 7;
		}
		v /= 10;
	}

	TM1637Start();
	TM1637WriteByte(0x40);
	TM1637ReadResult();
	TM1637Stop();

	TM1637Start();
	TM1637WriteByte(0xc0);
	TM1637ReadResult();

	for (int i = 0; i < 4; ++i)
	{
		TM1637WriteByte(digitArr[3 - i]);
		TM1637ReadResult();
	}

	TM1637Stop();
}

// Valid brightness values: 0 - 8.
// 0 = display off.
void tm1637SetBrightness(uint8_t brightness)
{
	// Brightness command:
	// 1000 0XXX = display off
	// 1000 1BBB = display on, brightness 0-7
	// X = don't care
	// B = brightness
	TM1637Start();
	TM1637WriteByte(0x87 + brightness);
	TM1637ReadResult();
	TM1637Stop();
}

void TM1637Start(void)
{
	TM1637ClkHigh();
	TM1637DioHigh();
	TM1637DelayUsec(2);
	TM1637DioLow();
}

void TM1637Stop(void)
{
	TM1637ClkLow();
	TM1637DelayUsec(2);
	TM1637DioLow();
	TM1637DelayUsec(2);
	TM1637ClkHigh();
	TM1637DelayUsec(2);
	TM1637DioHigh();
}

void TM1637ReadResult(void)
{
	TM1637ClkLow();
	TM1637DelayUsec(5);
	// while (dio); // We're cheating here and not actually reading back the response.
	TM1637ClkHigh();
	TM1637DelayUsec(2);
	TM1637ClkLow();
}

void TM1637WriteByte(uint8_t b)
{
	for (uint8_t i = 0; i < 8; ++i)
	{
		TM1637ClkLow();
		if (b & 0x01)
		{
			TM1637DioHigh();
		}
		else
		{
			TM1637DioLow();
		}
		TM1637DelayUsec(3);
		b >>= 1;
		TM1637ClkHigh();
		TM1637DelayUsec(3);
	}
}

void TM1637DelayUsec(uint16_t i)
{
	for (; i > 0; i--)
	{
		for (int j = 0; j < 10; ++j)
		{
			__asm__ __volatile__("nop\n\t":::"memory");
		}
	}
}

void TM1637ClkHigh(void)
{
	GPIO_SetBits(TM1637_CLK_GPIO, TM1637_CLK_Pin);
}

void TM1637ClkLow(void)
{
	GPIO_ResetBits(TM1637_CLK_GPIO, TM1637_CLK_Pin);
}

void TM1637DioHigh(void)
{
	GPIO_SetBits(TM1637_DIO_GPIO, TM1637_DIO_Pin);
}

void TM1637DioLow(void)
{
	GPIO_ResetBits(TM1637_DIO_GPIO, TM1637_DIO_Pin);
}

