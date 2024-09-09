import PySimpleGUI as sg ##creating simple graphical user inerfaces (GUI)
import serial as ser
import time
import serial.tools.list_ports
from tkinter import *
from tkinter.colorchooser import askcolor
import mouse
import os
from os import path
import threading
import binascii


class Paint(object):
    DEFAULT_PEN_SIZE = 4.0
    DEFAULT_COLOR = 'black'

    def __init__(self):
        self.root = Tk()
        self.root.title("PC Painter")

        self.pen_button = Button(self.root, text='', command=self.use_pen)

        self.brush_button = Button(self.root, text='Back', command=self.close_painter, width=10, bg='darkblue', fg='white')
        self.brush_button.grid(row=0, column=0, padx=5, pady=5)

        self.color_button = Button(self.root, text='', command=self.choose_color)

        self.eraser_button = Button(self.root, text='', command=self.use_eraser)

        self.choose_size_button = Scale(self.root, from_=6, to=10, orient=HORIZONTAL)
        #self.choose_size_button.grid(row=0, column=4)

        self.c = Canvas(self.root, bg='white', width=1600, height=700)
        self.c.grid(row=1, column=0, columnspan=1, padx=10, pady=10)

        self.setup()
        self.root.mainloop()

    def setup(self):
        self.old_x = None
        self.old_y = None
        self.line_width = self.choose_size_button.get()
        self.color = self.DEFAULT_COLOR
        self.eraser_on = False
        self.active_button = self.pen_button
        self.c.bind('<Motion>', self.paint)
        # self.c.bind('<ButtonRelease-1>', self.paint)
        self.c.bind('<ButtonRelease-1>', self.reset)

    def use_pen(self):
        self.activate_button(self.pen_button)

    def use_brush(self):
        self.activate_button(self.brush_button)

    def choose_color(self):
        self.eraser_on = False
        self.color = askcolor(color=self.color)[1]

    def use_eraser(self):
        self.activate_button(self.eraser_button, eraser_mode=True)

    def activate_button(self, some_button, eraser_mode=False):
        self.active_button.config(relief=RAISED)
        some_button.config(relief=SUNKEN)
        self.active_button = some_button
        self.eraser_on = eraser_mode

    def paint(self, event):
        global state
        if self.old_x and self.old_y:
            if state == 0:  # Paint mode
                paint_color = self.color
            elif state == 1:  # Erase mode
                paint_color = 'white'
            else:  # Neutral state, do nothing
                return
            self.c.create_line(self.old_x, self.old_y, event.x, event.y,
                               width=self.line_width, fill=paint_color,
                               capstyle=ROUND, smooth=TRUE, splinesteps=36)
        self.old_x = event.x
        self.old_y = event.y

    def reset(self, event):
        self.old_x, self.old_y = None, None

    def close_painter(self):
        # show_window(1)
        global PaintActive
        PaintActive = 0
        self.root.destroy()


class PortError(Exception):
    """Raised when the port not found"""
    pass


# Automatic PORT search
# This function attemps to automatically find the correct serial port connected to the MSP430 by searching for a device description that include "MSP430"
def port_search(between=None):
    # find the right com that connect between the pc and controller
    ports = serial.tools.list_ports.comports() #this line retrieves a list of all available serial ports on the system
    for desc in sorted(ports):
        if "MSP430" in desc.description:
            return desc.device
    raise PortError


# This function allows you to display one specific layout (or "column") while hiding the others
def show_window(layout_num, window):#An integer representing the number of the layout, The PySimpleGUI window object that contains the layouts
    for i in range(1, 7):
        if i == layout_num:
            window[f'COL{layout_num}'].update(visible=True) #this line is executed. It updates the visibility of the layout
        else:
            window[f'COL{i}'].update(visible=False)


