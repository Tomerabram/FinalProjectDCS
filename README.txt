This project involves a microcontroller system using the MSP430, controlled via a PC-side GUI. The project includes multiple source files, each responsible for specific tasks within the system. Below is an overview of each file and the functions it contains.

Source Files
##main.c
Purpose: Manages the main control flow of the application through a finite state machine (FSM) that handles different modes of operation, including manual control of a motor, joystick-based painting, motor calibration, and script execution.

Functions:

main(void): Initializes system states, configures the system, and continuously executes the FSM to handle different application states.
##api.c
Purpose: Implements high-level application functions, including the control of the stepper motor and joystick inputs, as well as functions for calibration and script execution.

Functions:

#JoyStickADC_Steppermotor(): Reads joystick input using the ADC.

#Stepper_manually_rotate(): Controls the stepper motor rotation based on joystick input.

#JoyStick_Painter(): Handles joystick input and sends data for the painting application.

#Motor_Calibration(): Performs motor calibration and sends results to the PC.

#Script_Execution(): Executes a script loaded from the flash memory.

#script_execute(void): Executes operations specified in a script, such as incrementing/decrementing LCD values, rotating the stepper motor, and scanning between angles.

#inc_lcd(char* count): Increments a value on the LCD.

#rra_lcd(int* rotateLCD): Rotates the LCD display.

#Stepper_clockwise(): Rotates the stepper motor clockwise.

#Stepper_counter_clockwise(): Rotates the stepper motor counterclockwise.

#int2str(char *str, unsigned int num): Converts an integer to a string.

#hex2int(char *hex): Converts a hexadecimal string to an integer.

#go_to_position(uint32_t stepper_degrees, char script_state): Moves the stepper motor to a specified position.

#timer_call_counter(): Configures the timer to manage delays.

#enterLPM(unsigned char LPM_level): Enters a specified low power mode.

#enable_interrupts(): Enables global interrupts.

#disable_interrupts(): Disables global interrupts.

#START_TIMERA0(unsigned int counter): Starts Timer A0 with a specific count.

#DelayUs(unsigned int cnt): Delays execution for a specified number of microseconds.

#DelayMs(unsigned int cnt): Delays execution for a specified number of milliseconds.

#delay(unsigned int t): General-purpose delay function.

#calc_angle(int16_t y_fp, int16_t x_fp): Calculates the angle based on joystick input.

## halGPIO.c
Purpose: Contains low-level hardware abstraction functions, managing the configuration of GPIOs, timers, ADC, and UART. It also handles LCD operations.

Functions:

#sysConfig(void): Configures the GPIO, ADC, and UART settings for the system.

#CLR_LCD(void): Clears the LCD display.

#lcd_cmd(unsigned char c): Sends a command to the LCD.

#lcd_data(unsigned char c): Sends data to the LCD.

#lcd_puts(const char * s): Writes a string to the LCD.

#lcd_init(): Initializes the LCD.

#lcd_strobe(): Strobes the LCD to latch data/commands.

#send_finish_to_PC(): Sends a "FIN" string to the PC to indicate the completion of an operation.

#send_degree_to_PC(): Sends the current stepper motor degree to the PC.

#TIMER_A0_config(unsigned int counter): Configures Timer A0.

#ADCconfig(void): Configures the ADC for joystick input.

#UART_init(void): Initializes the UART for serial communication.

#StopAllTimers(void): Stops all active timers.

#GPIOconfig(void): Configures GPIO pins for various functions.

#DelayUs(unsigned int cnt): Delays for a specified number of microseconds.

#DelayMs(unsigned int cnt): Delays for a specified number of milliseconds.

#delay(unsigned int t): Delays for a general purpose.

#TIMER_A0_config(unsigned int counter): Configures Timer A0.

#START_TIMERA1(unsigned int counter): Configures and starts Timer A1.

#int2str(char *str, unsigned int num): Converts an integer to a string.

#hex2int(char *hex): Converts a hexadecimal string to an integer.

#go_to_position(uint32_t stepper_degrees, char script_state): Moves the stepper motor to a specified position.

##flash.c
Purpose: Manages the functions related to flash memory operations, including reading, writing, and storing scripts in the flash memory of the microcontroller.

Functions: The specific functions in flash.c are not provided in the snippet, but typically this file would include:

#write_data(): Writes data to flash memory.

#read_data(): Reads data from flash memory.

#erase_flash(): Erases a section of flash memory.

#save_script_to_flash(): Saves a script to flash memory for later execution.

##bsp.c
Purpose: Handles the initialization of various hardware modules, including the GPIO, LCD, UART, ADC, and timers.

Functions:

#GPIOconfig(void): Configures GPIO pins for LCD, Joystick, and Stepper motor.

#StopAllTimers(void): Stops all timers in the system.

#TIMER_A0_config(unsigned int counter): Configures Timer A0 for specific timing operations.

#UART_init(void): Initializes UART communication.

#ADCconfig(void): Configures the ADC for reading joystick inputs.

##GUI.py
Purpose: Serves as the interface between the PC and the MSP430 microcontroller. It provides a graphical user interface (GUI) that allows the user to control various functions of the microcontroller, such as motor control, painting, calibration, and script execution.

Key Libraries Used:

#PySimpleGUI: For creating the graphical user interface.

#serial: For managing serial communication with the microcontroller.

#tkinter: For additional GUI components.

#mouse: For mouse control based on joystick input.

#threading: For handling multiple tasks concurrently.

#binascii: For binary data manipulation.
