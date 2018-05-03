#ifndef RC522_H_
#define RC522_H_


void SPI_MasterInit(void) //function to start up spi
{
	DDRB = 0xBF; PORTB = 0x40; //set MOSI, MISO, SCK as output and SS as input
	SPCR |= (1<<SPE) | (1<<MSTR) | (1<<SPR0);
}

unsigned char SPI_MasterTransmit(unsigned char cData) // the spi function to transmit as the microcontroler is the master
{
	//PORTB = PORTB & 0xEF;
	SPDR = cData; //start transmission
	
	while(!(SPSR & (1<<SPIF))) //wait for transmission to end
	{
		;
	}
	return SPDR; //get the data back
	//PORTB = PORTB | ~(0xEF);
}

unsigned char readrc522(unsigned char reg)
{
	PORTB = PORTB & 0xEF; //set the SS TO high 
	unsigned char data;
	SPI_MasterTransmit(((reg<<1)&0x7F) | 0x80); //transmits the reg to the RFID sensor 
	data = SPI_MasterTransmit(0x00); // then when reg is set then get the data
	PORTB = PORTB | ~(0xEF); // set SS to low
	return data;
}

void writerc522(unsigned char reg, unsigned char data)
{
	PORTB = PORTB & 0xEF; //set the SS TO high 
	SPI_MasterTransmit((reg<<1)&0x7F); //transmits the reg to the RFID sensor 
	SPI_MasterTransmit(data); //the when the reg is set write to that register 
	PORTB = PORTB | ~(0xEF); // set SS to low
}

void rc522init()
{
	unsigned char txcontorlreg;
	//reset 
	writerc522(0x01, 0x0F); //reset the rfid sesnor
	
	writerc522(0x2A,0x8D); // set the prescaler value 
	writerc522(0x2B,0x3E); //set the prescaler value 
	writerc522(0x2C,0x03); //set the timer when to reload again 
	writerc522(0x2D,0xE8); //set the timer when to reload again 
	writerc522(0x15,0x40); //100% ASK as explain in pg8 datasheet
	writerc522(0x11,0x3D); //change the CRCPreset to 6363h because it orgianlly FFFFh
	txcontorlreg = readrc522(0x14); //reads to see if the transmitter is on pg33 datasheet
	writerc522(0x14, txcontorlreg|0x03); //set the transmitter on is on pg33 datasheet
}

unsigned short int rc522id()
{
	unsigned char cmd_to_picc = 0x93; //command need to get the id
	unsigned char cmd_to_picc2 = 0x20; //command need to get the id
	unsigned char id;
	unsigned char transceive_cmd = 0x0C; // the command need to give to the RFID to now it will communcate with a tag
	unsigned char clear;
	
	writerc522(0x0D, 0x00); // set the txlastbits
	//clear = readrc522(0x04);
	//writerc522(0x04, clear&(~0x80)); //clear the interupt bits
	writerc522(0x04,0x7F); //clear interupt bits
	//clear = readrc522(0x0A);
	//writerc522(0x0A, clear|0x80); //flush the FIFO buffer
	writerc522(0x0A,0x80); //clear FIFO buffer
		
	writerc522(0x01,0x00); // stops any commands current executing
	
	//writerc522(0x09,0x52);
	writerc522(0x09, cmd_to_picc); //set the fifo register with data
	writerc522(0x09, cmd_to_picc2);
	
	writerc522(0x01, transceive_cmd); //transmit the data from fifo to tag 
	
	clear = readrc522(0x0D);
	writerc522(0x0D, clear|0x80); // setting bit to start the transmission
	
	
	for(int i = 2000; i > 0; i--) // loop to wait finish transmission
	{
		unsigned char check = readrc522(0x04);
		if(check & 0x30)
		{
			break;
		}
	}
	clear = readrc522(0x0D);
	writerc522(0x0D, clear&(~0x80)); //unset the transmission bit
	
	//readrc522(0x09);
	id = readrc522(0x09); // get the data from the fifo 

	
	return id;
	
	
}

unsigned char rc522_wakeup()
{
	writerc522(0x0D, 0x07);
	
	//unsigned char irqEn = 0x77;
	//unsigned char waitIRq = 0x30;
	
	unsigned char n;
	unsigned char temp;
	
	n = readrc522(0x04);
	writerc522(0x04, n&(~0x80)); //clear interupt bits
	
	//n = readrc522(0x0A);
	//writerc522(0x0A, n|0x80);
	writerc522(0x0A,0x80); //clear FIFO buffer
	writerc522(0x01, 0x00); // stops any commands current executing
	
	writerc522(0x09, 0x52); //set the fifo register with data
	
	writerc522(0x01, 0x0C); //transmit the data from fifo to tag 
	
	n = readrc522(0x0D);
	writerc522(0x0D, n|0x80); // setting bit to start the transmission
	
	for(int i = 2000; i > 0; i--)  // loop to wait finish transmission
	{
		n = readrc522(0x04);
	}
	
	
	return readrc522(0x09); // get the data from the fifo 
	
}
#endif