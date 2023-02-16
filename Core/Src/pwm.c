#include "pwm.h"

#include "stm32f4xx_hal.h"


#include "sine.h"
#include "angles.h"

#define pi ((float)3.1415926535897932384626433)

#define MODE_OFF '0'
#define MODE_ASYNC 'a'
#define MODE_SYNC 'b'
#define MODE_SYNC_SHE 'c'

#define ACCL_OFF '0'
#define ACCL_ACCL '1'
#define ACCL_KEEP '2'
#define ACCL_DECL '3'

//float triangle_wave(float Angle, float Modulation, float Period);
float f(float theta);

void asyn_modulation(void);
void sync_spwm_modulation(void);
void sync_folded_svpwm_modulation(void);
void shepwm_modulation(void);

char current_mode = MODE_OFF;
//char accelerate = ACCL_OFF;
char accelerate = ACCL_DECL;

uint16_t carrier_frequency = 300;
uint16_t synchronous_pulses = 21;

uint16_t hole_count = 2;

float frequency = 60.0f;
float target_modulation = 0;
float modulation = 0;
float sine_wave_angle = 0;
float trig_wave_angle = 0;

void modulate()
{
	if (current_mode == MODE_OFF)
	{
		TIM1->CCR1 = 0;
		TIM1->CCR2 = 0;
		TIM1->CCR3 = 0;
	}
	else if (current_mode == MODE_ASYNC)
	{
		asyn_modulation();
	} else if (current_mode == MODE_SYNC)
	{
		sync_spwm_modulation();
	} else if (current_mode == MODE_SYNC_SHE)
	{
		shepwm_modulation();
	}
}

inline float f(float theta)
{
	//return 1.15470054 * (arm_sin_f32(theta) + 1/6 * arm_sin_f32(3 * theta));

	return arm_sin_f32(theta);
}

void asyn_modulation()
{	
	float T, S1, S2, S3;
	
	
		
	T = triangle_wave(trig_wave_angle, 360); // check
	
	S1 = f(sine_wave_angle / 180 * pi);
	S2 = f(sine_wave_angle / 180 * pi + 2.0/3 * pi);
	S3 = f(sine_wave_angle / 180 * pi + 4.0/3 * pi);
	
	S1 *= modulation;
	S2 *= modulation;
	S3 *= modulation;
	
	TIM1->CCR1 = T > S1 ? 0 : 2;
	TIM1->CCR2 = T > S2 ? 0 : 2;
	TIM1->CCR3 = T > S3 ? 0 : 2;
}

void sync_spwm_modulation()
{
	float T, S1, S2, S3;
	
	T = triangle_wave(sine_wave_angle, 360.0 / synchronous_pulses); // check
		
	S1 = f(sine_wave_angle / 180 * pi);
	S2 = f(sine_wave_angle / 180 * pi + 2.0f/3 * pi);
	S3 = f(sine_wave_angle / 180 * pi + 4.0f/3 * pi);
	
	S1 *= modulation;
	S2 *= modulation;
	S3 *= modulation;
	
	TIM1->CCR1 = T > S1 ? 0 : 2;
	TIM1->CCR2 = T > S2 ? 0 : 2;
	TIM1->CCR3 = T > S3 ? 0 : 2;
}

