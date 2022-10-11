/*
    Part of the Real-Time Embedded Systems course at Halmstad University
    Copyright (c) 2017, Sebastian Kunze <sebastian.kunze@hh.se>
    All rights reserved.
*/
/*
 * Modified by Wagner Morais on Sep 2022.
 */

/* PiFace Control and Display Driver */
#include <stdint.h>
#include <string.h>
#include "rpi-gpio.h"
#include "piface.h"
#include <stdlib.h>

int cnt;

/* Bit-Banging SPI Driver */
static void spi_init(void){
	RPI_GetGpio()->GPFSEL0 |= (1 << 21);			    /* GPIO  7,  9 */
	RPI_GetGpio()->GPFSEL1 |= (1 <<  0) | (1 <<  3);	/* GPIO 10, 11 */
	RPI_GetGpio()->GPSET0 = (1 << 7);			        /* /CE high */
	RPI_GetGpio()->GPCLR0 = (1 << 11);			        /* CLK low */
}

static void spi_start(void){
	/* /CE low  */
	RPI_GetGpio()->GPCLR0 = (1 << 7);			        
}

static void spi_stop(void){
	/* /CE high */
	RPI_GetGpio()->GPSET0 = (1 << 7);			        
}

static void spi_byte(const uint8_t out, uint8_t *in){
	uint8_t tmpin = 0, tmpout = out;

	/* clock each bit out and read back */
	for(int i = 0; i < 8; i++){
		tmpin <<= 1;
		if(tmpout & 0x80)
			// set MOSI
			RPI_GetGpio()->GPSET0 = (1 << 10);
		else
			RPI_GetGpio()->GPCLR0 = (1 << 10);
		// read MISO
		if(RPI_GetGpio()->GPLEV0 & (1 << 9))
			tmpin |= 1;
		// set CLK
		RPI_GetGpio()->GPSET0 = (1 << 11);	
		// clear CLK		
		RPI_GetGpio()->GPCLR0 = (1 << 11);				
		tmpout <<= 1;
	}
	*in = tmpin;

	return;
}

static uint8_t mcp_read(const uint8_t reg){
	uint8_t in;

	spi_start();
	spi_byte(0x41, &in);
	spi_byte(reg, &in);
	spi_byte(0x00, &in);
	spi_stop();

	return in;
}

static void mcp_write(const uint8_t reg, const uint8_t val){
	uint8_t in;

	spi_start();
	spi_byte(0x40, &in);
	spi_byte(reg, &in);
	spi_byte(val, &in);
	spi_stop();
}

static void mcp_init(void){
	/* Port A: (S1 .. S5, S, L, R) */
	mcp_write(MCP_IODIRA, 0xFF);
	mcp_write(MCP_GPPUA, 0xFF);
	/* Port B: (DB4 .. DB7, /EN, R/W, RS, LED) */
	mcp_write(MCP_IODIRB, 0x00);
}


static uint8_t lcd_read_busy_flag_register(){
	// Set read pin
	mcp_write(MCP_GPIOB, LCD_RW);

	// Set enable pin, keeping read pin high
	mcp_write(MCP_GPIOB, LCD_RW | LCD_EN);

	// Read bit 7 to 4
	uint8_t firstNibble = (mcp_read(MCP_GPIOB) & 0x0F);

	// Reset enable pin, keeping read pin high
	mcp_write(MCP_GPIOB, LCD_RW);

	// Set enable pin, keeping read pin high
	mcp_write(MCP_GPIOB, LCD_RW | LCD_EN);

	// Read bit 3 to 0
	uint8_t secondNibble = (mcp_read(MCP_GPIOB) & 0x0F);

	return ((firstNibble << 4) | secondNibble);
}

static void lcd_busy_wait(){
	while (lcd_read_busy_flag_register() & 0x80)
		;
}

static void lcd_pulse(uint8_t val){
	mcp_write(MCP_GPIOB, val | LCD_EN );
	mcp_write(MCP_GPIOB, val);
}

static void lcd_write_cmd(uint8_t cmd){
	 lcd_busy_wait();
	/* write high nibble */
	lcd_pulse( LCD_BL | (cmd >> 4)   );
	lcd_busy_wait();
	/* write low nibble */
    lcd_pulse( LCD_BL | (cmd & 0x0F) );
    lcd_busy_wait();
}

static void lcd_write_data(uint8_t data){
	lcd_busy_wait();
	/* write high nibble */
	lcd_pulse( LCD_BL | LCD_RS | (data >> 4)   );
	lcd_busy_wait();
    /* write low nibble */
    lcd_pulse( LCD_BL | LCD_RS | (data & 0x0F) );
    //lcd_busy_wait();
}

static void lcd_init(void){
	/* enable 4 bit mode */
	LCD_DELAY;
	LCD_DELAY;
	lcd_pulse( 0x03 );
	LCD_DELAY;
	lcd_pulse( 0x03 );
	LCD_DELAY;
	lcd_pulse( 0x03 );
	LCD_DELAY;
	lcd_pulse( 0x02 );
	LCD_DELAY;
    /* function set; N = 1 for two rows, F = 0 for 5x8 display */
	lcd_write_cmd( 0x28 );	
	LCD_DELAY;
	/* display on/off; D = 1 for display on, C = 1 for cursor on; B = 0 for blinking off*/
	lcd_write_cmd( 0x0E );
	LCD_DELAY;
    /* display clear */
	lcd_write_cmd( 0x01 );
	LCD_DELAY;	
    /* entry mode set; I/D = 1 for direction left to right, S = 0 for shift off */
	lcd_write_cmd( 0x06 );
	LCD_DELAY;
}

