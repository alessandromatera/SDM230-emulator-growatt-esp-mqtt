# SDM230 emulator for Growatt Solar Inverter
The scope of the project is to simulate the SDM230 meter to limit the power export of a Growatt solar inverter ("MIC TL-X 1000" in my case) without the use of an SDM230 meter device hard wired to the system.

# Introduction
The SDM230 communicates via RS485 wired to the Growatt inverter. The inverter is the master which asks for power readings from the SDM meter (slave). In my case, I don't have an SDM meter but I have a shelly EM device that reads the grid power. So, to limit the power export to the grid, you can either buy and hard wire an SDM230 or... you can emulate one and do the job wirelessly.

I was inspired by two projects:
1. https://github.com/That-Dude/Emulated-Eastrom-SDM-Power-Meter which started a similar project without having the possibility to test it
2. https://github.com/EmbedME/growatt_simulate_sdm630 which was able to communicate with the inverter via a Python script

In my case, I pretty much converted the 2nd repo above and use it in the 1st repo configuration, so to have an ESP8266 which gets grid info from the shelly device and sends back the grid power - appropriately converted in Modbus - to the Growatt Inverter via an RS485 - TTL converter.

# Hardware Connections

![Hardware Connections](https://github.com/alessandromatera/SDM230-emulator-growatt-esp-mqtt/blob/main/Connections.png)

# Unsolved Issues
In repo number 2 above, the Python code waits for a specific slave ID and a registry address from the Modbus Master (inverter) before giving back the power readings. In my case didn't work with any address. So I decided to always give back power readings after a request from the Inverter.

# ESP8266 (arduino) code
You can find the code here:
https://github.com/alessandromatera/SDM230-emulator-growatt-esp-mqtt/blob/main/esp8266_SDM230_emulator.ino

Enjoy!