void sync_folded_svpwm_modulation()
{
	char sector, vector;
	float angle_in_sector;
	float hole_length;
	float fold_point = 9.0f;
	float angle_in_hole_section_in_sector;
	float hole_section_length;
	
	
	sector = (char)(sine_wave_angle / 60.0f);
	angle_in_sector = sine_wave_angle;
	while(angle_in_sector >= 60.0f) angle_in_sector -= 60.0f;
	
	hole_length = (1 - modulation) * 60.0f / hole_count;
	if (hole_length < 3.6f) hole_length = 3.6f;
	
	
	
	hole_section_length = 60.0 / hole_count;
	angle_in_hole_section_in_sector = angle_in_sector;
	while (angle_in_hole_section_in_sector > hole_section_length)
	{
		angle_in_hole_section_in_sector -= hole_section_length;
	}
	
	if (hole_count != 0 &&
		(angle_in_hole_section_in_sector - hole_section_length/2.0f) > (-hole_length) &&
		(angle_in_hole_section_in_sector - hole_section_length/2.0f) < (hole_length)
	)
	{	
	
		TIM1->CCR1 = 0;
		TIM1->CCR2 = 0;
		TIM1->CCR3 = 0;
	} else
	{
		if (angle_in_sector < fold_point) vector = 0;
		if (angle_in_sector > 60 - fold_point) vector = 2;
		else vector = 1;
		
		vector = vector + sector;
		if (vector == 0) vector = 6;
		if (vector == 7) vector = 1;
		switch(vector)
		{
			case 1:	
				TIM1->CCR1 = 2;
				TIM1->CCR2 = 2;
				TIM1->CCR3 = 0;
				break;
			case 2:	
				TIM1->CCR1 = 0;
				TIM1->CCR2 = 2;
				TIM1->CCR3 = 0;
				break;
			case 3:	
				TIM1->CCR1 = 0;
				TIM1->CCR2 = 2;
				TIM1->CCR3 = 2;
				break;
			case 4:	
				TIM1->CCR1 = 0;
				TIM1->CCR2 = 0;
				TIM1->CCR3 = 2;
				break;
			case 5:	
				TIM1->CCR1 = 2;
				TIM1->CCR2 = 0;
				TIM1->CCR3 = 2;
				break;
			case 6:	
				TIM1->CCR1 = 2;
				TIM1->CCR2 = 0;
				TIM1->CCR3 = 0;
				break;
		}
	}
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

void shepwm_modulation()
{
	char index;
	char polarities[3], flip = 0;
	float she_angles[5];
	switch(synchronous_pulses)
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
/**/

void update_angle()
{
	sine_wave_angle += frequency * 360.0f/ 40000.0f;
	if (sine_wave_angle >= 360) sine_wave_angle -= 360;
	
	trig_wave_angle += carrier_frequency * 360.0 / 40000.0;
	if (trig_wave_angle >= 360) trig_wave_angle -= 360;
}
void change_modulation_type(void)
{
	static uint16_t count = 0;
	
	//modulation = (frequency + 0.0) / 100.0f;
	
	//frequency = 90; 
	
	if (modulation < target_modulation)
	{
		modulation += 0.004f;
		if (modulation > target_modulation) modulation = target_modulation;
	}
	if (modulation > target_modulation)
	{
		modulation -= 0.004f;
		if (modulation < target_modulation) modulation = target_modulation;
	}
	if (modulation > 1) modulation = 1.0f;
	
	if(accelerate == ACCL_ACCL)
	{
		
		frequency += .1f;
		if (frequency <= 14.0f)
		{
			current_mode = MODE_ASYNC;
			carrier_frequency = 300;
			target_modulation = 0.3f;
		}
		else if (frequency > 14.0f && frequency <= 28.0f)
		{
			current_mode = MODE_ASYNC;
			carrier_frequency = 400;
			target_modulation = 0.3f;
		}
		else if (frequency > 28.0f && frequency <= 42.0f)
		{
			current_mode = MODE_SYNC;
			synchronous_pulses = 21;
			target_modulation = 0.5f;
		}
		else if (frequency > 42.0f && frequency <= 58.0f)
		{
			current_mode = MODE_SYNC;
			synchronous_pulses = 15;
			target_modulation = 0.7f;
		}
		else if (frequency > 58.0f && frequency <= 89.0f)
		{
			current_mode = MODE_SYNC_SHE;
			synchronous_pulses = 11;
			target_modulation = 0.9f;
		}
		else if (frequency > 89.0f)
		{
			current_mode = MODE_SYNC_SHE;
			synchronous_pulses = 7;
			target_modulation = 1.0f;	
		}
	}
	else if (accelerate == ACCL_DECL)
	{
		
		if (frequency > 0.5f) frequency -= 0.07f;
		
		if (frequency <= 7.0f)
		{
			target_modulation = 0.0f;
			if(modulation == 0) current_mode = MODE_OFF;
		}
		else if (frequency > 7.0f && frequency <= 12.0f)
		{
			current_mode = MODE_ASYNC;
			carrier_frequency = 300;
			target_modulation = 0.1f;
		}
		else if (frequency > 12.0f && frequency <= 23.0f)
		{
			current_mode = MODE_ASYNC;
			carrier_frequency = 400;
			target_modulation = 0.2f;
		}
		else if (frequency > 23.0f && frequency <= 37.3f)
		{
			current_mode = MODE_SYNC;
			synchronous_pulses = 21;
			target_modulation = 0.4f;
		}
		else if (frequency > 37.3f && frequency <= 52.0f)
		{
			current_mode = MODE_SYNC;
			synchronous_pulses = 15;
			target_modulation = 0.5f;
		}
		else if (frequency > 52.0f && frequency <= 89.0f)
		{
			current_mode = MODE_SYNC_SHE;
			synchronous_pulses = 11;
			target_modulation = 0.5f;
		} else if (frequency > 89.0f)
		{
			current_mode = MODE_SYNC_SHE;
			synchronous_pulses = 7;
			target_modulation = 1.0f;

			if (count > 0) --count;
			if (count < 100)
			{
				hole_count = 2;
			} else if (count >= 100 && count < 130)
			{
				hole_count = 1;
			} if (count >= 130)
			{
				hole_count = 0;
			}
		}
	}
	else if (accelerate == ACCL_KEEP)
	{
		//if (frequency <= 1.5) current_mode = MODE_OFF;
		if (frequency > 89.0f)
		{
			if (count > 0) --count;
			if (count < 100)
			{
				hole_count = 2;
			} else if (count >= 100 && count < 130)
			{
				hole_count = 1;
			} if (count >= 130)
			{
				hole_count = 0;
			}
		}
	}
	else if (accelerate == ACCL_OFF)
	{
		target_modulation = 0;
		if (frequency > 89.0f)
		{
			current_mode = MODE_SYNC_SHE;

			if (count > 0) --count;
			if (count < 100)
			{
				hole_count = 2;
			} else if (count >= 100 && count < 130)
			{
				hole_count = 1;
			} if (count >= 130)
			{
				hole_count = 0;
			}
		}
		
		if (count == 0) current_mode = MODE_OFF;
	}
}
/**/

//#include "Key.h"

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
	/**/
	//accelerate = ACCL_DECL;
	
}


