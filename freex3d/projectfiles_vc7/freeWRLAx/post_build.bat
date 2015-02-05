echo %1
copy ..\%1\freeWRLAx.ocx ..\..\projectfiles_vc9\%1
REM copy ..\%1\freeWRLAx.tlb ..\..\projectfiles_vc9\%1
copy ..\%1\freeWRLAx.ocx ..\..\projectfiles_vc12\%1
REM copy ..\%1\freeWRLAx.tlb ..\..\projectfiles_vc12\%1
REM need to use administrator console - do during .msi install
REM regsvr32 ..\..\projectfiles_vc9\%1\freeWRLAx.ocx


