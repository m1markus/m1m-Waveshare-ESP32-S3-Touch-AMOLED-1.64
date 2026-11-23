

1. Exact Waveshare V1.1 wiring
I checked the V1.1 schematic, rather than relying on pinouts for the similarly named V1 board. Its two expansion headers expose GPIOs including GPIO5, GPIO6 and GPIO7; these are suitable for the three-wire I²S link and are not assigned to the display, touch controller or onboard IMU in that schematic. 
files.waveshare.com+1


Use these connections:
Waveshare ESP32-S3 V1.1	    Signal	            ESP32-WROOM-32 DevKit V1
GPIO5	                    I²S BCLK	        GPIO26 (I²S BCLK input)
GPIO6	                    I²S WS / LRCLK	    GPIO25 (I²S WS input)
GPIO7	                    I²S DATA out	    GPIO22 (I²S data input)
GND	                        Common ground	    GND

Important: GPIO5, GPIO6 and GPIO7 are the ESP32-S3 sender pins. GPIO26, GPIO25 and GPIO22 are the classic ESP32 receiver pins in the sketches below. Connect the boards' grounds together. Do not connect either board's 5 V pin to the other's 3.3 V pin; power each board from its own USB connection while testing.


hardware:

MAX98357A i2s verstärker

WS2812B LED Strip (black)
WS2812B - 144LED/m LED NeoPixel Strip
MeanWell Power Supply Switch Mode 5V 110W

WaveShare 4 IPS Touchscreen: 720x720


WaveShare 7,9 IPS Touch-Display
WaveShare 11.9 kapazitives Touch Display für Raspberry Pi, 320×1480


Pimoroni Presto
ESP32 S31 Touch LCD 4.3C box
ESP32-P4-WIFI6 3.5inch Smart Vision Development Board, 320 × 480



https://www.berrybase.ch/