__attribute__((constructor))
void piface_init(void){
	spi_init();
	mcp_init();
	lcd_init();
	cnt=0;
}

uint8_t piface_getc(void){
	return mcp_read(MCP_GPIOA);
}

/** @brief Writes a character
 */
int nrChars = 0;
int mult16 = 1;
int mult32 = 1;
void piface_putc(char c)
{
    /* write character */

    nrChars = (nrChars + 1) % 32;
    if(nrChars == 16 * mult16){
        piface_setLine(2);
        mult16++;
    }
    else if(nrChars == 32 * mult32){
        piface_setLine(1);
        mult32++;
    }

    lcd_write_data(c);

}

/** @brief Writes a string
 */
void piface_puts(char s[])
{
    /* write string */
    int i = 0;

    while(s[i] != '\0' && i < (int)strlen(s)){

        piface_putc(s[i]);
        i++;
    }

}
void piface_setLine(int l){
    if(l == 1){
        lcd_write_cmd(0x80);

        for(int i = 0; i < 16; i++){
            piface_putc(0x20);
        }

        lcd_write_cmd(0x80);
    }
    if(l == 2){

        lcd_write_cmd(0xC0);
        for(int i = 0; i < 16; i++){
            piface_putc(0x20);
        }

        lcd_write_cmd(0xC0);
    }

}

/** @brief Clears the display
 */
void piface_clear(void)
{
    /* clear display */
    lcd_write_cmd(0x01);
}
/** @brief Sets the cursor on a specific row and column
 *  Please check the url: http://piface.github.io/libpifacedigital/
 */
void piface_set_cursor(uint8_t col, uint8_t row)
{
    // lcd_write_cmd(0x02);

    nrChars = (row * 16) + col;

	volatile uint8_t t = col < 39 ? col : 39;
    col = t > 0 ? t : 0;
    t = row < 1 ? row : 1;
    row = t > 0 ? row : 0;
    volatile uint8_t addr = col + ROW_OFFSETS[row];
    addr = addr % 80;
    lcd_busy_wait();
    lcd_write_cmd( 0x80 | addr );
	LCD_DELAY;

}


/** @brief Displays an integer content in a given segment in the PiFace display.
 *  Suppose that you decided to segment the display into 4 segments of 8 digits each.
 *  For example, the illustration below ...
 *      ------------------
 *      |S0:XXXXXS1:XXXXX| 
 *      |S2:XXXXXS3:XXXXX|
 *      ------------------ 
 *  shows each segment (seg:0...3) with digits each.
 *  @param int seg Is the segment, i.e., 0: top left, 1:top right, 2: bottom left and 3: bottom right.
 *  @param int num Is the integer input parameter to be displayed.
 *  Pre-condition: 0<=seg<=3
 
 *  NOTE: maybe, you want to display arbitrary content on a given segment. For example:
 *      printAtSeg(1, "S%i: %.1f", 1, 3.14);
 *  This requires the use of variadic arguments with proper string formatting.
 *  Please check the url: https://en.cppreference.com/w/c/language/variadic
 *  You can redeclare printAtSeg as:
 *     void printAtSeg(int seg, const char* fmt, ...);
 */
void print_at_seg(int seg, int num) {
    // To be implemented

    char str[10];
    char temp[10];

    int n = 0;
    sprintf(temp,"%d",num);
    for(int i = 0; i<5; i++){

        if(temp[i] == '\0'){
            str[i] = temp[i];
            break;
        }
        else{
            str[i] = temp[i];
        }
    }

    char *end;
    const long i = strtol(str, &end, 10);
    sprintf(str, "S%d:%05d",seg,i);

    switch (seg) {

        case(0):
            piface_set_cursor(0,0);
            lcd_busy_wait();
            piface_puts(str);
            break;
        case(1):
            piface_set_cursor(8,0);
            lcd_busy_wait();
            piface_puts(str);
            break;
        case(2):
            piface_set_cursor(0,1);
            lcd_busy_wait();
            piface_puts(str);
            break;
        case(3):
            piface_set_cursor(8,1);
            lcd_busy_wait();
            piface_puts(str);
            break;
        default:
            break;
    }
    lcd_busy_wait();

}

/** @brief Similar to print_at_seg, but displays arbitrary content on a given segment. For example:
 *      printAtSeg(1, "S%i: %.1f", 1, 3.14);
 *  This requires the use of variadic arguments with proper string formatting.
 *  Please check the url: https://en.cppreference.com/w/c/language/variadic
 *  You can redeclare printAtSeg as:
 *     void printAtSeg(int seg, const char* fmt, ...);
 */
void printf_at_seg(int seg, const char* fmt, ...) {
    // The implementation is optional.
}