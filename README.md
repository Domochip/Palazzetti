# Software

This C++ library is used to communicate with Fumis/Palazzetti Stove.  
I made it generic.  
It means that you need to provide some functions to handle Serial port management (open/close/flush/read/write/etc.).  
That way, you can use this library on ESP8266, but also on other kind of microcontroller and maybe on x86

# Hardware

Hardware part of this project has been moved to WirelessPalaControl repo.  
On this one, you will find schematic and PCB to build your adapter to connect to stove.  
https://github.com/Domochip/WirelessPalaControl