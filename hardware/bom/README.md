# BOM

The BOM is a snapshot of the parts I used in the last AirTalk production batch before the source release.  Nearly every part has a reasonable replacement: no call to stick to it *too* closely.

Some notes:

* The BOM does *not* include the mini-DIN sockets.  Fortunately the PCB footprints for mini DIN sockets is reasonably standard: my normal practice is to use whatever's decent quality, reasonably cheap, and in stock. 
* The BOM does *not* include the TashTalk.  In its place it includes an 8-pin DIP socket.  On my AirTalks, the TashTalk is socketed.  The TashTalk is a PIC12(L)F1840.  You can use either the F or LF for AirTalk: I've tended to use the LF just because it's been more reliably in stock at suppliers, but if you have the F version in stock, those'll work too.  The firmware for these is at https://github.com/lampmerchant/tashtalk - make sure that you're using a 1.x release of TashTalk, because future releases may change the pinout.
* The RS485 transceivers in the BOM are TP75176E-SRs.  There are plenty of alternatives to these.  A cheaper alternative that wasn't in stock when I made the BOM is the Gatemode GM3085E, which I tested but which was unavailable for a while.  Other transceivers that are known to work are listed at https://github.com/lampmerchant/tashtalk/blob/main/documentation/transceivers.md.  Not every RS485 transceiver out there will work, but there are many that will.  If in doubt, experiment.