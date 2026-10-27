/*****************************************************************************
 * $Id: keydef.h,v 1.2 2000/08/24 18:10:26 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * key symbol definitions
 *----------------------------------------------------------------------------
 *		Copyright (c) 1998,1999,2000 NexGen Software.
 *
 *	All rights reserved. NexGen Software' source code is an unpublished
 *	work and the use of a copyright notice does not imply otherwise.
 *	This source code contains confidential, trade secret material of
 *	NexGen Software. Any attempt or participation in deciphering, decoding,
 *	reverse engineering or in any way altering the source code is
 *	strictly prohibited, unless the prior written consent of
 *	NexGen Software is obtained.
 *
 *   	This software is  supplied  under  the terms of a
 *   	license agreement or nondisclosure agreement with
 *   	NexGen Software, and may not be copied or disclosed
 *   	except  in  accordance  with  the  terms of  that
 *  	agreement.
 *
 *----------------------------------------------------------------------------
 * 09/08/2000 - Regis Feneon
 *****************************************************************************/

#ifndef __NG_KEYDEF_H_INCLUDED__
#define __NG_KEYDEF_H_INCLUDED__

/* control keys */
#define NG_KEY_BackSpace    0xFF08
#define NG_KEY_Tab          0xFF09
#define NG_KEY_Linefeed     0xFF0A
#define NG_KEY_Clear        0xFF0B
#define NG_KEY_Return       0xFF0D
#define NG_KEY_Pause        0xFF13
#define NG_KEY_Scroll_Lock  0xFF14
#define NG_KEY_Sys_Req      0xFF15
#define NG_KEY_Escape       0xFF1B
#define NG_KEY_Delete       0xFFFF

/* multi-key character composition */
#define NG_KEY_Multi_key            0xFF20
#define NG_KEY_SingleCandidate      0xFF3C
#define NG_KEY_MultipleCandidate    0xFF3D
#define NG_KEY_PreviousCandidate    0xFF3E

/* Cursor */
#define NG_KEY_Home         0xFF50
#define NG_KEY_Left         0xFF51
#define NG_KEY_Up           0xFF52
#define NG_KEY_Right        0xFF53
#define NG_KEY_Down         0xFF54
#define NG_KEY_Prior        0xFF55
#define NG_KEY_Pg_Up        0xFF55
#define NG_KEY_Next         0xFF56
#define NG_KEY_Pg_Down      0xFF56
#define NG_KEY_End          0xFF57
#define NG_KEY_Begin        0xFF58

/* Misc Functions */
#define NG_KEY_Select       0xFF60
#define NG_KEY_Print        0xFF61
#define NG_KEY_Execute      0xFF62
#define NG_KEY_Insert       0xFF63
#define NG_KEY_Undo         0xFF65
#define NG_KEY_Redo         0xFF66
#define NG_KEY_Menu         0xFF67
#define NG_KEY_Find         0xFF68
#define NG_KEY_Cancel       0xFF69
#define NG_KEY_Help         0xFF6A
#define NG_KEY_Break        0xFF6B
#define NG_KEY_Mode_switch  0xFF7E
#define NG_KEY_Num_Lock     0xFF7F

