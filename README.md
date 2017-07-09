# README #

This is a ransomware-like client server application developed as the final project for the 'System programming' course of 'La Sapienza' university of Rome.

### How to use ###

coming soon

### How do I get set up? ###

#### Linux ####
Clone the repository and run `make [client|server]` in the project root folder. Make sure to have `gcc` and `make` installed.

#### Windows ####
This project uses Msys2 and MinGW-w64 for building using **gcc** under Windows. First, you need to install msys2 from [here](http://www.msys2.org/). Once installed launch the msys2 bash and run `pacman -Syu` to update the pakage databse. If needed, close MSYS2, run it again from Start menu. Update the rest with: `pacman -Su`. Once all is updated, run `pacman -S mingw-w64-x86_64-gcc` for 64 bit, or `pacman -S mingw-w64-i686-gcc` for 32 bit, to install Mingw-w64. Run `pacman -S base-devel` to install the tools needed for the build process (such as make, etc...). Once all of that is done, you can build the project exactly like you would on Linux. Open the msys2 terminal, navigate to the project root, and run `make [client|server]`. If you want to use the Windows command prompt or powershell instead of the msys2 bash, you'll need to put the folders `*msys2 inst path*/mingw64/bin` and `*msys2 inst path*/usr/bin` inside your PATH.
