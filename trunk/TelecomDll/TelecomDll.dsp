# Microsoft Developer Studio Project File - Name="TelecomDll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TelecomDll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TelecomDll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TelecomDll.mak" CFG="TelecomDll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TelecomDll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TelecomDll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TelecomDll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TELECOMDLL_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "D:\JOB\NetVoice 2.3\Library\Dialogic511.sp1.fp1\INC" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TELECOMDLL_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 libgc.lib libsrlmt.lib libdxxmt.lib libdtimt.lib libecmt.lib sctools.lib libfaxmt.lib libd42mt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /libpath:"D:\JOB\NetVoice 2.3\Library\Dialogic511.sp1.fp1\lib"

!ELSEIF  "$(CFG)" == "TelecomDll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TELECOMDLL_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../NMSInc/" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TELECOMDLL_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 cta.lib adiapi.lib nccapi.lib vceapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../../Debug/TelecomDll.dll" /pdbtype:sept /libpath:"../../NMSLib/"

!ENDIF 

# Begin Target

# Name "TelecomDll - Win32 Release"
# Name "TelecomDll - Win32 Debug"
# Begin Group "others"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\callback.h
# End Source File
# Begin Source File

SOURCE=.\CallState.h
# End Source File
# Begin Source File

SOURCE=.\LineState.h
# End Source File
# Begin Source File

SOURCE=.\LogLevel.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TelecomDll.cpp
# End Source File
# Begin Source File

SOURCE=.\TelecomDll.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\dlg_ex.cpp
# End Source File
# Begin Source File

SOURCE=.\dlg_ex.h
# End Source File
# Begin Source File

SOURCE=.\Line.cpp
# End Source File
# Begin Source File

SOURCE=.\Line.h
# End Source File
# Begin Source File

SOURCE=.\Telecom.cpp
# End Source File
# Begin Source File

SOURCE=.\Telecom.h
# End Source File
# End Target
# End Project
