Graphcan v9.0
CAN data visualizer for the '04 Toyota Prius on a Sharp SL-C700 written by Attila Vass

Put all the files to /home/zaurus/CAN directory

'mp' - is a modified madplay, an mp3 player.
'graphcan' - is the CAN onboard computer application

/home/zaurus/CAN/PriusData.txt is where information on MPG and last values are stored.
/home/zaurus/CAN/FuelData.txt is where the fuel gauge movement is kept.

Usage :
- go to /home/zaurus/CAN directory and start graphcan
  It will try to open the serial port at /dev/ttyS0, make sure you have set the right permissions
  ( su; chmod a+rws /dev/ttyS0 )
- It doesn't matter if the Prius is already sending the CAN info, if yes, graphcan just
  starts showing the data, if not, it will patiently wait for the data to be available.
- When you power down the car, graphcan will detect it and show information on your trip.
- Push the power button on the Zaurus, this will put the Zaurus into sleep mode.
- When you turn on the Zaurus next time, graphcan will sense the power restore and will reinitialize
  itself and tries to read the CAN again...

- If you want to 'zero' ( reset ) your trip data, you can manually do so in the
  /home/zaurus/CAN/PriusData.txt file by 0-ing out these 2 lines :
  "Trip Kilometers = 0" line and the
  "Trip Gallon Usage Indicator = 0" line at the very end.
( The kernel of a GUI to do this is already there, but not quite tested yet... )

- Use 'graphcan -h' for more help on using the touchscreen and commandline parameters
to access some functionalilites while running...

Cheers,
	Attila