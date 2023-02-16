#include "pwm_new.h"

#include "stm32f4xx_hal.h"
#include "sine.h"
#include "angles.h"

#define MODE_OFF '0'
#define MODE_ASYNC 'a'
#define MODE_SYNC_SPWM 'b'
#define MODE_SYNC_SHEPWM 'c'

#define ACCL_OFF '0'
#define ACCL_ACCL '1'
#define ACCL_KEEP '2'
#define ACCL_DECL '3'

#define pi ((float)3.1415926535897932384626433)

void modulation_async(void);
void modulation_spwm(void);
void modulation_shepwm(void);
void update_modulation_type(void);
float sine_wave(float angle);


struct modulation_properties
{
	uint8_t modulation_type;
	float frequency_low_bound;
	float frequency_high_bound;
	float modulation_low_bound;
	float modulation_high_bound;
	uint16_t pulses;
};


static const struct modulation_properties modulation_off = 
//   Mode               Freq(L)   Freq(H)   Modu(L)   Modu(H)   Pulses
{    MODE_OFF,            0.0f,   300.0f,     0.0f,     0.0f,       0};

/**/
static const struct modulation_properties modulation_list_accelerate[] = {
//   Mode               Freq(L)   Freq(H)   Modu(L)   Modu(H)   Pulses
	{MODE_ASYNC,          0.0f,    14.0f,     0.5f,     0.5f,     300},
	{MODE_ASYNC,         14.0f,    28.0f,     0.5f,     0.5f,     400},
	{MODE_SYNC_SPWM,     28.0f,    42.0f,     0.5f,     0.5f,      21},
	{MODE_SYNC_SPWM,     42.0f,    58.0f,     0.5f,     0.7f,      15},
	{MODE_SYNC_SHEPWM,   58.0f,    89.0f,     0.7f,     0.9f,      11},
	{MODE_SYNC_SHEPWM,   89.0f,   300.0f,     1.0f,     1.0f,       7},	
};
/**/
/**
static const struct modulation_properties modulation_list_accelerate[] = {
//   Mode               Freq(L)   Freq(H)   Modu(L)   Modu(H)   Pulses
	{MODE_ASYNC,          0.0f,    42.0f,     5.0f,     5.0f,     300},
	//{MODE_ASYNC,         18.0f,    28.0f,     1.0f,     0.9f,     400},
	//{MODE_SYNC_SPWM,     28.0f,    42.0f,     0.3f,     0.5f,      21},
	{MODE_SYNC_SPWM,     42.0f,    58.0f,     0.5f,     0.7f,      15},
	{MODE_SYNC_SHEPWM,   58.0f,    89.0f,     0.7f,     0.9f,      11},
	{MODE_SYNC_SHEPWM,   89.0f,   300.0f,     1.0f,     1.0f,       7},	
};
/**/
/*
static const struct modulation_properties modulation_list_accelerate[] = {
//   Mode               Freq(L)   Freq(H)   Modu(L)   Modu(H)   Pulses
	{MODE_ASYNC,          0.0f,    34.0f,     0.4f,     0.4f,    1050},
	//{MODE_ASYNC,          36.0f,   50.0f,     0.5f,     1.0f,    1050},
	{MODE_SYNC_SPWM,     34.0f,    58.0f,     0.4f,     1.0f,       9},
};
/**/
static const uint8_t size_accelerate = sizeof(modulation_list_accelerate) / sizeof(modulation_list_accelerate[0]);


static const struct modulation_properties modulation_list_decelerate[] = {
//   Mode               Freq(L)   Freq(H)   Modu(L)   Modu(H)   Pulses
	{MODE_ASYNC,          0.0f,     7.0f,     0.0f,     0.0f,     300},
	{MODE_ASYNC,          7.0f,    11.0f,    0.03f,     0.1f,     300},
	{MODE_ASYNC,         11.0f,    21.5f,     0.1f,     0.2f,     400},
	{MODE_SYNC_SPWM,     21.5f,    37.3f,     0.2f,     0.4f,      21},
	{MODE_SYNC_SPWM,     37.3f,    53.5f,     0.4f,     0.5f,      15},
	{MODE_SYNC_SHEPWM,   53.5f,    89.0f,     0.5f,     0.8f,      11},
	{MODE_SYNC_SHEPWM,   89.0f,   300.0f,     1.0f,     1.0f,       7},	
};

