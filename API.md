GSOSD API
=========
There are two command groups; basic commands for operating the OSD and config commands for changing various settings. Command arguments are separated with whitespace. To end a command you simply type any whitespace character after its last argument. This can be a space, form-feed ('\f'), newline ('\n'), carriage return ('\r'), horizontal tab ('\t') or vertical tab ('\v').
The API arguments are human-readable; all data is in ascii, so for example the number 10 is presented as digits '1' and '0'. This makes it possible to operate the OSD manually using a 'monitor' or'screen' serial connection. It also makes operation machine-independent.
Many existing machine-to-machine protocols communicate a length first before sending out any data, see e.g. the http protocol that uses the 'Content-Length' header. This would be nice to use, but in general it requires data to be buffered internally before sending it out. That will not work on our hardware because it is low on memory. Therefore we've chosen to use the following control characters to control the flow:

| character	| purpose
|---------------|--------------------------------
| 0x01 SOH	| start of header
| 0x06 ACK	| acknowledge; command completed successful
| 0x15 NAK	| negative acknowledge; command failure
| 0x13 DC3 	| (XOFF) pause transmission to prevent buffer overflow
| 0x11 DC1 	| (XON) resume transmission
| 0x18 CAN 	| cancel; buffer overflow
| 0x02 STX	| start of text / data
| 0x03 ETX	| end of text / data
| 0x04 EOT	| end of transmission

All these characters are output only. A response always starts with an SOH and it stops with an EOT. After a command has been parsed and executed correctly, there will be an ACK, otherwise a NAK. At any time there can be sent a DC3 that indicates an almost full input buffer. The receiving end should then wait for a DC1 first before resuming transmission. If the input buffer still overflows, a CAN character is sent that suggest to ignore previous data which likely is wrong due to the overflow situation. Perhaps it is even better to redo the command because overflow can result in a truncated command or parameter that is recognised as something different (but still 'correct'). The data if any, will be sent between STX and ETX characters. This makes it easier to skip other output like echoed characters, debug messages etcetera.
The input buffer is currently 64 bytes long, so a buffer overflow is never far away. However, a recipe for success is to send large commands/data in small chunks and check for DC3 often. Also you should wait for EOT before sending a new command. Without doing so, it may or may not work.


Basic commands
--------------
An overview of basic commands is shown below. 'argc' is the number of arguments that should be given, separated by whitespace. A few commands use string data to display, which is indicated by 'data'. Those commands are susceptible to timeouts (configurable). You first specify an integer length of data bytes to come, followed by these bytes. For example to print string "Hello world!" in the middle of the screen (NTSC), you can use "P_RAW 9 6 12 Hello world!". If the length is not known beforehand, you can specify -1. In that case, processing the data will end when the operation times out (configurable) or upon receiving a NUL-byte ('\0'). For positive lengths, processing also stops at the first NUL-byte.

| command	| argc	| data	| description
|---------------|-------|-------|-------------------------
| ABOUT	  	| 0 	| N	| show info about GSOSD to serial
| LIST		| 0 	| N	| list all commands to serial
| RESET	  	| 0 	| N	| reset OSD
| CLEARPART	| 4 	| N	| clear part of screen
| CLEAR	  	| 0 	| N	| clear entire screen
| DUMP		| 0 	| N	| show config settings to serial
| LOAD		| 0 	| N	| load config settings from eeprom
| DEFAULTS	| 0 	| N	| load default config settings
| SAVE		| 0 	| N	| save config settings to eeprom
| HOS		| 1 	| N	| adjust horizontal image offset
| VOS		| 1 	| N	| adjust vertical image offset
| INSMUX1	| 1 	| N	| adjust pixel sharpness
| INSMUX2	| 1 	| N	| adjust pixel sharpness
| CBL		| 2 	| N	| adjust character black level
| CWL		| 2 	| N	| adjust character white level
| P_RAW	  	| 4 	| Y	| print raw data to screen
| P_WINDOW	| 6  	| Y	| print window with data
| P_BANNER	| 6  	| Y	| print banner with data
| SET_FONT	| 3 	| Y	| upload font character to internal storage
| GET_FONT	| 1 	| N	| show font character to serial
| GET_SENSOR	| 1 	| N	| get sensor voltage
| GET_WIDTH	| 0 	| N	| get screen width
| GET_HEIGHT	| 0 	| N	| get screen height


