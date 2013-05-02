========================================================================
    STATIC LIBRARY : libfreeWRLEAI Project Overview
========================================================================

AppWizard has created this libfreeWRLEAI library project for you.

No source files were created as part of your project.


libfreeWRLEAI.vcproj
    This is the main project file for VC++ projects generated using an Application Wizard.
    It contains information about the version of Visual C++ that generated the file, and
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

========================================================================
	CHANGELOG
	
21 march 2013 (it's springtime!!!)

- The VC++ has been rebuilt from scratch.
- Source code has been updated in order to deal with the winsock/winsock2 overwriting. #ifdef has been used to grant portability

- NOTE: the library DOES NOT use anymore the GeneratedCode.c included under "libeai" folder but uses the common GeneratedCode.c under "lib\scenegraph". Given that the project is now part of the main "freeWRL" solution, it could be a good thing to unify as much as code as possible.

- KNOWN ISSUES.
- The test project crashes the freeWRL "server" process when closes. The problem will be investigated.
- (Not really an issue). The lib has to specify the "XP_WIN" preprocessor definition in order to avoid an error in compiling XPCOM headers. Since XPCOM was deprecated, the definition could become useless in the future.