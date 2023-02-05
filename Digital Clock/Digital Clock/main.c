/*
 * Digital Clock.c
 *
 * Created: 22/01/2023 12:21:13
 * Author : Mwas
*/


#define F_CPU 8000000UL
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <util/twi.h>
#include <util/delay.h>



#define Device_Write_address	0xD0	/* Define RTC DS1307 slave write address */
#define Device_Read_address	0xD1	/* Make LSB bit high of slave address for read */
#define TimeFormat12		0x40	/* Define 12 hour format */
#define AMPM			0x20

/* Define bit rate */
#define BITRATE(TWSR)	((F_CPU/20000)-16)/(2*pow(4,(TWSR&((1<<TWPS0)|(1<<TWPS1)))))





unsigned char number[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
int digit;
void display();
void read();
void write();

int hund;
int tens;
int ones;


void I2C_Init()			/* I2C initialize function */
{
	TWBR = BITRATE(TWSR=0x00);	/* Get bit rate register value by formula */
}
uint8_t I2C_Start(char write_address)/* I2C start function */
{
	uint8_t status;		/* Declare variable */
	TWCR=(1<<TWSTA)|(1<<TWEN)|(1<<TWINT); /* Enable TWI, generate START */
	while(!(TWCR&(1<<TWINT)));	/* Wait until TWI finish its current job */
	status=TWSR&0xF8;		/* Read TWI status register */
	if(status!=0x08)		/* Check weather START transmitted or not? */
	return 0;			/* Return 0 to indicate start condition fail */
	TWDR=write_address;		/* Write SLA+W in TWI data register */
	TWCR=(1<<TWEN)|(1<<TWINT);	/* Enable TWI & clear interrupt flag */
	while(!(TWCR&(1<<TWINT)));	/* Wait until TWI finish its current job */
	status=TWSR&0xF8;		/* Read TWI status register */
	if(status==0x18)		/* Check for SLA+W transmitted & ack received */
	return 1;			/* Return 1 to indicate ack received */
	if(status==0x20)		/* Check for SLA+W transmitted &nack received */
	return 2;			/* Return 2 to indicate nack received */
	else
	return 3;			/* Else return 3 to indicate SLA+W failed */
}

uint8_t I2C_Repeated_Start(char read_address) /* I2C repeated start function */
{
	uint8_t status;		/* Declare variable */
	TWCR=(1<<TWSTA)|(1<<TWEN)|(1<<TWINT);/* Enable TWI, generate start */
	while(!(TWCR&(1<<TWINT)));	/* Wait until TWI finish its current job */
	status=TWSR&0xF8;		/* Read TWI status register */
	if(status!=0x10)		/* Check for repeated start transmitted */
	return 0;			/* Return 0 for repeated start condition fail */
	TWDR=read_address;		/* Write SLA+R in TWI data register */
	TWCR=(1<<TWEN)|(1<<TWINT);	/* Enable TWI and clear interrupt flag */
	while(!(TWCR&(1<<TWINT)));	/* Wait until TWI finish its current job */
	status=TWSR&0xF8;		/* Read TWI status register */
	if(status==0x40)		/* Check for SLA+R transmitted &ack received */
	return 1;			/* Return 1 to indicate ack received */
	if(status==0x48)		/* Check for SLA+R transmitted &nack received */
	return 2;			/* Return 2 to indicate nack received */
	else
	return 3;			/* Else return 3 to indicate SLA+W failed */
}

uint8_t I2C_Write(char data)	/* I2C write function */
{
	uint8_t status;		/* Declare variable */
	TWDR=data;			/* Copy data in TWI data register */
	TWCR=(1<<TWEN)|(1<<TWINT);	/* Enable TWI and clear interrupt flag */
	while(!(TWCR&(1<<TWINT)));	/* Wait until TWI finish its current job */
	status=TWSR&0xF8;		/* Read TWI status register */
	if(status==0x28)		/* Check for data transmitted &ack received */
	return 0;			/* Return 0 to indicate ack received */
	if(status==0x30)		/* Check for data transmitted &nack received */
	return 1;			/* Return 1 to indicate nack received */
	else
	return 2;			/* Else return 2 for data transmission failure */
}
char I2C_Read_Ack()		/* I2C read ack function */
{
	TWCR=(1<<TWEN)|(1<<TWINT)|(1<<TWEA); /* Enable TWI, generation of ack */
	while(!(TWCR&(1<<TWINT)));	/* Wait until TWI finish its current job */
	return TWDR;			/* Return received data */
}

char I2C_Read_Nack()		/* I2C read nack function */
{
	TWCR=(1<<TWEN)|(1<<TWINT);	/* Enable TWI and clear interrupt flag */
	while(!(TWCR&(1<<TWINT)));	/* Wait until TWI finish its current job */
	return TWDR;		/* Return received data */
}
void I2C_Stop()			/* I2C stop function */
{
	TWCR=(1<<TWSTO)|(1<<TWINT)|(1<<TWEN);/* Enable TWI, generate stop */
	while(TWCR&(1<<TWSTO));	/* Wait until stop condition execution */
}



int second,minute,hour,day,date,month,year;

bool IsItPM(char hour_)
{
	if(hour_ & (AMPM))
	return 1;
	else
	return 0;
}


void RTC_Read_Clock(char read_clock_address)
{
	I2C_Start(Device_Write_address);/* Start I2C communication with RTC */
	I2C_Write(read_clock_address);	/* Write address to read */
	I2C_Repeated_Start(Device_Read_address);/* Repeated start with device read address */

	second = I2C_Read_Ack();	/* Read second */
	minute = I2C_Read_Ack();	/* Read minute */
	hour = I2C_Read_Nack();		/* Read hour with Nack */
	I2C_Stop();			/* Stop i2C communication */
}

void RTC_Read_Calendar(char read_calendar_address)
{
	I2C_Start(Device_Write_address);
	I2C_Write(read_calendar_address);
	I2C_Repeated_Start(Device_Read_address);

	day = I2C_Read_Ack();		/* Read day */
	date = I2C_Read_Ack();		/* Read date */
	month = I2C_Read_Ack();		/* Read month */
	year = I2C_Read_Nack();		/* Read the year with Nack */
	I2C_Stop();			/* Stop i2C communication */
}








int main(void)
{
	
	DDRB=0xff;
	DDRA=0xfe;
	

	/*char buffer[20];
	char* days[7]= {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};*/

	I2C_Init();			/* Initialize I2C */
	

	while(1)
	{
		
		int k=0;
		unsigned char value;
		RTC_Read_Clock(0);	/* Read clock with second add. i.e location is 0 */
		for(int i=0;i<=4;i++){
			
			value =(hour&(00100001 << i));
			if(value == 00000000){
				k=k+0;
				}else{
				k=k +(pow(2,i));
			}
		}
		
		write(k);
		
		_delay_ms(5);
		k=0;
		value =0;
		for(int i=0;i<=5;i++){
			
			value =(minute &(00000001 << i));
			if(value == 00000000){
				k=k+0;
				}else{
				k=k +(pow(2,i));
			}
		}
		read(k);
		_delay_ms(5);
		
		
	}
}





void write(int hrs){
	
	
	tens=hrs/10;
	PORTA=~(0x02);
	display(tens);
	PORTA=0x0f;
	
	ones=hrs%10;
	PORTA=~(0x01);
	display(ones);
	PORTA=0x0f;
		
}

void read(int hrs){
	
	tens = hrs/10;
	PORTA=~(0x04);
	display(tens);
	PORTA=0x0f;
	ones=hrs%10;
	PORTA=~(0x08);
	display(ones);
	PORTA=0x0f;
	
	
}

void display(int digit){
	PORTB=number[digit];
	_delay_ms(5);
	
}
