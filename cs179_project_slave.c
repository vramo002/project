#include <avr/io.h>
#include "usart_ATmega1284.h"
#include "scheduler.h"
#include "timer.h"
#include "pwn.h"

unsigned char variable = 0;

enum recivingdata {wait, recive};

int tick1(int state){
	switch(state){
		case wait:
			state = recive;
			break;
		case recive:
			state = wait;
			break;
		default:
			state = wait;
			break;
	}
	switch(state){
		case wait:
			//PORTA = 0x00;
			break;
		case recive:
			if(USART_HasReceived(0)){
				variable = USART_Receive(0); // will get the info from the master atmega
				//PORTA = variable;
			}
			break;
		default:
			break;
	}
	return state;
}

enum usingdata {wait1, turnon, turnoff};

int tick2(int state){
	switch(state){
		case wait1:
			if(variable == 0x01){ //if signal received the turn on the buzzer and led
				state = turnon;
			}
			else{
				state = wait1;
			}
			break;
		case turnon:
			if(~PINB & 0x01){
				state = wait1;
			}
			else{
				state = turnoff;
			}
			break;
		case turnoff:
			if(~PINB & 0x01){
				state = wait1;
			}
			else{
				state = turnon;
			}
			break;
		default:
				state = wait1;
			break;
	}	
	switch(state){
		case wait1:
			PORTA = 0x00;
			PWM_off();
			break;
		case turnon:				//both state turnon and turnoff are used to flash the led
			PORTA = 0x01; //turn on led 
			PWM_on(); //turn on buzzer
			break;
		case turnoff:
			PORTA = 0x00; //turn off led
			PWM_off(); // turn off buzzer 
			break;
		default:
			break;
	}
	return state;
}
	


int main(void)
{
	DDRA = 0xFF;  PORTA = 0x00; // Outputs
	DDRB = 0x08;  PORTB = 0xF7;
	//Period for the tasks
	unsigned long int SMtest1_calc = 10;
	unsigned long int SMtest2_calc = 250; //flash led for 250ms
	
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
	
	initUSART(0);
	

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

