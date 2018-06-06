#ifndef USART_ATtiny4313_H_
#define USART_ATtiny4313_H_

#define F_CPU 8000000UL // Assume uC operates at 8MHz
#define BAUD_RATE 9600
#define BAUD_PRESCALE (((F_CPU / (BAUD_RATE * 16UL))) - 1)

void USART_Init(){
	/* Enable receiver and transmitter */
	UCSRB = (1<<RXEN)|(1<<TXEN);
	/* Set frame format: 8data, 2stop bit */
	UCSRC = (1<<USBS)|(1<<UCSZ0);
	/* Set baud rate */
	UBRRH = (unsigned char)(BAUD_PRESCALE>>8);
	UBRRL = (unsigned char)BAUD_PRESCALE;
	
}

void USART_Transmit4313(unsigned char data){
	/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)) );
	/* Put data into buffer, sends the data */
	UDR = data;
}

unsigned char USART_Receive4313(void){
	/* Wait for data to be received */
	while ( !(UCSRA & (1<<RXC)) );
	/* Get and return received data from buffer */
	return UDR;
}

#endif