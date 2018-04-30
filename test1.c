#include <avr/io.h>
#include "usart_ATmega1284.h"
#include "scheduler.h"
#include "timer.h"
#include "rc522.h"
#include "pwn.h"

unsigned char code;
unsigned char piccpresent;

enum sendingdata {wait, senditem1, senditem2};
	
int tick1(int state){
	switch(state){
		case wait:
			if(~PINA & 0x01){
				state = senditem1;
			}
			else if(~PINA & 0x02){
				state = senditem2;
			}
			else{
				state = wait;
			}
			break;
		case senditem1:
			state = wait;
			break;
		case senditem2:
			state = wait;
			break;
		default:
			state = wait;
			break;
	}
	switch(state){
		case wait:
			//PORTB = 0x00;
			if(USART_IsSendReady(0) && USART_IsSendReady(1)){
				USART_Send(0x00,0);
				USART_Send(0x00,1);
			}
			//USART_Send(0x00,0);
			//USART_Send(0x00,1);
			break;
		case senditem1:	
			if(USART_IsSendReady(0)){
				USART_Send(0x01,0);
				//USART_Send(0x01,1);
				//PORTB = 0x01;
			}
			//USART_Send(0x01,1);
			break;
		case senditem2:
			if(USART_IsSendReady(1)){
				USART_Send(0x01,1);
				//PORTB = 0x01;
			}
		default:
			break;
	}
	return state;
}

enum rfid{wait1, getcode};

int tick2(int state){
	switch(state){
		case wait1:
			state = getcode;
			break;
		case getcode:
			state = wait1;
			break;
		default:
			state = wait1;
			break;
	}
	switch(state){
		case wait1:
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
			//code = 0x00;
			break;
	}
	return state;
}

enum securitysystem{wait2, alarmoff, alarmon, alarmstate1, alarmstate2};

int tick3(int state){
	switch(state){
		case wait2:
			state = alarmoff;
			break;
		case alarmoff:
			if(~PINA & 0x04){
				state = alarmon;
			}
			else
			{
				state = alarmoff;
			}
			break;
		case alarmon:
			if(~PINA & 0x08 || code == 0xEE){
				state = alarmoff;
			}
			//else if(~PINA & 0x10){
				//state = alarmstate1;
			//}
			else{
				state = alarmon;
			}
 			break;
		case alarmstate1:
			if(~PINA & 0x08){
				state = alarmoff;
			}
			else
			{
				state = alarmstate2;
			}
			break;
		case alarmstate2:
			if(~PINA & 0x08){
				state = alarmoff;
			}
			else
			{
				state = alarmstate1;
			}
			break;
		default:
			state = wait2;
			break;
	}
	switch(state){
		case wait2:
			break;
		case alarmoff:
			PORTC = 0x01;
			//PWM_off();
			break;
		case alarmon:
			PORTC = 0x02;
			break;
		case alarmstate1:
			PORTC = 0x03;
			PWM_on();
			break;
		case alarmstate2:
			PORTC = 0x00;
			PWM_off();
			break;
		default:
			break;
	}
	return state;
}


int main(void)
{
	SPI_MasterInit();
	rc522init();
	DDRA = 0x00; PORTA = 0xFF; //Inputs
	DDRC = 0xFF; PORTC = 0x00; //outputs
	DDRD = 0xFF; PORTD = 0x00; //outputs
	//DDRB = 0xFF; PORTB = 0x00; //Outputs
	
	//Period for the tasks
	unsigned long int SMtest1_calc = 10;
	unsigned long int SMtest2_calc = 100;
	unsigned long int SMtest3_calc = 100;
	
	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMtest1_calc, SMtest2_calc);
	tmpGCD = findGCD(tmpGCD, SMtest3_calc);
	///Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMtest1_period = SMtest1_calc/GCD;
	unsigned long int SMtest2_period = SMtest2_calc/GCD;
	unsigned long int SMtest3_period = SMtest3_calc/GCD;
	
	//Declare an array of tasks
	static task task1, task2, task3;
	task *tasks[] = { &task1, &task2, &task3};
	
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	
	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMtest1_period;//Task Period.
	task1.elapsedTime = SMtest1_period;//Task current elapsed time.
	task1.TickFct = &tick1;//Function pointer for the tick.
	
	task2.state = -1;//Task initial state.
	task2.period = SMtest2_period;//Task Period.
	task2.elapsedTime = SMtest2_period;//Task current elapsed time.
	task2.TickFct = &tick2;//Function pointer for the tick.
	
	task3.state = -1;//Task initial state.
	task3.period = SMtest3_period;//Task Period.
	task3.elapsedTime = SMtest3_period;//Task current elapsed time.
	task3.TickFct = &tick3;//Function pointer for the tick.
	
	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();
	
	initUSART(0);
	initUSART(1);

	unsigned short i; // Scheduler for-loop iterator
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
    }
}
