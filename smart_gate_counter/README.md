# Smart Gate Counter with I2C LCD Display

This project implements a car counting system on the Raspberry Pi Pico using a PIR motion sensor, two LEDs, a push button, and a 16x2 I2C LCD display.

## Features

- Counts cars passing by detecting PIR sensor triggers
- Displays total number of cars on the LCD
- Red LED turns ON for 5 seconds when a car passes, preventing multiple counts from the same vehicle
- Green LED indicates the system is ready for the next car
- Reset button sets car counter back to 0 and updates the display immediately
- Uses GPIO interrupts for responsive detection and one shared ISR for both PIR and reset button
- One-shot hardware alarms (`add_alarm_in_ms`) handle LED timing
- I2C communication drives the LCD in 4-bit mode

## Hardware Setup

- **Raspberry Pi Pico** microcontroller
- **PIR motion sensor**
- **16x2 character LCD display** with I2C interface (address 0x27)
- **Green LED** on GPIO 5
- **Red LED** on GPIO 6
- **Push button** for reset on GPIO 4
- I2C lines on GPIO 0 (SDA) and GPIO 1 (SCL)
- Resistors for LED current limiting
- Internal pull-up/pull-down resistors enabled on buttons and sensor pins

## Software Details

- Written in C using the Raspberry Pi Pico SDK
- LCD initialized and controlled in 4-bit mode using custom commands
- GPIO IRQ handler detects PIR rising edges and reset button falling edges
- Car counter increment and LED timing handled via interrupts and one-shot alarms
- LCD is updated in the main loop only when a change occurs (`screen_needs_update` flag)
- Only one alarm can be active at a time to ensure deterministic LED timing

## Notes

- Red LED indicates a car is currently being counted (locked for 5 seconds)
- Green LED indicates the system is ready to detect the next car
- Timing and LED behavior depend on hardware connections and CPU load
- The project can be extended with features like direction detection, multiple sensors, or persistent storage for the car count