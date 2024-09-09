
#include  "../header/api.h"    		// private library - API layer
#include  "../header/halGPIO.h"     // private library - HAL layer
#include  "../header/flash.h"     // private library - FLASH layer
#include "stdio.h"
#include "math.h"

int flag_script = 1; // for load the script im working on, while im doing execute
unsigned int debouncevaljoy=50;
int16_t Vrx = 0;
int16_t Vry = 0;
char CountUp_Str[5];


//---------------------------------------------------------------------------------------------------------------------------------------
//                                                             STATE0
//---------------------------------------------------------------------------------------------------------------------------------------
void JoyStickADC_Steppermotor(){
        ADC10CTL0 &= ~ENC; //clears the ENC (Enable Conversion), disables the ADC, which is a necessary step
        while (ADC10CTL1 & ADC10BUSY);               // Wait if ADC10 core is active, if ADC10BUSY bit is set, the expression evaluates to true and the loop continues
        ADC10SA = &Vr;                        // Data buffer start, ADC10SA register tells the ADC where in memory to put this result
        ADC10CTL0 |= ENC + ADC10SC; // starts the ADC conversion process. Sampling and conversion start, The ENC (Enable Conversion) bit is set to enable the conversion, and the ADC10SC (Start Conversion)
        __bis_SR_register(LPM0_bits + GIE);        // LPM0, ADC10_ISR will force exit
}

