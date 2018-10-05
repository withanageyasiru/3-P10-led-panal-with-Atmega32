
//Scrolls a text on P10 32x16 LED display matrix.



#include <string.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "types.h"
#include "font.h"
#include "fonts/Arial12.h"

/***************************************

Configure Connections

****************************************/

#define HC595_PORT   PORTD
#define HC595_DDR    DDRD

#define HC595_DS_POS PD3      //Data pin (DS) pin location

#define HC595_SH_CP_POS PD4      //Shift Clock (SH_CP) pin location 
#define HC595_ST_CP_POS PD5      //Store Clock (ST_CP) pin location

#define P10_CH_PORT	PORTD
#define P10_CH_DDR	DDRD
#define P10_CH_A_POS	PD6
#define P10_CH_B_POS	PD7

#define P10_EN_PORT	PORTB
#define P10_EN_DDR	DDRB
#define P10_EN_POS	PB0

#define GFX_SCREEN_WIDTH	96
#define GFX_SCREEN_HEIGHT	16
#define GFX_CHAR_SPACING 3



/***************************************
Configure Connections ***ENDS***
****************************************/


uint8_t p10_vram[192];

INT8 GFXWriteStringXY(INT8 x,INT8 y,const char *string,UINT8 color);
INT8 GFXGetCharWidth(char c);
uint8_t CharIndexOfPixel(const char *s, uint16_t pixel);

void ScrollMsg(const char *msg);

void P10SelCh(uint8_t ch);
void P10DispOff();
void P10DispOn();

//Initialize HC595 System
void HC595Init()
{
   //Make the Data(DS), Shift clock (SH_CP), Store Clock (ST_CP) lines output
   HC595_DDR|=((1<<HC595_SH_CP_POS)|(1<<HC595_ST_CP_POS)|(1<<HC595_DS_POS));
   
   P10_CH_DDR|=((1<<P10_CH_A_POS)|(1<<P10_CH_B_POS));
   
   P10_EN_DDR|=(1<<P10_EN_POS);
   
   P10SelCh(3);
}

void P10SelCh(uint8_t ch)
{
	P10_CH_PORT&=(~((1<<P10_CH_A_POS)|(1<<P10_CH_B_POS)));
	
	switch(ch)
	{
		case 0:
			break;
		case 1:
			P10_CH_PORT|=(1<<P10_CH_A_POS);
			break;
		case 2:
			P10_CH_PORT|=(1<<P10_CH_B_POS);
			break;
		case 3:
			P10_CH_PORT|=((1<<P10_CH_A_POS)|(1<<P10_CH_B_POS));
			break;
			
	}
}
void P10DispOff()
{
	P10_EN_PORT&=~(1<<P10_EN_POS);		
}

void P10DispOn()
{
	P10_EN_PORT|=(1<<P10_EN_POS);		
}

void P10PutPixel(int8_t x,int8_t y)
{
	if(x<0 || x>=GFX_SCREEN_WIDTH || y<0 || y>=GFX_SCREEN_HEIGHT) return;
	
	x=GFX_SCREEN_WIDTH-x-1;
	uint8_t xx=x/8;
	
	uint8_t d=p10_vram[xx*16+y];
	
	uint8_t dd=x%8;
	
	dd=7-dd;
	
	d&=~(1<<dd);
	
	p10_vram[xx*16+y]=d;
	
}

void P10Clear()
{
	for(uint8_t i=0;i<192;i++)
		p10_vram[i]=0xff;	
}

//Low level macros to change data (DS)lines
#define HC595DataHigh() (HC595_PORT|=(1<<HC595_DS_POS))

#define HC595DataLow() (HC595_PORT&=(~(1<<HC595_DS_POS)))

//Sends a clock pulse on SH_CP line
void HC595Pulse()
{
   //Pulse the Shift Clock

   HC595_PORT|=(1<<HC595_SH_CP_POS);//HIGH

   HC595_PORT&=(~(1<<HC595_SH_CP_POS));//LOW

}