/* Keypad */
#define NG_KEY_KP_Space     0xFF80
#define NG_KEY_KP_Tab       0xFF89
#define NG_KEY_KP_Enter     0xFF8D
#define NG_KEY_KP_F1        0xFF91
#define NG_KEY_KP_F2        0xFF92
#define NG_KEY_KP_F3        0xFF93
#define NG_KEY_KP_F4        0xFF94
#define NG_KEY_KP_Home      0xFF95
#define NG_KEY_KP_Left      0xFF96
#define NG_KEY_KP_Up        0xFF97
#define NG_KEY_KP_Right     0xFF98
#define NG_KEY_KP_Down      0xFF99
#define NG_KEY_KP_Prior     0xFF9A
#define NG_KEY_KP_Page_Up   0xFF9A
#define NG_KEY_KP_Next      0xFF9B
#define NG_KEY_KP_Page_Down 0xFF9B
#define NG_KEY_KP_End       0xFF9C
#define NG_KEY_KP_Begin     0xFF9D
#define NG_KEY_KP_Insert    0xFF9E
#define NG_KEY_KP_Delete    0xFF9F
#define NG_KEY_KP_Equal     0xFFBD
#define NG_KEY_KP_Multiply  0xFFAA
#define NG_KEY_KP_Add       0xFFAB
#define NG_KEY_KP_Separator 0xFFAC
#define NG_KEY_KP_Subtract  0xFFAD
#define NG_KEY_KP_Decimal   0xFFAE
#define NG_KEY_KP_Divide    0xFFAF
#define NG_KEY_KP_0         0xFFB0
#define NG_KEY_KP_1         0xFFB1
#define NG_KEY_KP_2         0xFFB2
#define NG_KEY_KP_3         0xFFB3
#define NG_KEY_KP_4         0xFFB4
#define NG_KEY_KP_5         0xFFB5
#define NG_KEY_KP_6         0xFFB6
#define NG_KEY_KP_7         0xFFB7
#define NG_KEY_KP_8         0xFFB8
#define NG_KEY_KP_9         0xFFB9

/* Auxilliary Functions */
#define NG_KEY_F1           0xFFBE
#define NG_KEY_F2           0xFFBF
#define NG_KEY_F3           0xFFC0
#define NG_KEY_F4           0xFFC1
#define NG_KEY_F5           0xFFC2
#define NG_KEY_F6           0xFFC3
#define NG_KEY_F7           0xFFC4
#define NG_KEY_F8           0xFFC5
#define NG_KEY_F9           0xFFC6
#define NG_KEY_F10          0xFFC7
#define NG_KEY_F11          0xFFC8
#define NG_KEY_F12          0xFFC9
#define NG_KEY_F13          0xFFCA
#define NG_KEY_F14          0xFFCB
#define NG_KEY_F15          0xFFCC
#define NG_KEY_F16          0xFFCD
#define NG_KEY_F17          0xFFCE
#define NG_KEY_F18          0xFFCF
#define NG_KEY_F19          0xFFD0
#define NG_KEY_F20          0xFFD1
#define NG_KEY_F21          0xFFD2
#define NG_KEY_F22          0xFFD3
#define NG_KEY_F23          0xFFD4
#define NG_KEY_F24          0xFFD5
#define NG_KEY_F25          0xFFD6
#define NG_KEY_F26          0xFFD7
#define NG_KEY_F27          0xFFD8
#define NG_KEY_F28          0xFFD9
#define NG_KEY_F29          0xFFDA
#define NG_KEY_F30          0xFFDB
#define NG_KEY_F31          0xFFDC
#define NG_KEY_F32          0xFFDD
#define NG_KEY_F33          0xFFDE
#define NG_KEY_F34          0xFFDF
#define NG_KEY_F35          0xFFE0

/* Modifiers */
#define NG_KEY_Shift_L      0xFFE1
#define NG_KEY_Shift_R      0xFFE2
#define NG_KEY_Control_L    0xFFE3
#define NG_KEY_Control_R    0xFFE4
#define NG_KEY_Caps_Lock    0xFFE5
#define NG_KEY_Shift_Lock   0xFFE6
#define NG_KEY_Meta_L       0xFFE7
#define NG_KEY_Meta_R       0xFFE8
#define NG_KEY_Alt_L        0xFFE9
#define NG_KEY_Alt_R        0xFFEA
#define NG_KEY_Super_L      0xFFEB
#define NG_KEY_Super_R      0xFFEC
#define NG_KEY_Hyper_L      0xFFED
#define NG_KEY_Hyper_R      0xFFEE

