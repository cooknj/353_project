#include "ws2812b.h"

void ws2812b_rotate(WS2812B_t *base, uint8_t num_leds) {
	WS2812B_t temp = base[num_leds];
	
	int i;
	for (i = num_leds; i > 0; i--) {
		base[i] = base[i-1];
	}
	
	base[0] = temp;
	
}

void ws2812b_pulse(WS2812B_t *base, uint8_t num_leds) {
	static uint8_t direction = 1;
	int i;
	if (direction == 1 && base[0].red < 0xFF) {
		for (i = 0; i < num_leds; i++) {
			base[i].red++;
		}
	} else if (direction == 1 && base[0].red == 0xFF) {
		for (i = 0; i < num_leds; i++) {
			base[i].red--;
		}
		direction = 0;
	} else if (direction == 0 && base[0].red > 0x00) {
		for (i = 0; i < num_leds; i++) {
			base[i].red--;
		}
	} else if (direction == 0 && base[0].red == 0x00) {
		for (i = 0; i < num_leds; i++) {
			base[i].red++;
		}
		direction = 1;
	}
}