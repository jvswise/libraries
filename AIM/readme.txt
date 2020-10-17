Library creation notes for AIM		2013-09-06
==============================

(J. van Schouwen)

Background:
----------
The AIM Protocol library comes in two flavours: a fast, low-RAM usage version
(AIM.h); and, a slow, high-RAM usage debug version (AIM_DEBUG.h) that spits
out protocol messages to the Arduino serial monitor. (AIM_DEBUG.h uses about
6 kB more RAM.) Applications will normally use the following include:
  #include <AIM.h>

If debugging the AIM protocol, or if you want to see a protocol trace from
the Arduino's perspective, use the following include:
  #include <AIM_DEBUG.h>


Updating the AIM libraries:
--------------------------
When creating a new version of the AIM and AIM_DEBUG libraries, first update
the AIM.h and AIM.cpp source files in the AIM library directory.  In AIM.h,
ensure that DEBUG_ON is not defined by commenting out the following line:
	//#define   DEBUG_ON
In AIM.cpp, ensure that AIM.h is included (not AIM_DEBUG.h)
Remember to update keywords.txt if any symbols you want highlighted in the
Arduino IDE have been added, deleted, or modified.


Then copy AIM.h and AIM.cpp to the AIM_DEBUG library directory and rename
those files to AIM_DEBUG.h and AIM_DEBUG.cpp, respectively. (Don't copy
over the keywords.txt file.)
In AIM_DEBUG.h, ensure DEBUG_ON is defined by uncommenting the line:
	#define    DEBUG_ON
In AIM_DEBUG.cpp, ensure that AIM_DEBUG.h is included rather than AIM.h


Creating a ZIP file of the libraries:
------------------------------------
If you want to create a ZIP archive useful for sharing the libraries, cd to
the parent directory that contains both the AIM/ and AIM_DEBUG/ directories
and run the following command:
	zip -r AIM_Arduino_libraries.zip AIM AIM_DEBUG

AIM_libraries.zip can then be shared. Users can install the libraries
in the usual way (refer to http://arduino.cc/en/Guide/Libraries).
