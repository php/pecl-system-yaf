# Microsoft Developer Studio Project File - Name="ap" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ap - Win32 Release_TS
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ap.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ap.mak" CFG="ap - Win32 Release_TS"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ap - Win32 Release_TS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ap - Win32 Debug_TS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ap - Win32 Release_TS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_TS"
# PROP BASE Intermediate_Dir "Release_TS"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_TS"
# PROP Intermediate_Dir "Release_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\.." /I "..\..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\..\TSRM" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "COMPILE_DL_YAF" /D ZTS=1 /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\.." /I "..\..\main" /I "..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\TSRM" /D ZEND_DEBUG=0 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AP_EXPORTS" /D "COMPILE_DL_AP" /D ZTS=1 /D "ZEND_WIN32" /D "PHP_WIN32" /D HAVE_AP=1 /D "LIBZEND_EXPORTS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x406 /d "NDEBUG"
# ADD RSC /l 0x406 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php5ts.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php5ts.lib /nologo /dll /machine:I386 /out:"..\..\Release_TS/php_ap.dll" /libpath:"..\..\Release_TS" /libpath:"..\..\Release_TS_Inline"

!ELSEIF  "$(CFG)" == "ap - Win32 Debug_TS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Debug_TS"
# PROP BASE Intermediate_Dir "Debug_TS"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Debug_TS"
# PROP Intermediate_Dir "Debug_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\.." /I "..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\TSRM" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "COMPILE_DL_YAF" /D ZTS=1 /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /O2 /I "..\.." /I "..\..\main" /I "..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\TSRM" /D ZEND_DEBUG=1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "YAF_EXPORTS" /D "COMPILE_DL_YAF" /D ZTS=1 /D "ZEND_WIN32" /D "PHP_WIN32" /D HAVE_YAF=1 /D "LIBZEND_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x406 /d "NDEBUG"
# ADD RSC /l 0x406 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php5ts.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php5ts_debug.lib /nologo /dll /machine:I386 /out:"..\..\Debug_TS/php_ap.dll" /libpath:"..\..\Debug_TS"

!ENDIF 

# Begin Target

# Name "ap - Win32 Release_TS"
# Name "ap - Win32 Debug_TS"
# Begin Group "Ap Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ap.c
# End Source File
# Begin Source File

SOURCE=.\ap_loader.c
# End Source File
# Begin Source File

SOURCE=.\ap_dispatcher.c
# End Source File
# Begin Source File

SOURCE=.\ap_bootstrap.c
# End Source File
# Begin Source File

SOURCE=.\ap_config.c
# End Source File
# Begin Source File

SOURCE=.\ap_registry.c
# End Source File
# Begin Source File

SOURCE=.\ap_controller.c
# End Source File
# Begin Source File

SOURCE=.\ap_action.c
# End Source File
# Begin Source File

SOURCE=.\ap_view.c
# End Source File
# Begin Source File

SOURCE=.\ap_request.c
# End Source File
# Begin Source File

SOURCE=.\ap_response.c
# End Source File
# Begin Source File

SOURCE=.\ap_router.c
# End Source File
# Begin Source File

SOURCE=.\ap_exception.c
# End Source File
# Begin Source File

SOURCE=.\ap_plugin.c
# End Source File

# End Group
# Begin Group "Ap Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\php_ap.h
# End Source File
# Begin Source File

SOURCE=.\ap_autoloader.h
# End Source File
# Begin Source File

SOURCE=.\ap_bootstrap.h
# End Source File
# Begin Source File

SOURCE=.\ap_config.h
# End Source File
# Begin Source File

SOURCE=.\ap_registry.h
# End Source File
# Begin Source File

SOURCE=.\ap_controller.h
# End Source File
# Begin Source File

SOURCE=.\ap_view.h
# End Source File
# Begin Source File

SOURCE=.\php_request.h
# End Source File
# Begin Source File

SOURCE=.\ap_router.h
# End Source File
# Begin Source File

SOURCE=.\ap_exception.h
# End Source File
# Begin Source File

SOURCE=.\ap_plugin.h
# End Source File
# End Group
# End Target
# End Project
