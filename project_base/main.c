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
//#define SetBit(j, k, x) (pixels[(j/8)][k] |= (x << (j%8))) // Use byte array as a bit array to save space

char group[] = "Group33";
char individual_1[] = "Jackson Melchert";
char individual_2[] = "Nicholas Cook";

volatile bool SW1_pressed; // Global for SW1
volatile bool analogConvert; // Global for adc convertion (setting new spaceship location)
volatile bool refreshAliens; // Set every 20 Timer0Bs
int32_t xVal, yVal; // X and Y values of joystick
uint16_t xPos, yPos; // Current pixel position
//uint8_t pixels[30][320]; // Array of on pixels


// Timer0A Handler, used for Blue LED and SW1 debounce
void TIMER0A_Handler(void) {
	// Counters for debouncing
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

// Timer0B Handler, used for ADC
void TIMER0B_Handler(void) {
	static uint8_t alien_count = 0;

	ADC0_Type  *myADC;
	myADC = (ADC0_Type *)PS2_ADC_BASE;	

	// delay buffer for displaying next row of aliens
	if (alien_count == 20) {r
		refreshAliens = true;
		alien_count = 0;
	} else {
		alien_count++;
	}
	
	// Enable SS2
	myADC->PSSI |= ADC_PSSI_SS2;
	
	clearInterrupt(TIMER0_BASE, false);
	
	return;
}

// ADC interrupt handler for analog conversion
void ADC0SS2_Handler(void) {
	ADC0_Type  *myADC;
	analogConvert = true; // Set global variable
	
	myADC = (ADC0_Type *)PS2_ADC_BASE;
		
	// Clear interrupt
	myADC->ISC |= ADC_ISC_IN2;
	
	return;
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
	
	return;
}

//*****************************************************************************
//*****************************************************************************
void initialize_hardware(void) {
  initialize_serial_debug();
	
	// Initialize adc for the joystick
	adc0_ps2_initialize();
	
	// Set timer0A and timer0B
	gp_timer_config_16(TIMER0_BASE, 10000, 30000, 50, 25);
	
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
	uint8_t randomBit;
	uint8_t alienShoot;
	uint8_t columnCount = 239;
	int alienStartingXPos = 120;
	int alienStartingYPos = 250;
	
	// Arrays for the 16 bullets and their positions
	int bulletXPos[16];
	int bulletYPos[16];
	bool bullets[16];
	int bullet = 0; 
	int i;
	for (i = 0; i < 16; i++) {
		bullets[i] = false;
	}

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

	// Clear screen and set spaceship starting position
	lcd_clear_screen(LCD_COLOR_BLACK);
	xPos = 120;
	yPos = 160;

	lcd_draw_image(100, scoresWidthPixels, 200, scoresHeightPixels, scoresBitmap, LCD_COLOR_GREEN, LCD_COLOR_BLACK);

  // Main loop
  while(1) {
		
		// Displaying new row of aliens at top of LCD
		// TODO: Previous row is moved down
		if (refreshAliens) {
			refreshAliens = false;

			//lcd_draw_image(120, alienWidthPixels, 160, alienHeightPixels, alienBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
			//lcd_draw_image(120, alienWidthPixels, 160, alienHeightPixels, alienBitmap, LCD_COLOR_GREEN, LCD_COLOR_BLACK);

			// Attempt to display row of "random" aliens
			while(columnCount > alienWidthPixels) { //alien has largest width
				randomBit = rand() % 2;

				if(randomBit == 0) {
					lcd_draw_image(bottomAlien_x, alienWidthPixels, bottomAlien_y, alienHeightPixels, alienBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					lcd_draw_image(bottomAlien_x, alienWidthPixels, bottomAlien_y, alienHeightPixels, alienBitmap, LCD_COLOR_RED, LCD_COLOR_BLACK);

					columnCount -= alienWidthPixels;
				}
				else if(randomBit == 1) {
					lcd_draw_image(bottomAlien_x, alien2WidthPixels, bottomAlien_y, alien2HeightPixels, alienBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					lcd_draw_image(bottomAlien_x, alien2WidthPixels, bottomAlien_y, alien2HeightPixels, alienBitmap, LCD_COLOR_RED, LCD_COLOR_BLACK);

					columnCount -= alien2WidthPixels;
				}

				bottomAlien_y--;

			}//end while

			//lcd_draw_image(alienStartingXPos, alienWidthPixels, alienStartingYPos--, alienHeightPixels, alienBitmap, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
		}
		
		if (SW1_pressed) {
			SW1_pressed = false;
			
			// If 16 bullets are already on screen then erase the last one
			if (bullets[bullet]) {
				lcd_draw_image(bulletXPos[bullet], bulletWidthPixels, bulletYPos[bullet], bulletHeightPixels, bulletBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
			}
			bullets[bullet] = true;
			bulletXPos[bullet] = xPos + 12;
			bulletYPos[bullet] = yPos + 23;
			
			// Loop around the array so that we can have infinite bullets
			if (bullet == 15) {
				bullet = 0;
			} else {
				bullet++;
			}
		}
		
		// Reading PS2 joystick for moving spaceship on screen
		if (analogConvert) {
			analogConvert = false;
			
			readPS2();

			xVal = (xVal - 2048)/1020;
			yVal = (yVal - 2048)/1020;

			if (xPos + yVal >= 0 && xPos + yVal <= 240 - spaceshipWidthPixels && yPos + xVal >= 0 && yPos + xVal <= 320 - spaceshipHeightPixels) {
			
			yPos += xVal;
			xPos += yVal;

			}
			lcd_draw_image(xPos, spaceshipWidthPixels, yPos, spaceshipHeightPixels, spaceshipBitmap, LCD_COLOR_CYAN, LCD_COLOR_BLACK);
			
			for (i = 0; i < 16; i++){
				if (bulletYPos[i] > 314) {
					bullets[i] = false;
					lcd_draw_image(bulletXPos[i], bulletWidthPixels, bulletYPos[i], bulletHeightPixels, bulletBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
				}
				
				if (bullets[i]){
					bulletYPos[i] = bulletYPos[i] + 2; // Bullet speed
					lcd_draw_image(bulletXPos[i], bulletWidthPixels, bulletYPos[i], bulletHeightPixels, bulletBitmap, LCD_COLOR_RED, LCD_COLOR_BLACK);
				}
			}
		}	
  }
}