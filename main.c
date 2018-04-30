#include <avr/io.h>
#include "scheduler.h"
#include "timer.h"

unsigned char receivedData;
unsigned char code;
unsigned char piccpresent;

void SPI_MasterInit(void)
{
	DDRB = 0xBF; PORTB = 0x40;
	SPCR |= (1<<SPE) | (1<<MSTR) | (1<<SPR0);
}

unsigned char SPI_MasterTransmit(unsigned char cData)
{
	//PORTB = PORTB & 0xEF;
	SPDR = cData;
	
	while(!(SPSR & (1<<SPIF)))
	{
		;
	}
	return SPDR;
	//PORTB = PORTB | ~(0xEF);
}

unsigned char readrc522(unsigned char reg)
{
	PORTB = PORTB & 0xEF;
	unsigned char data;
	SPI_MasterTransmit(((reg<<1)&0x7F) | 0x80);
	data = SPI_MasterTransmit(0x00);
	PORTB = PORTB | ~(0xEF);
	return data;
}

void writerc522(unsigned char reg, unsigned char data)
{
	PORTB = PORTB & 0xEF;
	SPI_MasterTransmit((reg<<1)&0x7F);
	SPI_MasterTransmit(data);
	PORTB = PORTB | ~(0xEF);
}

void rc522init()
{
	unsigned char txcontorlreg;
	//reset 
	writerc522(0x01, 0x0F);
	
	writerc522(0x2A,0x8D);
	writerc522(0x2B,0x3E);
	writerc522(0x2C,0x03); //
	writerc522(0x2D,0xE8); //
	writerc522(0x15,0x40); //100% ASK as explain in pg8 datasheet
	writerc522(0x11,0x3D);
	txcontorlreg = readrc522(0x14); //reads to see if the transmitter is on pg33 datasheet
	writerc522(0x14, txcontorlreg|0x03); //set the transmitter on is on pg33 datasheet
}

unsigned short int rc522id()
{
	unsigned char cmd_to_picc = 0x93;
	unsigned char cmd_to_picc2 = 0x20;
	unsigned char id;
	unsigned char transceive_cmd = 0x0C;
	unsigned char clear;
	
	writerc522(0x0D, 0x00); // set the txlastbits
	//clear = readrc522(0x04);
	//writerc522(0x04, clear&(~0x80)); //clear the interupt bits
	writerc522(0x04,0x7F);
	//clear = readrc522(0x0A);
	//writerc522(0x0A, clear|0x80); //flush the FIFO buffer
	writerc522(0x0A,0x80);
		
	writerc522(0x01,0x00);
	
	//writerc522(0x09,0x52);
	writerc522(0x09, cmd_to_picc);
	writerc522(0x09, cmd_to_picc2);
	
	writerc522(0x01, transceive_cmd);
	
	clear = readrc522(0x0D);
	writerc522(0x0D, clear|0x80);
	
	
	for(int i = 2000; i > 0; i--)
	{
		unsigned char check = readrc522(0x04);
		if(check & 0x30)
		{
			break;
		}
	}
	clear = readrc522(0x0D);
	writerc522(0x0D, clear&(~0x80));
	
	//readrc522(0x09);
	id = readrc522(0x09);

	
	return id;
	
	
}

unsigned char rc522_wakeup()
{
	writerc522(0x0D, 0x07);
	
	unsigned char irqEn = 0x77;
	unsigned char waitIRq = 0x30;
	
	unsigned char n;
	unsigned char temp;
	
	n = readrc522(0x04);
	writerc522(0x04, n&(~0x80));
	
	//n = readrc522(0x0A);
	//writerc522(0x0A, n|0x80);
	writerc522(0x0A,0x80);
	writerc522(0x01, 0x00);
	
	writerc522(0x09, 0x52);
	
	writerc522(0x01, 0x0C);
	
	n = readrc522(0x0D);
	writerc522(0x0D, n|0x80);
	
	for(int i = 2000; i > 0; i--)
	{
		n = readrc522(0x04);
	}
	
	//int i = 2000;
	//do 
	//{
		//n = readrc522(0x04);
		//i--;
	//} while ((i!=0) && !(n&0x01) && !(n&waitIRq));
	
	//temp = readrc522(0x0D);
	//writerc522(0x0D, temp&(~0x80));
	
	//if(!(readrc522(0x06)&0x1B))
	//{
		//return readrc522(0x09);
	//}
	//else
	//{
		//return 0xFF;
	//}
	return readrc522(0x09);
	
}

