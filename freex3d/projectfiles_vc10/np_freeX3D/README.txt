------------------------------------------------------------------------------------------------------------------------
-----------------	NPAPI freeX3D/freeWRL plugin adapter for windows operating systems (np_freeX3D)
-----------------	by Luca Cerutti (luca.ceruttimtsATgmail.com)
------------------------------------------------------------------------------------------------------------------------

LEGAL NOTICE: this software uses Mozilla foundation xulrunner SDK (mainly the NPAPI part) and freeWRL/freeX3D library. http://www.mozilla.org/NPL/
xulrunner SDK is released under Netscape Public License v1.1 but it is permitted to use it also under LGPL 2.1 and GPL 2.
freeWRL/freeX3D is released under GNU Library General Public License that has been superseded by LGPL. http://www.gnu.org/licenses/old-licenses/lgpl-2.0.html
So let's just say that this code is released under LGPL 2.1, that you can apply a more updated version of the LGPL you like, and be happy with it. :D




SOFTWARE REQUIREMENTS
- MicroSoft Windows XP(tm) Service Pack 3 or more recent Windows Operating Systems
- Visual Studio 2010 Express Edition or higher


XULRUNNER COMPLIANCE
The dll is built against XULRUNNER 3.6.27 (Gecko 1.9.2) https://developer.mozilla.org/en-US/docs/Gecko_SDK in order to be compatible also with Google Chrome and other NPAPI compliant browsers.
Since the whole Gecko/Xulrunner source is pretty big, I've included the necessary headers (a handful of KB) in the "include" subfolder, so the solution can be compiled without external sources. 
Everyone is free to change the inclusion to a folder containing an updated version. Just know that people at Mozilla foundation have the habit to change header names or even function definition without giving so much of a notice. So the code may not compile (no big deal though, just see where they did the change).


HOW TO BUILD
You obviously have to build the solution. np_freeWRL does not compile without having dllfreeWRL and libfreeWRL compiled.
To build the solution, see the short but clear page at: http://freewrl.sourceforge.net/windowsBuild.html


HOW TO INSTALL
First the dll library containing the plugin interface must have the prefix "np_" (in our case is "np_freeX3D.dll")
Then the dll library containing the plugin must go with each and any of the dll it links. In our case:
- dllfreeWRL.dll
- glew32mx.dll
- mozjs185-1.0.dll
- pthreadVC2.dll (or pthreadVC2d.dll if you wish to install the debug version)

After you've grouped all the needed dlls, there are two way to make a plugin work with Mozilla or NPAPI compliant browser.

The NOT recommended one is to place them under your mozilla (or other browser) installation folder.

The recommended one is to place them under a folder of your choice and then:
- from the command line open your registry editor (regedit). Under Windows 7 or other UAC compliant system, you will be asked to grant administrative right to regedit.
- search for the "MozillaPlugins" registry folder.
-- under Windows XP it should be under: HKEY_LOCAL_MACHINE\SOFTWARE\MozillaPlugins
-- under Windows 7 it should be under: HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432node\MozillaPlugins
-- If you did not find it, search for "MozillaPlugins"
- right-click on the "MozillaPlugins" registry folder and choose "new" and then "key"
- name the new key as you see fit (we suggest np_freeX3D)
- right-click on the newly created key and choose "new" and then "string value"
- name the new string value "Path" and set to the value with the absolute path to the np_freeX3D.dll (e.g.: c:\myassemblies\npapi_plugins\np_freeX3D.dll)

Complex as it may be, this method keeps your mozilla installation folder clean and grants that your plugin will be seen by each and every NPAPI compliant browser.


HOW TO TEST
Easy, open the "test.html" web page you find under the project folder. If you did it right, the page will show the freeWRL interface and open the file at http://www.web3d.org/x3d/content/examples/Basic/X3dSpecification/Chopper.x3d


HOW TO EMBED THE CONTROL IN ANY OTHER HTML PAGE AND/OR TO OPEN A DIFFERENT wrl/x3d FILE
Even easier. Copy the content of the <EMBED> tag where you see fit and/or change the "target" attribute value to a URI you like.