# Sends a command to the MSP430 over the serial connection, responsible for sending data
def send_state(message=None, file_option=False): # file_option: A boolean flag that determines whether the message should be treated as binary data (True) or as a plain ASCII string (False)
    s.reset_output_buffer()
    if file_option:
        bytesMenu = message
    else:
        try:
            bytesMenu = bytes(message, 'ascii') #This conversion is necessary because serial communication requires data to be sent as bytes, not as a plain string
        except UnicodeEncodeError:
        # If ASCII encoding fails, fallback to UTF-8 encoding
            bytesMenu = bytes(message, 'utf-8')
    print("bytesMenu is : ", bytesMenu)
    s.write(bytesMenu)
    while s.out_waiting > 0:
        print(f"Bytes in output buffer: {s.out_waiting}")
    #time.sleep(0.001)
    #for byte in bytesMenu:
    #    s.write(byte)
    #    time.sleep(0.1)  # Add a small delay between each byte


# Reads data from the MSP430, parsing it differently depending on the current mode
def read_from_MSP(current_state,size):  # current_state: A string that determines how the data read from the MSP430 should be processed. Possible values include 'Painter' and 'script'
    n = 4  # This variable is used later in the function to determine how the data is split into chunks, specifically in the 'Painter' state
    while True:
        while s.in_waiting > 0:  # while the input buffer isn't empty, s.in_waiting attribute gives the number of bytes waiting to be read from the serial connection
            if current_state == 'Painter':
                message = s.read(size=size)   #read of size bytes
                message = binascii.hexlify(message).decode('utf-8')  # This line converts the binary message into a hexadecimal string
                message_split = "".join([message[x:x + 2] for x in range(0, len(message), 2)][::-1])
                final_message = [message_split[i:i + n] for i in range(0, len(message_split), n)]

            elif current_state == 'script':
                final_message = s.read(size=size).decode('utf-8')
                print("final message: " + final_message)
            else:
                final_message = s.readline().decode('utf-8')

            # Check for state change command
            if "state_change" in final_message:
                new_state = int(final_message.split(":")[1])
                global state
                state = new_state  # Update the global state variable
                print(f"State changed to: {state}")

            return final_message


# Reads joystick telemetry from the MSP430 and interprets the joystick's X and Y values to control the cursor or painting action.
def getJoystickTelemetry():
    global state #The state might be used to track the current mode or status of the program
    telemetry_values = read_from_MSP('Painter', 4) #It reads 4 bytes of data from the MSP430
    # These lines convert the first two elements of the telemetry_values data (presumably the X and Y joystick positions) from hexadecimal strings to integers
    Vx = int((telemetry_values[0]), 16)
    Vy = int((telemetry_values[1]), 16)

    if Vx > 1024 or Vy > 1024: #Joysticks often use a 10-bit ADC, where the range is from 0 to 1023
        telemetry_values[0] = "".join([telemetry_values[0][x:x + 2] for x in range(0, len(telemetry_values[0]), 2)][::-1])
        telemetry_values[1] = "".join([telemetry_values[1][x:x + 2] for x in range(0, len(telemetry_values[1]), 2)][::-1])
        Vx = int((telemetry_values[0]), 16)
        Vy = int((telemetry_values[1]), 16)
    print("Vx: " + str(Vx) + ", Vy: " + str(Vy) + ", state: " + str(state))
    return Vx, Vy

# Handles the transmission of messages to the MSP430, potentially including file contents in the script mode.
def message_handler(message=None, FSM=False, file=False):
    s.reset_output_buffer()
    txChar = message  # enter key to transmit
    if FSM:
        s.write(b"\x7f")  # the function sends the special control character \x7f (which is the ASCII "Delete" character, often used as a special marker) to the MSP430
    if not file:
        bytesChar = bytes(txChar, 'ascii')
    else:
        bytesChar = txChar
    s.write(bytesChar)
    print(f"cp send: {message}")


# initiate a painting application within a GUI context
def start_painter(window):
    #  window.close()
    Paint()