enum rfid{wait, getcode};
	
int tick1(int state){
	switch(state){
		case wait:
			state = getcode;
			break;
		case getcode:
			state = wait;
			break;
		default:
			state = wait;
			break;
	}
	switch(state){
		case wait:
			code = 0x00;
			//PORTD = 0xFF;
			break;
		case getcode:
			piccpresent = rc522_wakeup();
			if(piccpresent == 0x04){
				code = rc522id();
			}
			else
			{
				code = 0x00;
			}
			//PORTD = 0x01;
			break;
		default:
			code = 0x00;
			break;
	}
	return state;
}

enum openclose {wait1, open, close};

int tick2(int state){
	switch(state){
		case wait1:
			if(code == 0xCD)
			{
				state = open;
			}
			else
			{
				state = close;
			}
			break;
		case open:
			if(code == 0xCD)
			{
				state = open;
			}
			else
			{
				state = close;
			}
			break;
		case close:
			if(code == 0xCD)
			{
				state = open;
			}
			else
			{
				state = close;
			}
			break;
		default:
			state = wait1;
			break;
	}
	switch(state){
		case wait1:
			PORTD = 0x00;
			break;
		case open:
			PORTD = 0xFF;
			break;
		case close:
			PORTD = 0x00;
			break;
		default:
			break;
	}
	return state;
}

int main(void)
{
	//TimerSet(100);
	//TimerOn();
	SPI_MasterInit();
	rc522init();
	//unsigned char version = 0x00;
	//unsigned char idd = 0x00;
	//version = rc522_wakeup();
	//version = rc522id();
	//version = readrc522(0x0A);
	
	//Period for the tasks
	unsigned long int SMtest1_calc = 100;
	unsigned long int SMtest2_calc = 40;
	
	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMtest1_calc, SMtest2_calc);
	///Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMtest1_period = SMtest1_calc/GCD;
	unsigned long int SMtest2_period = SMtest2_calc/GCD;
	
	//Declare an array of tasks
	static task task1, task2;
	task *tasks[] = { &task1, &task2};
	
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	
	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMtest1_period;//Task Period.
	task1.elapsedTime = SMtest1_period;//Task current elapsed time.
	task1.TickFct = &tick1;//Function pointer for the tick.
	
	// Task 1
	task2.state = -1;//Task initial state.
	task2.period = SMtest2_period;//Task Period.
	task2.elapsedTime = SMtest2_period;//Task current elapsed time.
	task2.TickFct = &tick2;//Function pointer for the tick.
	
	
	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();
	
	unsigned short i; // Scheduler for-loop iterator
	
	DDRD = 0xFF; PORTD = 0x00; //Output
	while (1)
	{
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
		//version = rc522id();
		
		/*version = rc522_wakeup();
		if(version == 0x04)
		{
			idd = rc522id();
			PORTD = idd;
		}
		else
		{
			PORTD = 0x00;
			version = 0x00;
		}*/
		
		//version = rc522id();
		//version = readrc522(0x09);
		/*if(idd == 0xCD)
		{
			PORTD = idd;
		}
		else if(idd == 0x69)
		{
			PORTD = idd;
		}
		else
		{
			PORTD = 0x00;
		}*/
		//PORTD = version;
		
		//PORTD = version;
		//SPI_MasterTransmit(0x02);
		//while(!TimerFlag);
		//TimerFlag = 0;
	}
}


