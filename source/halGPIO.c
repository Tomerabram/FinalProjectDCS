#include  "../header/halGPIO.h"     // private library - HAL layer
#include  "../header/flash.h"     // private library - FLASH layer
#include "stdio.h"
#include "stdint.h"
#include "string.h"
//#include "stdlib.h"


// Global Variables
int j=0;
char *ptr1, *ptr2, *ptr3;
short flag_MSB = 0;
short flag_state = 1; // 0-state changed -> send state(pb pressed)
int flag_rotate = 1;
unsigned int delay_time = 500;
const unsigned int timer_half_sec = 65535;
unsigned int i = 0;
unsigned int tx_index;
char counter_str[4];
short Vr[] = {0, 0}; //Vr[0]=Vry , Vr[1]=Vrx, is intended to hold the analog-to-digital conversion results from the joystick's x and y axes
const short state_changed[] = {1000, 1000}; // send if button pressed - state changed
char stringFromPC[80];
char file_content[80];
int flag_execute = 0;
int flag_flashburn = 0;
int SendFlag = 0;
int counter = 510;
char step_str[4];
char finish_str[3] = "FIN";
int curr_counter = 0; // This variable tracks the current position of the stepper motor
short finishIFG = 0;
//--------------------------------------------------------------------
//             System Configuration  
//--------------------------------------------------------------------
void sysConfig(void){ 
	GPIOconfig();
	ADCconfig();
	StopAllTimers();
	UART_init();
}
//--------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------LCD--------------------------------------------------------------------------
//---------------------------------------------------------------------
void CLR_LCD(void) {
    lcd_clear();
}
//---------------------------------------------------------------------
  //            LCD
  //---------------------------------------------------------------------
  //******************************************************************
  // send a command to the LCD
  //******************************************************************
  void lcd_cmd(unsigned char c){

      LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h

      if (LCD_MODE == FOURBIT_MODE)
      {
          LCD_Data_Write &= ~OUTPUT_DATA;// clear bits before new write
          LCD_Data_Write |= ((c >> 4) & 0x0F) << LCD_DATA_OFFSET;
          lcd_strobe();
          LCD_Data_Write &= ~OUTPUT_DATA;
          LCD_Data_Write |= (c & (0x0F)) << LCD_DATA_OFFSET;
          lcd_strobe();
      }
      else
      {
          LCD_Data_Write = c;
          lcd_strobe();
      }
  }


  //******************************************************************
  // send data to the LCD
  //******************************************************************
  void lcd_data(unsigned char c){

      LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h

      LCD_Data_Write &= ~OUTPUT_DATA;
      LCD_RS(1);
      if (LCD_MODE == FOURBIT_MODE)
      {
              LCD_Data_Write &= ~OUTPUT_DATA;
              LCD_Data_Write |= ((c >> 4) & 0x0F) << LCD_DATA_OFFSET;
              lcd_strobe();
              LCD_Data_Write &= (0xF0 << LCD_DATA_OFFSET) | (0xF0 >> 8 - LCD_DATA_OFFSET);
              LCD_Data_Write &= ~OUTPUT_DATA;
              LCD_Data_Write |= (c & 0x0F) << LCD_DATA_OFFSET;
              lcd_strobe();
      }
      else
      {
              LCD_Data_Write = c;
              lcd_strobe();
      }

      LCD_RS(0);
  }

  //******************************************************************
  // write a string of chars to the LCD
  //******************************************************************
  void lcd_puts(const char * s){

      while(*s)
          lcd_data(*s++);
  }

  //******************************************************************
  // initialize the LCD
  //******************************************************************
  void lcd_init(){

      char init_value;

      if (LCD_MODE == FOURBIT_MODE) init_value = 0x3 << LCD_DATA_OFFSET;
      else init_value = 0x3F;

      LCD_RS_DIR(OUTPUT_PIN);
      LCD_EN_DIR(OUTPUT_PIN);
      LCD_RW_DIR(OUTPUT_PIN);
      LCD_Data_Dir |= OUTPUT_DATA;
      LCD_RS(0);
      LCD_EN(0);
      LCD_RW(0);

      DelayMs(15);
      LCD_Data_Write &= ~OUTPUT_DATA;
      LCD_Data_Write |= init_value;
      lcd_strobe();
      DelayMs(5);
      LCD_Data_Write &= ~OUTPUT_DATA;
      LCD_Data_Write |= init_value;
      lcd_strobe();
      DelayUs(200);
      LCD_Data_Write &= ~OUTPUT_DATA;
      LCD_Data_Write |= init_value;
      lcd_strobe();

      if (LCD_MODE == FOURBIT_MODE){
          LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h
          LCD_Data_Write &= ~OUTPUT_DATA;
          LCD_Data_Write |= 0x2 << LCD_DATA_OFFSET; // Set 4-bit mode
          lcd_strobe();
          lcd_cmd(0x28); // Function Set
      }
      else lcd_cmd(0x3C); // 8bit,two lines,5x10 dots

      lcd_cmd(0xF); //Display On, Cursor On, Cursor Blink
      lcd_cmd(0x1); //Display Clear
      lcd_cmd(0x6); //Entry Mode
      lcd_cmd(0x80); //Initialize DDRAM address to zero
  }



  //******************************************************************
  // lcd strobe functions
  //******************************************************************
  void lcd_strobe(){
    LCD_EN(1);
    asm("nOp");
    //asm("NOP");
    LCD_EN(0);
  }
