# cypress-protocol-port

Simple example code running on the
[Cypress CY8CKIT-059](http://www.cypress.com/CY8CKIT-059) connected to the
ROHM BD18398 EVK.

Diagnostic messages are sent via the KitProg's UART and can be read by
connecting to it with a terminal application (e.g. putty). The baud rate of the
KitProg UART is 115200.

Prebuilt hex-files for releases can be found under the
[../../../bin](../../../bin) directory.


## Building

Building the project can be done using PSOC Creator version 4.4 by

### General setup (do this first)

1. Install [PSoC Creator v4.4][PSoC Creator].
2. Open the project file (`*.cyprj`) in PSoC Creator.
3. Right-click the project in the Workspace Explorer and select Update
   Components.
4. Install any missing components.

[PSoC Creator]: https://www.cypress.com/products/psoc-creator-integrated-design-environment-ide


### IDE build

1. Open the project file (`*.cyprj`) in PSoC Creator.
2. Choose either the Release or Debug build.
3. Build it (`Shift+F6`).


### CLI build

1. Open a shell (e.g. `cmd.exe` or PowerShell) and change directory to
   `cypress-protocol-port.cydsn`.
2. Run `cyprjmgr.exe -build -c Release -prj cypress-protocol-port -w cypress-protocol-port-000.cywrk`.
   (If the program is not in `PATH`, it should be available at
   `C:/Program Files (x86)/Cypress/PSoC Creator/x.x/PSoC Creator/bin/`,
   where the `x.x` is PSoC Creator's version number).


## Programming (aka flashing)

Programming can be done with either the PSoC Creator IDE or the stand-alone
[PSoC Programmer][PSoC Programmer].

The first and most important step, regardless of the tool being used, is to
connect the KitProg USB to the PC.

*If programming with the IDE*, open the project file and press `Ctrl+F5`
(or select `Debug->Program` in the menu).

*If programming with the PSoC Programmer*, do the following steps:

1. Launch the PSoC Programmer. (If the KitProg isn't connected automatically,
   choose the device from the port selection box.)
2. Use `File->File Load` to open the hex file. (PSoC Creator puts the hex files
   at `cypress-protocol-port.cydsn/CortexM3/ARM_GCC_xxx/Release/`. Replace
   `Release/` with `Debug/` for the debug build.)
3. Use `File->Program` to program the device.

[PSoC Programmer]: http://www.cypress.com/products/psoc-programming-solutions


## USB drivers

On Windows 10, the operating system should automatically use the correct
driver. Earlier versions of Windows cannot automatically find the CDC driver
and will need to install `inf`. One option is to use the unsigned `inf` file
 that is generated as part of the build process
(`cypress-protocol-port.cydsn/Generated_Source/PSoC5/USBFS_cdc.inf`).


## Pin and bus1 configuration

SPI:

- SPI mode is set to 0 (CPOL=0, CPHA=0) and is not configurable.
- SPI SCLK frequency defaults to 9 MHz and can be configured from 1 kHz to 18 MHz.
	
	**Note!** SPI clock divider is calculated by the firmware using the 72 MHz master clock and the frequency requested by client. Divider is rounded up nearest integer avoid excessive SPI clock speeds. SPI SCLK's frequency is half of the component's. 
 		
	Calculating SPI clock frequency from the main clock (72 MHz): 
	
	f\_actual = f\_master\_clk / ceil(f\_master_clk / (f\_desired * 2)) / 2
	
	Examples using Python:

		import math

		72000000.0 / math.ceil((72000000.0 / (1000000*2))) / 2 = 1 Mhz

		72000000.0 / math.ceil((72000000.0 / (6000000*2))) / 2 = 6 MHz

		72000000.0 / math.ceil((72000000.0 / (7000000*2))) / 2 = 6 MHz        

UART:

- Following UART baud rates are inside of 5% tolerance: 
	- 2400,9600,14400,19200,28800,38400,57600,76800,115200,230400,250000,460800, 921600
	- RX timeout is set in firmware as following: expected receive time + 2 seconds margin. 
	  
	**Note!** UART baud rate clock divider is calculated by the firmware using the 72 MHz master clock and the frequency requested by client. Divider is rounded up nearest integer avoid excessive UART clock speeds. By the default 8 x oversampling is used so the baud-rate is one-eighth of the input clock frequency.
	
	Calculating actual UART clock frequency from the main clock (72 MHz):
	
	f\_actual = f\_master\_clk / ceil(f\_master_clk / (f\_desired * 8)) / 8
	
	Examples using Python:

		import math

		72000000.0 / math.ceil((72000000.0 / (921600*8))) / 8 = 9000000 Hz  
	
 	If arbitrary baud rate is set use the following formula for checking the tolerance:

	%err = (f_des-f_actual)/f_des*100%


[devkit-pin-table]: ../doc/devkit-pin-table.xlsx


## PWM

Three independent PWM channels are available on pins P2.3, P2.4, and P2.5.
The supported frequency range is from 16 Hz to 65535 Hz with a resolution
of 1 us. Frequency and duty cycle are rounded to the nearest value (half-up).
Note that while the time resolution is always 1 us, the duty cycle resolution
in % and frequency resolution in Hz depend on the PWM frequency and get worse
with higher frequencies. See the equations below for how to calculate the
values after adjusting for resolution.

Frequency after adjusting for time resolution:

   f = 1 MHz / round(1 MHz / f_desired)

Duty cycle after adjusting for time resolution:

   D = round(round(1 MHz / f_desired) * D_desired)

If the polarity is changed while a PWM channel is enabled, the output waveform
may become inverted for one cycle. To avoid this, disable the PWM channel
before changing polarity.


## Known issues

Some AVs (e.g. F-Secure) flag the official Cypress tools (e.g. PSoC Programmer)
as malware.
