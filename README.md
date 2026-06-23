# Line-Follower-Robot

This is the code for an autonomous line-follower robot that I built during the microcontroller and soldering practical course in the robotics bachelor program at THWS. The bot uses infrared sensors to autonomously follow a black line on the ground.

## Hardware & Components

The robot is based on a custom PCB provided by the university, but the concept can easily be recreated on a breadboard. The main components under the hood are:

* **Brain:** Arduino Nano.
* **Motor driver:** L293D H-bridge to control the two DC motors.
* **Eyes:** Two CNY70 optical infrared sensors mounted on the bottom.
* **Power:** Classic 9V battery holder.
* **Other:** A few NPN transistors (BC337), trim potentiometers for adjustment, and push buttons.

## Features of the Code

I programmed this in C/C++ (AVR):

* **Two driving modes integrated:** The robot supports a classic "bang-bang" control for standard operation. Alternatively, there is a P-control (proportional controller) for much smoother and more stable tracking.
* **Dynamic mode switching:** Switching between the driving modes is done via hardware. If there is no wire jumper between pin PD4 and GND, the bang-bang mode is active. If the jumper is inserted, the P-control is activated automatically.
* **EEPROM sensor calibration:** The threshold for the color "white" is measured at the push of a button and permanently stored in the EEPROM of the Arduino. Upon the next restart, the code automatically loads this value.
* **Speed control:** The base speed of the robot can be adjusted continuously via the onboard potentiometer (POT1).
* **Hardware protection:** A weak battery and hard direction changes (e.g., from forward right directly to backward left) can lead to voltage drops and erratic behavior. Therefore, I implemented a short 50 ms delay in the motor control to prevent overloads during abrupt turning maneuvers.

## Usage & Setup

If you want to try out the code, follow these steps:

1. **Flashing:** Upload the `.ino` file to your Nano via the Arduino IDE.
2. **Calibration:** Place the robot on a clean, white surface. Press the button `SW2`. The robot will now read the reflection values of both sensors, calculate an average, add a small buffer for "black", and store the threshold in the EEPROM. To confirm, the motors will stop briefly for one second.
3. **Select driving mode:** Decide whether you want to bridge pin `PD4` to `GND` (P-control) or leave it open (bang-bang control).
4. **Start:** Place the bot on the line and, if necessary, adjust the driving speed using the potentiometer on the board.

## Repository Structure

* `LineFollower.ino` - The complete C/C++ program with initializations for ADC, PWM (Timer 1), and the main loop for the control engineering logic.
