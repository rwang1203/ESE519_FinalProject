/*
 * Lab3PartE.c
 *
 * Created: 10/19/2021 11:12:50 AM
 * Author : rawang
 */ 
#define F_CPU 16000000UL
#define BAUD_RATE 9600
#define BAUD_PRESCALER (((F_CPU / (BAUD_RATE * 16UL))) - 1)


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <util/delay.h>
#include <math.h>

#include "SerialPrint.h"
#include "LCD_GFX.h"
#include "ST7735.h"
#include "main.h"

volatile bool masterTrig = false;
bool resetFlag = false;

volatile int PCTimer0, PCTimer1, PCTimer2 = 0;

const int micDist02 = 14; //cm
const int micDist10 = 16; //cm
const int micDist21 = 16; //cm
const double SpdSound = 0.0340290; //cm/us
const float PI =  3.1415926;

int angle = 0;
int lineColor = 0;
float uSecTimer0, uSecTimer1, uSecTimer2 = 0;
int delay = 0;
float distance = 0;

char String[25];
volatile char micOrder[3];
volatile char micState;

void Initialize()
{
	lcd_init();
	
	cli();	

	//Initialize pin change interrupts and mask
	//PCINT0 - pin PB4: pcint[4]
	//PCINT1 - pin PC0: pcint[8]
	//PCINT2 - pin PD2: pcint[18]
	
	//Initialize input pins for pin change interrupt
	//PB4, PC0, PD4 all inputs
	DDRB &= ~(1<<DDB4);
	DDRC &= ~(1<<DDC0);
	DDRD &= ~(1<<DDD4);
	
	PCICR |= (1<<PCIE0);
	PCICR |= (1<<PCIE1);
	PCICR |= (1<<PCIE2);
	
	PCMSK0 |= (1<<PCINT4);
	PCMSK1 |= (1<<PCINT8);
	PCMSK2 |= (1<<PCINT18);
	
	//Timer 2 setup, scaled by 256 to 62.5khz
	TCCR2B &= ~(1<<CS20);
	TCCR2B |= (1<<CS21);
	TCCR2B |= (1<<CS22);
	
	//Timer 2 setup to normal mode
	TCCR2B &= ~(1<<WGM22);
	TCCR2A &= ~(1<<WGM21);
	TCCR2A &= ~(1<<WGM20);
	
	//Enable timer 2 interrupt overflow
	//TIMSK2 |= (1<<TOIE2);
	//Looking for timer 2 overflow
	//TIFR2 |= (1<<TOV2);
	

	/************************************************************************/
	/* Three microphones -> delay between each of them by subtracting timer values when each mic is triggered by a sound source(clap)
	Using the equation theta 
	= (180/pi) * arcsin((delay * speed of sound)/mic distance)  
	delay: delay from subtracting timers
	speed of sound: 343m/s
	mic distance: the distance between our three mics. Constant values!!
	
	**d = delay * speed of sound
	
	If d > mic distance
		take arccos of mic distance/d
	If d < mic distance
		take arcsin of d/mic distance
	
	for theta, we know how theta from the base plane reacts
	The base plane is a line we set as our initial condition
	a positive theta shifts our base line in the CW direction, a negative theta shifts it in the CCW direction
	                                                                     */
	/************************************************************************/
		
	
	LCD_setScreen(BLACK);
	
	sei();
}

int main(void)
{
	Initialize();
	Serial_init(BAUD_PRESCALER);
	
		
	drawLines();
	 
    while (1) 
    {
		if(masterTrig)
		{
			for (int i = 0; i < 3; i++)
			{
				sprintf(String, "mic order = %d\n",micOrder[i]);
				Serial_putstring(String);
				
			}
 			processTimers();
 			resetMics();
		}		
    }
}

ISR(PCINT0_vect)
{
	if (PINB & (1<<PINB4))
	{
		switch(micState)
		{
			case 0:
			TCNT2 = 0;
			PCTimer0 = TCNT2;
			micState = 1;
			micOrder[0] = 0;
			PCICR &= ~(1<<PCIE0); //disable interrupt so micOrder won't have duplicates
			break;
			
			case 1:
			PCTimer0 = TCNT2;
			micState = 2;
			micOrder[1] = 0;
			PCICR &= ~(1<<PCIE0);
			break;
			
			case 2:
			PCTimer0 = TCNT2;
			micState = 3;
			masterTrig = true;
			micOrder[2] = 0;
			break;
			
			default:
			micState = 4; 
			break;
		}
	}	
}

