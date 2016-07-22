/*


???

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


/* NOTE: we have to re-implement the loading of movie textures; the code in here was a decade old and did not
keep up with "the times". Check for ifdef HAVE_TO_REIMPLEMENT_MOVIETEXTURES in the code */

/* July 2016 note:
	http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/texturing.html#MovieTexture
	- specs say to support MPEG1
	http://www.web3d.org/x3d/content/examples/ConformanceNist/
	- see Movie texture example with VTS.mpg

	There are still patents and licensing issues with recent mp4 and even mpeg-2 I've heard.
	MPEG1 - any patents have expired
	Goal: a generic interface we can use to wrap 3 implementations:
		1. stub
		2. option: old-fashioned cross-platform mpeg1 c code, as fallback/default
		3. platform-supplied/platform-specific video API/libraries 
	and to use both audio and video streams, with audio compatible with Sound node api needs.

	Links > 
	2. old fashioned mpeg1
	Mpeg-1 libs
	https://en.wikipedia.org/wiki/MPEG-1
	- See source code link at bottom, reference implementation
	- which has an ISO license which dug9 reads to mean: 
	--- use for simulation of what electronic devices will do, presumably by electronics companies
	--- (but not intendended / foreseen as a software library product, uncertain if allowed)
	- it includes audio and video
	http://www.gerg.ca/software/mpeglib/
	- Mpeg1 - hack of berkley mpeg1 code, no separate license on hacks show
	http://www.uow.edu.au/~nabg/MPEG/mpeg1.html
	- Mpeg1 explanation with hacked Berkley code and license: berkley + do-what-you-want-with-hacked-code 

*/
