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
	if (alien_count == 5) {
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
	int columnCount = 30;
	int alienStartingXPos = 10;
	int alienStartingYPos = 260;
	int aliensAlive[7][3];
	int score = 0;
	bool moveHorizontal = true;
	bool moveRight = true;
	bool gameOver = false;
	
	//for displaying score to player
	char msg[8];
	
	// Arrays for the 16 bullets and their positions
	int alienBulletXPos[16];
	int alienBulletYPos[16];
	bool alienBullets[16];
	int alienBullet = 0; 
	
	int bulletXPos[16];
	int bulletYPos[16];
	bool bullets[16];
	int bullet = 0; 
	int i, j, x, z;
	
	//initialize bullet arrays
	for (i = 0; i < 16; i++) {
		bullets[i] = false;
		alienBullets[i] = false;
	}
	
	//randomly initialize aliens array with type of alien and all alive
	for (i = 0; i < 7; i++) {
		for (j = 0; j < 3; j++) {
			aliensAlive[i][j] = (rand() % 2) + 1;
		}
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
	yPos = 50;
	
	//create 'score: ' message
	msg[0] = 'S';
	msg[1] = 'C';
	msg[2] = 'O';
	msg[3] = 'R';
	msg[4] = 'E';
	msg[5] = ':';

  // Main game loop
  while(!gameOver) {
		
		// Displaying rows of aliens at top of LCD
		if (refreshAliens) {
			refreshAliens = false;

			//logic for alien movement right
			if (moveHorizontal && moveRight) {
				if (columnCount == 1){
					moveHorizontal = false;
				} else {
					columnCount--;
				}
			//logic for alien movement left
			} else if (moveHorizontal && !moveRight) {
				if (columnCount == 30){
					moveHorizontal = false;
				} else {
					columnCount++;
				}
			//logic for alien movement down
			}	else {
				alienStartingYPos -= 10;
				lcd_draw_image(0, blackBarWidthPixels, alienStartingYPos + 60, blackBarHeightPixels, blackBarBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
				moveHorizontal = true;
				moveRight = !moveRight;
			}
			
			//draw alive aliens to screen
			for (i = 0; i < 7; i++) {
				for (j = 0; j < 3; j++) {
					if (aliensAlive[i][j] == 1){
						lcd_draw_image(columnCount + (30 * i), alienWidthPixels, alienStartingYPos + (20 * j), alienHeightPixels, alienBitmap, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
					} else if (aliensAlive[i][j] == 2){
						lcd_draw_image(columnCount + (30 * i), alien2WidthPixels, alienStartingYPos + (20 * j), alien2HeightPixels, alien2Bitmap, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
					} else {
						lcd_draw_image(columnCount + (30 * i), alienWidthPixels, alienStartingYPos + (20 * j), alienHeightPixels, alienBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
				}
			}
		} //end if refresh aliens
		
		//switch 1 controls player's bullets
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

			xVal = (xVal - 2048)/900;
			yVal = (yVal - 2048)/900;

			//keeping the spaceship in bounds
			if (xPos + yVal >= 0 && xPos + yVal <= 240 - spaceshipWidthPixels && yPos + xVal >= 0 && yPos + xVal <= 150 - spaceshipHeightPixels) {
				yPos += xVal;
				xPos += yVal;
			}
			
			lcd_draw_image(xPos, spaceshipWidthPixels, yPos, spaceshipHeightPixels, spaceshipBitmap, LCD_COLOR_CYAN, LCD_COLOR_BLACK);
			
			//this block handles all bullet logic
			for (i = 0; i < 16; i++){
				
				//if the player's bullets go off the top of the screen then erase them
				if (bulletYPos[i] > 314) {
					bullets[i] = false;
					lcd_draw_image(bulletXPos[i], bulletWidthPixels, bulletYPos[i], bulletHeightPixels, bulletBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
				}
				
				//if the alien's bullets go off the bottom of the screen erase them
				if (alienBulletYPos[i] < 1) {
					alienBullets[i] = false;
					lcd_draw_image(alienBulletXPos[i], alienBulletWidthPixels, alienBulletYPos[i], alienBulletHeightPixels, alienBulletBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					alienBulletYPos[i] = -10;
				}
				
				//aliens randomly shoot bullets if they are in the bottom row, or if the aliens beneath them have all died
				//also handles when an alien runs into the player
				for (x = 0; x < 7; x++) {
					for (z = 0; z < 3; z++) {
						
						//if alien runs into player
						if(aliensAlive[x][z] && alienStartingYPos + (20*z) > yPos+2 && alienStartingYPos + (20*z) < yPos+25 && columnCount + (30*x) > xPos+2 && columnCount + (30*x) < xPos+26) {
							lcd_draw_image(alienBulletXPos[i], alienBulletWidthPixels, alienBulletYPos[i], alienBulletHeightPixels, alienBulletBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
							alienBullets[i] = false;
							lcd_draw_image(xPos, spaceshipWidthPixels, yPos, spaceshipHeightPixels, spaceshipBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
							lcd_draw_image(xPos, explosionWidthPixels, yPos, explosionHeightPixels, explosionBitmap, LCD_COLOR_RED, LCD_COLOR_BLACK);
							gameOver = true;
						}
						if (z == 0 && aliensAlive[x][z] && !alienBullets[i]){
							if (rand() % 10000 == 0) {
								alienBullets[i] = true;
								alienBulletXPos[i] = columnCount + (30*x) + 10;
								alienBulletYPos[i] = alienStartingYPos - 8;
							}
						//if in bottom row aliens can shoot
						} else if (z == 1) {
							if (aliensAlive[x][z] && !aliensAlive[x][z - 1] && !alienBullets[i]) {
								if (rand() % 10000 == 0) {
									alienBullets[i] = true;
									alienBulletXPos[i] = columnCount + (30*x) + 10;
									alienBulletYPos[i] = alienStartingYPos + 12;
								}
							}
						//else if the alien is in a row other than the bottom one and the aliens beneath it are all dead it can shoot
						} else if (z == 2) {
							if (aliensAlive[x][z] && !aliensAlive[x][z - 2] && !aliensAlive[x][z - 1] && !alienBullets[i]) { 
								if (rand() % 10000 == 0) {
									alienBullets[i] = true;
									alienBulletXPos[i] = columnCount + (30*x) + 10;
									alienBulletYPos[i] = alienStartingYPos + 32;
								}
							}
						}
						
						//if a player's bullet hits an alien display the explosion image and erase that alien from the game
						if (bullets[i] && aliensAlive[x][z] != 0 && bulletYPos[i] + 6 >= alienStartingYPos + (20*z) && bulletYPos[i] <= alienStartingYPos + (20*z) + 16 && bulletXPos[i] >= columnCount + (30*x) + 8 && bulletXPos[i] <= columnCount + (30*x) + 26) {
							aliensAlive[x][z] = 0;
							bullets[i] = false;
							lcd_draw_image(bulletXPos[i], bulletWidthPixels, bulletYPos[i], bulletHeightPixels, bulletBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
							lcd_draw_image(columnCount + (30*x), explosionWidthPixels, alienStartingYPos + (20*z), explosionHeightPixels, explosionBitmap, LCD_COLOR_RED, LCD_COLOR_BLACK);
							score++; //increment player score
							refreshAliens = false;
						}
					}
				}
				
				//if an alien's bullet hits the player then it is game over
				if (alienBulletXPos[i] > xPos + 2 && alienBulletXPos[i] < xPos + 25 && alienBulletYPos[i] > yPos + 2 && alienBulletYPos[i] < yPos + 26) {
					lcd_draw_image(alienBulletXPos[i], alienBulletWidthPixels, alienBulletYPos[i], alienBulletHeightPixels, alienBulletBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					alienBullets[i] = false;
					lcd_draw_image(xPos, spaceshipWidthPixels, yPos, spaceshipHeightPixels, spaceshipBitmap, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					lcd_draw_image(xPos, explosionWidthPixels, yPos, explosionHeightPixels, explosionBitmap, LCD_COLOR_RED, LCD_COLOR_BLACK);
					gameOver = true;
				}
				
				//controls bullet travel for player
				if (bullets[i]){
					bulletYPos[i] = bulletYPos[i] + 2; // Bullet speed
					lcd_draw_image(bulletXPos[i], bulletWidthPixels, bulletYPos[i], bulletHeightPixels, bulletBitmap, LCD_COLOR_RED, LCD_COLOR_BLACK);
				}
				//controls bullet travel for aliens
				if (alienBullets[i]){
					alienBulletYPos[i] = alienBulletYPos[i] - 2; // Bullet speed
					lcd_draw_image(alienBulletXPos[i], alienBulletWidthPixels, alienBulletYPos[i], alienBulletHeightPixels, alienBulletBitmap, LCD_COLOR_RED, LCD_COLOR_BLACK);
				}
			}//end bullet logic block
		}//end analogConvert block
  }//end while gameOver loop
	
	//display score (# aliens killed) to player on game over
	if(gameOver) {
		//must separate score digits if over 9 because of ASCII table in ece fonts c file
		if(score <= 9 )
			msg[6] = score + '0';
		else {
			msg[6] = (score/10)+'0'; //first digit
			msg[7] = (score%10)+'0'; //second digit
		}
		
		lcd_print_stringXY(msg,199,10,LCD_COLOR_YELLOW,LCD_COLOR_BLACK);
	}
}
