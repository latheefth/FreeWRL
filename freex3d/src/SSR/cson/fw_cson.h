/*cson C json toolkit
needed for:
- SSRserver (SSR: server side rendering) to communicate with html client:
	- viewer postion and orientation in the most absolute coordinates
		- ie world coords (.pos, .quat transformed via View part of ModelView matrix)
		- or if geo nodes, then geographic coordinates
	- placemarks in absolute coords
	- tree of placemarks - so each SSR responsible for its own placemarks, and zoneserver aggregates them
	- SSR_API in general
vs. xml toolkits: 
- about the same, except in libfreewrl we aren't doing any xml generation, just parsing, 
	- so we would need xml generation api when responding in json, or requesting via json in C of another SSR
- benefits of doing json vs xml
	- its a new learning stretch for freewrlians
	- may need json for EAI revamp (if we had JSON 15 years ago, EAI might have used JSON?)
	- may need json for web3dz (like x3z) for supporting web3d.org new json format
vs. json-c, jansson, other json c toolkits listed on http://json.org/: 
- cson about the same as jansson -toss up- got cson running first, others may work, not investigated
- cson meets all our requirements:
- 64bit doubles, which we need for absolute coords for large geographic scenes
- both generator (for writing ie sending back to html client, requesting from another SSR)
	 and parser (for reading POST from client, or sniffing for position)
- single .h, .c so no .lib needed, just add .c to existing project (because we are suffering lib exhaustion)
	(cson amalgamation. json-c, jansson are .lib)
- MIT or equivalent license, so can fall within fw LGPL license constraints
- 32 and 64 bit compilation
- documentation, internal docs, example / tutorial / test code 
	(jansson had more testsuite, but a bit light on docs. cson had just enough to get going, 
	and good internal docs for hacking)
- possibie session management in future (cson and jansson seemed to have something,
	cson would need re-amalgamation to get session layer)
- attempt to minimize memory fragmentation: (cson claims effort)
	all the toolkits seem to parse to a tree of malloced objects (not sax-style callback parsing)
	then you walk the tree to do your object-to-my-appdata gathering or searching/sniffing
	then call a cleanup(root_object) to erase the object tree
	generally we are relying on malloc/free to re-use and coallesce fragments produced

dug9 downloaded cson from http://fossil.wanderinghorse.net/wikis/cson/?page=cson
May 23, 2015 using the instructions given for downloading from trunk on fossil
logged in as anonymous user, and picking the top trunk revision, and 
finding a download .zip link
and amalgamated (packed into single .c, .h) using its bash script
sh createAmalgamationCore.sh
found in /cson trunk
(dug9 ran .sh under cygwin64 on windows)
did some x64 touchups for 'compiler noise reduction' directly in the amalg .c, 
-- would need to redo it (about 5 minutes work) if re-amalgamating, or
	do it 'upstream' (generally x64 size_t gets downcaste to int or even to char (8 bit signed int +- 127)
*/
//As per instructions in cson.h, for double/64bit floats 
//needed for GIS / geographic coordinates
#define CSON_DOUBLE_T_PFMT ".8Lf" // for Modified Julian Day values
#define HAVE_LONG_DOUBLE
#include "cson_amalgamation_core.h"