//----------------------------------------------------------------------------------------------------------------------LCD--------------------------------------------------------------------------

//--------------------------------------------------------------------
//              Send FINISH to PC
//--------------------------------------------------------------------
void send_finish_to_PC(){
    finishIFG = 1;
    tx_index = 0;
    UCA0TXBUF = finish_str[tx_index++]; // send the string "FIN" byte after byte to the PC
    IE2 |= UCA0TXIE;                        // Enable USCI_A0 TX interrupt
    __bis_SR_register(LPM0_bits + GIE);
    START_TIMERA0(10000);
    finishIFG = 0;
}

//--------------------------------------------------------------------
//              Send degree to PC
//--------------------------------------------------------------------
void send_degree_to_PC(){
    tx_index = 0;
    UCA0TXBUF = step_str[tx_index++];
    IE2 |= UCA0TXIE;                        // Enable USCI_A0 TX interrupt
    __bis_SR_register(LPM0_bits + GIE); // Sleep
    START_TIMERA0(10000);
}

//---------------------------------------------------------------------
//            General Function
//---------------------------------------------------------------------

// converts an integer value (counter) into a string format (counter_str)
void int2str(char *str, unsigned int num){
    int strSize = 0;
    long tmp = num, len = 0;
    int j;
    if (tmp == 0){
        str[strSize] = '0';
        return;
    }
    // Find the size of the intPart by repeatedly dividing by 10
    while(tmp){
        len++;
        tmp /= 10;
    }

    // Print out the numbers in reverse
    for(j = len - 1; j >= 0; j--){
        str[j] = (num % 10) + '0';
        num /= 10;
    }
    strSize += len;
    str[strSize] = '\0';
}

