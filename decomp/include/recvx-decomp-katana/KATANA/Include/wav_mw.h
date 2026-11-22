/*
 *  Middleware Library
 *  Copyright (c) 1998 SEGA
 *  
 *		Wave Player
 *
 *  Module  : Library Header File
 *  File    : WAV_MW.H
 *  Create  : 1998-09-01
 *  Modify  : 1999-03-10
 *  Note    :
 */

#ifndef	_WAV_MW_H_
#define	_WAV_MW_H_

/***************************************************************************
 *		Variable Declaration
 ***************************************************************************/

extern	DCAIF	mwg_dcaif_wav;

/***************************************************************************
 *      Function Declaration
 ***************************************************************************/

/*	Initialize Wave-Decode Library		*/
#define	mwPlyInitWav(mode)			mwPlaInit(mode, &mwg_dcaif_wav)

/*	Finish using Wave-Decode Library	*/
#define	mwPlyFinishWav()			mwPlaFinish(&mwg_dcaif_wav)

/*	Create MWPLY handle for Wave-Deocde	*/
#define	mwPlyCreateWav(cprm)		mwPlaCreate(cprm, &mwg_dcaif_wav)

#endif	/*	_WAV_MW_H_	*/