def translate_binary_content(file_content_b):
    # Define the opcode map based on the table
    opcode_map = {
        b"inc_lcd": b"01",
        b"dec_lcd": b"02",
        b"rra_lcd": b"03",
        b"set_delay": b"04",
        b"clear_lcd": b"05",
        b"stepper_deg": b"06",
        b"stepper_scan": b"07",
        b"sleep": b"08"
    }

    lines = file_content_b.splitlines()
    translated_lines = []

    for line in lines:
        parts = line.strip().split()

        if not parts:
            continue  # Skip empty lines

        instruction = parts[0]
        operands = parts[1:] if len(parts) > 1 else []

        # Get the opcode from the map
        opcode = opcode_map.get(instruction)
        if opcode is None:
            continue  # Skip unknown instructions

        operand_hex = b""
        for operand in operands:
            # Handle comma-separated operands
            sub_operands = operand.split(b',')
            for sub_operand in sub_operands:
                operand_hex += format(int(sub_operand), '02X').encode('ascii')

        # Combine opcode and operands
        translated_line = opcode + operand_hex

        # Add the translated line to the list
        translated_lines.append(translated_line)

    return b"\r\n".join(translated_lines)


def GUI():
    sg.theme('DarkBlue7')

    # The function defines several layouts, each corresponding to a different screen or mode in the application.
    layout_main = [
        [sg.Text("Final Project DCS", size=(35, 4), justification='center', font='Verdana 15')],
        [sg.Button("Manual control of motor based machine", key='Manualcontrol', size=(35, 3), font='Verdana 15')],
        [sg.Button("Joystick based PC painter", key='_Painter_', size=(35, 3), font='Verdana 15')],
        [sg.Button("Stepper Motor Calibration", key='_Calib_', size=(35, 3), font='Verdana 15')],
        [sg.Button("Script Mode", key='_Script_', size=(35, 3), font='Verdana 15')]
    ]

    layout_manualstepper = [
        [sg.Text("Manual control of motor based machine", size=(35, 2), justification='center', font='Verdana 15')],
        [sg.Button("Rotate StepperMotor", key='_Rotation_', size=(20, 2), font='Verdana 12', pad=(10, 20)),
         sg.Button("Stop Rotate", key='_Stop_', size=(20, 2), font='Verdana 12',pad=(10, 20)),
         sg.Button("Give Control To JoyStick", key='_JoyStickCrtl_', size=(20, 2), font='Verdana 12',pad=(10, 20))],
        [sg.Button("Back", key='_BackMenu_', size=(20, 2), font='Verdana 12', pad=(335, 100))]
    ]

    layout_painter = [
        [sg.Text("Joystick-based PC painter", size=(35, 2), justification='center', font='Verdana 15')],
        [sg.Canvas(size=(100, 100), background_color='red', key='canvas')],
        [sg.Button("Back", key='_BackMenu_', size=(5, 1), font='Verdana 15', pad=(300, 180))]
    ]

    layout_calib = [
        [sg.Text("Stepper Motor Calibration", size=(35, 2), justification='center', font='Verdana 15')],
        [sg.Button("Start Rotate", key='_Rotation_', size=(15, 2), font='Verdana 12', pad=(10, 20)),
         sg.Button("Stop Rotate", key='_Stop_', size=(15, 2), font='Verdana 12', pad=(10, 20))],
        [sg.Text("Counter:", size=(10, 1), justification='right', font='Verdana 12', pad=(20, 10)),
         sg.Text(" ", size=(15, 1), key="Counter", justification='left', font='Verdana 12', relief='sunken',
                 background_color='black', text_color='white')],
        [sg.Text("Phi:", size=(10, 1), justification='right', font='Verdana 12', pad=(20, 10)),
         sg.Text(" ", size=(15, 1), key="Phi", justification='left', font='Verdana 12', relief='sunken',
                 background_color='black', text_color='white')],
        [sg.Button("Back", key='_BackMenu_', size=(10, 2), font='Verdana 12', pad=(5, 30))]
    ]

    # Define the file_viewer layout, which is part of the script mode. It allows users to browse for a folder, select a file, and burn the file to the MSP430. It also includes a feedback area (_ACK_) for showing messages like "burned successfully."
    file_viewer = [[sg.Text("File Folder:", size=(12, 1), justification='right', font='Verdana 12'),
                    sg.In(size=(25, 1), enable_events=True, key='_Folder_', background_color='darkblue'),#Creates an input field
                    sg.FolderBrowse(font='Verdana 12')], #Adds a button to the GUI that, when clicked, opens a dialog box allowing the user to browse and select a folder.
                   [sg.Listbox(values=[], enable_events=True, size=(50, 15), key="_FileList_",font='Verdana 12', background_color='darkblue')],
                   [sg.Button('Back', key='_BackMenu_', size=(10, 2), font='Verdana 12', pad=(10, 10)),
                    sg.Button('Burn', key='_Burn_', size=(10, 2), font='Verdana 12', pad=(10, 10))],
                    [sg.Text(' ', key='_ACK_', size=(50, 1), font='Verdana 10', justification='center', pad=(10, 20))]]

    file_description = [
        [sg.Text("File Description", size=(35, 1), justification='center', font='Verdana 15')],
        [sg.Text("", size=(50, 1), key="_FileHeader_", font='Verdana 12', background_color='darkblue')],
        [sg.Multiline("", size=(50, 15), key="_FileContent_", font='Verdana 12', background_color='darkblue')],
        # Removed relief
        [sg.HSeparator()],
        [sg.Text("Executed List", size=(35, 1), justification='center', font='Verdana 15')],
        [sg.Listbox(values=[], enable_events=True, size=(50, 5), key="_ExecutedList_", font='Verdana 12', background_color='darkblue')],
        [sg.Button('Execute', key='_Execute_', size=(15, 2), font='Verdana 12', pad=(10, 10)),
         sg.Button('Clear', key='_Clear_', size=(15, 2), font='Verdana 12', pad=(10, 10))]
    ]

    layout_calc = [
        [sg.Text(" ", key='_FileName_', size=(35, 2), justification='center', font='Verdana 15', text_color='white')],
        [sg.Text("Current Degree: ", justification='right', font='Verdana 15', text_color='white', size=(15, 1)),
         sg.Text(" ", size=(20, 2), key="Degree", justification='left', font='Verdana 15', background_color='black',
                 text_color='white')],
        [sg.Button("Back", key='_BackScript_', size=(15, 2), font='Verdana 15', button_color=('white', 'darkblue'),
                   pad=(5, 10)),
         sg.Button('Run', key='_Run_', size=(15, 2), font='Verdana 15', button_color=('white', 'darkblue'),
                   pad=(5, 10))]
    ]

    layout_script = [[sg.Column(file_viewer),
                      sg.VSeparator(), #This adds a vertical line between columns, visually separating them
                      sg.Column(file_description)]]

    #This is the variable name that holds the layout configuration, and it contains the structure and content of what the window will display.
    layout = [[sg.Column(layout_main, key='COL1', ),
               sg.Column(layout_manualstepper, key='COL2', visible=False),
               sg.Column(layout_painter, key='COL3', visible=False),
               sg.Column(layout_calib, key='COL4', visible=False),
               sg.Column(layout_script, key='COL5', visible=False),
               sg.Column(layout_calc, key='COL6', visible=False)]]

    global window
    # Create the Window, This window serves as the container for all the GUI elements (buttons, text, input fields, etc.) that the user will interact with.
    window = sg.Window(title='Control System of motor-based machine', element_justification='c', layout=layout,
                       size=(1800, 850))
    canvas = window['canvas']
    execute_list = []
    empty_list = []
    file_name = ''
    file_size = ''
    file_path = ''
    global PaintActive
    while True:
        #reads user interactions (event) and any input values from the window
        event, values = window.read()
        # This variable event captures the specific event that occurred, like Clicking a button, Closing the window.The event variable will contain a string that represents the event, which usually corresponds to
        if event == "Manualcontrol":
            #The function sends the appropriate command to the MSP430 ('m' for manual stepper mode)
            send_state('m')  # manual stepper
            show_window(2, window)
            while "_BackMenu_" not in event:
                event, values = window.read()
                if event == "_Rotation_":
                    send_state('A')  # Auto Rotate
                elif event == "_Stop_":
                    send_state('M')  # Stop , was S
                elif event == "_JoyStickCrtl_":
                    send_state('J')  # JoyStick Control
            #send_state('B')

        if event == "_Painter_":
            global state
            #The state variable is used to keep track of the current mode or state of the painter (e.g., different painting modes, eraser mode, etc.).
            #   state = 0
            #indicating that the painter mode is now active
            PaintActive = 1
            send_state('P')  # Painter
            #starts the painter in a separate thread
            #By running this function in a separate thread, the GUI remains responsive, allowing the main program to continue processing user inputs and joystick telemetry concurrently
            paint_thread = threading.Thread(target=start_painter, args=(window,)) #target: This parameter specifies the function that should be run in the new thread., This means that when start_painter is called, it will receive window as its argument
            paint_thread.start()#the thread begins executing the start_painter function in parallel with the main program
            firstTime = 1
            while PaintActive:
                try:
                    Vx, Vy = getJoystickTelemetry()
                except Exception as e:
                    print(f"Error reading joystick telemetry: {e}")
                    continue

                if Vx == 1000 and Vy == 1000:  # this values mean there is a state chnaged
                    state = (state + 1) % 3
                elif firstTime:
                    x_init, y_init = Vx, Vy
                    firstTime = 0
                else:
                    dx, dy = Vx - x_init, Vy - y_init  # Calculate joystick movement
                    curr_x, curr_y = mouse.get_position()
                    mouse.move(curr_x - int(dx / 50), curr_y - int(dy / 50))  # Move cursor based on joystick input

            show_window(1, window)

        if event == "_Calib_":
            send_state('C')  # Calibration
            show_window(4, window)
            while "_BackMenu_" not in event:
                event, values = window.read()
                if "_Rotation_" in event:
                    send_state('A')  # Auto Rotate
                elif "_Stop_" in event:
                    send_state('M')  # Stop, was S
                    counter = read_from_MSP('calib', 4)  # read the counter from the MSP
                    window["Counter"].update(value=counter)  # update in the black box the counter from the MSP430
                    phi = 360 / int(counter.split('\x00')[0])
                    window["Phi"].update(value=str(round(phi, 4)) + "[deg]")  # update in the second black box the degree

        if event == "_Script_":
            burn_index = 0
            send_state('S')  # Script
            show_window(5, window)

        if event == '_Folder_':  # folder selection input
            selected_folder = values['_Folder_']  # holds the path to the folder that the user selected
            try:
                window["_FileHeader_"].update('')
                window["_FileContent_"].update('')  # Updates the content of this element to an empty string, effectively clearing any previous text.
                file_path = ''
                file_list = os.listdir(selected_folder)
            except Exception as e:
                file_list = []
            fnames = [f for f in file_list if
                      os.path.isfile(os.path.join(selected_folder, f)) and f.lower().endswith(".txt")]
            window["_FileList_"].update(fnames)  # write in the list box, the files names

        if event == '_FileList_':  # while we click on the file name in the list box after event == '_Folder_', and the title is the file name
            try:
                file_path = os.path.join(values["_Folder_"], values["_FileList_"][0])  # save the path to the file
                file_size = path.getsize(file_path)  # get the size of the file
                try:
                    with open(file_path, "rt", encoding='utf-8') as f:
                        file_content = f.read()  # reads the content of the file
                except Exception as e:
                    print("Error: ", e)
                file_name = values["_FileList_"][0]  # save the file name
                window["_FileHeader_"].update(f"File name: {file_name} and Size: {file_size} Bytes.")  # add to right size
                window["_FileContent_"].update(file_content)  # write the content of the file on the right size
            except Exception as e:
                print("Error: ", e)
                window["_FileContent_"].update('')

        if event == '_Burn_':
            print("the name is: " +file_name)
            send_state(f"{file_name}\n")#Sends the name of the selected file to the connected device
            execute_list.append(f"{file_name}")
            try:
                with open(file_path, "rb") as f:  # Open file in binary read mode
                    file_content_b = f.read() #Reads the entire contents of the file into the variable
                    file_content_b = translate_binary_content(file_content_b)
                    print(file_content_b)
            except Exception as e:
                print("Error: ", e)
            send_state(file_content_b + bytes('Z', 'utf-8'), file_option=True)  # Appends a special byte Z (encoded in UTF-8) to the end of the file content. This might serve as a delimiter or end-of-file marker for the device
            print(file_name)
            print(f"{file_size}")
            time.sleep(0.5)
            #  send_state('Q')
            if (burn_index == 0):
                ptr_state = 'W'
            elif (burn_index == 1):
                ptr_state = 'X'
            elif (burn_index == 2):
                ptr_state = 'Y'
            #    send_state(str(burn_index))  # send burn index - 0,1,2
            burn_index += 1
            send_state(ptr_state)
            try:
                burn_ack = read_from_MSP('script', 3).rstrip('\x00')  # Attempts to read an acknowledgment message from the device after sending the file
                # .rstrip('\x00'): Removes any trailing null bytes (\x00) from the response, cleaning up the message.
            except:
                print("error")
            if burn_ack == "FIN":  # get from the MSP after burning to flash
                window['_ACK_'].update('"'+file_name+'"'+' burned successfully!')
                window.refresh()
                print(burn_ack)
            print("burn file index: " + ptr_state)
            time.sleep(0.3)
            window['_ExecutedList_'].update(execute_list)

        if event == '_ExecutedList_':
            file_name_to_execute = values["_ExecutedList_"][0] #got the file name that we want to excute
            #file_name_to_execute: This variable now holds the name of the file that the user wants to execute.
            file_index = execute_list.index(file_name_to_execute)  # for send state - 0,1,2
            if (file_index == 0):
                exe_state = 'T'
            elif (file_index == 1):
                exe_state = 'U'
            elif (file_index == 2):
                exe_state = 'V'

        if event == '_Execute_':
            curr_phi = 0
            step_phi = 360 / 510
            show_window(6, window)
            window['_FileName_'].update('"' + file_name_to_execute + '"' + " execute")
            while "_BackScript_" not in event:
                event, values = window.read()
                if event == '_Run_':
                    time.sleep(0.5)
                    send_state(exe_state)
                    print("execute file index: " + exe_state)
                    time.sleep(0.6)
                    s.reset_input_buffer()
                    s.reset_output_buffer()
                    curr_counter = read_from_MSP('script', 3).rstrip('\x00')#This variable holds the current counter value or status received from the device.
                    print("the current counter is: " + curr_counter)
                    while curr_counter != "FIN":
                        # curr_counter = curr_counter
                        while 'F' not in curr_counter:
                            window.refresh() #Refreshes the GUI to ensure it remains responsive and up-to-date.
                            curr_phi = int(curr_counter) * step_phi # the current degree that the stepper points
                            window["Degree"].update(value=str(round(curr_phi, 4)) + "[deg]") #Updates the GUI to display the current phase angle in degrees.
                            curr_counter = read_from_MSP('script', 3).rstrip('\x00') #reads the current degree from the MSP
                            print(curr_counter)
                        curr_counter = read_from_MSP('script', 3).rstrip('\x00')
                        print(curr_counter)
                        window.refresh()
                        if 'F' not in curr_counter:
                            curr_phi = int(curr_counter) * step_phi # the current degree that the stepper points
                            window["Degree"].update(value=str(round(curr_phi, 4)) + "[deg]")
                    window.refresh()

        if event == '_Clear_':
            window['_ExecutedList_'].update(empty_list) #Updates the content of the Listbox to display whatever is passed to it.
            window['_ACK_'].update(' ')

        if event == sg.WIN_CLOSED:
            break

        if event is not None and "_BackMenu_" in event:
            show_window(1, window)
        if event is not None and "_BackScript_" in event:
            show_window(5, window)
    window.close()


if __name__ == '__main__':
    # MSP430_port = port_search()
    s = ser.Serial('COM6', baudrate=9600, bytesize=ser.EIGHTBITS, parity=ser.PARITY_NONE, stopbits=ser.STOPBITS_ONE, timeout=1)  # timeout of 1 sec so that the read and write operations are blocking,
    # after the timeout the program continues
    s.flush()  # clear the port
    enableTX = True
    s.set_buffer_size(1024, 1024)
    # clear buffers
    s.reset_input_buffer()
    s.reset_output_buffer()
    firstTime = 1
    state = 2  # Start at neutral state
    PaintActive = 0
    GUI()
