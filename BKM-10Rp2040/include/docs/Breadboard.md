# Instructions for building on a breadboard
This is pretty straight forward to build on a breadboard and the only tricky to source part will be the RS485 component.

## BOM
- Raspberry Pico
- [RS485](#rs485) module or IC on breakout
- Hook up wire
- Standard breadboard
- some kind of serial cable

### Optional
#### Display – I2C OLED
- SH1106 or SSD1306

#### IR
- TSOP34xxx IR Receiver
- Remote control w/ 36+ buttons, [NEC IR format](#nec-ir)
- Tact switch (1)

#### Buttons
- Tactile switch/button (5)

## Pinout

|Pin   |Dest  |
|---|---|
|GPIO3|IR Out|
|GPIO4 (SDA)   | OLED SDA  |
|GPIO5 (SCL)   | OLED SCL  |

|Buttons|Pin|
|---|---|
| IR Learn | GPIO2 |
| Power | GPIO15 |
| Menu | GPIO16 |
| Up | GPIO17 |
| Down | GPIO14 |
| Enter | GPIO18 |

### RS485

|RS485|Pin|
|---|---|
|DI|GPIO0 (TX)|
|RO|GPIO1 (RX)|
|DE|GPIO6|
|RE - Full Duplex|GPIO7|
|RE - Half Duplex|GND|

```
GND B   -   -   VCC
1   2   3   4   5
°   °   °   °   ° 
  °   °   °   °
  6   7   8   9    
  -   A   -   -
```
_Looking at front of Male DB9_

### Power
There two ways to power this thing, from USB through the Pico, or directly from the BVM. For initial testing, it is recommended to take power from the Pico, then once it all checks out, switch to take power from the serial cable.

|RS485|Pico|
|---|---|
|VCC|VBUS or VSYS*|
|GND|Any ground|

*If your RS485 solution can operate off 3.3V, then VSYS is the preferred option.

|RS485|DB9|
|---|---|
|VCC|Pin 5|
|GND|Pin 1|


Can also be handy to add a button between RUN and GND for easy reset.

## Firmware
The repo contains two pre-built binaries for the Pico, choose the one that matches you OLED driver, if using a display. If you aren't using the display, then just pick either, it won't matter. 

## Notes
### RS485<a name="rs485"></a>
There's a few options for RS485: a breakout like the [MAX485](https://core-electronics.com.au/ttl-uart-to-rs485-converter-module.html) or using any bare IC like the [Analog Devices ADM3065E](https://www.analog.com/en/products/adm3065e.html).

If using a breakout board, there don't seem to be any available in full duplex, so it won't be able to receive status updates from the BVM. You can of course just use two; one each for TX and RX.

A bare IC is of course going to require advanced soldering skills and either a SOIC breakout board. With even more advanced skills and it could also be dead bugged.

### NEC IR<a name="nec-ir"></a>
Most non-Sony IR remote controls will use the NEC command format, but there are a few other formats out there too. If in doubt, try a couple in learning mode.