/*
static const struct modulation_properties modulation_list_decelerate[] = {
//   Mode               Freq(L)   Freq(H)   Modu(L)   Modu(H)   Pulses
	{MODE_ASYNC,          0.0f,     7.0f,     0.0f,     0.0f,    1050},
	{MODE_ASYNC,          7.0f,    31.0f,     0.1f,     0.4f,    1050},
	
	{MODE_SYNC_SPWM,     31.0f,    70.0f,     0.4f,     0.9f,      9},
};
*/
static const uint8_t size_decelerate = sizeof(modulation_list_decelerate) / sizeof(modulation_list_decelerate[0]);

char accelerate;
float frequency;
float target_modulation;
float modulation;
float sine_wave_angle;
float trig_wave_angle;
struct modulation_properties const *current_modulation_props;

void pwm_initialize()
{
	accelerate = ACCL_DECL;
	frequency = 63.0f;
	target_modulation = 0.0f;
	modulation = 0.0f;
	sine_wave_angle = 0.0f;
	trig_wave_angle = 0.0f;
	current_modulation_props = &modulation_off;
}

void modulate()
{
	switch(current_modulation_props->modulation_type)
	{
		case MODE_OFF:
			TIM1->CCR1 = 0;
			TIM1->CCR2 = 0;
			TIM1->CCR3 = 0;
			break;
		case MODE_ASYNC:
			modulation_async();
			break;
		case MODE_SYNC_SPWM:
			modulation_spwm();
			break;
		case MODE_SYNC_SHEPWM:
			modulation_shepwm();
			break;
		default:
			TIM1->CCR1 = 0;
			TIM1->CCR2 = 0;
			TIM1->CCR3 = 0;
	}
}

void update_modulation_type()
{
	uint8_t i;
	
	
	if (accelerate == ACCL_ACCL)
	{
/**		if (frequency < 20)/**/ frequency += .1f;
		for(i = 0; i < size_accelerate; ++i)
		{
			if (frequency >  modulation_list_accelerate[i].frequency_low_bound &&
				frequency <= modulation_list_accelerate[i].frequency_high_bound)
			{
				current_modulation_props = &modulation_list_accelerate[i];
			}
		}
	} else if (accelerate == ACCL_DECL)
	{
		frequency -= .07f;
		
		
		for(i = 0; i < size_decelerate; ++i)
		{
			if (frequency >  modulation_list_decelerate[i].frequency_low_bound &&
				frequency <= modulation_list_decelerate[i].frequency_high_bound)
			{
				current_modulation_props = &modulation_list_decelerate[i];
			}
		}
	}
	
	target_modulation = (frequency - current_modulation_props->frequency_low_bound) / 
						(current_modulation_props->frequency_high_bound  - current_modulation_props->frequency_low_bound) * 
						(current_modulation_props->modulation_high_bound - current_modulation_props->modulation_low_bound) + 
						current_modulation_props->modulation_low_bound;

	if (modulation < target_modulation)
	{
		modulation += 0.01f;
		if (modulation > target_modulation) modulation = target_modulation;
	}
	if (modulation > target_modulation)
	{
		modulation -= 0.01f;
		if (modulation < target_modulation) modulation = target_modulation;
	}
	if (modulation > 1) modulation = 1.0f;
	
	if (target_modulation == 0 && modulation == 0)
	{
		current_modulation_props = &modulation_off;
	}
}

void update_stats()
{
	sine_wave_angle += frequency * 360.0f/ 50000.0f;
//	sine_wave_angle += 4.0 * 360.0f/ 50000.0f;
	if (sine_wave_angle >= 360) sine_wave_angle -= 360;
	
	if (current_modulation_props->modulation_type == MODE_ASYNC)
	{
		trig_wave_angle += current_modulation_props->pulses * 360.0 / 50000.0;
		if (trig_wave_angle >= 360) trig_wave_angle -= 360;
	}
}

