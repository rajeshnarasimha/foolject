# Microsoft Developer Studio Project File - Name="VNCLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=VNCLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "VNCLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "VNCLib.mak" CFG="VNCLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "VNCLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "VNCLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "VNCLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\Temp\VNC\Release"
# PROP Intermediate_Dir "..\..\Temp\VNC\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /WX /GX /O2 /I "inc" /I "../nvsystem/inc" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\Temp\VNC\VNCLib.lib"

!ELSEIF  "$(CFG)" == "VNCLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "VNCLib___Win32_Debug"
# PROP BASE Intermediate_Dir "VNCLib___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\Temp\VNC\Debug"
# PROP Intermediate_Dir "..\..\Temp\VNC\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /WX /Gm /GX /Zi /Od /I "inc" /I "../nvsystem/inc" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "ONLYFOR_RM" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\Temp\VNC\VNCLib_d.lib"

!ENDIF 

# Begin Target

# Name "VNCLib - Win32 Release"
# Name "VNCLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\VNChannel.cpp
# End Source File
# Begin Source File

SOURCE=.\src\VNConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\src\VNNetMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\VNQueue.cpp
# End Source File
# Begin Source File

SOURCE=.\src\VNQueueGroup.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\inc\Nettypes.h
# End Source File
# Begin Source File

SOURCE=.\inc\VNChannel.h
# End Source File
# Begin Source File

SOURCE=.\inc\VNConnection.h
# End Source File
# Begin Source File

SOURCE=.\inc\VNNetMsg.h
# End Source File
# Begin Source File

SOURCE=.\inc\VNQueue.h
# End Source File
# Begin Source File

SOURCE=.\inc\VNQueueGroup.h
# End Source File
# End Group
# End Target
# End Project