ISR(PCINT1_vect)
{
	if (PINC & (1<<PINC0))
	{
		switch(micState)
		{
			case 0:
			TCNT2 = 0;
			PCTimer1 = TCNT2;
			micState = 1;
			micOrder[0] = 1;
			PCICR &= ~(1<<PCIE1); //disable interrupt so micOrder won't have duplicates
			break;
			
			case 1:
			PCTimer1 = TCNT2;
			micState = 2;
			micOrder[1] = 1;
			PCICR &= ~(1<<PCIE1);
			break;
			
			case 2:
			PCTimer1 = TCNT2;
			micState = 3;
			micOrder[2] = 1;
			masterTrig = true;
			break;
			
			default:
			micState = 4;
			break;
		}
	}		
}

ISR(PCINT2_vect)
{
	if (PIND & (1<<PIND2))
	{
		switch(micState)
		{
			case 0:
			TCNT2 = 0;
			PCTimer2 = TCNT2;
			micState = 1;
			micOrder[0] = 2;
			PCICR &= ~(1<<PCIE2); //disable interrupt so micOrder won't have duplicates
			break;
			
			case 1:
			PCTimer2 = TCNT2;
			micState = 2;
			micOrder[1] = 2;
			PCICR &= ~(1<<PCIE2);
			break;
			
			case 2:
			PCTimer2 = TCNT2;
			micState = 3;
			micOrder[2] = 2;
			masterTrig = true;
			break;
			
			default:
			micState = 4;
			break;
		}
	}	
}

void processTimers()
{
	
	uSecTimer0 =  8 * PCTimer0; //uSec
	uSecTimer1 =  8 * PCTimer1; //uSec
	uSecTimer2 =  8 * PCTimer2; //uSec
	
	ColorLines(lineColor, WHITE); //reset the prior highlighted direction	
	
	//Determine the reference line 
	switch (micOrder[2])
	{
	case 0:
	Reference21();
		break;
		
	case 1:
	Reference02();
		break;
		
	case 2:
	Reference10();
		break;
	}
		
}

void Reference10()
{
	delay = abs(uSecTimer0 - uSecTimer1);
	CalculateAngle(delay, micDist10);
	if(!resetFlag)
	{
		if((micOrder[0] == 1))
		{
			switch (angle)
			{
				case 0 ... 14:
				lineColor = 5;
				ColorLines(lineColor, RED);
				break;
				
				case 15 ... 44:
				lineColor = 4;
				ColorLines(lineColor, RED);
				break;
				
				case 45 ... 74:
				lineColor = 3;
				ColorLines(lineColor, RED);
				break;
				
				case 75 ... 90:
				lineColor = 2;
				ColorLines(lineColor, RED);
				break;
			}
		}
		else
		{
			switch (angle)
			{
				case 0 ... 14:
				lineColor = 5;
				ColorLines(lineColor, RED);
				break;
				
				case 15 ... 44:
				lineColor = 6;
				ColorLines(lineColor, RED);
				break;
				
				case 45 ... 74:
				lineColor = 7;
				ColorLines(lineColor, RED);
				break;
				
				case 75 ... 90:
				lineColor = 8;
				ColorLines(lineColor, RED);
				break;
			}
		}
	}	
}

void Reference21()
{
	delay = abs(uSecTimer2 - uSecTimer1);
	CalculateAngle(delay, micDist21);
	if(!resetFlag)
	{
		if((micOrder[0] == 1))
		{
			switch (angle)
			{
				case 0 ... 14:
				lineColor = 1;
				ColorLines(lineColor, RED);
				break;
				
				case 15 ... 44:
				lineColor = 2;
				ColorLines(lineColor, RED);
				break;
				
				case 45 ... 74:
				lineColor = 3;
				ColorLines(lineColor, RED);
				break;
				
				case 75 ... 90:
				lineColor = 4;
				ColorLines(lineColor, RED);
				break;
			}
		}
		else
		{
			switch (angle)
			{
				case 0 ... 14:
				lineColor = 1;
				ColorLines(lineColor, RED);
				break;
				
				case 15 ... 44:
				lineColor = 12;
				ColorLines(lineColor, RED);
				break;
				
				case 45 ... 74:
				lineColor = 11;
				ColorLines(lineColor, RED);
				break;
				
				case 75 ... 90:
				lineColor = 10;
				ColorLines(lineColor, RED);
				break;
			}
		}
	}
	
}