Config commands
---------------
These commands are used to set/get configuration values to/from the eeprom. Unless you use the SAVE basic command, the configured values are volatile and are lost when powering down the OSD. However, they will keep their default or previously saved values from an earlier session. For example, to suppress command echoing, you can set it off with "SET_ECHO 0".

| command		| storage_type	| default	| description
|-----------------------|---------------|---------------|-----------------------------------
| SET/GET_VERSION 	| str16_t	| ...		| set/get GSOSD version
| SET/GET_BAUDRATE	| uint32_t	| 9600		| set/get serial baud rate
| SET/GET_TIMEOUT 	| uint32_t      | 10000		| set/get data timeout in (ms)
| SET/GET_VDETECT 	| uint32_t      | 1000		| set/get video detect time (ms)
| SET/GET_REFRESH 	| uint32_t      | 100		| set/get screen refresh time (ms)
| SET/GET_LOGO    	| uint32_t      | 2000		| set/get logo display time (ms)
  SET/GET_ENABLE	| bool		| 1 		| set/get OSD image visibility
| SET/GET_DEBUG   	| bool   	| 0		| set/get debugging
| SET/GET_ECHO    	| bool   	| 1		| set/get echoing serial input to output
| SET/GET_SILENT  	| bool   	| 0		| set/get silence
| SET/GET_CONTROL    	| uint8_t   	| 3		| set/get transmission flow data
| SET/GET_SENSADJ0	| uint16_t      | 20000		| set/get sensor 0 calibration value
| SET/GET_SENSADJ1	| uint16_t      | 20000		| set/get sensor 1 calibration value
| SET/GET_SENSADJ2	| uint16_t      | 20000		| set/get sensor 2 calibration value
| SET/GET_SENSADJ3	| uint16_t      | 20000		| set/get sensor 3 calibration value


Commands reference
------------------

**ABOUT**  
Purpose	: 	Show information about GSOSD.  
Arguments :	none  

Use this command before you start pulling your hair out.

----

**LIST**  
Purpose	: 	List all commands.  
Arguments :	none  

This is the only command to remember. I might rename it to HELP some day.

----

**RESET**  
Purpose	: 	Reset the OSD.  
Arguments :	none  

