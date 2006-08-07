/*****************************************************************************\
 *
 * Copyright (c) 2001 eWings Technologies Inc. - All Rights Reserved.
 *
 * Purpose : Active Call service implementation
 *           
 * 
 * Author  : John Zurawski
 *
 * History : $Log: \\Sc\R&D Source\US_R&DQVCSArchives\Shared\NetLib\Inc\Nettypes.i $
//  
//    Auto-added
//  
//  Revision 1.6  by: Oding  Rev date: Fri 07/26/2002 10:13:20 UTC
//    
//  
//  Revision 1.5  by: Oding  Rev date: Fri 07/05/2002 08:45:46 UTC
//    Opened parameter for adjusting dialogic volume. 
//    jTTS implemented. 
//    Remove bilingual ASR feature from Philips ASR. 
//    Remove VoicePlatform and GrammarFormat from VPSYS.ini. 
//    Add global call for E1/analog 
//    Add inbound call on VoIP 
//    VXML 1.0 all features 
//    Obsolete the support VXML tag <DTMF> 
//    
//  
//  Revision 1.4  by: Oding  Rev date: Mon 06/17/2002 01:14:46 UTC
//    A temporary release for DEMO
//  
//  Revision 1.3  by: Oding  Rev date: Tue 06/04/2002 12:57:32 UTC
//    Relase for: 
//     JASR and JTTS 
//     Apply AMO in grammar language tag 
//     English PSGF open grammar support 
//     Add Fax tag for VXML 
//     Add script support for VXML 
//     New TTS chopping function 
//     Implement Philips-VAD and open parameters in PVAD.ini
//  
//  Revision 1.2  by: Oding  Rev date: Thu 05/30/2002 03:44:34 UTC
//    JASR and JTTS added. 
//    PVAD implemented and PVAD.ini added. 
//    New chopping algorithm and code convert algorithm. 
//    Encapsulated ASR grammar and ASR engine to class. 
//    Optimize Philips ASR grammar compile algorithm. 
//    Add Script support for VXML. 
//    Add PSGF open grammar support
//  
//  Revision 1.1  by: John  Rev date: Tue 06/26/2001 08:12:46 UTC
//    Removed no longer used service ID's. Added new binary parameter type.
//  
//  Revision 1.0  by: John  Rev date: Fri 05/18/2001 22:55:38 UTC
//    Initial revision.
//  
//  $Endlog$
\*****************************************************************************/
#ifndef __Nettypes_h__
#define __Nettypes_h__

// Warning-- these are used for an on-the-wire protocol and should
// not be changed without serious risk of breaking external applications.


// Parameter types 
enum NetParamType
{
    Param_Int8 = 0x00,
    Param_Int16 = 0x01,
    Param_Int32 = 0x02,
    Param_Bool = 0x03,
    Param_String = 0x10,
    Param_Binary = 0x11
};


#endif /* __Nettypes_h__ */