void Reference02()
{
	delay = abs(uSecTimer0 - uSecTimer2);
	CalculateAngle(delay, micDist02);
	if(!resetFlag)
	{
		if((micOrder[0] == 2))
		{
			switch (angle)
			{
				case 0 ... 14:
				lineColor = 9;
				ColorLines(lineColor, RED);
				break;
				
				case 15 ... 44:
				lineColor = 10;
				ColorLines(lineColor, RED);
				break;
				
				case 45 ... 74:
				lineColor = 11;
				ColorLines(lineColor, RED);
				break;
				
				case 75 ... 90:
				lineColor = 12;
				ColorLines(lineColor, RED);
				break;
			}
		}
		else
		{
			switch (angle)
			{
				case 0 ... 14:
				lineColor = 9;
				ColorLines(lineColor, RED);
				break;
				
				case 15 ... 44:
				lineColor = 8;
				ColorLines(lineColor, RED);
				break;
				
				case 45 ... 74:
				lineColor = 7;
				ColorLines(lineColor, RED);
				break;
				
				case 75 ... 90:
				lineColor = 6;
				ColorLines(lineColor, RED);
				break;
			}
		}
	}
	
}

void CalculateAngle(int delay, int micDistance)
{
	distance = delay * (float)SpdSound;
 	sprintf(String, "delay = %d\n", delay);
 	Serial_putstring(String);
	sprintf(String, "micDistance = %d\n", micDistance);
	Serial_putstring(String);
	if(distance < (float)micDistance)
	{
		angle = (int) (asin(distance / (float)micDistance ) * (float)180 / PI);
	}
	else
	{
		resetFlag = true;
	}
	
 	sprintf(String, "angle = %d\n", angle);
 	Serial_putstring(String);
}

void resetMics() 
{
	micState = 0;
	//re-enable pin change interrupts
	PCICR |= (1<<PCIE0);
	PCICR |= (1<<PCIE1);
	PCICR |= (1<<PCIE2);
	resetFlag = false;
	masterTrig = false;
		
}


void ColorLines(int lineColor, uint16_t color)
{
	switch (lineColor)
	{
	case 1:
	//1 o clock line
	LCD_drawLine((LCD_WIDTH/2) + 52, (LCD_HEIGHT/2)-30, LCD_WIDTH/2, LCD_HEIGHT/2, color);
	break;
	
	case 2:
	//2 o clock line
	LCD_drawLine((LCD_WIDTH/2) + 30, (LCD_HEIGHT/2)-52, LCD_WIDTH/2, LCD_HEIGHT/2, color);
	break;	
		
	case 3:
	//3 o clock line
	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT/2, LCD_WIDTH-5, LCD_HEIGHT/2, color);
	//arrowhead left
	LCD_drawLine(LCD_WIDTH-5, LCD_HEIGHT/2, LCD_WIDTH-10, (LCD_HEIGHT/2)-5, color);
	//arrowhead right
	LCD_drawLine(LCD_WIDTH-5, LCD_HEIGHT/2, LCD_WIDTH-10, (LCD_HEIGHT/2)+5, color);
	break;	
		
	case 4:
	//4 o clock line
	LCD_drawLine((LCD_WIDTH/2) + 52, (LCD_HEIGHT/2)+30, LCD_WIDTH/2, LCD_HEIGHT/2, color);
	break;	
		
	case 5:
	//5 o clock line
	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT/2, (LCD_WIDTH/2) + 30, (LCD_HEIGHT/2)+52, color);
	break;
	
	case 6:
	//6 o clock line
	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT/2, LCD_WIDTH/2, LCD_HEIGHT-5, color);
	//arrowhead left
	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT-5, (LCD_WIDTH/2)-5, LCD_HEIGHT-10, color);
	//arrowhead right
	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT-5, (LCD_WIDTH/2)+5, LCD_HEIGHT-10, color);
	break;
	
	case 7:
	//7 o clock line
	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT/2, (LCD_WIDTH/2) - 30, (LCD_HEIGHT/2)+52, color);
	break;
	
	case 8:
	//8 o clock line
	LCD_drawLine((LCD_WIDTH/2) - 52, (LCD_HEIGHT/2)+30, LCD_WIDTH/2, LCD_HEIGHT/2, color);
	break;	
	
	case 9:
	//9 o clock line
	LCD_drawLine(5, LCD_HEIGHT/2, LCD_WIDTH/2, LCD_HEIGHT/2, color);
	//arrowhead left
	LCD_drawLine(5, LCD_HEIGHT/2, 10, (LCD_HEIGHT/2)-5, color);
	//arrowhead right
	LCD_drawLine(5, LCD_HEIGHT/2, 10, (LCD_HEIGHT/2)+5, color);
	break;
	
	case 10:
	//10 o clock line
	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT/2, (LCD_WIDTH/2) - 52, (LCD_HEIGHT/2)-30, color);
	break;
	
	case 11:
	//11 o clock line
	LCD_drawLine((LCD_WIDTH/2) - 30, (LCD_HEIGHT/2)-52, LCD_WIDTH/2, LCD_HEIGHT/2, color);
	break;
	
	case 12:
	//12 o clock line
	LCD_drawLine(LCD_WIDTH/2, 5, LCD_WIDTH/2, LCD_HEIGHT/2, color);
	//arrowhead left
	LCD_drawLine(LCD_WIDTH/2, 5, (LCD_WIDTH/2)-5, 10, color);
	//arrowhead right
	LCD_drawLine(LCD_WIDTH/2, 5, (LCD_WIDTH/2)+5, 10, color);
	break;
	}
}

