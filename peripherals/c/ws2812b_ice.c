#include "ws2812b.h"

void ws2812b_rotate(WS2812B_t *base, uint8_t num_leds)
{
	int i;
	base[0] = base[num_leds-1];
	
	for(i = num_leds-1; i >= 0; i--)
	{
		base[i] = base[i-1];
	}
};

void ws2812b_pulse(WS2812B_t *base, uint8_t num_leds)
{
	static int direction = 1;
	int i;
	
	//if incrementing and value of red < 0xFF
	if(direction == 1 & base[0].red < 0xFF)
	{
		for(i = 0; i < num_leds; i++)
		{
			base[i].red++;
		}
	}
	else if(direction == 1 & base[0].red == 0xFF)
	{
		direction = -1;
		for(i = 0; i < num_leds; i++)
		{
			base[i].red--;
		}
	}
	else if(direction == -1 & base[0].red > 0x00)
	{
		for(i = 0; i < num_leds; i++)
		{
			base[i].red--;
		}
	}
	else if(direction == -1 & base[0].red == 0x00)
	{
		direction = 1;
		for(i = 0; i < num_leds; i++)
		{
			base[i].red++;
		}
	}
};