void update_accelerate(void)
{
	/**
	uint8_t pressed_key;
	
	pressed_key = Key_GetNum();
	
	//OLED_ShowNum(2, 1, (int)pressed_key, 5);
	
	switch(pressed_key)
	{
		case 0:	accelerate = ACCL_KEEP; break;
		case 1:	accelerate = ACCL_ACCL; break;
		case 2:	accelerate = ACCL_DECL; break;
		
	}
	**/
	//accelerate = ACCL_DECL;
	
}

char shepwm_get_polarity(double sine_wave_angle, float *she_angles, char flip)
{
	double sine_angle = 0;
	int8_t polarity = 0;
	
	if (sine_wave_angle >= 360) sine_wave_angle -= 360;
	
	sine_angle = sine_wave_angle;
	
	if (sine_angle >= 180) sine_angle -= 180;
	
	if (sine_angle >= 90)
	{
		sine_angle -= 90;
		sine_angle = 90 - sine_angle;
	}
	
	if ((sine_angle >= 0             && sine_angle < she_angles[0]) ||
		(sine_angle >= she_angles[1] && sine_angle < she_angles[2]) ||
		(sine_angle >= she_angles[3] && sine_angle < she_angles[4])
	)
	{
		polarity = -1;
	}
	else 
	{
		polarity = 1;
	}
	
	if (sine_wave_angle >= 180)
	{
		polarity = -polarity;
	}
	
	if (flip) polarity = -polarity;
	
	return polarity + 1;
}

#define _2_divided_by_sqrt3 1.154700538379f
#define _1_divided_by_6 0.166666666667f
float sine_wave(float angle)
{
	//return _2_divided_by_sqrt3 * (arm_sin_f32(angle) + _1_divided_by_6 * arm_sin_f32(3 * angle));
	return arm_sin_f32(angle);
}

void modulation_async()
{	
	float T, S1, S2, S3;

	T = triangle_wave(trig_wave_angle, 360);
	
	S1 = sine_wave(sine_wave_angle / 180 * pi);
	S2 = sine_wave(sine_wave_angle / 180 * pi + 2.0/3 * pi);
	S3 = sine_wave(sine_wave_angle / 180 * pi + 4.0/3 * pi);
	
	S1 *= modulation;
	S2 *= modulation;
	S3 *= modulation;
	
	TIM1->CCR1 = T > S1 ? 0 : 2;
	TIM1->CCR2 = T > S2 ? 0 : 2;
	TIM1->CCR3 = T > S3 ? 0 : 2;
}

void modulation_spwm()
{
	float T, S1, S2, S3;
	
	T = triangle_wave(sine_wave_angle, 360.0 / current_modulation_props->pulses);
		
	S1 = sine_wave(sine_wave_angle / 180 * pi);
	S2 = sine_wave(sine_wave_angle / 180 * pi + 2.0f/3 * pi);
	S3 = sine_wave(sine_wave_angle / 180 * pi + 4.0f/3 * pi);
	
	S1 *= modulation;
	S2 *= modulation;
	S3 *= modulation;
	
	TIM1->CCR1 = T > S1 ? 0 : 2;
	TIM1->CCR2 = T > S2 ? 0 : 2;
	TIM1->CCR3 = T > S3 ? 0 : 2;
}

void modulation_shepwm()
{
	char index;
	char polarities[3], flip = 0;
	float she_angles[5];
	switch(current_modulation_props->pulses)
	{
		case 11: 
			for (index = 0; index < 5; ++index)
			{
				she_angles[index] = modulation * (_5Alpha_SHE[index] - _5Alpha_SHE_Begin[index]) + _5Alpha_SHE_Begin[index];
			}
			flip = 1;
			break;
		case 7:
			for (index = 0; index < 5; ++index)
			{
				she_angles[index] = modulation * (_3Alpha_SHE[index] - _3Alpha_SHE_Begin[index]) + _3Alpha_SHE_Begin[index];
			}
			flip = 0;
			break;
	}
	polarities[0] = shepwm_get_polarity(sine_wave_angle,       she_angles, flip);
	polarities[1] = shepwm_get_polarity(sine_wave_angle + 120, she_angles, flip);
	polarities[2] = shepwm_get_polarity(sine_wave_angle + 240, she_angles, flip);
	
	TIM1->CCR1 = polarities[0];
	TIM1->CCR2 = polarities[1];
	TIM1->CCR3 = polarities[2];
}