This does a soft reset as described in the Max7456 manual. In short, it sets all registers back to their defaults except the OSD black level register (OSDBL) and the enabled flag of video mode 0 register (VM0). Then it refreshes the screen (which only has visible effect when the enabled flag is true. See also command SET_ENABLE.

----

**CLEARPART**  
Purpose	: 	Clear part of the screen.  
Arguments :  
*x-coordinate* (integer)  
*y-coordinate* (integer)  
*width* (integer)  
*height* (integer)  

This clears (a portion of) the screen. *x-coorinate* is between 0 and 29 for both PAL and NTSC. *y-coordinate* is between 0 and 15 for PAL and between 0 and 12 for NTSC. For *x-coorindate* smaller than 0, the value taken will be 0. Likewise for *y-coorindate*. For *width* smaller than 0, max-width will be taken. Likewise for *height*.

Examples :  
`CLEARPART 9 5 12 2`	// Clear the LOGO at startup  
`CLEARPART 0 0 -1 -1`	// Clear the entire screen  
`CLEARPART -1 -1 -1 -1`	// Clear the entire screen  

----

**CLEAR**  
Purpose	: 	Clear screen.  
Arguments :	none  

This is just a special case of CLEARPART that clears the entire screen.

----

**DUMP**  
Purpose	: 	Dump all configuration.  
Arguments :	none  

Configuration is dumped to serial output like it would be stored in eeprom. There are two parts of configuration; register settings and config variable settings.

----

**LOAD**  
Purpose	: 	Load configuration from eeprom.  
Arguments :	none  

At startup, all configuration will be loaded from eeprom. You can redo this with LOAD.

----

**DEFAULTS**  
Purpose	: 	Load default configuration.  
Arguments :	none  

Loads hard-coded default configuration values. Configuration consists of register settings and variable settings. For registers, the defaults are as per the Max7456 manual, except for the "enable" flag of register VM0 (our default: 1). For variables, the defaults are listed above. All these values are persistent if you SAVE before powering down the OSD.

----

**SAVE**  
Purpose	: 	Save configuration to eeprom.  
Arguments :	none  

This makes configuration & register changes permanent by storing them into eeprom. The eeprom has an expected lifespan of 100.000 write cycles. Only changed values are physically written to the eeprom to extend the eeprom's lifespan. To prevent damage by mistakes or abuse, there is an increasing delay with each write. In theory it would be possible to wear out the eeprom within minutes, but the delay prevents that. The delay starts at zero and rises very slowly, so normally you won't notice it.

----

**HOS**  
Purpose	: 	Adjust horizontal offset.  
Arguments :	*amount* (integer)  

This shifts the OSD image horizontally by *amount* number of pixels. The OSD image can be shifted 32 pixels to the left and 31 pixels to the right. Absolute values:  
0  : -32 pixels  
...  
32 : 0 pixels  
...  
63 : 31 pixels  
The new value is stored in the horizontal offset register (HOS) which means it will be persistent if you SAVE before powering down the OSD.

Examples :  
`HOS 0`		// show current value  
`HOS 1`		// increment by 1 (max)  
`HOS 10`	// increment by 10 (max)  
`HOS -1`	// decrement by 1 (max)  

----

**VOS**  
Purpose	: 	Adjust vertical offset.  
Arguments :	*amount* (integer)  

This shifts the OSD image vertically by *amount* number of pixels. The OSD image can be shifted 16 pixels to the top and 15 pixels to the bottom. Absolute values:  
0  : -16 pixels  
...  
16 : 0 pixels  
...  
31 : 15 pixels  
The new value is stored in the vertical offset register (VOS) which means it will be persistent if you SAVE before powering down the OSD.

Examples :  
`VOS 0`		// show current value  
`VOS 1`		// increment by 1 (max)  
`VOS 10`	// increment by 10 (max)  
`VOS -1`	// decrement by 1 (max)  

----

**INSMUX1**  
Purpose	: 	Adjust pixel sharpness "insmux1".  
Arguments :	*amount* (integer)  

This controls the OSD Rise and Fall Time; typical transition times between adjacent OSD pixels. The current value is adjusted by *amount*. A lower value results in sharper pixels, but more cross-colour/cross-luma artefacts. A higher value does the opposite. Absolute values:  
0 : 20ns  
1 : 30ns  
2 : 35ns  
3 : 60ns  
4 : 80ns  
5 : 110ns  
The new value is stored in the OSD insertion mux register (OSDM) which means it will be persistent if you SAVE before powering down the OSD.

Examples :  
`INSMUX1 0`	// show current value  
`INSMUX1 1`	// increment by 1 (max)  
`INSMUX1 -1`	// decrement by 1 (max)  
`INSMUX1 -5`	// set maximum sharpness & artifacts  
`INSMUX1 5`	// set minimum sharpness & artifacts  

----

**INSMUX2**  
Purpose	: 	Adjust pixel sharpness "insmux2".  
Arguments :	*amount* (integer)  

This controls the OSD Insertion Mux Switching Time; typical transition times between input video and OSD pixels. The current value is adjusted by *amount*. A lower value results in sharper pixels, but more cross-colour/cross-luma artefacts. A higher value does the opposite. Absolute values:  
0 : 30ns  
1 : 35ns  
2 : 50ns  
3 : 75ns  
4 : 100ns  
5 : 120ns  
The new value is stored in the OSD insertion mux register (OSDM) which means it will be persistent if you SAVE before powering down the OSD.

Examples :  
`INSMUX2 0`	// show current value  
`INSMUX2 1`	// increment by 1 (max)  
`INSMUX2 -1`	// decrement by 1 (max)  
`INSMUX2 -5`	// set maximum sharpness & artifacts  
`INSMUX2 5`	// set minimum sharpness & artifacts  

----

**CBL**  
Purpose	: 	Adjust character black level.  
Arguments :  
*line* (integer)  
*amount* (integer)  

This setting controls the black pixel brightness. *line* is the line number to adjust, starting at 0. Any negative value means all lines. *amount* is the amount to adjust. Absolute values:  
0 : 0%  
1 : 10%  
2 : 20%  
3 : 30%  
The new values are stored in the row N brightness registers (RB0 - RB15) which means they will be persistent if you SAVE before powering down the OSD.

Examples :  
`CBL 3 0`	// show current value for line 3  
`CBL -1 0`	// show highest value of all lines  
`CBL -1 1`	// increment all lines by 1 (max)  
`CBL -1 -1`	// decrement all lines by 1 (max)  
`CBL -1 3`	// set maximum black level brightness for all lines  
`CBL -1 -3`	// set minimum black level brightness for all lines  

----

**CWL**  
Purpose	: 	Adjust character white level.  
Arguments :  
*line* (integer)  
*amount* (integer)  

This setting controls the white pixel brightness. *line* is the line number to adjust, starting at 0. Any negative value means all lines. *amount* is the amount to adjust. Absolute values:  
0 : 120%  
1 : 100%  
2 : 90%  
3 : 80%  
The new values are stored in the row N brightness registers (RB0 - RB15) which means they will be persistent if you SAVE before powering down the OSD.

Examples :  
`CWL 3 0`	// show current value for line 3  
`CWL -1 0`	// show highest value of all lines  
`CWL -1 1`	// increment all lines by 1 (max)  
`CWL -1 -1`	// decrement all lines by 1 (max)  
`CWL -1 3`	// set minimum white level brightness for all lines  
`CWL -1 -3`	// set maximum white level brightness for all lines  

----

**P_RAW**  
Purpose	: 	Print raw data to screen.  
Arguments :  
*x-coordinate* (integer)  
*y-coordinate* (integer)  
*length* (integer)  
*data* (string)  

This command prints raw data to screen at the position given by *x-coordinate* and *y-coordinate*. As soon as *length* number of *data* characters have been received or a NUL-byte ('\0') has been sent, processing ends. NUL-bytes are internally used to indicate empty screen positions, so this value cannot be used as displayable data anyway. A negative *length* means infinite for which case processing will stop upon a NUL-byte or by data timeout. See also the TIMEOUT command.

Examples :  
`P_RAW 9 5 12 Hello World!`	// print 12 characters in the middle of the screen  
`P_RAW 9 5 -1 Hello World!`	// print 12 characters and keep waiting for more  

----

**P_WINDOW**  
Purpose	: 	Print data to scrolling viewport.  
Arguments :  
*x-coordinate* (integer)  
*y-coordinate* (integer)  
*width* (integer)  
*height* (integer)  
*length* (integer)  
*data* (string)  

This command prints interpreted data to given view-port on the screen. The view-port is given by *x-coordinate*, *y-coordinate*, *width* and *height*. A negative *width* will cause borders to be drawn at left and right. Likewise for *height* at top and bottom. As soon as the window is full, it will scroll to the top, line by line. A carriage-return character (\x0D) will cause the remainder of current line to be filled with spaces. An implementation detail due to carriage-return handling is that the function starts writing data at the first empty position in the window. So if there is text already present, it will handled like it was just printed with this function. Like with P_RAW, *length* may be negative.

Examples :  
`P_WINDOW 9 5 12 2 12 Hello\x0DWorld!`		// print two lines in a 12x2 window  
`P_WINDOW 9 5 -12 -2 12 Hello\x0DWorld!`	// print two lines in a 12x2 window with borders  

----

**P_BANNER**  
Purpose	: 	Print data as scrolling banner.  
Arguments :  
*x-coordinate* (integer)  
*y-coordinate* (integer)  
*width* (integer)  
*height* (integer)  
*length* (integer)  
*data* (string)  

This command prints data to screen as a scrolling banner. It works much like P_WINDOW, so it prints data to given view-port, shows borders on negative *width* and *height* and accepts negative *length*. Only the scrolling works different; it scrolls from right to left and from bottom to top, character by character.

Example :	`P_BANNER 9 5 9 1 27 .........Catch me!.........`  

----

**SET_FONT**  
Purpose	: 	Set font character.  
Arguments :  
*character-number* (integer)  
*length* (integer)  
*data* (string)  

This command uploads a font character to internal storage. The example uploads a capital character 'A'. The data is binary and should be grouped per byte, MSB first. Whitespace is only for readability and doesn't have functional impact. Character 0 can be uploaded but remains unused because NUL-bytes ('\0') have a special meaning in the software (they indicate empty screen positions). This binary format can cause buffer overflows easily when uploading multiple font characters at once. Possible solutions (use multiple at once):  
- Set debugging off ("SET_DEBUG 0")  
- Set echoing off ("SET_ECHO 0")  
- Use a low baudrate, e.g. 9600 or 4800 (SET_BAUDRATE 4800", restart).  
- Control the upload speed with a script, possibly using control characters ("SET_CONTROL 1").  
- In the near future we may introduce a hex format which uses four times less data. For very long runs this doesn't make a difference but short runs may complete before the buffer overflows.

Example :  
`SET_FONT 65 485 `  
`01010101 01010101 01010101 01010101 01010101 01010101 01010101 01010101 01010101`  
`01010101 01000000 01010101 01010101 00101010 00010101 01010101 00101010 00010101`  
`01010101 00100010 10000101 01010100 10100010 10000101 01010100 10100010 10000101`  
`01010100 10100000 10100001 01010010 10101010 10100001 01010010 10101010 10100001`  
`01010010 10000000 10100001 01001010 10000100 10101000 01001010 10000100 10101000`  
`01010000 00010101 00000001 01010101 01010101 01010101 01010101 01010101 01010101`  

----

**GET_FONT**  
Purpose	: 	Get font character.  
Arguments :	*character-number* (integer)  

This does the opposite of SET_FONT and downloads a font character from internal storage.

Example :	`GET_FONT 65`  

----

**GET_SENSOR**  
Purpose	: 	Get voltage sensor value.  
Arguments :	*sensor-id* (integer)  

The Micro MinimOSD board has 4 voltage sensors that are labelled "VBAT1", "VBAT2", "RSSI" and "CURR". Each of these sensors return a voltage that first should be calibrated, see "Calibrating sensors". While doing that and once done that, use GET_SENSOR to get the measured voltage. Sensor 0 and 1 can handle higher voltages but are less sensitive. For sensors 2 and 3 the opposite is true.
The Micro board with KV-mod uses a 1k/14k6 resistor divider at pins VBAT1/VBAT2 so that a full scale measurement will be a bit more than 17V based on a 1.1V reference voltage. So this suits up to 4s lipos perfectly. The ATmega328 chip can use several reference voltages. But the KV-mod is designed with 1.1V reference voltage in mind. So we hard-coded that in the software. The RSSI/CURR pins have no resistor divider, only a resistor in series, so their max. voltage will be 1.1V. Unfortunately we can only choose one analogue reference voltage. If you need to measure higher voltages than 1.1V, put your own resistor divider in front.

Examples :  
`GET_SENSOR 0`	// shows "VBAT1" voltage  
`GET_SENSOR 1`	// shows "VBAT2" voltage  
`GET_SENSOR 2`	// shows "RSSI" voltage  
`GET_SENSOR 3`	// shows "CURR" voltage  

----

**GET_WIDTH**  
Purpose	: 	Get screen width in number of characters.  
Arguments :	none  

The screen width is currently identical for PAL vs. NTSC. But that could change in the future.

Example :	`GET_WIDTH`	// likely returns "30"  

----

**GET_HEIGHT**  
Purpose	: 	Get screen height in number of characters.  
Arguments :	none  

The screen height is currently different for PAL vs. NTSC.

Example :	`GET_HEIGHT`	// likely returns "16" (PAL) or "13" (NTSC)  

----

Below getters and setters are generated commands. All getters have no arguments and all setters have just one integer argument. These commands alter settings for the current session. See also basic commands SAVE, LOAD and DEFAULTS.

**GET/SET_VERSION**  
Purpose	: 	Get/set version string.  
Type : 		16 bytes string  
Range :		15 characters  
Default :	*version specific string*  

This holds the software version. At startup this value is compared to what is stored in the eeprom. If there's a mismatch, all configuration values are loaded with their defaults. This is to avoid possible corruption when the previously saved eeprom layout doesn't match any more due to software changes.

Examples :  
`GET_VERSION`		// shows version string  
`SET_VERSION blah`	// doesn't work!  

----

**GET/SET_BAUDRATE**  
Purpose	: 	Get/set serial baud rate.  
Type : 		32 bits integer value  
Range :		0 - 4G  
Default :	9600  

This holds the serial baud rate. As the baud rate is fixed per session, it makes no sense to set a new baud rate without saving it to eeprom (using command SAVE). So the baud rate can be changed, but only for future sessions. If you've forgotten the baud rate, clear the eeprom e.g. with Arduino. At next startup, all defaults will be restored then.

Examples :  
`GET_BAUDRATE`  
`SET_BAUDATE 57600`	// use easy to remember value!  
`SET_BAUDATE 7456`	// asking for trouble  

----

**GET/SET_TIMEOUT**  
Purpose	: 	Get/set data timeout.  
Type : 		32 bits integer value  
Range :		0 - 4G  
Default :	10000  

This sets the timeout for data commands. These are P_RAW, P_WINDOW, P_BANNER and SET_FONT. The configured value is in milliseconds. When the timeout triggers, the current command is ended by definition and the OSD is ready to receive a new command.

Examples :  
`GET_TIMEOUT`  
`SET_TIMEOUT 100`	// 0.1 seconds  
`SET_TIMEOUT 60000`	// 1 minute  

----

**GET/SET_VDETECT**  
Purpose	: 	Get/set video detect time.  
Type : 		32 bits integer value  
Range :		0 - 4G  
Default :	1000  

This holds the time in milliseconds to periodically auto-detect an attached video source (PAL/NTSC). Upon PAL/NTSC detect, the OSD will switch to the detected video standard. Note that PAL has currently 16 text lines and NTSC 13. To ensure proper operation for all cases, the application that controls the OSD should use command GET_HEIGHT beforehand to check how many lines are available.

Examples :  
`GET_VDETECT`  
`SET_VDETECT 1000`	// check video source every second  

----

**GET/SET_REFRESH**  
Purpose	: 	Get/set screen refresh rate.  
Type : 		32 bits integer value  
Range :		0 - 4G  
Default :	100  

Periodical screen refresh time in milliseconds. The OSD uses a shadow screen buffer that is processed periodically at he start of vertical synchronisation, which occurs 50 times per second for PAL and 60 times per second for NTSC. But the screen will only be refreshed if the configured time has elapsed. On the other hand, it doesn't make sense to set the time below the minimum although it won't harm either.

Examples :  
`GET_REFRESH`  
`SET_REFRESH 1000`	// once a second  
`SET_REFRESH 20`	// minimal value for PAL  
`SET_REFRESH 17`	// minimal value for NTSC  

----

**GET/SET_LOGO**  
Purpose	: 	Get/set logo display time.  
Type : 		32 bits integer value  
Range :		0 - 4G  
Default :	2000  

At startup a logo will be displayed if this setting is non-zero, for the configured number of milliseconds. The logo has 12 characters width and 2 characters height and can be configured by changing font characters 0x80 - 0x8B and 0x90 - 0x9B.

Examples :  
`GET_LOGO`  
`SET_LOGO 1000`	// 1 second  
`SET_LOGO 0`	// no logo at startup  

----

**GET/SET_ENABLE**  
Purpose	: 	Get/set OSD image visibility.  
Type : 		8 bit boolean value  
Range :		0 - 1  
Default :	1  

By default, the OSD image is enabled (for convenience). By setting it to zero, the OSD image will be disabled. This setting is used periodically (multiple times per second) to check and correct the ENABLE flag of the VM0 register. We do this because the OSD seems to reset the VM0 register now and then. By storing it separately we can correct such situations. So in fact the value is stored twice.

Examples :  
`GET_ENABLE`  
`SET_ENABLE 1`	// enable OSD image  
`SET_ENABLE 0`	// disable OSD image  

----

**GET/SET_DEBUG**  
Purpose	: 	Get/set debug flag.  
Type : 		8 bit boolean value  
Range :		0 - 1  
Default :	0  

This s a boolean flag that determines whether to print debug information to serial output. Beware that turning debugging on slows down the serial connection considerably, thereby risking a buffer overflow. It is recommended to keep this setting at zero. Or at least make sure that it is not saved in the eeprom so that it is zero again at next startup.

Examples :  
`GET_DEBUG`  
`SET_DEBUG 1`	// enable  
`SET_DEBUG 0`	// disable  

----

**GET/SET_ECHO**  
Purpose	: 	Get/set echo flag.  
Type : 		8 bit boolean value  
Range :		0 - 1  
Default :	1  

By default the OSD echoes back all input received over the serial connection. This can be used for manual testing and if one wants to implement a mechanism that prevents buffer overflow not based on flow control characters that can be enabled with the CONTROL command. All echoed characters are processed by the OSD so as long as the difference between sent and received number of characters is less than the buffer size (64 bytes), no overflow will occur. However, a better way definitely is to use control characters DC3 (XOFF) and DC1 (XON) as these are implemented for this purpose exclusively.

Examples :  
`GET_ECHO`  
`SET_ECHO 1`	// enable  
`SET_ECHO 0`	// disable  

----

**GET/SET_SILENT**  
Purpose	: 	Get/set silent flag.  
Type : 		8 bit boolean value  
Range :		0 - 1  
Default :	0  

Setting this boolean flag will set the serial control output idle, This overrides DEBUG, ECHO and CONTROL. Data output to serial is still written.

Examples :  
`GET_SILENT`  
`SET_SILENT 1`	// enable  
`SET_SILENT 0`	// disable  

----

**GET/SET_CONTROL**  
Purpose	: 	Get/set control flag.  
Type : 		8 bit value (boolean flags)  
Range :		0 - 3  
Default :	3  

By default the OSD prints ascii control characters and control strings that can be used to control the flow of operation. The first lower bit turns on ascii control characters for machine-to-machine operation. For example, ascii character 0x06 (ACK) on success and ascii character 0x15 (NAK) on failure. The second lower bit turns on human readable control strings like "\<OK\>" on success and "\<FAIL\>" on failure. See the general description for more info.

Examples :  
`GET_CONTROL`  
`SET_CONTROL 1`	// enable control characters (machine-to-machine)  
`SET_CONTROL 2`	// enable human readable control strings  
`SET_CONTROL 3`	// enable both  
`SET_CONTROL 0`	// disable  

----

**GET/SET_SENSADJ0**  
Purpose	: 	Get/set calibration value for sensor 0 ("VBAT1").  
Type : 		16 bits integer value  
Range :		0 - 64K  
Default :	20000  

This holds the calibration value for the sensor labelled "VBAT1".

Examples :  
`GET_SENSADJ0`  
`SET_SENSADJ0 12345`  

----

**GET/SET_SENSADJ1**  
Purpose	: 	Get/set calibration value for sensor 1 ("VBAT2").  
Type : 		16 bits integer value  
Range :		0 - 64K  
Default :	20000  

This holds the calibration value for the sensor labelled "VBAT2".

Examples :  
`GET_SENSADJ1`  
`SET_SENSADJ1 12345`  

----

**GET/SET_SENSADJ2**  
Purpose	: 	Get/set calibration value for sensor 2 ("RSSI").  
Type : 		16 bits integer value  
Range :		0 - 64K  
Default :	20000  

This holds the calibration value for the sensor labelled "RSSI".

Examples :  
`GET_SENSADJ2`  
`SET_SENSADJ2 12345`  

----

**GET/SET_SENSADJ3**  
Purpose	: 	Get/set calibration value for sensor 3 ("CURR").  
Type : 		16 bits integer value  
Range :		0 - 64K  
Default :	20000  

This holds the calibration value for the sensor labelled "CURR".

Examples :  
`GET_SENSADJ3`  
`SET_SENSADJ3 12345`  
