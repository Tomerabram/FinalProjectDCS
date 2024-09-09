#ifndef _api_H_
#define _api_H_

#include  "../header/halGPIO.h"     // private library - HAL layer

extern void JoyStick_Painter();
extern void JoyStickADC_Steppermotor();
extern void Stepper_manually_rotate();
extern void Stepper_clockwise();
extern void Stepper_counter_clockwise();
extern void inc_lcd(char* count);
extern void Motor_Calibration();
extern void Script_Execution();
extern void script_execute();
extern void rra_lcd(int* rotateLCD);

extern int16_t Vrx;
extern int16_t Vry;






#endif







