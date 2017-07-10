# README #

This is a ransomware-like client server application developed as the final project for the 'System programming' course of 'La Sapienza' university of Rome.

### How to use ###

#### server #####
run the *cryptord* binary from terminal. The program accepts various arguments:

 * *-n* : the max number of thread the server will create to handle connections. (optional, default 20)
 * *-p* : the port over which the server will be listening for incoming connections. (optional, default 8888)
 * *-c* : the path to the folder in which the server will be running (its PWD). (optional, but only if the directory is specified in the conf. file)
 * *-f* : the path to a config file. (optional)

 Every line in the config file should have the form: *opt_name* *value*.

 The options available in the config files are: **directory**, **port** and **threads** and they correspond respectively to: *-c*, *-p* and *-n*. 

 If an option is specified in both the config file and as an argument, the option in the config file will be used.

 On Linux the config file can be reloaded while the server is running by sending a SIGHUP to the process.

#### client #####
run the *cryptor* binary from terminal.

The program accepts one of the following options, which correspond protocol commands:

 * *-l ip:port* will send a **LSTF** (list directory) command to the server at **ip** over port **port**.
 * *-R ip:port* will send a **LSTR** (list directory recursive) command to the server at **ip** over port **port**.
 * *-e seed path ip:port* will send an **ENCR** command to the server at **ip** over port **port** to encrypt the file at **path** using the seed **seed** to generate the key.
 * *-d seed path ip:port* similar to the previous one, but decrypts the file. Naturally the seed must be the same used for encryption.

### How do I get set up? ###

#### Linux ####
Clone the repository and run `make [client|server]` in the project root folder. Make sure to have `gcc` and `make` installed.

#### Windows ####
This project uses Msys2 and MinGW-w64 for building using **gcc** under Windows. First, you need to install msys2 from [here](http://www.msys2.org/). Once installed launch the msys2 bash and run `pacman -Syu` to update the pakage databse. If needed, close MSYS2, run it again from Start menu. Update the rest with: `pacman -Su`. Once all is updated, run `pacman -S mingw-w64-x86_64-gcc` for 64 bit, or `pacman -S mingw-w64-i686-gcc` for 32 bit, to install Mingw-w64. Run `pacman -S base-devel` to install the tools needed for the build process (such as make, etc...). Once all of that is done, you can build the project exactly like you would on Linux. Open the msys2 terminal, navigate to the project root, and run `make [client|server]`. If you want to use the Windows command prompt or powershell instead of the msys2 bash, you'll need to put the folders `*msys2 inst path*/mingw64/bin` and `*msys2 inst path*/usr/bin` inside your PATH.