//-----------------------------------------------------------------------
uint32_t hex2int(char *hex) {
    uint32_t val = 0;
    int o;
    for(o=0; o<2; o++) {
        // get current character then increment
        uint8_t byte = *hex++;
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;
        // shift 4 to make space for new digit, and add the 4 bits of the new digit
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}
//-----------------------------------------------------------------------
void go_to_position(uint32_t stepper_degrees, char script_state){
    int clicks_cnt;
    char degreestr = "Degrees";
    uint32_t step_counts;
    uint32_t calc_temp;
    calc_temp = stepper_degrees * counter;
    step_counts = (calc_temp / 360); // how much moves to wanted degree

    int diff = step_counts - curr_counter;// how many step i need to do
    if(0 <= diff){
        //for every step
        for (clicks_cnt = 0; clicks_cnt < diff; clicks_cnt++){
            curr_counter++;
            Stepper_clockwise();
            START_TIMERA0(10000);
            //send the degree to the PC of operation 6
            if(script_state == '6'){
                int2str(step_str, curr_counter);
                send_degree_to_PC();// show the current degree every step
            }
        }
        if (script_state == '7') {
            int2str(step_str, stepper_degrees);
            lcd_clear();
            lcd_puts(step_str);
            lcd_puts(degreestr);
            }
        send_degree_to_PC();
    }
    else{
        for (clicks_cnt = diff; clicks_cnt < 0; clicks_cnt++){
            curr_counter--;
            Stepper_counter_clockwise();
            START_TIMERA0(10000);
            //send the degree to the PC of operation 6
            if(script_state == '6'){
                int2str(step_str, curr_counter);
                send_degree_to_PC();
            }
        }
        if (script_state == '7') {
            int2str(step_str, stepper_degrees);
            lcd_clear();
            lcd_puts(step_str);
            lcd_puts(degreestr);
        }
        send_degree_to_PC();
    }
}

//----------------------Count Timer Calls---------------------------------
void timer_call_counter(){
    unsigned int num_of_halfSec;
    num_of_halfSec = (int) delay_time / half_sec;
    unsigned int res;
    res = delay_time % half_sec;
    res = res * clk_tmp;
    for (i=0; i < num_of_halfSec; i++){
        TIMER_A0_config(timer_half_sec);
        __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ int until Byte RXed
    }
    if (res > 1000){
        TIMER_A0_config(res);
        __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ int until Byte RXed
    }
}
//---------------------------------------------------------------------
//            Enter from LPM0 mode
//---------------------------------------------------------------------
void enterLPM(unsigned char LPM_level){
	if (LPM_level == 0x00) 
	  _BIS_SR(LPM0_bits);     /* Enter Low Power Mode 0 */
        else if(LPM_level == 0x01) 
	  _BIS_SR(LPM1_bits);     /* Enter Low Power Mode 1 */
        else if(LPM_level == 0x02) 
	  _BIS_SR(LPM2_bits);     /* Enter Low Power Mode 2 */
	else if(LPM_level == 0x03) 
	  _BIS_SR(LPM3_bits);     /* Enter Low Power Mode 3 */
        else if(LPM_level == 0x04) 
	  _BIS_SR(LPM4_bits);     /* Enter Low Power Mode 4 */
}
//---------------------------------------------------------------------
//            Enable interrupts
//---------------------------------------------------------------------
void enable_interrupts(){
  _BIS_SR(GIE);
}
//---------------------------------------------------------------------
//            Disable interrupts
//---------------------------------------------------------------------
void disable_interrupts(){
  _BIC_SR(GIE);
}

//---------------------------------------------------------------------
//            Start Timer With counter
//---------------------------------------------------------------------
void START_TIMERA0(unsigned int counter){// counter represent the numbers of the raising edge till interrupt
    TIMER_A0_config(counter);
    __bis_SR_register(LPM0_bits + GIE);
}
//---------------------------------------------------------------------
//            Start Timer1 With counter
//---------------------------------------------------------------------
void START_TIMERA1(unsigned int counter){
    TIMER_A1_config(counter);
    __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ interrupt
}
// ------------------------------------------------------------------
//                     Polling delays
//---------------------------------------------------------------------
//******************************************************************
// Delay usec functions
//******************************************************************
void DelayUs(unsigned int cnt){

    unsigned char i;
    for(i=cnt ; i>0 ; i--) asm("nop"); // tha command asm("nop") takes raphly 1usec

}
//******************************************************************
// Delay msec functions
//******************************************************************
void DelayMs(unsigned int cnt){

    unsigned char i;
    for(i=cnt ; i>0 ; i--) DelayUs(1000); // tha command asm("nop") takes raphly 1usec

}
//******************************************************************
//            Polling based Delay function
//******************************************************************
void delay(unsigned int t){  //
    volatile unsigned int i;

    for(i=t; i>0; i--);
}
//---------------**************************----------------------------
//               Interrupt Services Routines
//---------------**************************----------------------------
//*********************************************************************
//                        TIMER A0 ISR
//*********************************************************************
#pragma vector = TIMER0_A0_VECTOR // For delay
__interrupt void TimerA_ISR (void)
{
    StopAllTimers();
    LPM0_EXIT;
}

//*********************************************************************
//                        TIMER A ISR
//*********************************************************************
#pragma vector = TIMER1_A0_VECTOR // For delay
__interrupt void Timer1_A0_ISR (void)
{
    if(!TAIFG) { StopAllTimers();
    LPM0_EXIT;
    }
}

//*********************************************************************
//                         ADC10 ISR
//*********************************************************************
#pragma vector = ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
   LPM0_EXIT;
}