void drawLines()
{
	//Vertical Line with arrowheads (12 & 6)
 	LCD_drawLine(LCD_WIDTH/2, 5, LCD_WIDTH/2, LCD_HEIGHT-5, WHITE);
 	//arrowhead left
 	LCD_drawLine(LCD_WIDTH/2, 5, (LCD_WIDTH/2)-5, 10, WHITE);
 	//arrowhead right
 	LCD_drawLine(LCD_WIDTH/2, 5, (LCD_WIDTH/2)+5, 10, WHITE);
 	
 	//arrowhead left
 	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT-5, (LCD_WIDTH/2)-5, LCD_HEIGHT-10, WHITE);
 	//arrowhead right
 	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT-5, (LCD_WIDTH/2)+5, LCD_HEIGHT-10, WHITE);
 	
 	
 	//Horizontal line with arrowheads (3 & 9)
 	LCD_drawLine(5, LCD_HEIGHT/2, LCD_WIDTH-5, LCD_HEIGHT/2, WHITE);
 	//arrowhead left
 	LCD_drawLine(5, LCD_HEIGHT/2, 10, (LCD_HEIGHT/2)-5, WHITE);
 	//arrowhead right
 	LCD_drawLine(5, LCD_HEIGHT/2, 10, (LCD_HEIGHT/2)+5, WHITE);
 	
 	//arrowhead left
 	LCD_drawLine(LCD_WIDTH-5, LCD_HEIGHT/2, LCD_WIDTH-10, (LCD_HEIGHT/2)-5, WHITE);
 	//arrowhead right
 	LCD_drawLine(LCD_WIDTH-5, LCD_HEIGHT/2, LCD_WIDTH-10, (LCD_HEIGHT/2)+5, WHITE);
 	
 	//1 o clock line
 	LCD_drawLine((LCD_WIDTH/2) + 52, (LCD_HEIGHT/2)-30, LCD_WIDTH/2, LCD_HEIGHT/2, WHITE);
 	
 	//2 o clock line
 	LCD_drawLine((LCD_WIDTH/2) + 30, (LCD_HEIGHT/2)-52, LCD_WIDTH/2, LCD_HEIGHT/2, WHITE);
 	
 	//4 o clock line
 	LCD_drawLine((LCD_WIDTH/2) + 52, (LCD_HEIGHT/2)+30, LCD_WIDTH/2, LCD_HEIGHT/2, WHITE);
 	
 	//5 o clock line
 	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT/2, (LCD_WIDTH/2) + 30, (LCD_HEIGHT/2)+52, WHITE);
 	
 	//7 o clock line
 	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT/2, (LCD_WIDTH/2) - 30, (LCD_HEIGHT/2)+52, WHITE);
 	
 	//8 o clock line
 	LCD_drawLine((LCD_WIDTH/2) - 52, (LCD_HEIGHT/2)+30, LCD_WIDTH/2, LCD_HEIGHT/2, WHITE);
 	
 	//10 o clock line
 	LCD_drawLine(LCD_WIDTH/2, LCD_HEIGHT/2, (LCD_WIDTH/2) - 52, (LCD_HEIGHT/2)-30, WHITE);
 	
 	//11 o clock line
 	LCD_drawLine((LCD_WIDTH/2) - 30, (LCD_HEIGHT/2)-52, LCD_WIDTH/2, LCD_HEIGHT/2, WHITE);
}