//Sends a clock pulse on ST_CP line
void HC595Latch()
{
   //Pulse the Store Clock

   HC595_PORT|=(1<<HC595_ST_CP_POS);//HIGH
   _delay_loop_1(1);

   HC595_PORT&=(~(1<<HC595_ST_CP_POS));//LOW
   _delay_loop_1(1);
}


/*

Main High level function to write a single byte to
Output shift register 74HC595. 

Arguments:
   single byte to write to the 74HC595 IC

Returns:
   NONE

Description:
   The byte is serially transfered to 74HC595
   and then latched. The byte is then available on
   output line Q0 to Q7 of the HC595 IC.

*/
void HC595Write(uint8_t data)
{
   //Send each 8 bits serially

   //Order is MSB first
   for(uint8_t i=0;i<8;i++)
   {
      //Output the data on DS line according to the
      //Value of MSB
      if(data & 0b10000000)
      {
         //MSB is 1 so output high

         HC595DataHigh();
      }
      else
      {
         //MSB is 0 so output high
         HC595DataLow();
      }

      HC595Pulse();  //Pulse the Clock line
      data=data<<1;  //Now bring next bit at MSB position

   }

   //Now all 8 bits have been transferred to shift register
   //Move them to output latch at one
   //HC595Latch();
}

void main()
{

   HC595Init();
   
   GFXSetFont(Arial12);
   
   P10Clear();
   
   //Setup timer 0 to automatically refresh the display
   
   // Prescaler = FCPU/64
   TCCR0|=((1<<CS01)|(1<<CS00));

   //Enable Overflow Interrupt Enable
   TIMSK|=(1<<TOIE0);

   //Initialize Counter
   TCNT0=0;

   //Enable Global Interrupt
   sei();


   while(1)
   {
	   P10Clear();
	   
	  // ScrollMsg("  Ishanka");  //Change this to your custom message
	 GFXWriteStringXY(0,0,"L canteen akka",0);
	 GFXWriteStringXY(0,16,"ishanka",0);
	   
	   _delay_ms(1500);
	   
   }


}

ISR(TIMER0_OVF_vect)
{
   static uint8_t ch=0;
   
   for(uint8_t i=0;i<48;i++)
   {
	   HC595Write(p10_vram[i*4+ch]);	   
   }
   
   P10DispOff();
   
   HC595Latch();
   
   P10SelCh(3-ch);
   
   P10DispOn();
   
   ch++;
   
   if(ch==4)
	ch=0;

}

int8_t GFXPutCharXY(int8_t x, int8_t y,char c,uint8_t color)
{

	uint8_t width = 0;
	uint8_t height = __GFXReadFontData(FONT_OFFSET_HEIGHT);
	uint8_t bytes = (height+7)/8;
	
	uint8_t firstChar = __GFXReadFontData(FONT_OFFSET_FIRSTCHAR);
	uint8_t charCount = __GFXReadFontData(FONT_OFFSET_CHARCOUNT);
	
	UINT16 index = 0;
	
	if(c < firstChar || c >= (firstChar+charCount)) {
		return -1;//Error
	}
	
	c-= firstChar;
	
	// read width data, to get the index
	for(uint8_t i=0; i<c; i++) {
		index += __GFXReadFontData(FONT_OFFSET_WTABLE+i);
	}
	index =index*bytes+charCount+FONT_OFFSET_WTABLE;
	width = __GFXReadFontData(FONT_OFFSET_WTABLE+c);


	//Draw
	int8_t _x,_y,fx,fy,b;

	UINT16 address;
	
	_y=y;


	uint8_t shift_val;
	shift_val=(bytes*8)-height;

	bytes--;
	
	for(b=0;b<bytes;b++)
	{
		uint8_t mask=0b00000001;
		for(fy=0;fy<8;fy++)
		{

			if((_y+fy)>(GFX_SCREEN_HEIGHT-1))
			break;

			address=(index+b*width);

			for(_x=x,fx=0;fx<width;fx++,_x++)
			{
				if(_x>(GFX_SCREEN_WIDTH-1))	break;

				uint8_t data=__GFXReadFontData(address);

				uint8_t bit= (data & mask);

				if(bit)
				P10PutPixel(_x,_y+fy);
				//else
				//GFXRawPutPixel(_x,_y+fy,color_invert);

				address++;

			}
			mask=mask<<1;
			
		}
		_y+=8;
	}

	//Last Byte May require shifting so draw it separately

	uint8_t mask=0b00000001<<shift_val;
	for(fy=0;fy<(8-shift_val);fy++)
	{
		if((_y+fy)>(GFX_SCREEN_HEIGHT-1))
		break;
		
		address=(index+b*width);

		for(_x=x,fx=0;fx<width;fx++,_x++)
		{
			if(_x>(GFX_SCREEN_WIDTH-1))	break;

			uint8_t data=__GFXReadFontData(address);

			uint8_t bit= (data & mask);

			if(bit)
			P10PutPixel(_x,_y+fy);
			//else
			//GFXRawPutPixel(_x,_y+fy,color_invert);

			address++;

		}
		mask=mask<<1;
		
	}
	return 1;
}