//-------------------ATAN2- Fixed point - returns degrees---------------------------
int16_t calc_angle(int16_t y_fp, int16_t x_fp)
{
    int32_t coeff_1 = 45;
    int32_t coeff_1b = -56; // 56.24;
    int32_t coeff_1c = 11;  // 11.25
    int16_t coeff_2 = 135;

    //store the final calculated angle in degrees
    int16_t angle = 0;
    int32_t r;
    int32_t r3;
    int16_t y_abs_fp = y_fp;

    if (y_abs_fp < 0)
        y_abs_fp = -y_abs_fp;

    if (y_fp == 0)
    {
        if (x_fp >= 0)
        {
            angle = 0;
        }
        else
        {
            angle = 180;
        }
    }
    else if (x_fp >= 0)
    {
        r = (((int32_t)(x_fp - y_abs_fp)) << MULTIPLY_FP_RESOLUTION_BITS) /((int32_t)(x_fp + y_abs_fp));
        r3 = r * r;
        r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
        r3 *= r;
        r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
        r3 *= coeff_1c;
        angle = (int16_t) (coeff_1 + ((coeff_1b * r + r3) >> MULTIPLY_FP_RESOLUTION_BITS));
    }
    else
    {
        r = (((int32_t)(x_fp + y_abs_fp)) << MULTIPLY_FP_RESOLUTION_BITS) /((int32_t)(y_abs_fp - x_fp));
        r3 = r * r;
        r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
        r3 *= r;
        r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
        r3 *= coeff_1c;
        angle = coeff_2 + ((int16_t)(((coeff_1b * r + r3) >> MULTIPLY_FP_RESOLUTION_BITS)));
    }

    if (y_fp < 0)
        return (360-angle);
    else
        return (angle);
}