//-------------------------------------------------------------
//                Stepper Using JoyStick
//-------------------------------------------------------------
void Stepper_manually_rotate(){
    uint32_t counter_phi; //represents an unsigned 32-bit integer
    uint32_t phi;
    uint32_t temp;
    int32_t test;
    while (counter != 0 && state==state0 && MRstate==stateJSRotate){
        JoyStickADC_Steppermotor();// function is designed to read the analog joystick inputs using the ADC10 module of a microcontroller, the result of the conversion are stored in the 'Vr' array

        //This condition checks if the joystick is not in the central, The joystick typically outputs values around 450 when centered, so the range 350 to 550 represents a dead zone where no action should be taken
        if (!( Vr[1] > 350 && Vr[1] < 550 && Vr[0] > 350 && Vr[0] < 550)){
            // These lines calculate the offset of the joystick's x and y positions from the center
            Vrx = Vr[1] - 464;
            Vry = Vr[0] - 454;
            phi = calc_angle(Vry, Vrx);
//            temp = phi * counter/360; // How many steps needed from angle 0 to the joystick's angle
//            test = temp - curr_counter; // Distance between the joystick's angle and the current of the stepper motor
//
//            if (test > 0){
//                if (abs(test) <= 180){
//                    Stepper_clockwise(); // Move stepper motor clockwise direction
//                    curr_counter++;
//                }
//                else if (abs(test) > 180){
//                    Stepper_counter_clockwise(); // Move stepper motor counter clockwise direction
//                    curr_counter--;
//                }
//            }
//            else if (test < 0){
//                if (abs(test) <= 180){
//                    Stepper_counter_clockwise(); // Move stepper motor clockwise direction
//                    curr_counter--;
//                }
//                else if (abs(test) > 180){
//                    Stepper_clockwise(); // Move stepper motor counter clockwise direction
//                    curr_counter++;
//                }
//            }
//            // else for the case of test = 0: don't move the stepper motor
//            curr_counter = curr_counter % 514; // Modulo 514

            temp = phi * counter; //Multiplies the calculated angle phi by counter to produce a temporary value that will be used to determine how far the motor needs to rotate

            if (270 < phi) {
                counter_phi = ((counter * 7) / 4) - (temp / 360);  // ((630-phi)/360)*counter;
            }
            else {
                counter_phi = ((counter * 3) / 4) - (temp / 360);  // ((270-phi)/360)*counter;
            }
            if ((int)(curr_counter - counter_phi) < 0) {//determines whether the motor should move clockwise or counterclockwise
                Stepper_clockwise();
                curr_counter++;
            }
            else if ((int)(curr_counter - counter_phi) > 0){
                Stepper_counter_clockwise();
                curr_counter--;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
//                                                   STATE1 - Painter
//---------------------------------------------------------------------------------------------------------------------------------------
void JoyStick_Painter(){
    JoyStickIntEN &= ~BIT5; // allow interrupt only in the end of cycle
    i = 0;
    if(!flag_state) { //send data
        ADC10CTL0 &= ~ENC; //Disables the ADC conversion
        while (ADC10CTL1 & ADC10BUSY);               // Wait if ADC10 core is active
        ADC10SA = &Vr;                        // Data buffer start
        ADC10CTL0 |= ENC + ADC10SC; // starts the ADC conversion process. Sampling and conversion start, The ENC (Enable Conversion) bit is set to enable the conversion, and the ADC10SC (Start Conversion)
        __bis_SR_register(LPM0_bits + GIE);        // LPM0, ADC10_ISR will force exit
        delay(debouncevaljoy);
        UCA0TXBUF = Vr[i] & 0xFF; //the purpose is to ensure that only the least significant 8 bits of Vr[i] are sent to the UART transmit buffer. This is necessary because UCA0TXBUF is 8 bits wide, meaning it can only handle one byte (8 bits) of data at a time.
        flag_MSB = 1;
        IE2 |= UCA0TXIE; //Enables the UART transmit interrupt (UCA0TXIE), which will trigger when the transmit buffer is ready for more data
        __bis_SR_register(LPM0_bits + GIE);        // LPM0, will exit when finish tx
    }

    else if (flag_state) { //send state
        UCA0TXBUF = state_changed[i] & 0xFF; //the & 0xFF operation send only the 8 lower bits, means 1 lower byte
        flag_MSB = 1; //Sets the MSB flag
        IE2 |= UCA0TXIE; // Enables the UART transmit interrupt.
        __bis_SR_register(LPM0_bits + GIE);        // LPM0, will exit when finish tx
        START_TIMERA0(5000); //wait for PC to get and sync after all the debounce and interrupt delay
        JoyStickIntPend &= ~BIT5;
    }

    JoyStickIntEN |= BIT5; // allow interrupt only in the end of cycle
}




//----------------------------------------------------------------------------------------------------------------------------------
//                                          STATE2 - Calibration
//----------------------------------------------------------------------------------------------------------------------------------
void Motor_Calibration(){
    int2str(counter_str, counter);
    tx_index = 0;
    UCA0TXBUF = counter_str[tx_index++]; //Loads the first character of counter_str to the tx buffer
    IE2 |= UCA0TXIE;                        // Enable USCI_A0 TX interrupt
    __bis_SR_register(LPM0_bits + GIE); // Sleep
    curr_counter = 0;
}


//----------------------------------------------------------------------------------------------------------------------------------
//                                          STATE3 - Script
//----------------------------------------------------------------------------------------------------------------------------------
void Script_Execution() {
    if(flag_flashburn){
        flag_flashburn=0;
        FCTL2 = FWKEY + FSSEL0 + FN1;   // MCLK/3 for Flash Timing Generator
        file.file_size[file.num_of_files - 1] = strlen(file_content) - 1; //save in the flash the size of the file
        write_data(); // save the data to the flash
        send_finish_to_PC(); // send the string "FIN" to PC
        IE2 |= UCA0RXIE;
    }
    if(flag_execute){
        flag_execute=0;
        flag_script = 1;
        delay_time = 500;  // delay default time
        script_execute();
        send_finish_to_PC(); // send the string "FIN" to PC
    }
    __bis_SR_register(LPM0_bits + GIE);
}

//---------------Execute Script--------------------------------
void script_execute(void)
{
    char *Flash_ptrscript;
    char OPCstr[10], Operand1Flash[20], Operand2Flash[20];
    unsigned int Oper2ToInt, X, start, stop, y, count=1, k;
    if (flag_script)
        Flash_ptrscript = file.file_ptr[file.num_of_files - 1]; //the pointer points on the data file from the flash
    flag_script = 0;

    for (y = 0; y < 64;)
    {
        count = 1;
        //the two lines is for the operation that we want to execute
        OPCstr[0] = *Flash_ptrscript++;
        OPCstr[1] = *Flash_ptrscript++;
        y = y + 2;
        switch (OPCstr[1])
        {

        //operation for inc_lcd
        case '1':
            //the two lines is for the number
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;
            y = y + 2;
            //convert the number from hex to int
            Oper2ToInt = hex2int(Operand1Flash);
            while (Oper2ToInt > 0)
            {
                int2str(CountUp_Str, count) ;// convert the to str
                inc_lcd(CountUp_Str);
                count++;
                Oper2ToInt--;
            }
            break;

        //operation for dec_lcd
        case '2':
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;
            y = y + 2;
            Oper2ToInt = hex2int(Operand1Flash);
            count = Oper2ToInt;
            while (Oper2ToInt)
            {
                int2str(CountUp_Str, count) ;
                inc_lcd(CountUp_Str);
                count--;
                Oper2ToInt--;
            }
            count=1;
            break;

        //operation for rra_lcd
        case '3':
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;
            y = y + 2;
            Oper2ToInt = hex2int(Operand1Flash);
            CLR_LCD();
            for(k=0;k<32;k++){
                if(k==16)
                {
                    lcd_new_line;
                }
                CLR_LCD();

                for (j=0;j<k;j++){
                    if(j==16){lcd_new_line;}
                    lcd_data(' ');
                }
                rra_lcd(Operand1Flash[1]);
            }
            break;

        //operation for set_delay
        case '4':
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;
            y = y + 2;
            delay_time = hex2int(Operand1Flash);
            delay_time = delay_time * 10 ; //10ms
            break;

        //operation for clear_lcd
        case '5':
            CLR_LCD();
            break;

        //operation for stepper_deg, that points of the stepper motor to deg p
        case '6':
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;
            y = y + 2;
            X = hex2int(Operand1Flash);
            go_to_position(X, OPCstr[1]);
            break;

        //operation for stepper_scan, scan area between the angle l to r
        case '7':
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;
            y = y + 2;
            Operand2Flash[0] = *Flash_ptrscript++;
            Operand2Flash[1] = *Flash_ptrscript++;
            y = y + 2;

            start = hex2int(Operand1Flash);
            go_to_position(start, OPCstr[1]);
            stop = hex2int(Operand2Flash);
            go_to_position(stop, OPCstr[1]);
            break;

        case '8':
            break;

        }
    }
}

//-------------------------------------------------------------
//                 inc_lcd - count up from zero the x
//-------------------------------------------------------------
void inc_lcd(char* count){
    lcd_clear();
    lcd_puts(count);
    timer_call_counter();

}

//-------------------------------------------------------------
//                Rotate x on the LCD
//-------------------------------------------------------------
void rra_lcd(int* rotateLCD){
    lcd_data(rotateLCD);
    timer_call_counter();


}


//--------------------------------------------------------------------------------------------------------------------------
//                                          STEPPER FUNCTIONS
//--------------------------------------------------------------------------------------------------------------------------
//                Stepper clockwise
//-------------------------------------------------------------
void Stepper_clockwise(){
    int speed_clk;// for 45Hz
    speed_clk = 2912; //(2^20/8)*(1/45[Hz]) = 2912
    StepmotorPortOUT = 0x01; // out = 0001 while DCBA
    START_TIMERA0(speed_clk);
    StepmotorPortOUT = 0x08; // out = 1000 while DCBA
    START_TIMERA0(speed_clk);
    StepmotorPortOUT = 0x04; // out = 0100 while DCBA
    START_TIMERA0(speed_clk);
    StepmotorPortOUT = 0x02; // out = 0010 while DCBA
    START_TIMERA0(speed_clk);
}

//-------------------------------------------------------------
//                Stepper counter-clockwise
//-------------------------------------------------------------
void Stepper_counter_clockwise(){
    int speed_clk;
    // 1 step counter-clockwise of stepper
//    speed_clk = 131072/speed_Hz;
    speed_clk = 2900; //(2^20/8)*(1/200[Hz]) = 655
    StepmotorPortOUT = 0x08; // out = 0001
    START_TIMERA0(speed_clk); // (2^20/8)*(1/20[Hz]) = 6553
    StepmotorPortOUT = 0x01; // out = 1000
    START_TIMERA0(speed_clk); // (2^20/8)*(1/20[Hz]) = 6553
    StepmotorPortOUT = 0x02; // out = 0100
    START_TIMERA0(speed_clk); // (2^20/8)*(1/20[Hz]) = 6553
    StepmotorPortOUT = 0x04; // out = 0010
    START_TIMERA0(speed_clk); // (2^20/8)*(1/50[Hz]) = 2621
}
