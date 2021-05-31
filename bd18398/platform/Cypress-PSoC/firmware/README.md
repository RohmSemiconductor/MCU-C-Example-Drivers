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
   at `bd18398-demo.cydsn/CortexM3/ARM_GCC_xxx/Release/`. Replace
   `Release/` with `Debug/` for the debug build.)
3. Use `File->Program` to program the device.

[PSoC Programmer]: http://www.cypress.com/products/psoc-programming-solutions


## USB drivers

On Windows 10, the operating system should automatically use the correct
driver. Earlier versions of Windows cannot automatically find the CDC driver
and will need to install `inf`. One option is to use the unsigned `inf` file
 that is generated as part of the build process
(`bd18398-demo.cydsn/Generated_Source/PSoC5/USBFS_cdc.inf`).


## Pin and bus1 configuration

UART:

- UART Serial logs from Cypress are printed over the USB-A connector. You can
  capture the logs using suitable serial console program like PUTTY. After
  connecting the USB cable to Windows PC you should find "KitProg" from the
  device-manager under the COM ports. Please use the assigned COM-port in
  PUTTY configuration. Set baud-rate to '115200', data-bits tpo '8',
  stop-bits to '1', parity to 'None' and Flow-control to 'XON/XOFF'.

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