//***********************************TX ISR******************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCI0TX_ISR (void)
#else
#error Compiler not supported!
#endif
{
    //---------------------------for script mode------------------------------------
    //-----------------------for burn-----------------------
    if(state == state3 && finishIFG == 1){  //sending the "FIN" string to PC
        UCA0TXBUF = finish_str[tx_index++];                 // TX next character
        if (tx_index == sizeof step_str - 1) {   //checking if the tx is finish
            tx_index=0;
            IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
            MRstate = stateDefault;
            LPM0_EXIT;
        }
    }
    //-----------------------check for execute-----------------------
    if (state == state3 && finishIFG == 0){  // For script
        UCA0TXBUF = step_str[tx_index++];                 // TX next character

        if (tx_index == sizeof step_str - 1) {   // TX over?
            tx_index=0;
            IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
            MRstate = stateDefault;
            LPM0_EXIT;
        }
    }
    //---------------------------for calibration---------------------------------
    else if (state==state2 && MRstate==stateStopRotate){
        UCA0TXBUF = counter_str[tx_index++];                 // load the next character to the tx buffer
        if (tx_index == sizeof counter_str - 1) {   // check if tx is done?
            tx_index=0;
            IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
            MRstate = stateDefault;
            LPM0_EXIT;
        }
    }

    //----------------------------for painter mode--------------------------------
    // Send Push Button state
    else if (flag_state && state == state1){
        if(flag_MSB) UCA0TXBUF = (state_changed[i++]>>8) & 0xFF; // if MSB flag is 1 then take the 8 upper bits, 1 upper byte, then the operation & 0xFF send only thie byte
        else UCA0TXBUF = state_changed[i] & 0xFF;
        flag_MSB ^= 1; //XOR

        if (i == 2) {  // TX over?
            i=0;
            IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
            START_TIMERA0(10000);
            flag_state = 0;
            LPM0_EXIT;
        }
    }

    //send data for painter
    else if(!flag_state && state == state1){
        if(flag_MSB) UCA0TXBUF = (Vr[i++]>>8) & 0xFF;
        else UCA0TXBUF = Vr[i] & 0xFF;
        flag_MSB ^= 1;
        if (i == 2) {  // TX is over?
            i=0;
            IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
            START_TIMERA0(10000);
            LPM0_EXIT;
        }
    }
}
//***********************************RX ISR******************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
    stringFromPC[j] = UCA0RXBUF;  // Get Whole string from PC
    j++;
    if (stringFromPC[0] == 'm') {state = state0; MRstate=stateDefault; flag_rotate = 0; j = 0;}
    else if (stringFromPC[0] == 'P') { state = state1; MRstate=stateDefault; flag_rotate = 0; j = 0;}
    else if (stringFromPC[0] == 'C') { state = state2; MRstate=stateDefault; flag_rotate = 0; j = 0;}
    else if (stringFromPC[0] == 'S') { state = state3; MRstate=stateDefault; flag_rotate = 0; j = 0;}
    //else if (stringFromPC[0] == 'B') { flag_rotate = 0;}

    //state0 functions
    else if (stringFromPC[0] == 'A'){ MRstate = stateAutoRotate; flag_rotate = 1; j = 0;}// Auto Rotate
    else if (stringFromPC[0] == 'M'){ MRstate = stateStopRotate; flag_rotate = 0; j = 0;}// Stop Rotate
    else if (stringFromPC[0] == 'J'){ MRstate = stateJSRotate; flag_rotate = 0; j = 0;}// JoyStick

    //--------------------------------------for script mode---------------------------------------
    //-------------------------for burn----------------------

    // for sending the file name to flash, its recognize the \n for the end of the string
        if (!SendFlag && stringFromPC[j-1] == '\x0a'){
            for (i=0; i < j; i++){
                file.file_name[i] = stringFromPC[i];
            }
            SendFlag = 1;
            j = 0;
        }
    // for sending the data to a string that holds the data, its recognize the Z for the end of the data
    if (stringFromPC[j-1] == 'Z'){
        j = 0;
        SendFlag = 0;
        strcpy(file_content, stringFromPC);
    }

    //this it to burn the first file
    if (stringFromPC[j-1] == 'W'){
        flag_flashburn = 1;
        ptr1 = (char*) 0x1000;
        file.file_ptr[0]=ptr1;
        file.num_of_files = 1;
        j = 0;
    }

    //this it to burn the second file
    if (stringFromPC[j-1] == 'X'){
        flag_flashburn = 1;
        ptr2 = (char*) 0x1040;
        file.file_ptr[1]=ptr2;
        file.num_of_files = 2;
        j = 0;
    }

    //this it to burn the 3 file
    if (stringFromPC[j-1] == 'Y'){
        flag_flashburn = 1;
        ptr3 = (char*) 0x1080;
        file.file_ptr[2]=ptr3;
        file.num_of_files = 3;
        j = 0;
    }

    //-------------------------for execute----------------------
    //this it to execute the first file
    if (stringFromPC[j-1] == 'T'){
        flag_execute = 1;
        j = 0;
        file.num_of_files = 1;
    }

    //this it to execute the second file
    if (stringFromPC[j-1] == 'U'){
        flag_execute = 1;
        j = 0;
        file.num_of_files = 2;
    }

    //this it to execute the 3 file
    if (stringFromPC[j-1] == 'V'){
        flag_execute = 1;
        j = 0;
        file.num_of_files = 3;
    }

    LPM0_EXIT;
}

//*********************************************************************
//            Port1 Interrupt Service Routine
//*********************************************************************
#pragma vector=PORT1_VECTOR
  __interrupt void Joystick_handler(void){
      IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
      delay(debounceVal);
      if(JoyStickIntPend & BIT5){ //int at P1.5
          flag_state = 1; // send state!
          JoyStickIntPend &= ~BIT5;
      }
      IE2 |= UCA0TXIE;                       // enable USCI_A0 TX interrupt
}
