# 🕹 RetroPie Gamepad for Raspberry Pi Zero 🍓
Yet another open sourced portable gamepad design & RetroPie setup guide. Designed to free the Rpi Zero anytime. Not jailed in the case. Attaching like a GameBoy cartridge. The CAD file for 3d printing is open sourced. You will need to be familiar with Autodesk Fusion360 to adjust the interia to fit your TFT screen module, power switch, lipo battery, charging module, speaker amplifier module, etc.


# List of parts needed to build
- MCU, Any Raspberry Pi Zero series + SD Card
- Pin Header
  - Pi zero side: 90 degree 20-pin Strip Dual Male Header
  - Gamepad side: 20-pin Strip Dual Female Header
- TFT Display, ILI9341 driven E.g. HiLetgo 240X320 2.8" SPI TFT LCD
- 12 switches, for SNES layout, with D-pad, A, B, X, Y, Start, Select, TL, TR
- Lipo Battery, plus charging module, and a tiny power switch
- Speaker, about 23mm in diameter
- Wires, as thin as you can handle


# Build & Setup Guide

## Wiring
```
Device------------Raspberry Pi
TFT VIN-----------pin 17 (3.3v)
TFT GND-----------pin 20 (GND)
TFT CS------------pin 24 (GPIO 8)
TFT RST-----------pin 22 (GPIO 25)
TFT D/C-----------pin 18 (GPIO 24)
TFT MOSI----------pin 19 (GPIO 10)
TFT SCK-----------pin 23 (GPIO 11)
TFT BL------------pin 16 (GPIO 23)
TFT MISO----------pin 21 (GPIO 9)
SPEAKER VCC-------pin 12 (GPIO 18)
SPEAKER GND-------pin 14 (GND)

(Experimental, for analog inputs, supported in my mk_arcade_joystick_rpi fork)
ADS1115 VCC-------pin 4 (5v)
ADS1115 GND-------pin 6 (GND)
ADS1115 SCL-------pin 5 (GPIO 3)
ADS1115 SDA-------pin 3 (GPIO 2)
ADS1115 ADDR------pin 6 (GND)
```


## Install ILI9341 modules

```
sudo modprobe fbtft_device custom name=fb_ili9341 gpios=reset:25,dc:23,led:18 speed=81000000 bgr=1
```
LED will light on

```
sudo vi /etc/modules
```
Append below two lines:
```
spi-bcm2835
fbtft_device
```

```
sudo vi /etc/modprobe.d/fbtft.conf
```
Add a line:
```
options fbtft_device name=fb_ili9341 gpios=reset:25,dc:24,led:23 speed=81000000 bgr=1 rotate=90 custom=1
```
LED will light on by default after reboot


## Install framebuffer-copy (to tft display)

```
sudo apt-get update
sudo apt-get install cmake
git clone https://github.com/tasanakorn/rpi-fbcp
cd rpi-fbcp/
mkdir build
cd build/
cmake ..
make
sudo install fbcp /usr/local/bin/fbcp
```

Test now
```
fbcp
```

```
sudo vi /etc/rc.local
```
Add a line before 'exit 0'
```
fbcp&
```

Install WiringGPIO for dimming LED backlight
```
sudo apt-get install wiringpi
gpio -g mode 23 pwm
gpio -g pwm 23 1023
```

Set framebuffer size to 320x240 for TFT
```
sudo vi /boot/config.txt
```
Append below lines:
```
hdmi_force_hotplug=1
disable_overscan=1
dtparam=spi=on

## ** enable below lines will breake HDMI output **
hdmi_group=2
hdmi_mode=87
hdmi_cvt=320 240 60 1 0 0 0
```


## Install mkjoystick module

Install libraries
```
sudo apt-get update
sudo apt-get install -y dkms cpp-4.7 gcc-4.7 git joystick
```

Optionally, upgrade linux kernel & headers
```
{execute either, } sudo rpi-update
{or, } sudo apt-get install --reinstall raspberrypi-bootloader raspberrypi-kernel
sudo apt-get install raspberrypi-kernel-headers
```

Checkout sources
```
git clone https://github.com/badjeff/mk_arcade_joystick_rpi/
cd mk_arcade_joystick_rpi/
sudo mkdir /usr/src/mk_arcade_joystick_rpi-0.1.5/
sudo cp -a * /usr/src/mk_arcade_joystick_rpi-0.1.5/
```

Compile and install the module
```
export MKVERSION=0.1.5
sudo -E dkms build -m mk_arcade_joystick_rpi -v 0.1.5
sudo -E dkms install -m mk_arcade_joystick_rpi -v 0.1.5
{to rebuild, remove first} sudo -E dkms remove mk_arcade_joystick_rpi/0.1.5 -k $(uname -r)
```

A TFT screen is connected on your RPi B+ you can't use all the gpios. Run the following command for using only the gpios not used by the tft screen.
* GPIOs used with this map: 21,13,26,19,5,6,22,4,20,17,27,16
* Switches with this map: key up, down, left, right, start, select, a, b, tr, y, x, tl
```
sudo modprobe mk_arcade_joystick_rpi map=4
```

If needing a custom gpio map, use map=5 gpio=xx,xx,xx,
```
sudo modprobe mk_arcade_joystick_rpi map=5 gpio=pin1,pin2,pin3,.....,pin12
```

Set module load on startup
```
sudo vi /etc/modules
```
Add a line:
```
mk_arcade_joystick_rpi
```

```
sudo vi /etc/modprobe.d/mk_arcade_joystick.conf
```
Add a line:
```
options mk_arcade_joystick_rpi map=4
```

Test it
```
jstest /dev/input/js0
```

Rebuild mkjoystick
```
export MKVERSION=0.1.5
sudo modprobe -r mk_arcade_joystick_rpi
sudo -E dkms remove mk_arcade_joystick_rpi/0.1.5 -k $(uname -r)
sudo cp -rf /home/pi/mk_arcade_joystick_rpi/* /usr/src/mk_arcade_joystick_rpi-0.1.5/
sudo -E dkms build -m mk_arcade_joystick_rpi -v 0.1.5
sudo -E dkms install -m mk_arcade_joystick_rpi -v 0.1.5
sudo modprobe mk_arcade_joystick_rpi map=4
jstest /dev/input/js0
```

Troubleshooting
if Button B is grounded seldomly, check i2c 1-wire interface is disabled.
Otherwise, GPIO 4 is used as CLK reference, pull-up as half of 3.3 only.


## Setup mono audio
```
sudo vi /boot/config.txt
```
Append a line
```
dtoverlay=pwm,pin=18,func=2
```

After setup hotkey in EmulatorStation, quit ES
```
cd /opt/retropie/configs/all/retroarch-joypads
```
Modify your controller file by adding the following lines
```
input_volume_up_axis = -1
input_volume_down_axis = +1
```

# License
MIT