/* ASCII */
#define NG_KEY_space               0x020
#define NG_KEY_exclam              0x021
#define NG_KEY_quotedbl            0x022
#define NG_KEY_numbersign          0x023
#define NG_KEY_dollar              0x024
#define NG_KEY_percent             0x025
#define NG_KEY_ampersand           0x026
#define NG_KEY_apostrophe          0x027
#define NG_KEY_parenleft           0x028
#define NG_KEY_parenright          0x029
#define NG_KEY_asterisk            0x02a
#define NG_KEY_plus                0x02b
#define NG_KEY_comma               0x02c
#define NG_KEY_minus               0x02d
#define NG_KEY_period              0x02e
#define NG_KEY_slash               0x02f
#define NG_KEY_0                   0x030
#define NG_KEY_1                   0x031
#define NG_KEY_2                   0x032
#define NG_KEY_3                   0x033
#define NG_KEY_4                   0x034
#define NG_KEY_5                   0x035
#define NG_KEY_6                   0x036
#define NG_KEY_7                   0x037
#define NG_KEY_8                   0x038
#define NG_KEY_9                   0x039
#define NG_KEY_colon               0x03a
#define NG_KEY_semicolon           0x03b
#define NG_KEY_less                0x03c
#define NG_KEY_equal               0x03d
#define NG_KEY_greater             0x03e
#define NG_KEY_question            0x03f
#define NG_KEY_at                  0x040
#define NG_KEY_A                   0x041
#define NG_KEY_B                   0x042
#define NG_KEY_C                   0x043
#define NG_KEY_D                   0x044
#define NG_KEY_E                   0x045
#define NG_KEY_F                   0x046
#define NG_KEY_G                   0x047
#define NG_KEY_H                   0x048
#define NG_KEY_I                   0x049
#define NG_KEY_J                   0x04a
#define NG_KEY_K                   0x04b
#define NG_KEY_L                   0x04c
#define NG_KEY_M                   0x04d
#define NG_KEY_N                   0x04e
#define NG_KEY_O                   0x04f
#define NG_KEY_P                   0x050
#define NG_KEY_Q                   0x051
#define NG_KEY_R                   0x052
#define NG_KEY_S                   0x053
#define NG_KEY_T                   0x054
#define NG_KEY_U                   0x055
#define NG_KEY_V                   0x056
#define NG_KEY_W                   0x057
#define NG_KEY_X                   0x058
#define NG_KEY_Y                   0x059
#define NG_KEY_Z                   0x05a
#define NG_KEY_bracketleft         0x05b
#define NG_KEY_backslash           0x05c
#define NG_KEY_bracketright        0x05d
#define NG_KEY_asciicircum         0x05e
#define NG_KEY_underscore          0x05f
#define NG_KEY_grave               0x060
#define NG_KEY_a                   0x061
#define NG_KEY_b                   0x062
#define NG_KEY_c                   0x063
#define NG_KEY_d                   0x064
#define NG_KEY_e                   0x065
#define NG_KEY_f                   0x066
#define NG_KEY_g                   0x067
#define NG_KEY_h                   0x068
#define NG_KEY_i                   0x069
#define NG_KEY_j                   0x06a
#define NG_KEY_k                   0x06b
#define NG_KEY_l                   0x06c
#define NG_KEY_m                   0x06d
#define NG_KEY_n                   0x06e
#define NG_KEY_o                   0x06f
#define NG_KEY_p                   0x070
#define NG_KEY_q                   0x071
#define NG_KEY_r                   0x072
#define NG_KEY_s                   0x073
#define NG_KEY_t                   0x074
#define NG_KEY_u                   0x075
#define NG_KEY_v                   0x076
#define NG_KEY_w                   0x077
#define NG_KEY_x                   0x078
#define NG_KEY_y                   0x079
#define NG_KEY_z                   0x07a
#define NG_KEY_braceleft           0x07b
#define NG_KEY_bar                 0x07c
#define NG_KEY_braceright          0x07d
#define NG_KEY_asciitilde          0x07e

