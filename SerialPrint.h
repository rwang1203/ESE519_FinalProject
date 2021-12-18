/*
 * SerialPrint.h
 *
 * Created: 10/8/2021 10:30:19 AM
 *  Author: rawang
 */ 


#ifndef SERIALPRINT_H_
#define SERIALPRINT_H_

void Serial_init(int prescale);

void Serial_send(unsigned char data);

void Serial_putstring(char* StringPtr);

#endif /* SERIALPRINT_H_ */