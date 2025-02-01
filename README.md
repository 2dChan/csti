# csti - Contest system terminal interface

csti is a lightweight universal assistant for interacting with the contest without leaving the terminal.
```shell
$ csti # Send solution.
Send task1 succsessfull
$ csti -s # Get status of task.
Task  |Status |Tests |Score 
task1 |Ok     |100   |100  
```

## Supported systems
- **Ejudge**

To add a system, you need to write a patch that will implement the API of the required system. And create issue if you want share the patch with others.

## Getting started
### Building csti
csti has the following dependencies:
 - gcc/clang
 - gnumake
 - libressl or libtls-dev (for send requests)
### Install
```
$ cp src/config.def.h src/config.h # Create config file.
$ vim src/config.h # Setup config file.
$ make install
```

## Features
 - Setup pre send actions for (format code)/(cut some fragments)/(other sh instructions).
