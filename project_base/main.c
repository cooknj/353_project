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
uint32_t xVal, yVal; // X and Y values of joystick
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
	static uint8_t BLUE_LED_count = 0;
	
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
	

	// Toggle Blue led every 10 interrupts
	if (BLUE_LED_count == 10) {
		lp_io_set_pin(BLUE_BIT);
		BLUE_LED_count++;
	} else if (BLUE_LED_count == 20) {
		lp_io_clear_pin(BLUE_BIT);
		BLUE_LED_count = 0;
	} else {
		BLUE_LED_count++;
	}
	
	
	clearInterrupt(TIMER0_BASE, true);
	
	return;
	
}

// Timer0B Handler, used for Green led and ADC
void TIMER0B_Handler(void) {
	static uint8_t GREEN_LED_count = 0;
	
	ADC0_Type  *myADC;
	
	myADC = (ADC0_Type *)PS2_ADC_BASE;	
	
	// Turn on green led every 10 interrupts
	if (GREEN_LED_count == 10) {
		lp_io_set_pin(GREEN_BIT);
		GREEN_LED_count++;
	} else if (GREEN_LED_count == 20) {
		lp_io_clear_pin(GREEN_BIT);
		GREEN_LED_count = 0;
	} else {
		GREEN_LED_count++;
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
//  FSM for determining what state the program is in (DRAW, MOVE, ERASE
//*****************************************************************************
void fsm(void)
{

	static ETCH_MODES state = DRAW; // Default state is DRAW
	uint8_t image[1]; // One pixel image used for drawing
	image[0] = 1;
	
	switch (state)
	{
		case DRAW:
		{
			// If SW1 is pressed go to MOVE
			if(SW1_pressed)
			{
				state = MOVE;
				SW1_pressed = false;
			}
			else
			{
					// Read X and Y values based on ADC interrupt
					if (analogConvert) {
						analogConvert = false;
						readPS2();
						
						// Draw green pixel
						lcd_draw_image(xPos, 1, yPos, 1, image, LCD_COLOR_GREEN, LCD_COLOR_GREEN);
						
						// Move current pixel and wrap around
						if	(xVal < 1024){
							if (yPos <= 1) {
								yPos = 319;	
							} else {
								yPos--;
							}
						} else if (xVal > 3072) {
							if (yPos >= 319) {
								yPos = 0;	
							} else {
								yPos++;
							}
						}
						
						if	(yVal < 1024){
							if (xPos <= 1) {
								xPos = 239;	
							} else {
								xPos--;
							}
							
						} else if (yVal > 3072) {
							if (xPos >= 239) {
								xPos = 0;	
							} else {
								xPos++;
							}
						}
						
						// Add current pixel to array of visted pixels
						SetBit(xPos, yPos, 1);
					}
			}
			break;
		}
		case MOVE:
		{
			// If SW2 is held then go to ERASE
			if(!lp_io_read_pin(SW2_BIT))
			{
				state = ERASE;
				
			}
			else if (!SW1_pressed)
			{
				state = MOVE;
				
				// Ready X and Y on joystick based on ADC interrupt
				if (analogConvert) {
					
						// Determine if passing through a pixel that is green, and reset it to what it originally was
						if (pixels[xPos/8][yPos] & (1 << (xPos % 8))) {
							lcd_draw_image(xPos, 1, yPos, 1, image, LCD_COLOR_GREEN, LCD_COLOR_GREEN);
						} else {
							lcd_draw_image(xPos, 1, yPos, 1, image, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
						}
						
						// Set global analogConvert to false and read X and Y
						analogConvert = false;
						readPS2();
						
						// Move current pixel and wrap around
						if	(xVal < 1024){
							if (yPos <= 1) {
								yPos = 319;	
							} else {
								yPos--;
							}
						} else if (xVal > 3072) {
							if (yPos >= 319) {
								yPos = 0;	
							} else {
								yPos++;
							}
						}
						
						if	(yVal < 1024){
							if (xPos <= 1) {
								xPos = 239;	
							} else {
								xPos--;
							}
							
						} else if (yVal > 3072) {
							if (xPos >= 239) {
								xPos = 0;	
							} else {
								xPos++;
							}
						}
						
						// Set current pixel to red
						lcd_draw_image(xPos, 1, yPos, 1, image, LCD_COLOR_RED, LCD_COLOR_RED);
						
					}
			} else {
				// Go to draw if SW1 is pressed
				state = DRAW;
				SW1_pressed = false;
			}
			break;
		}
		case ERASE:
		{
			// If SW2 is held, stay in ERASE
			if(!lp_io_read_pin(SW2_BIT))
			{
				state = ERASE;
				if (analogConvert) {
					
						// Delete current pixel and remove from array
						lcd_draw_image(xPos, 1, yPos, 1, image, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
						SetBit(xPos, yPos, 0);
						
						analogConvert = false;
						readPS2();
						// Move current pixel and wrap around
						if	(xVal < 1024){
							if (yPos <= 1) {
								yPos = 319;	
							} else {
								yPos--;
							}
						} else if (xVal > 3072) {
							if (yPos >= 319) {
								yPos = 0;	
							} else {
								yPos++;
							}
						}
						
						if	(yVal < 1024){
							if (xPos <= 1) {
								xPos = 239;	
							} else {
								xPos--;
							}
							
						} else if (yVal > 3072) {
							if (xPos >= 239) {
								xPos = 0;	
							} else {
								xPos++;
							}
						}
						
						// Draw new pixel red
						lcd_draw_image(xPos, 1, yPos, 1, image, LCD_COLOR_RED, LCD_COLOR_RED);
						
					}
			}
			else
			{
				// If SW2 is released then go back to MOVE
				state = MOVE;
			}
			break;
		}
		default:
		{
			// Never should get here
			while(1){};
		}
	}
	
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

  // Reach infinite loop
  while(1){
		fsm();
  }
}