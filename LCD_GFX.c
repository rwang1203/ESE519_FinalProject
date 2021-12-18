/*
 * LCD_GFX.c
 *
 * Created: 9/20/2021 6:54:25 PM
 *  Author: You
 */ 

#include "LCD_GFX.h"
#include "ST7735.h"
#include "SerialPrint.h"
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
char String[25];

/******************************************************************************
* Local Functions
******************************************************************************/



/******************************************************************************
* Global Functions
******************************************************************************/

/**************************************************************************//**
* @fn			uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
* @brief		Convert RGB888 value to RGB565 16-bit color data
* @note
*****************************************************************************/
uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
	return ((((31*(red+4))/255)<<11) | (((63*(green+2))/255)<<5) | ((31*(blue+4))/255));
}

/**************************************************************************//**
* @fn			void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color)
* @brief		Draw a single pixel of 16-bit rgb565 color to the x & y coordinate
* @note
*****************************************************************************/
void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color) {
	LCD_setAddr(x,y,x,y);
	SPI_ControllerTx_16bit(color);
}

/**************************************************************************//**
* @fn			void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor)
* @brief		Draw a character starting at the point with foreground and background colors
* @note
*****************************************************************************/
void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor){
	uint16_t row = character - 0x20;		//Determine row of ASCII table starting at space
	int i, j;
	if ((LCD_WIDTH-x>7)&&(LCD_HEIGHT-y>7)){
		for(i=0;i<5;i++){
			uint8_t pixels = ASCII[row][i]; //Go through the list of pixels
			for(j=0;j<8;j++){
				if ((pixels>>j)&(1==1)){
					LCD_drawPixel(x+i,y+j,fColor);
				}
				else {
					LCD_drawPixel(x+i,y+j,bColor);
				}
			}
		}
	}
}


/******************************************************************************
* LAB 4 TO DO. COMPLETE THE FUNCTIONS BELOW.
* You are free to create and add any additional files, libraries, and/or
*  helper function. All code must be authentically yours.
******************************************************************************/

/**************************************************************************//**
* @fn			void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
* @brief		Draw a colored circle of set radius at coordinates
* @note
*****************************************************************************/
void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
{
	// Circle Equation y = v +- sqrt(r^2 - (x-u)^2)
	int x, y;
	for(x=(x0-radius);x<=(x0+radius);x++){
		for(y=(y0-radius);y<=(y0+radius);y++){ 
			if(y < (y0 + sqrt(pow(radius, 2) - pow(x - x0, 2))) && y > (y0 - sqrt(pow(radius, 2) - pow(x - x0, 2)))) {
				LCD_drawPixel(x, y, color);
				// sprintf(String,"x,y: %d, %d \n", x, y);
				// UART_putstring(String);
			}
		}
	}
}


/**************************************************************************//**
* @fn			void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
* @brief		Draw a line from and to a point with a color
* @note
*****************************************************************************/
void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
{
	int x, y; bool Tends_Horizontal;
	
	Tends_Horizontal = (abs(x0 - x1) >= abs(y0 - y1));
	
	if(x1 > x0 && Tends_Horizontal) {
		for(x=x0;x<=x1;x++){ 
			y = round(( 1.0 * (y1-y0) / (x1-x0)) * (x - x0) + y0); // Point Slope Form y - y1 = m( x - x1 )
// 			sprintf(String,"x,y: %d, %d \n", x, y);
// 			Serial_putstring(String);
			LCD_drawPixel(x,y,c);
		} 
	} else if (x1 <= x0 && Tends_Horizontal) {
		for(x=x1;x<=x0;x++){
			
			y = round(( 1.0 * (y1-y0) / (x1-x0)) * (x - x0) + y0); 
// 			sprintf(String,"x,y: %d, %d \n", x, y);
// 			Serial_putstring(String);
			LCD_drawPixel(x,y,c);
		}
	} else if (y1 > y0 && !Tends_Horizontal) {
		for(y=y0;y<=y1;y++){
			x = round(( 1.0 * (x1-x0) / (y1-y0)) * (y - y0) + x0);
			LCD_drawPixel(x,y,c);
		} 
	} else if (y1 <= y0 && !Tends_Horizontal) {
		for(y=y0;y<=y1;y++){
			x = round(( 1.0 * (x1-x0) / (y1-y0)) * (y - y0) + x0); 
			LCD_drawPixel(x,y,c);
		}
	}
}



/**************************************************************************//**
* @fn			void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
* @brief		Draw a colored block at coordinates
* @note
*****************************************************************************/
void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
{
	LCD_drawLine(x0,y0,x0,y1, color);
	LCD_drawLine(x1,y0,x1,y1, color);
	LCD_drawLine(x0,y0,x1,y0, color);
	LCD_drawLine(x0,y1,x1,y1, color);
}

/**************************************************************************//**
* @fn			void LCD_setScreen(uint16_t color)
* @brief		Draw the entire screen to a color
* @note
*****************************************************************************/
void LCD_setScreen(uint16_t color) 
{
	int x = 0; int y = 0;
	int i, j;
	for(i=0;i<LCD_WIDTH;i++){ //Go through the list of pixels
		for(j=0;j<LCD_HEIGHT;j++){
			LCD_drawPixel(x+i,y+j,color);
		}
	}
}

/**************************************************************************//**
* @fn			void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
* @brief		Draw a string starting at the point with foreground and background colors
* @note
*****************************************************************************/
void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
{
	int x_loc = x;
	for (int i = 0; i < strlen(str); i++) {
		LCD_drawChar(x_loc, y, str[i], fg, bg);
		x_loc += 5;  
	}
}