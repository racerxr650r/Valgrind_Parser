# Valgrind_Parser
Simple command line utility to parse a Valgrind log file and present it in a more human readable format with the offending source code.

## Download this repository
```console
sudo apt update
sudo apt install git
git clone https://github.com/racerxr650r/Valgrind_Parser
```

## Build the application
```console
cd Valgrind_Parser
make
```
Change to the Valgrind_Parser directory and build the application

## Install the application
```console
make install
```
This will install the application binary and the man page file to common paths. This will require sudo permission so you will be
prompted for your password.

> [!NOTE]
> BINDIR (directory to install the binary file) and MANDIR 
(directory to install the man page documentation) can be defined from the 
make command line. This will replace the default values. The defaults for 
BINDIR and MANDIR should work for most Linux distributions

## Run the application
```console
valgrind --leak-check=full --log-file=valgrind-out.txt --fullpath-after=string ./your_program_to_test
vgp valgrind-out.txt
```
First run your application under test with Valgrind exporting the debug information to a log file named valgrind-out.txt. Then run
the Valgrind parser application (vgp) to parse the log file in human readable format with offending source code included.

> [!NOTE]
> When you build your application use the -g or -g3 debug info option and provide full pathnames for the source files. This will
> store the full pathnames along with the other debug information. To see an example of how to include the full pathnames for your
> source files see the makefile for vgp. When you run Valgrind be sure to include the --fullpath-after=string option so Valgrind
> outputs the full pathnames from the debug information. Doing this will insure that *vgp* can find the source files to display
> the offending function and line.

## Get usage information
```console
man vgp
```
man only works if you have installed vgp.

## Command line usage
```
USAGE: vgp <Valgrind_log_file>

The log filename is required.
```

## Uninstall vgp
```console
make uninstall
```
Uninstalls the vgp application and documentation.