/* Latin 1 */
#define NG_KEY_nobreakspace        0x0a0
#define NG_KEY_exclamdown          0x0a1
#define NG_KEY_cent        	       0x0a2
#define NG_KEY_sterling            0x0a3
#define NG_KEY_currency            0x0a4
#define NG_KEY_yen                 0x0a5
#define NG_KEY_brokenbar           0x0a6
#define NG_KEY_section             0x0a7
#define NG_KEY_diaeresis           0x0a8
#define NG_KEY_copyright           0x0a9
#define NG_KEY_ordfeminine         0x0aa
#define NG_KEY_guillemotleft       0x0ab
#define NG_KEY_notsign             0x0ac
#define NG_KEY_hyphen              0x0ad
#define NG_KEY_registered          0x0ae
#define NG_KEY_macron              0x0af
#define NG_KEY_degree              0x0b0
#define NG_KEY_plusminus           0x0b1
#define NG_KEY_twosuperior         0x0b2
#define NG_KEY_threesuperior       0x0b3
#define NG_KEY_acute               0x0b4
#define NG_KEY_mu                  0x0b5
#define NG_KEY_paragraph           0x0b6
#define NG_KEY_periodcentered      0x0b7
#define NG_KEY_cedilla             0x0b8
#define NG_KEY_onesuperior         0x0b9
#define NG_KEY_masculine           0x0ba
#define NG_KEY_guillemotright      0x0bb
#define NG_KEY_onequarter          0x0bc
#define NG_KEY_onehalf             0x0bd
#define NG_KEY_threequarters       0x0be
#define NG_KEY_questiondown        0x0bf
#define NG_KEY_Agrave              0x0c0
#define NG_KEY_Aacute              0x0c1
#define NG_KEY_Acircumflex         0x0c2
#define NG_KEY_Atilde              0x0c3
#define NG_KEY_Adiaeresis          0x0c4
#define NG_KEY_Aring               0x0c5
#define NG_KEY_AE                  0x0c6
#define NG_KEY_Ccedilla            0x0c7
#define NG_KEY_Egrave              0x0c8
#define NG_KEY_Eacute              0x0c9
#define NG_KEY_Ecircumflex         0x0ca
#define NG_KEY_Ediaeresis          0x0cb
#define NG_KEY_Igrave              0x0cc
#define NG_KEY_Iacute              0x0cd
#define NG_KEY_Icircumflex         0x0ce
#define NG_KEY_Idiaeresis          0x0cf
#define NG_KEY_ETH                 0x0d0
#define NG_KEY_Ntilde              0x0d1
#define NG_KEY_Ograve              0x0d2
#define NG_KEY_Oacute              0x0d3
#define NG_KEY_Ocircumflex         0x0d4
#define NG_KEY_Otilde              0x0d5
#define NG_KEY_Odiaeresis          0x0d6
#define NG_KEY_multiply            0x0d7
#define NG_KEY_Ooblique            0x0d8
#define NG_KEY_Ugrave              0x0d9
#define NG_KEY_Uacute              0x0da
#define NG_KEY_Ucircumflex         0x0db
#define NG_KEY_Udiaeresis          0x0dc
#define NG_KEY_Yacute              0x0dd
#define NG_KEY_THORN               0x0de
#define NG_KEY_ssharp              0x0df
#define NG_KEY_agrave              0x0e0
#define NG_KEY_aacute              0x0e1
#define NG_KEY_acircumflex         0x0e2
#define NG_KEY_atilde              0x0e3
#define NG_KEY_adiaeresis          0x0e4
#define NG_KEY_aring               0x0e5
#define NG_KEY_ae                  0x0e6
#define NG_KEY_ccedilla            0x0e7
#define NG_KEY_egrave              0x0e8
#define NG_KEY_eacute              0x0e9
#define NG_KEY_ecircumflex         0x0ea
#define NG_KEY_ediaeresis          0x0eb
#define NG_KEY_igrave              0x0ec
#define NG_KEY_iacute              0x0ed
#define NG_KEY_icircumflex         0x0ee
#define NG_KEY_idiaeresis          0x0ef
#define NG_KEY_eth                 0x0f0
#define NG_KEY_ntilde              0x0f1
#define NG_KEY_ograve              0x0f2
#define NG_KEY_oacute              0x0f3
#define NG_KEY_ocircumflex         0x0f4
#define NG_KEY_otilde              0x0f5
#define NG_KEY_odiaeresis          0x0f6
#define NG_KEY_division            0x0f7
#define NG_KEY_oslash              0x0f8
#define NG_KEY_ugrave              0x0f9
#define NG_KEY_uacute              0x0fa
#define NG_KEY_ucircumflex         0x0fb
#define NG_KEY_udiaeresis          0x0fc
#define NG_KEY_yacute              0x0fd
#define NG_KEY_thorn               0x0fe
#define NG_KEY_ydiaeresis          0x0ff

#endif