INT8 GFXWriteStringXY(INT8 x,INT8 y,const char *string,UINT8 color)
{
	INT8 w;
	while(*string!='\0')
	{
		if(GFXPutCharXY(x,y,*string,color)==-1)
		return -1;
		
		w=GFXGetCharWidth(*string);
		if(w==-1)
		return -1;

		x+=w;
		x+=GFX_CHAR_SPACING; //Blank Line after each char
		string++;
	}
	return 1;
}

INT8 GFXGetCharWidth(char c)
{

	UINT8 firstChar = __GFXReadFontData(FONT_OFFSET_FIRSTCHAR);
	UINT8 charCount = __GFXReadFontData(FONT_OFFSET_CHARCOUNT);
	
	
	if(c < firstChar || c >= (firstChar+charCount)) {
		return -1;//Error
	}
	
	c-= firstChar;
	
	return __GFXReadFontData(FONT_OFFSET_WTABLE+c);
}

INT16 GFXGetStringWidth(const char *string)
{
	INT16 w,r;

	w=0;
	while(*string!='\0')
	{
		r=GFXGetCharWidth(*string);
		if(r==-1) return -1;

		w+=r;
		w+=GFX_CHAR_SPACING;//Extra Spacing Between Chars

		string++;
	}

	return w;
}
INT16 GFXGetStringWidthN(const char *string,UINT8 n)
{
	INT16 w,r;
	INT8 i=0;

	w=0;
	while(*string!='\0')
	{
		r=GFXGetCharWidth(*string);
		if(r==-1) return -1;

		w+=r;
		w+=GFX_CHAR_SPACING;//Extra Spacing Between Chars

		string++;
		
		if(i==n) break; else i++;
	}

	return w;
}
uint8_t CharIndexOfPixel(const char *s, uint16_t pixel)
{
	uint8_t index=0;
	
	while(1)
	{
		if(pixel<GFXGetStringWidthN(s,index))
			return index;
		else
			index++;
	}
	
}



void ScrollMsg(const char *msg)
{
	 uint16_t msg_pixel_len=GFXGetStringWidth(msg);
	 
	 msg_pixel_len-=GFX_SCREEN_WIDTH;
	 
	 //msg_pixel_len+=4;
	 
	 for(uint16_t scroll=0; scroll<msg_pixel_len;scroll++)
	 {
		 uint8_t start_char=CharIndexOfPixel(msg,scroll);
		 
		 char temp_string[18];
		 
		 strncpy(temp_string,msg+start_char,17);
		 
		 uint8_t first_char_width=GFXGetCharWidth(temp_string[0]);
		 
		 first_char_width+=GFX_CHAR_SPACING;
		 
		 for(uint8_t scroll2=0;scroll2<first_char_width;scroll2++)
		 {
			 P10Clear();
			 GFXWriteStringXY(-scroll2,0,temp_string,0);
			 
			 
			 if(scroll==0)
			 {
				 _delay_ms(3000);//was 500
			 }
			 else
			 {
				_delay_ms(25);	 
			 }
			 
			 scroll++;
			 
		 } 
		 
	 }
	
}