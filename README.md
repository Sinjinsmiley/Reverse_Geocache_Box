# Reverse Geocache Box

[![Arduino-lint](https://flat.badgen.net/badge/Arduino-lint/passing/2da44e?labelColor=24292f)](https://github.com/ripred/Reverse_Geocache_Box/actions/workflows/arduino-lint.yml)
[![Release](https://flat.badgen.net/badge/Release/v1.0/8250df?labelColor=24292f)](https://github.com/ripred/Reverse_Geocache_Box/releases/latest)
[![License](https://flat.badgen.net/badge/License/MIT/0969da?labelColor=24292f)](https://github.com/ripred/Reverse_Geocache_Box/blob/main/LICENSE)
[![Stars](https://flat.badgen.net/badge/Stars/13/bf8700?labelColor=24292f)](https://github.com/ripred/Reverse_Geocache_Box/stargazers)
[![Forks](https://flat.badgen.net/badge/Forks/1/6f42c1?labelColor=24292f)](https://github.com/ripred/Reverse_Geocache_Box/network/members)

A gift box, locked from the inside, that will only open at a special location that can be set by the giver. 

When the button is pressed the box displays how far away they currently are. They must travel a few miles and check again to see if they are getting closer or further away from the destination! The receiver of the box only has so many tries to triangulate in on the special location.

When they finally arrive at the correct location the box unlocks to present any gifts inside, and a message is displayed!

![reverse geocache box](exampleImage.png)  
[video](https://youtu.be/QjWmHyE3k_Q)

### Arduino Project Parts

This can be built using pretty much any Atmel microcontroller including ATTiny85's if you only implement the LiquidCrystal_I2C, PWMServo, and SoftwareSerial that talks to the GPS module, and the GPS module itself. All other parts are optional.

### Required
* [Wooden craft box from a craft/hobby store](https://www.amazon.com/Unfinished-Dedoot-Rectangle-Organizer-5-5x2-75x2-87/dp/B07Q18H8CX/). Pick any size and shape, there are a lot of great choices that make good project boxes.
* 2 18650 Lipo rechargeable batteries [with holder](https://www.amazon.com/AIMPGSTL-Battery-Storage-Parallel-Batteries/dp/B0B96JSW78/)
* [Pushbutton](https://www.amazon.com/STARELO-Momentary-Button-pre-soldered-R13-507/dp/B09YTYHZQM/)
* [LM2931A 5V LDO regulator](https://media.digikey.com/pdf/Data%20Sheets/Fairchild%20PDFs/LM2931A.pdf) or other 5V regulator
* 0.1 µF ceramic capacitor
* 100 µF electrolytic capacitor
* [Arduino Uno, Nano or any other Atmel processor with at least 4 pins available](https://www.amazon.com/ELEGOO-Arduino-ATmega328P-Without-Compatible/dp/B0713XK923/). 1 pin is needed for the servo, 1 pin is needed for the serial Rx from the GPS module, and 2 pins are needed for the I2C bus. 2 more pins are used as a secret backdoor so that as a developer you don't lock yourself out of the box!
* [1602 Liquid Crystal I2C display](https://www.amazon.com/GeeekPi-Character-Backlight-Raspberry-Electrical/dp/B07S7PJYM6/)
* Small Servo. I used [a metal gear digital mg90](https://www.walmart.com/ip/MG90-UAV-RC-Model-Servo-Servo-Fixed-Wing-9G-Metal-Teeth-Mini-Digital-Servo-Servo-for-Rc-Helicopter-Airplane-Boat-Car/1781823832).
* [Serial GPS module](https://www.amazon.com/Microcontroller-Compatible-Sensitivity-Navigation-Positioning/dp/B07P8YMVNT/)

### Optional
* [Solid state power button](https://www.adafruit.com/product/1400) NOT SHOWN IN SCHEMATIC!
* resistors, capacitors, wire

![schematic](ReverseGeocacheBoxSchem1.png)

Note: the schematic still shows the original audio circuit (8 output pins, resistors, amplifier and speaker). Audio has since been removed from the code, so that part can be left out.
