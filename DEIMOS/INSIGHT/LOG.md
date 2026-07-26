#### **6-22-26**
* MPM3610 wired in schematic. both 3.3 and 5v modes
* Switch moved to between battery and diode. to allow for RFD900 to only be powered by battery and not USB. This also means that if plugged in to USB, the board will always be on regardless of switch state. 
* Rescources for MPM added to the folders. Been keeping external parts in the 'Local' folder so that everyone will have access to it.

* Switch will need to be beefed up to handle the potential current going through it if RFD active.
* Look into buck converter placement in regards to each other. not sure if the two being next to each other will be a problem.
* look into buck converter routing in 4 layer PCB like ours. Might be Different
* need to figure out how to get everything to fit while keeping appearances. significantly, how will we get the switch on?