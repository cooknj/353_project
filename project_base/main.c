// Copyright (c) 2015-16, Joe Krachey
// All rights reserved.
//
// Redistribution and use in source or binary form, with or without modification, 
// are permitted provided that the following conditions are met:
//
// 1. Redistributions in source form must reproduce the above copyright 
//    notice, this list of conditions and the following disclaimer in 
//    the documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "main.h"
#include "timers.h"
#include "ps2.h"
#include "launchpad_io.h"
#include "lcd.h"
#define SetBit(j, k, x) (pixels[(j/8)][k] |= (x << (j%8))) // Use byte array as a bit array to save space



char group[] = "Group33";
char individual_1[] = "Jackson Melchert";
char individual_2[] = "Nicholas Cook";

volatile bool SW1_pressed; // Global for SW1
volatile bool analogConvert; // Global for adc convertion
volatile bool refreshPlayer;
volatile bool refreshAliens;
int32_t xVal, yVal; // X and Y values of joystick
uint16_t xPos, yPos; // Current pixel position
uint8_t pixels[30][320]; // Array of on pixels


// FSM states
typedef enum 
{
  DRAW,
	MOVE,
	ERASE
} ETCH_MODES;



// Timer0A Handler, used for Blue LED and SW1 debounce
void TIMER0A_Handler(void) {
		
	// Counters for debouncing and flashing the led
	static uint8_t button_count = 0;

	
	// Increment the counter for debouncing
	if (!lp_io_read_pin(SW1_BIT)) {
		button_count++;
	} else {
		button_count = 0;
	}
	
	// Debounce logic
	if (button_count == 6) {
		SW1_pressed = true;
	} else {
		SW1_pressed = false;
	}
	


	clearInterrupt(TIMER0_BASE, true);
	
	return;
	
}

// Timer0B Handler, used for Green led and ADC
void TIMER0B_Handler(void) {
	
	static uint8_t Alien_count = 0;
	ADC0_Type  *myADC;
	
	myADC = (ADC0_Type *)PS2_ADC_BASE;	

	refreshPlayer = true;
	// Increment the counter for debouncing
	if (Alien_count == 4) {
		refreshAliens = true;
		Alien_count = 0;
	} else {
		Alien_count++;
	}
	
	
	// Enable SS2
	myADC->PSSI |= ADC_PSSI_SS2;
	
	clearInterrupt(TIMER0_BASE, false);
	
	return;
	
}


void ADC0SS2_Handler(void) {
	
	
	ADC0_Type  *myADC;
	analogConvert = true; // Set global variable
	
	myADC = (ADC0_Type *)PS2_ADC_BASE;
		
	// Clear interrupt
	myADC->ISC |= ADC_ISC_IN2;
	
}

// Read joystick values
void readPS2() {
	
	ADC0_Type  *myADC;
	myADC = (ADC0_Type *)PS2_ADC_BASE;
	
	// Set channel and read x and y values
	myADC->SSMUX2 = PS2_X_ADC_CHANNEL;
	xVal = myADC->SSFIFO2 & 0xFFF;
	
	myADC->SSMUX2 = PS2_Y_ADC_CHANNEL;
	yVal = myADC->SSFIFO2 & 0xFFF;
	
}



//*****************************************************************************
//*****************************************************************************
void initialize_hardware(void)
{
  initialize_serial_debug();
	
	
	// Initialize adc for the joystick
	adc0_ps2_initialize();
	
	// Set timer0A and timer0B
	gp_timer_config_16(TIMER0_BASE, 10000, 30000, 50, 50);
	
	// Initialize GPIO for the switch and leds
	lp_io_init_SW1_LED();
	
	// Config gpio and LCD
	lcd_config_gpio();
	lcd_config_screen();
}


//*****************************************************************************
//*****************************************************************************
int 
main(void)
{
	

  initialize_hardware();
	
	
  put_string("\n\r");
  put_string("************************************\n\r");
  put_string("ECE353 - Fall 2016 HW3\n\r  ");
  put_string(group);
  put_string("\n\r     Name:");
  put_string(individual_1);
  put_string("\n\r     Name:");
  put_string(individual_2);
  put_string("\n\r");  
  put_string("************************************\n\r");

	// Clear screen and set starting position to the middle
	lcd_clear_screen(LCD_COLOR_BLACK);
	xPos = 120;
	yPos = 160;
	

	

	lcd_draw_image(100, scoresWidthPixels, 200, scoresHeightPixels, scoresBitmap, LCD_COLOR_GREEN, LCD_COLOR_BLACK);

	
  // Reach infinite loop
  while(1){
		
		if (refreshAliens) {
			refreshAliens = false;
			lcd_draw_image(120, alienWidthPixels, 160, alienHeightPixels, alienBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
			lcd_draw_image(120, alienWidthPixels, 160, alienHeightPixels, alienBitmap, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
		}
		
		if (analogConvert) {
			analogConvert = false;
			
		
			readPS2();
		
			lcd_draw_image(xPos, spaceshipWidthPixels, yPos, spaceshipHeightPixels, spaceshipBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
			
			xVal = (xVal - 2048)/512;
			yVal = (yVal - 2048)/512;

			yPos += xVal;
			xPos += yVal;

			lcd_draw_image(xPos, spaceshipWidthPixels, yPos, spaceshipHeightPixels, spaceshipBitmap, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
			
		}
  }
}