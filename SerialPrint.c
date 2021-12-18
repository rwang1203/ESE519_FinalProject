/*
 * SerialPrint.c
 *
 * Created: 10/8/2021 10:28:11 AM
 * Author : rawang
 */ 
#include <avr/io.h>
#include "SerialPrint.h"


void Serial_init(int prescale)
{
	//Set baud rate using UBRR0H/UBRR0L
	UBRR0H = (unsigned char)(prescale>>8); //shift prescale to upper 4 bits
	UBRR0L = (unsigned char) prescale; //bottom 8 bits of prescaled baud rate
	//Enable receiver and transmitter
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	//Frame format: 1 stop bit, 8 data bits
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00); // 8 data bits
	UCSR0C &= ~(1<<USBS0); //clear for 1 stop bit
}

void Serial_send(unsigned char data)
{
	// Wait for empty transmit buffer
	while(!(UCSR0A & (1<<UDRE0)));
	// Put data into buffer and send data
	UDR0 = data;
}

void Serial_putstring(char* StringPtr)
{
	while(*StringPtr != 0x00)
	{
		Serial_send(*StringPtr);
		StringPtr++;
	}
}

