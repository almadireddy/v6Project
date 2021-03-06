# v6 File System Explorer

This program is a class assignment for CS 4348: Operating Systems Concepts. It takes as input a binary file representing a modified v6 file system that has 1024 bytes per block, and a filename. It will look in the file system for the given filename and attempt to read that file, then write it out to `myoutputfile.txt`.

## Running the program

I assume that you will have the modified v6 filesystem at a known directory. Run `make` to compile or `make run` to make, and then run the output binary.

The program will have some output, and will await for you to put in the path to the v6 filesystem. If it successfully finds and opens the file, it will prompt you to enter the file you are looking for.

It will run the procedures to find that file, and if it completes, it will attempt to open a file called "myoutputfile.txt" and put the contents of the file you're looking for in there.

## Assumptions

I made the following assumptions:
  - We didn't need to handle anything but plain files and directories.
  - We didn't need to handle large directories (directories with large flag set to 1). (This was per a message sent on telegram by Professor).

