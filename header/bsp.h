#ifndef _bsp_H_
#define _bsp_H_

#include  <msp430g2553.h>          // MSP430x2xx
//#include  <msp430xG46x.h>  // MSP430x4xx


#define   debounceVal      2000
#define     HIGH    1
#define     LOW     0
//--------------------------------LCD------------------------------------------

// LCD Data
#define LCD_Data_Sel    P2SEL
#define LCD_Data_Dir    P2DIR
#define LCD_Data_Write  P2OUT
#define LCD_Data_Read   P2IN

// LCD Control
#define LCD_CTL_Sel     P1SEL
#define LCD_CTL_Dir     P1DIR
#define LCd_CTL_Write   P1OUT

 //-------------------------------LCD-------------------------------------------


//#define   LEDs_SHOW_RATE   0xFFFF  // 62_5ms

//--------------------------------LEDS------------------------------------------
// LEDS abstraction P2.4-P2.7
//#define LEDsArrPortOut      P2OUT
//#define LEDsArrPortSel      P2SEL
//#define LEDsArrPortDir      P2DIR
//--------------------------------LEDS------------------------------------------

//--------------------------------Joystick------------------------------------------
#define JoyStickPortOUT     P1OUT
#define JoyStickPortSEL     P1SEL
#define JoyStickPortDIR     P1DIR
#define JoyStickPortIN      P1IN
#define JoyStickIntEdgeSel  P1IES
#define JoyStickIntEN       P1IE
#define JoyStickIntPend     P1IFG
#define JoyStickPortREN     P1REN
//--------------------------------Joystick------------------------------------------



//--------------------------------Stepmotor------------------------------------------
#define StepmotorPortOUT     P2OUT
#define StepmotorPortSEL     P2SEL
#define StepmotorPortDIR     P2DIR
//--------------------------------Stepmotor------------------------------------------


#define TXLED BIT0
#define RXLED BIT6
#define TXD BIT2
#define RXD BIT1


extern void GPIOconfig(void);
extern void ADCconfig(void);
extern void TIMER_A0_config(unsigned int counter);
extern void TIMER_A1_config(unsigned int counter);
extern void StopAllTimers(void);
extern void UART_init(void);




#endif



