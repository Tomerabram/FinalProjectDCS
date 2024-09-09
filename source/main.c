#include  "../header/api.h"    		// private library - API layer
#include  "../header/app.h"    		// private library - APP layer
#include  <stdio.h>

//----------------------------------------------
//----------------------------------------------

enum FSMstate state;
enum MOTORstate MRstate;
enum SYSmode lpm_mode;


void main(void){
  state = state0;  // start in idle state on RESET
  MRstate = stateDefault;
  lpm_mode = mode0;     // start in idle state on RESET
  sysConfig();
  lcd_init();

  while(1){
	switch(state){

	case state0: // Manual control of motor-based machine
	    IE2 |= UCA0RXIE; // Enable USCI_A0 RX interrupt
	    switch(MRstate){
            case stateAutoRotate: // start rotation
                while(flag_rotate){
                    curr_counter++;
                    Stepper_clockwise();
                }
                break;

            case stateStopRotate: // stop rotation
                break;

            case stateJSRotate:
                counter = 510;
                Stepper_manually_rotate();
                break;

            case stateDefault:
                __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ int until Byte RXed
                break;
	    }
	    break;

	case state1: // Joystick based PC painter
	    JoyStickIntEN |= BIT5; // Enabling PB0 for changing states
	    while (state == state1){
	        JoyStick_Painter();
	    }
        JoyStickIntEN &= ~BIT5;
	    break;

	case state2: // Stepper Motor Calibration
        IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt

        switch(MRstate){
            case stateDefault:
                __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ int until Byte RXed
                break;

            case stateAutoRotate: // start rotation
                counter = 0;
                while(flag_rotate){
                    Stepper_clockwise();
                    counter++;
                }
                break;

            case stateStopRotate: // stop rotation
                Motor_Calibration();
                break;
        }
	    break;

	case state3: // Script Mode
        IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
	    while ( state == state3){
	        Script_Execution();
	    }
		break;
		
	}
  }
}

