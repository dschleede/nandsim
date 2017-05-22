Overall

This code runs on a Raspberry PI. The intent is to interface the GPIO Pins to a Board that has a NAND Flash chip.
The GPIO pins will emulate the Open NAND Flass Interface, or ONFi standard.


Version
The current version of this simulator will accept only a few commands. It is untested at this point.


Commands Emulated:
Read ID
Reset
Read

* These seem to be the basic commands to READ a Flash. Right now, no write functions are implemented. The intent is that
the data can be set to be a file as the backing store, so setting the data can be done out of band. Once these basic
functions work, we can implement more ONFi commands.

WHY?
You may ask, why are we doing this. Well, what if your board DOESN'T have JTAG? How do you write to memory and validate things from the board?  What if you can't get jtag to work and write to the memory, but you can desolder a flash chip. You can use this utility to dump the flash contents (Perhaps a FT2232H board?) and then Simulate the flash in circuit. You then can make minor tweaks to the code to have fun!


ToDO:
1) Validate that communication works, can we get a Chip ID?
2) Make sure a READ works. There is some concern with timing? is this method fast enough?
3) Once this has been validated, we can add a Backing store for data.
4) Implement more commands so that flash writing is possible.

Author:
Donald Schleede
dschleede@gmail.com
