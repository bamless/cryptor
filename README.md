# README #

This is a ransomware-like client server application developed as the final project for the 'System programming' course of 'La Sapienza' university of Rome.

### How to use ###

coming soon

### How do I get set up? ###

#### Linux ####
Clone the repository and run `make [client|server]` in the project root folder. Make sure to have `gcc` and `make` installed.

#### Windows ####
Make sure to have [MinGW-w64](https://sourceforge.net/projects/mingw-w64/) `bin/` folder into the PATH. Also the `msys/*ver*/bin/` folder of [MinGW](http://www.mingw.org/) (N.B. this is different from the previous one) should be into the PATH. \
Once this 2 folder are into the path run `make [client|server]`.

#### Why both MinGW-w64 and MinGW? ####
MinGW-w64 `bin` folder contains the gcc compiler bundled with a porting of the Windows API while the `msys/*ver*/bin/` folder of MinGW constains programs used during the build process, such as `mkdir` and `make`. MinGW, in addition to the msys tools, also has a gcc/winAPI port, but the MinGW-w64 has been chosen for the wider suppport of windows API (For example it supports synchapi.h, not supported by MinGW).