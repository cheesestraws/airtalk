# Hardware

**Please note that the hardware is under a different license from the firmware.  The firmware is BSD licensed.  The hardware is under the CERN-OHL-S-2.0**.

Now that we've got that out the way: this directory hopefully contains all you need to build your own AirTalk hardware.

* The schematics are in the schematic/ directory.  How imaginative.
* The PCB designs are in the boards/ directory, in both Gerber and EasyEDA format.
* The BoM for the PCB design is in the bom/ directory.

The whole thing has previously been fabbed and SMD assembled using the JLCPCB service.  Other services will probably work just as well.

As per usual, these come with no guarantee and I can't promise to provide support, because my time is limited.  I'll do my best, though.


# Using the AirTalk name

I would request people who build these to follow these guidelines:

* You can call your AirTalks AirTalks if they use the same firmware, and specifically if they are configured using the AirTalk Chooser extension.  If they need to be configured or used in some other way, please call them something else, although you can call them "AirTalk-compatible".  This is not about protecting a brand, it's about avoiding confusing users about what software they need.
* If you are making AirTalks for other people or for sale, please put your logo or name on either the board (if bare boards) or case (if cased).  This is so that I and other people can see which ones are ones I built and which ones other people have built.
* "Airtalk-compatible" is a perfectly reasonable way to refer to things that use LToUDP, if you want to.