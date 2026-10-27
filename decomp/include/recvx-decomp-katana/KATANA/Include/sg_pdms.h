/*
 *  Shinobi Library
 *  Copyright (c) 1998-2000 SEGA
 *
 *       Mouse
 *
 *  Module  : Library Header File
 *  File    : sg_pdms.h
 *  Create  : 2000-01-14
 *  Modify  : 2000-02-09
 *  Version : 2.02
 *  Note    :
 */

#ifndef _SG_PDMS_H_
#define _SG_PDMS_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <sg_xpt.h>


/*
 *  DEFINES / MACROS
 */

/******** Peripheral type **************************************************/
#ifndef PDD_DEVTYPE_POINTING
#define PDD_DEVTYPE_POINTING   (1 << 9)    /* Pointing Function            */
#endif

/******** Button assign ****************************************************/
#define PDD_MOUSE_BUTTON_M     (1 <<  0)  /* Middle Button                 */
#define PDD_MOUSE_BUTTON_R     (1 <<  1)  /* Right Button                  */
#define PDD_MOUSE_BUTTON_L     (1 <<  2)  /* Left Button                   */
#define PDD_MOUSE_BUTTON_S     (1 <<  3)  /* Sub Button                    */
#define PDD_MOUSE_BUTTON_KU    (1 <<  4)  /* Up Key                        */
#define PDD_MOUSE_BUTTON_KD    (1 <<  5)  /* Down Key                      */
#define PDD_MOUSE_BUTTON_KL    (1 <<  6)  /* Left Key                      */
#define PDD_MOUSE_BUTTON_KR    (1 <<  7)  /* Right Key                     */
#define PDD_MOUSE_BUTTON_WU    (1 << 17)  /* Wheel Up                      */
#define PDD_MOUSE_BUTTON_WD    (1 << 16)  /* Wheel Down                    */


/******** Axis assign ******************************************************/
#define PDD_MOUSE_AXIS_X       (1 << 0)   /* X Axis                        */
#define PDD_MOUSE_AXIS_Y       (1 << 1)   /* Y Axis                        */
#define PDD_MOUSE_AXIS_Z       (1 << 2)   /* Z Axis (Wheel)                */
#define PDD_MOUSE_AXIS_3       (1 << 3)   /* Axis No.3                     */
#define PDD_MOUSE_AXIS_4       (1 << 4)   /* Axis No.4                     */
#define PDD_MOUSE_AXIS_5       (1 << 5)   /* Axis No.5                     */
#define PDD_MOUSE_AXIS_6       (1 << 6)   /* Axis No.6                     */
#define PDD_MOUSE_AXIS_7       (1 << 7)   /* Axis No.7                     */


/*
 *  TYPEDEFS
 */

/******** Mouse digital data ***********************************************/
typedef struct {
	Uint32 on;                            /* Buttons pressed               */
	Uint32 off;                           /* Buttons released              */
	Uint32 press;                         /* Buttons pressed now(edge)     */
	Uint32 release;                       /* Buttons released now(edge)    */
	Uint32 old;                           /* Old buttons                   */
	Uint8 connect;                        /* Is connected?                 */
	Uint8 opt;                            /* Option status                 */
	Uint8 reserved[2];                    /* Reserved                      */
} PDS_MOUSE_BUTTON;

/******** Mouse axis data **************************************************/
typedef struct {
	Uint8 over;                           /* Axes overflow flags           */
	Uint8 reserved;                       /* Reserved                      */
	Sint16 x;
	Sint16 y;
	Sint16 z;
	Sint16 axis[5];
} PDS_MOUSE_AXIS;

/******** Mouse information ************************************************/
typedef struct {
	Uint8 reserved;                       /* Reserved                      */
	Uint8 button;                         /* Supported buttons             */
	Uint8 axis;                           /* Supported analog axes         */
} PDS_MOUSE_INFO;



/*
 *  PROTOTYPES
 */

void pdMouseInit(void);
void pdMouseExit(void);
const PDS_MOUSE_BUTTON* pdMouseGetButton(Uint32 port);
const PDS_MOUSE_AXIS* pdMouseGetAxis(Uint32 port);
const PDS_MOUSE_INFO* pdMouseGetInfo(Uint32 port);
Sint32 pdMouseIsReady(Uint32 port);
void pdMouseExecServer(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SG_PDMS_H_ */

