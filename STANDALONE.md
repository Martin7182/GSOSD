GSOSD STAND ALONE
=================
To be honest, operating the OSD via the serial connection can be a bit complicated and in cases where you only need to show the battery volage or runtime, it is just overkill. Therefore a stand alone version has been made to serve these simple cases. The key feature is that all settings can be changed on the field in a couple of seconds.


Hardware
--------
Operating the OSD is done using a single push button that can be attached between any of the sensor inputs (VBAT1, VBAT2, RSSI or CURR) and ground, provided that you use a pullup resistor to set the input high by default. You can still use all of these inputs to measure voltages though. But when doing so, you should not short the voltage source if that creates a problem (smoke!). So use a resistor in series with your voltage source if needed. A 1k resistor will do in all cases, see below schematics. This won't affect the sensor's accuracy in any way.
A voltage below ~5% of full-scale will be seen as "low", above that it will be seen as "high". With the internal 1k5/22k voltage dividers this will be at ~0.9V externally. The OSD checks all sensor voltages at startup to determine which inputs are above that threshold. Any of these sensors can be used to control the OSD.

![standalone.jpg](images/standalone.jpg)


Demo
----
This video demonstrates how to setup the screen layout and calibrate a sensor-voltage:

https://www.youtube.com/watch?v=ESbT348I-Sw


Compilation
-----------
Stand alone mode can be chosen by setting the "STANDALONE" compiler option, e.g. in the Makefile:

CXXFLAGS_STD = -D STANDALONE

Note that this disables the serial mode.


Operation
---------
The OSD is operated with short and long button presses. A short press triggers on button release while a long press triggers on button hold (>1s). There's also a long-long press that triggers when the button is held down a really long time (>10s). The following button actions are defined:

While normally running:
- Short press : toggle OSD on/off.
- Long press : enter main menu.

While navigating through menus:
- Short press : select next item.
- Long press : enter selected item (menu or adjust).

While adjusting a setting:
- Short press : advance to next value.
- Long press : repeatedly advance to next value.
- Long-long press : like long press, but with bigger steps.

During inactivity (>2s), the menu collapses automatically. As a last step, all changed values are written to eeprom so that they are preserved when turning the OSD off. A note on menu Screen->Enable->No, if you turn the OSD off using this menu, this setting (like all others) will be preserved in eeprom memory. Turning on the OSD using a short button press will only have a temporary effect.


Menu structure
--------------

Menu **main**:

| menu-item     | description
|---------------|-------------------------------------------------
| "Layout"      | set elements x and y positions on screen
| "Calibration" | calibrate sensor inputs
| "Screen"      | general screen settings
| "About"       | show details about this software

----

Menu **main->layout**:

| menu-item     | description
|---------------|-------------------------------------------------
| "Sensor0"	| set sensor 0 (VBAT1) appearance
| "Sensor1"	| set sensor 1 (VBAT2) appearance
| "Sensor2"	| set sensor 2 (RSSI) appearance
| "Sensor3"	| set sensor 3 (CURR) appearance
| "Runtime"	| set runtime appearance

----

Menu **main->layout->item(sensor0, sensor1, sensor2, sensor3, runtime)**:

| menu-item     | description
|---------------|-------------------------------------------------
| "Visible"	| show value
| "Invisible"	| hide value

----

Menu **main->layout->item->visible**:

| menu-item     | description
|---------------|-------------------------------------------------
| "Horizontal"	| adjust horizontal position
| "Vertical"	| adjust vertical position

----

Menu **main->layout->item->visible->item(horizontal, vertical)**:

| menu-item     | description
|---------------|-------------------------------------------------
| "+"		| next column (hor) or row (ver)
| "-"		| previous column (hor) or row (ver)

----

Menu **main->calibration**:

| menu-item     | description
|---------------|-------------------------------------------------
| "Sensor0"	| calibrate sensor 0 (VBAT1)
| "Sensor1"	| calibrate sensor 1 (VBAT2)
| "Sensor2"	| calibrate sensor 2 (RSSI)
| "Sensor3"	| calibrate sensor 3 (CURR)

----

Menu **main->calibration->item(sensor0, sensor1, sensor2, sensor3)**:

| menu-item     | description
|---------------|-------------------------------------------------
| "+"		| add 0.01V (normal rate) or 0.1V (fast rate)
| "-"		| subtract 0.01V (normal rate) or 0.1V (fast rate)

----

Menu **main->screen**:

| menu-item               | description
|-------------------------|-------------------------------------------------
| "Enable"	          | turn OSD on or off instantly and at startup
| "Horizontal offset"     | adjust horizontal offset
| "Vertical offset"       | adjust vertical offset
| "Character black level" | adjust character black level
| "Character white level" | adjust character white level
| "Sharpness 1"      	  | adjust rise and fall time
| "Sharpness 2"	          | adjust insertion mux switching time

----

Menu **main->screen->enable**:

| menu-item     | description
|---------------|-------------------------------------------------
| "Yes"		| turn OSD on at startup
| "No"		| turn OSD off instantly and at startup

----

Menu **main->screen->item(hos, vos, cbl,cwl, insmux1, insmux2)**:

| menu-item     | description
|---------------|-------------------------------------------------
| "+"		|	choose next value
| "-"		|	choose previous value

