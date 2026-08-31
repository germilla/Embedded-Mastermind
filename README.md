# ECE353-S26-HW05: Event-Driven Embedded System with FreeRTOS

This project implements an event-driven embedded system using FreeRTOS on an ARM Cortex-M microcontroller. The system is designed to handle real-time user interactions, LED control, and LCD rendering tasks, demonstrating advanced embedded systems concepts such as inter-task communication, event handling, and peripheral control.

## Features

- **Event-Driven Architecture**:  
  The firmware is structured around FreeRTOS tasks and event-driven programming principles. It utilizes FreeRTOS queues for inter-task communication and synchronization between user inputs and peripheral actions.

- **LED Control with PWM**:  
  Implements precise LED brightness control using Pulse Width Modulation (PWM). The red LED brightness is dynamically adjusted based on user input, while green and blue LEDs are toggled on/off in response to specific events.

- **LCD Tile Rendering**:  
  The system supports dynamic LCD tile rendering with customizable properties such as foreground/background colors, inverted drawing modes, and row/column positioning. FreeRTOS queues are used to send rendering requests to the LCD task, ensuring efficient and responsive updates.

- **Button Event Handling**:  
  Implements robust button press detection and event handling. Button events trigger specific actions, such as updating the LCD display or providing error messages to guide user interaction.

- **Error Messaging and User Feedback**:  
  Provides real-time feedback to the user via the LCD display, including error messages when invalid actions are attempted (e.g., pressing buttons out of sequence).

## Technical Highlights

- **FreeRTOS Integration**:  
  - Tasks are used to manage concurrent operations, such as monitoring button events, controlling LEDs, and updating the LCD display.  
  - Queues (`xQueueSend`) are leveraged for inter-task communication, ensuring data consistency and synchronization between tasks.  

- **Peripheral Control**:  
  - **PWM for LED Brightness**: The red LED's brightness is controlled via the `cyhal_pwm_set_duty_cycle` function, allowing for smooth transitions and precise brightness adjustments.  
  - **LCD Rendering**: The LCD is updated using commands sent through a queue, with support for inverted tiles and dynamic color themes.  

- **Real-Time Responsiveness**:  
  The system is designed to handle user inputs and peripheral updates in real time, ensuring a seamless and interactive user experience.

## Requirements

- **Hardware**:  
  - ARM Cortex-M-based microcontroller (e.g., PSoC™ Control MCU).  
  - LCD module for display output.  
  - LEDs (red, green, blue) for visual feedback.  
  - Buttons for user input.  

- **Software**:  
  - [ModusToolbox™](https://www.infineon.com/modustoolbox) v3.3 or later.  
  - FreeRTOS for real-time task management.  

- **Associated Parts**:  
  - All PSoC™ Control C3 MCU parts.  

## How It Works

1. **Event Monitoring**:  
   The system continuously monitors button events and updates the state of LEDs and the LCD display based on the detected events.  

2. **LED Brightness Control**:  
   When a specific button is pressed, the red LED's brightness is increased by adjusting its PWM duty cycle.  

3. **LCD Updates**:  
   LCD rendering requests are sent to a dedicated task via a FreeRTOS queue. The task processes these requests to update the display with new tiles, colors, and inverted modes.  

4. **Error Handling**:  
   If the user attempts an invalid action (e.g., pressing a button before entering required data), an error message is displayed on the LCD.  

## Learning Outcomes

This project demonstrates the following embedded systems concepts:  
- Real-time task management with FreeRTOS.  
- Inter-task communication using queues.  
- Peripheral control (PWM, LCD rendering).  
- Event-driven programming for user interaction.  
- Debugging and error handling in embedded systems.  

## Conclusion

ECE353-S26-HW05 showcases the integration of FreeRTOS with peripheral control and event-driven programming to create a responsive and interactive embedded system. This project highlights key skills in real-time systems, low-level hardware control, and user interface design, making it an excellent demonstration of embedded systems expertise.
