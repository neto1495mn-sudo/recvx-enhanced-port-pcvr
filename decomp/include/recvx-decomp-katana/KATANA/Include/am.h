// -------------------------------------------------------------------------
// am.h
//
// Header for the AM layer code
//
// COPYRIGHT (C) SEGA OF AMERICA, INC. 1998
// All rights reserved.
//
// Author: Ralph "Cooksey" Thomas
//
// Tabs: 4
//
// Rev:		12/02/98	File Originated
//
//			04/28/99	1.75.d.4
//
//			06/11/99	1.80.b.1
//
//			09/29/99	8.2.a
//
//			10/01/99	8.2.b
//
//			10/05/99	8.2.b.2
// -------------------------------------------------------------------------

#ifndef _AM_H
#define _AM_H

#include <actypes.h>
#include <ac.h>
#include <ambnkhdr.h>	// the bank header declaration
#include <amstrhdr.h>	// the stream header declaration

#ifdef  __MWERKS__
#include<SH4_mwk.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------------------------

// amInit.c
#define AM_DA_DRIVER_NAME								"audio64.drv"
#define	AM_DA_INT_ARRAY_LENGTH							64;
#define AM_MIDI_DRIVER_NAME								"MidiDa.drv"	
#define	AM_MIDI_INT_ARRAY_LENGTH						32;

// amFile.c
#define AM_FILE_BLOCK_SIZE								2048
#define AM_FILE_BOUNDRY_ALIGNMENT						32		// aligned on 32 byte boundries

// amMem.c
#define AM_HEAP_BOUNDRY_ALIGNMENT						32
#define AM_HEAP_SIZE_IN_BYTES							2048000
#define AM_HEAP_SIZE_IN_DWORDS							(AM_HEAP_SIZE_IN_BYTES/4)
#define AM_HEAP_MAX_MASTER_POINTERS						100
#define AM_HEAP_FINGERPRINT_LENGTH						4
#define AM_HEAP_ALIGNMENT								4

// amMidi.c
#define AM_MIDI_EOT_OFFSET								2
#define AM_MIDI_LOOP_OFFSET								6
#define AM_MIDI_ADDRESS_MASK							0x003fffff

// this will go when the driver is updated to parse the header of the SMF
#define AM_MIDI_MTRK_OFFSET								22	

// amdma.c
#define AM_DMA_CHANNEL									2
#define AM_DMA_FRAME_SIZE								32
#define AM_DMA_ENABLE		(1 << 0)		// Bit0:DE Starts transfer (enables operation) when set
#define AM_DMA_DONE			(1 << 1)		// Bit1:TE Set when transfer complete
#define AM_DMA_INTERRUPT	(1 << 2)		// Bit2:IE Enables an interrupt at end of transfer
#define AM_DMA_BYTE			(1 << 4)		// Bit4:TS0 Byte transfers (default is quadword)
#define AM_DMA_WORD			(1 << 5)		// Bit5:TS1 Word transfers (default is quadword)
#define AM_DMA_DWORD		((1<<4)|(1<<5))	// Bit4:TS0 Dword transfers (default is quadword)
#define AM_DMA_QWORD		(0)				// Bit456:TS012 Qword transfers (default is quadword)
#define AM_DMA_32BYTE		(1 << 6)		// Bit6:TS2 32 byte block transfer
#define AM_DMA_BURSTMODE	(1 << 7)		// Bit7:TM Burst transfer mode (default is cycle steal)
#define AM_DMA_DUAL_ADDR	(1 << 10)		// Bit10:RS2
#define AM_DMA_DEST_INC		(1 << 14)		// Bit14:DM0 Dest addr incremented (default is fixed)
#define AM_DMA_DEST_DEC		(1 << 15)		// Bit15:DM1 Dest address decremented (default is fixed)
#define AM_DMA_SRC_INC		(1 << 12)		// Bit12:SM0 Src address incremented  (default is fixed)
#define AM_DMA_SRC_DEC		(1 << 13)		// Bit13:SM1 Src address decremented  (default is fixed)
#define AM_DMAOR_DDT		(1 << 15)
#define AM_DMAOR_ADDR_ERROR	(1 << 2)		// Bit2:AE dmaor address boundry error detected
#define AM_DMAOR_ROUNDROBIN ((1 << 8) | (1 << 9))

// volume and pan ranges
#define AM_MAX_VOLUME		127
#define AM_MIN_VOLUME		0
#define AM_MAX_PAN			127
#define AM_MIN_PAN			0
#define AM_PAN_CENTER		64

// dsp ranges
#define 	AM_MAX_DSP_MIXER_CHANNELS					15
#define 	AM_MAX_Q_CHANNELS							16
#define 	AM_MAX_DSP_SOURCE_MIX						100

// return value for amGetVoiceChannel() if vc is not been set.
#define AM_UNINITIALIZED_VOICE_CHANNEL					0x666


//-----------------------------------------------------------------------------
// abstract types
//-----------------------------------------------------------------------------

// amBank.c
typedef volatile KTU32 * AM_BANK_PTR;

//-----------------------------------------------------------------------------
// proc pointer types
//-----------------------------------------------------------------------------
	
// amMem.c
typedef KTBOOL	(*AM_SH4_ALLOC_PROC)	(volatile KTU32 **,volatile KTU32 **,KTU32 ,KTU32);
typedef void	(*AM_SH4_FREE_PROC)		(volatile KTU32 *);
// amHeap.c
typedef void	(*AM_MEMORY_CALLBACK)	(volatile KTU32 *blockAddress);
// amVoice.c
typedef void	(*AM_USER_CALLBACK)		(KTU32);
typedef void	(*AM_SYSTEM_CALLBACK)	(KTU32);
// amStream.c
typedef void	(*AM_STREAM_ISR)		(KTU32);



//-----------------------------------------------------------------------------
// enum types
//-----------------------------------------------------------------------------

// arg for amInitSelectDriver()
typedef enum	
	{
	AM_NO_DRIVER_SELECTED	=	0,
	AM_DA_DRIVER			=	1,
	AM_MIDI_DRIVER			=	2
	}AM_DRIVER_TYPE;
	
typedef AM_DRIVER_TYPE * AM_DRIVER_TYPE_PTR;

typedef enum
	{
	AM_MONO		=	1,
	AM_STEREO	=	2
	}AM_TRACK_TYPE;

// amio.c
typedef enum
	{
	AM_OPEN		=	0,
	AM_READ		=	1,
	AM_CLOSE	=	2,
	AM_GET_SIZE	=	3,
	AM_LOAD		=	4,
	AM_REWIND	=	5
	}AM_FILE_OPERATION_MODE;

// amio.c
// this has a dependency to the above enum
typedef KTBOOL (*AM_IO_PROC)(KTSTRING,ACFILE *,KTU8 *,KTU32 *,AM_FILE_OPERATION_MODE);

// amstreamio.c
typedef KTBOOL (*AM_STREAM_IO_PROC)(KTSTRING,ACFILE *,KTU8 *,KTU32 *,AM_FILE_OPERATION_MODE);

// amstream.c
// arg for KTBOOL amStreamSetTransferMethod(AM_STREAM_PTR theStream,AM_STREAM_TRANSFER_METHOD transferMethod,KTU32 dmaChannel,KTU32 dmaFrameSize)

typedef enum
	{
	AM_FILL_FRONT	=	0,
	AM_FILL_BACK	=	1
	}AM_STREAM_SERVER_OPERATION;


// amHeap.c
typedef enum
	{
	AM_UNINITIALIZED_MEMORY_TYPE	=	0,
	AM_FIXED_MEMORY					=	1,
	AM_PURGABLE_MEMORY				=	2,
	AM_ALL_MEMORY					=	3
	}AM_HEAP_MEMORY_TYPE;

  
// amError.c
typedef enum
	{
	AM_NO_ERROR						=	0,		// all is well
	// memory allocation 1-100
	
	// bad parameters 101-200
	AM_NULL_PARAMETER				=	101,	// a pointer parameter is NULL
	AM_OUT_OF_RANGE_PARAMETER		=	102,	// a parameter is out of range
	AM_0_SIZE_PARAMETER				=	103,	// the parameter size is 0
	AM_0_ALIGNMENT_PARAMETER		=	104, 	// can't have memory alignment of 0
	AM_0_CHANNELS_PARAMETER			=	105,
	// file system 201-300
	AM_SEEK_FAIL					=	201,
	AM_READ_FAIL					=	202,
	AM_GET_SIZE_FAIL				=	203,
	AM_CLOSE_FAIL					=	204,
	AM_FILE_NOT_FOUND				=	205,
	AM_LOAD_FAIL					=	206,
	AM_REWIND_FAIL					=	207,
	AM_OPEN_FAIL					=	208,
	// driver 301-400
	AM_CANT_SEND_DRIVER_COMMAND		=	301,
	AM_DRIVER_NOT_INSTALLED			=	302,
	AM_UNABLE_TO_INSTALL_DRIVER		=	303,
	AM_DRIVER_IS_INSTALLED			=	304,
	AM_UNABLE_TO_INIT_AC_SYSTEM		=	305,
	AM_RETRY_ISR_EVENT_WARN			=	306,
	AM_CANT_SET_STREAM_CALLBACK		=	307,
	// warns 401-500
	AM_OUT_OF_RANGE_PARAMETER_FIXED	=	401,	// an out of range parameter was "fixed" see the docs for the behavior of that specific function
	// memory 501-600
	AM_NON_32_BYTE_ALIGNED_ADDRESS	=	501,
	AM_NON_DWORD_ALIGNED_ADDRESS	=	502,
	AM_AUDIO_HEAP_NOT_INITIALIZED	=	503,
	AM_UNSUPPORTED_MEMORY_TYPE		=	504,
	AM_INSUFFICIENT_PURGABLE_MEMORY	=	505,
	AM_CAN_ONLY_FREE_TOP_BLOCK		=	506,
	AM_INVALID_BLOCK_HANDLE			=	507,
	AM_INSUFFICIENT_MEMORY			=	508,
	AM_OUT_OF_MEMORY_CONTROL_BLOCKS	=	509,
	AM_HEAP_IS_ALREADY_INITIALIZED	=	510,
	AM_HEAP_IS_CORRUPT				=	511,
	AM_IMPROPER_ADDRESS_ALIGNMENT	=	512,
	AM_IMPROPER_SIZE_ALIGNMENT		=	513,
	AM_INVALID_DMA_CHANNEL			=	514,
	AM_BUFFER_IS_NOT_IN_AICA_MEMORY	=	516,
	AM_CANT_LOAD_INTO_AICA_MEMORY	=	517,
	
	// bank 601-700
	AM_ASSET_INDEX_NOT_PRESENT		=	601,
	AM_NOT_A_MIDI_ASSET				=	602,
	AM_NOT_A_WAVE_ASSET				=	603,
	AM_NOT_AN_UNKKNOWN_ASSET		=	604,
	// voice pool 701-800
	AM_NO_VOICES_AVAILABLE			=	701,
	AM_UNSUPPORTED_VOICE_TYPE		=	702,
	AM_VOICE_CHANNEL_UNINITIALIZED	=	703,
	// system 801-900
	AM_INTERNAL_ERROR				=	801,	// this should never happen
	AM_UNSUPPORTED_AICA_AUDIO_TYPE	=	802,
	AM_UNSUPPORTED_AICA_SAMPLE_RATE	=	803,	// the aica rates are even rates 44100,22050,11025 etc, see ac.h
	AM_UNSUPPORTED_BIT_DEPTH		=	804,
	AM_FAILED_TO_CHAIN_INT_HANDLER	=	805,	// failed to add the interrupt handler to the chain
	// midi 901-1000
	AM_TONE_BANK_SLOT_OUT_OF_RANGE	=	901,
	AM_MIDI_PORT_OUT_OF_RANGE		=	902,
	AM_MIDI_CHANNEL_OUT_OF_RANGE	=	903,
	AM_MIDI_PROGRAM_OUT_OF_RANGE	=	904,
	AM_MIDI_NOTE_OUT_OF_RANGE		=	905,
	AM_MIDI_VELOCITY_OUT_OF_RANGE	=	906,
	// digital audio 1001-1100
	AM_UNSUPPORTED_SAMPLE_RATE		=	1001,	// a sample rate exceeding 1128900 was requested
	AM_SOUND_IS_NOT_PLAYING			=	1002,
	AM_SOUND_IS_PLAYING				=	1003,
	// stream 1101-1200
	AM_CORRUPT_STREAM_OBJECT		=	1101,	// most of the time this means that a track has more then 2 channels, this is not supported so the data must be corrupt	
	AM_STREAM_NOT_OPEN				=	1102,
	AM_STREAM_BUFFER_NOT_MULTIPLE_OF_2048	=	1103,
	AM_TRACK_NUMBER_OUT_OF_RANGE	=	1104,
	AM_STREAM_IS_NOT_PLAYING		=	1105,
	AM_CANT_SET_PAN_ON_STEREO_STREAM	=	1106,
	AM_STREAM_IS_DONE				=	1107,
	AM_STREAM_IS_NOT_INITIALIZED	=	1108,
	AM_WRONG_INTERLEAVE_RATE		=	1109,	// the interleave rate that the stream was built with does not match up with the play buffer size, adjust one or the other...
	AM_DMA_TRANSFER_NOT_IMPLEMENTED_YET	=	1110,
	AM_STREAM_IS_OPEN				=	1111,
	AM_FILENAME_TOO_LONG			=	1112,
	AM_STREAM_SEEK_FAIL				=	1113,
	AM_STREAM_READ_FAIL				=	1114,
	AM_STREAM_GET_SIZE_FAIL			=	1115,
	AM_STREAM_CLOSE_FAIL			=	1116,
	AM_STREAM_FILE_NOT_FOUND		=	1117,
	AM_STREAM_REWIND_FAIL			=	1118,
	AM_STREAM_OPEN_FAIL				=	1119,
	AM_STREAM_TRANSFER_BUFFER_OVERRUN	=	1120,
	// sound effects (amSound)
	AM_SOUND_STRUCT_NOT_INITIALIZED	=	1201,
	AM_SOUND_IS_PLAYING_ERROR		=	1202,
	AM_LAST_ERROR					=	5000
	}AM_ERROR_TYPE;

#define AM_ERROR_MESSAGE_LENGTH	255
 
typedef struct 
	{
	char 			message[AM_ERROR_MESSAGE_LENGTH];
	AM_ERROR_TYPE	number;
	}AM_ERROR;

typedef AM_ERROR * AM_ERROR_PTR;

extern AM_ERROR		gAmError; // errors are reported via this structure

// ----------------------------------------------------------------------------------------------
// aggregates
// ----------------------------------------------------------------------------------------------

// amInit.c
typedef struct
	{
	KTU32				interruptArrayLength;
	AM_DRIVER_TYPE		driverType;
	KTCHAR				driverFileName[15];	
	}AM_SYSTEM_STRUCT;
	
typedef AM_SYSTEM_STRUCT * AM_SYSTEM_STRUCT_PTR;

// acInt.c and acVoice.c
typedef enum
	{
	AM_UNINITIALIZED_VOICE	=	0,
	AM_STREAM_VOICE			=	1,
	AM_ONESHOT_VOICE		=	2,
	AM_MIDI_VOICE			=	3,
	AM_ASYNC_STREAM_VOICE	=	4
	}AM_VOICE_TYPE;

typedef struct
	{
	AM_SYSTEM_CALLBACK		systemCallback;
	AM_USER_CALLBACK		userCallback		[AC_TOTAL_VOICES];
	AM_STREAM_ISR			streamIsr			[AC_TOTAL_VOICES];
	volatile KTBOOL			channels			[AC_TOTAL_VOICES];
	volatile AM_VOICE_TYPE	channelType			[AC_TOTAL_VOICES];
	volatile void *			channelOwner		[AC_TOTAL_VOICES];
	}AM_VOICE_POOL;

extern AM_VOICE_POOL gAmVoicePool;

// amMem.c
typedef struct 
	{
	volatile KTU32 *			address;			// aligned address of the block
	volatile KTU32 *			fingerprintAddress;	// address of the fingerprint "RCTT"
	KTU32						size;				// size of the block + overhead
	KTBOOL						allocated;			// KTTRUE if block is allocated
	AM_MEMORY_CALLBACK			callback;			// address of callback function
	}AM_MCB_STRUCT;

typedef AM_MCB_STRUCT * AM_MCB_STRUCT_PTR;

typedef struct
	{
	KTBOOL					open;					// true if the heap has been initialized
	KTU32					bytesRemaining;			// bytes remaining for allocation
	KTU32					heapSize;				// total size of the heap zone
	volatile KTU32 *		baseOfHeap;				// the unaligned start of the zone
	volatile KTU32 *		bottomOfHeap;			// the DWORD aligned start, first address allocated
	volatile KTU32 *		topOfHeap;				// the last byte owned by the zone
	AM_MCB_STRUCT			purgableMCB	[AM_HEAP_MAX_MASTER_POINTERS];	// purgable memory control blocks
	AM_MCB_STRUCT			fixedMCB	[AM_HEAP_MAX_MASTER_POINTERS];	// fixed memory control blocks
	}AM_HEAP_STRUCT;




typedef struct
{
// mono play buffer
volatile KTU32 *		startOfMonoPlayBuffer;			// start of buffer in audio memory
volatile KTU32 *		middleOfMonoPlayBuffer;			// middle of mono buffer in audio memory
// stereo play buffers
volatile KTU32 *		startOfLeftPlayBuffer;			// start of left buffer in audio memory
volatile KTU32 *		startOfRightPlayBuffer;			// start of right buffer in audio memory
volatile KTU32 *		middleOfLeftPlayBuffer;			// middle of left buffer in audio memory
volatile KTU32 *		middleOfRightPlayBuffer;		// middle of right buffer in audio memory
// counter
volatile KTU32			interruptsTillEnd;				// number of interrupts till end
volatile KTU32			channels;						// 1 for mono, 2 for stereo
//volatile KTU32			size;							// size of the track in bytes (both channels if stereo)
volatile KTU32			volume;							// the aica volume, 0-15
volatile KTU32			pan;							// the aica pan, 0-31 
// aica
KTU32					monoPort;						// da port used for the mono stream
KTU32					leftPort;						// da port used for the left stream
KTU32					rightPort;						// da port used for the right stream
}AM_STREAM_TRACK;	

typedef enum
	{
	AM_STREAM_NON_DMA	=	0,
	AM_STREAM_DMA
	}AM_STREAM_TRANSFER_METHOD;

#define AM_STREAM_FILENAME_LEN							80		// used in AM_STREAM structure

typedef struct
{
volatile AM_STREAM_TRACK track[AM_STREAM_MAX_TRACKS];// the track data
KTU32					numberOfTracks;				// number of tracks in this stream
// file read buffer
volatile KTU32 *		transferBuffer;				// pointer to transfer buffer in sh4 memory
volatile KTU32			transferBufferSize;			// size of transfer buffer in any memory
volatile KTU32			transferBufferOffset;		// bytes transferred from current transfer buffer
volatile KTU32			playBufferSize;				// size of play buffer in audio memory
volatile KTU32			interleaveRate;				// the interleave rate of the str file, 1/2 playBufferSize
// frame markers
volatile KTU32			middleOfBufferInFrames;			// middle of mono or left buffer in audio memory in flames
volatile KTU32			endOfBufferInFrames;			// end of mono or left buffer in audio memory in flames
// isr play buffer flag
volatile KTU32			bufferToggle;				// flips to alternatly load the buffers
// play buffer fill flags
volatile KTBOOL			fillFront;
volatile KTBOOL			fillBack;
// move memory management, DMA or memcpy()
AM_STREAM_TRANSFER_METHOD	transferMethod;			// either AM_STREAM_DMA or AM_STREAM_NON_DMA
KTU32					dmaChannel;					// DMA channel if transferMethod==AM_STREAM_DMA
KTU32					dmaFrameSize;				// number of bytes in a DMA transfer
// state flags
volatile KTU32			serviceFlag;				// KTTRUE when bufferNumber is ready for transfer
volatile KTU32			openFlag;					// KTTRUE if the file is open
volatile KTU32			initFlag;					// KTTRUE if struct has pass the init routine
// completion status
volatile KTU32			active;						// KTTRUE when playing
volatile KTU32			done;						// KTTRUE when all bytes have been transferred
// file system stuff
KTCHAR					fileName[AM_STREAM_FILENAME_LEN]; // the file name
ACFILE					sd;							// descriptor for the stream io file system
volatile KTU32			fileSize;					// size of the asset to be streamed
// wave info
volatile KTU32			sampleRate;					// the real world sample rate i.e. AC_22K_SAMPLE_RATE, AC_44K_SAMPLE_RATE etc...
AC_AUDIO_TYPE			type;						// the aica sample type 
// the ISR
volatile KTU32			callbackFlag;				// KTTRUE if callback installed
AM_STREAM_ISR			callback;					// the callback function, stereo or mono
KTU32					eventId;					// the ID from the callback system
KTU32					isrPort;					// the port used for event messages
volatile KTU32			isrCounter;					// number of interrupts served
volatile KTU32			interruptsTillEnd;			// number of interrupts till end
KTBOOL					bufferPlay;					// true if transfer buffer supplied as external buffer
}AM_STREAM;

typedef volatile AM_STREAM * AM_STREAM_PTR;

// multi track stream scene mixing 
typedef struct
	{
	KTU32	volume;			//	0-127
	KTU32	pan;			//	0-127
	}AM_STREAM_MIX_TRACK;
 
typedef struct
	{
	AM_STREAM_MIX_TRACK	track[AM_STREAM_MAX_TRACKS];
	}AM_STREAM_MIX;
	
typedef AM_STREAM_MIX * AM_STREAM_MIX_PTR;

// sound effects

typedef struct
	{
	KTU32				sampleRate;
	AC_AUDIO_TYPE		aicaSampleType;
	KTU32				volume;
	KTU32				pan;
	KTU32				effectVolume;
	KTU32				effectMixerChannel;
	KTU32				qPanAngle;
	KTS32				aicaLoopFlag;
	KTU32				pitchOffsetInCents;
	KTU32				sizeInBytes;
	KTU32				voiceChannel;
	KTBOOL				randomPitchAmount;
	KTBOOL				effectFlag;
	KTU32 *				soundData;
	volatile KTBOOL		isPlaying;
	AM_USER_CALLBACK	callback;
	}AM_SOUND;

typedef AM_SOUND * AM_SOUND_PTR;

typedef struct
	{
	KTU32 firstQChannel;
	KTU32 lastQChannel;
	KTBOOL qEnabledFlag;
	}AM_SOUND_SYSTEM;

typedef void	(*AM_MIDI_CALLBACK)		(KTU32); // this will take some work in amVoice.c

// midi sequences
typedef struct
	{
	KTU32				voiceChannel;	// the voice channel number
	KTU32				midiPort;		// the MIDI port number
	KTS32				gmMode;			// AC_GM_ON or AC_GM_OFF
	KTU32				ticksPQN;		// the time division in ticks per quarter note
	KTU32				usPQN;			// the time division in microseconds per quarter note
	KTU32				volume;			// midi master volume 0-127
	KTU32				loop;			// 1 if looping, 0 if not
	KTU32				size;			// the size of the sequence data
	KTU32 *				address;		// the address of the sequence data
	AM_MIDI_CALLBACK	callback;		// the callback proc
	KTU32				callbackOffset;	// the offset in sound memory to the EOT marker
	}AM_SEQUENCE;

typedef AM_SEQUENCE * AM_SEQUENCE_PTR;


extern AM_SYSTEM_STRUCT	gAmSystem;


//-----------------------------------------------------------------------------
// prototypes
//-----------------------------------------------------------------------------


// amError.c
void 			amErrorSet					(AM_ERROR_TYPE errorNumber,KTSTRING errorMessage);
AM_ERROR_PTR 	amErrorGetLast				(void);
KTBOOL 			amErrorExists				(void);
void 			amErrorClear				(void);


// amSound.c, play one shot wave files from a katbank
KTBOOL	amSoundPlay					(AM_SOUND_PTR theSound);
KTBOOL	amSoundStop					(AM_SOUND_PTR theSound);
KTBOOL 	amSoundStopAll				(void);
KTBOOL	amSoundIsPlaying			(AM_SOUND_PTR theSound);
KTBOOL	amSoundSetCurrentPlaybackRate(AM_SOUND_PTR theSound,KTU32 sampleRate);
KTBOOL	amSoundSetVolume			(AM_SOUND_PTR theSound,KTU32 aicaVolume);
KTBOOL	amSoundSetPan				(AM_SOUND_PTR theSound,KTU32 aicaPan);
KTBOOL	amSoundSetVoiceChannel		(AM_SOUND_PTR theSound,KTU32 voiceChannel);
KTBOOL	amSoundSetCallback			(AM_SOUND_PTR theSound,AM_USER_CALLBACK callback);
KTBOOL	amSoundAllocateVoiceChannel	(AM_SOUND_PTR theSound);
KTBOOL	amSoundIsLooping			(AM_SOUND_PTR theSound,KTBOOL *loopFlag);
KTBOOL	amSoundGetSampleRate		(AM_SOUND_PTR theSound,KTU32 *realWorldSampleRate);
KTBOOL	amSoundGetVolume			(AM_SOUND_PTR theSound,KTU32 *aicaVolume);
KTBOOL	amSoundGetPan				(AM_SOUND_PTR theSound,KTU32 *aicaPan);
KTBOOL	amSoundGetVoiceChannel		(AM_SOUND_PTR theSound,KTU32 *voiceChannel);
KTBOOL	amSoundGetCallback			(AM_SOUND_PTR theSound,AM_USER_CALLBACK *theCallback);
KTBOOL	amSoundFetchSample			(AM_BANK_PTR theBank,KTU32 soundNumber,AM_SOUND_PTR theSound);
KTBOOL 	amSoundSetEffectsBus		(AM_SOUND_PTR theSound,KTU32 dspMixerChannel,KTU32 sourceMix);
KTBOOL 	amSoundSetQSoundChannels	(KTU32 firstQChannel,KTU32 numberOfQChannels);
KTBOOL 	amSoundPlayRaw				(KTS32 voiceChannel,KTU32 sizeInBytes,KTU32 address,KTU32 sampleRate,AC_AUDIO_TYPE aicaAudioType,KTU32 pitchOffsetInCents,KTS32 aicaLoopFlag,AM_USER_CALLBACK userCallbackProc,KTU32 dryVolume,KTU32 wetVolume,KTU32 pan,KTU32 mixerChannel,KTBOOL effectsOnOrOff);
KTBOOL 	amSoundReleaseVoiceChannel	(AM_SOUND_PTR theSound);

// amStream.c, play PCM streaming audio from memory or disk
KTU32	amStreamGetIsrCount				(AM_STREAM_PTR theStream);
KTBOOL 	amStreamGetPan					(AM_STREAM_PTR theStream,KTU32 trackNumber,KTU32 *pan);
KTBOOL 	amStreamGetVolume				(AM_STREAM_PTR theStream,KTU32 trackNumber,KTU32 *volume);
KTBOOL	amStreamGetMsPerIrq				(AM_STREAM_PTR theStream,KTU32 *millisecondsPerIrq);
KTBOOL	amStreamGetTrackLengthInMs		(AM_STREAM_PTR theStream,KTU32 trackNumber,KTU32 *lengthInMs);
KTBOOL	amStreamGetTrackLengthInFrames	(AM_STREAM_PTR theStream,KTU32 trackNumber,KTU32 *trackLengthInFrames);
KTBOOL	amStreamGetNibblesPerFrame		(AM_STREAM_PTR theStream,KTU32 *nibblesPerFrame);
KTBOOL	amStreamGetSampleRate			(AM_STREAM_PTR theStream,KTU32 *sampleRate);
KTBOOL	amStreamGetMemoryRequirement	(AM_STREAM_PTR theStream,KTU32 *transferBufferSize,KTU32 *playBufferSize);
KTBOOL	amStreamSetVolume				(AM_STREAM_PTR theStream,KTU8 newVolume,KTU32 trackNumber);
KTBOOL	amStreamSetPan					(AM_STREAM_PTR theStream,KTU8 newPan,KTU32 trackNumber);
KTBOOL	amStreamSetMix					(AM_STREAM_PTR theStream,AM_STREAM_MIX_PTR theMix);
KTBOOL	amStreamSetStereoBuffers		(AM_STREAM_PTR theStream,KTU32 leftPlayBuffer,KTU32 rightPlayBuffer,KTU32 playBufferSize,KTU32 transferBuffer,KTU32 transferBufferSize);
KTBOOL	amStreamSetMonoBuffers			(AM_STREAM_PTR theStream,KTU32 monoPlayBuffer,KTU32 playBufferSize,KTU32 transferBuffer,KTU32 transferBufferSize);
KTBOOL	amStreamSetBufferSizes			(AM_STREAM_PTR theStream,KTU32 transferBufferSize,KTU32 playBufferSize);
KTBOOL	amStreamSetTransferMethod		(AM_STREAM_PTR theStream,AM_STREAM_TRANSFER_METHOD transferMethod,KTU32 dmaChannel,KTU32 dmaFrameSize);
KTBOOL	amStreamSetIsr					(AM_STREAM_PTR theStream,AM_STREAM_ISR theIsr);
KTBOOL	amStreamInstallUserCallback		(AM_STREAM_PTR theStream,AM_USER_CALLBACK userCallback);
KTBOOL	amStreamPrimeBuffers			(AM_STREAM_PTR theStream);
KTBOOL	amStreamAllocateVoiceChannels	(AM_STREAM_PTR theStream);
KTBOOL	amStreamServer					(AM_STREAM_PTR theStream);
KTBOOL	amStreamStart					(AM_STREAM_PTR theStream);
KTBOOL	amStreamStop					(AM_STREAM_PTR theStream);
KTBOOL	amStreamPlaying					(AM_STREAM_PTR theStream);
KTBOOL	amStreamClose					(AM_STREAM_PTR theStream);
KTBOOL	amStreamOpen					(AM_STREAM_PTR theStream);
KTBOOL	amStreamIsStereo				(AM_STREAM_PTR theStream,KTU32 trackNumber);
KTBOOL	amStreamIsMono					(AM_STREAM_PTR theStream,KTU32 trackNumber);
KTBOOL 	amStreamInitFile				(AM_STREAM_PTR theStream,KTSTRING fileName);
KTBOOL 	amStreamInitBuffer				(AM_STREAM_PTR theStream,volatile KTU32 *buffer,KTU32 size,KTU32 sampleRate,KTU32 bitDepth,AM_TRACK_TYPE monoOrStereo);
KTBOOL 	amStreamSetMix					(AM_STREAM_PTR theStream,AM_STREAM_MIX_PTR theMix);
KTBOOL 	amStreamReleaseVoiceChannels	(AM_STREAM_PTR theStream);

// amIsr.c, ISR's for streaming audio
void _amStreamIsr0(KTU32 streamPtr); 
void _amStreamIsr1(KTU32 streamPtr); 
void _amStreamIsr2(KTU32 streamPtr); 
void _amStreamIsr3(KTU32 streamPtr); 
void _amStreamIsr4(KTU32 streamPtr); 

// amMem.c, allocate sh4 memory via a redirectable shell
KTBOOL	amMemSh4Alloc					(volatile KTU32 ** base,volatile KTU32 ** aligned,KTU32 size,KTU32 alignment);
void	amMemSh4Free					(volatile KTU32 * block);
void	amMemInit						(void);
void	amMemInstallAlternateMemoryManager(AM_SH4_ALLOC_PROC allocProc,AM_SH4_FREE_PROC freeProc);

// amUtil.c, general utility functions
KTBOOL	amUtilGetNibblesPerFrame		(AC_AUDIO_TYPE type,KTU32 *nibblesPerFrame);
KTBOOL	amUtilGetLengthInMs				(AC_AUDIO_TYPE type,KTU32 channels,KTU32 size,KTU32 aicaSampleRate,KTU32 *lengthInMs);
KTBOOL	amUtilGetMsPerIrq				(AC_AUDIO_TYPE type,KTS32 aicaSampleRate,KTU32 playBufferSizeInBytes,KTU32 *msPerIrq);
KTBOOL	amUtilGetLengthInFrames			(AC_AUDIO_TYPE type,KTU32 channels,KTU32 size,KTU32 *lengthInFrames);
KTBOOL	amUtilGetAicaSampleType			(KTU32 bitDepth,AC_AUDIO_TYPE_PTR aicaSampleType);
KTBOOL 	amUtilGetEndOfBufferInFrames	(KTU32 bitDepthOfSample,KTU32 sizeOfBufferInBytes,KTU32 * endInFrames);
KTBOOL 	amUtilGetMiddleOfBufferInFrames	(KTU32 bitDepthOfSample,KTU32 sizeOfBufferInBytes,KTU32 * middleInFrames);
KTBOOL	amUtilGetAicaSampleRate			(KTU32 realWorldSampleRate,KTS32 *aicaSampleRate);
KTBOOL	amUtilGetSampleRate				(KTU32 aicaSampleRate,KTU32 *sampleRate);
KTBOOL	amUtilGetAicaSampleType			(KTU32 bitDepth,AC_AUDIO_TYPE_PTR aicaSampleType);
KTBOOL	amUtilGetAicaVolume				(KTU32 midiVolume,KTU32 *aicaVolume);
KTBOOL	amUtilAlignNumber				(KTU32 theNumber,KTU32 theAlignment,KTU32 *theResult);

// amMidi.c, play type 0 SMF files from a katbank
KTBOOL	amMidiLoadToneBank				(KTSTRING fileName,KTS32 gmMode,volatile KTU32 *buffer,KTU32 bankSize,KTU8 toneBankSlot);
KTBOOL	amMidiTransferToneBank			(volatile KTU32 *destination,KTU32 *source,KTS32 gmMode,KTU32 bankSize,KTU8 toneBankSlot);

KTBOOL	amMidiFetchSequence				(AM_BANK_PTR theBank,KTU32 sequenceNumber,AM_SEQUENCE_PTR theSequence);
KTBOOL	amMidiFetchToneBank				(AM_BANK_PTR theBank,KTU32 assetNumber,KTU8 toneBankSlot);

KTBOOL	amMidiStop						(AM_SEQUENCE_PTR theSequence);
KTBOOL	amMidiPlay						(AM_SEQUENCE_PTR theSequence);
KTBOOL	amMidiResume					(AM_SEQUENCE_PTR theSequence);
KTBOOL	amMidiPause						(AM_SEQUENCE_PTR theSequence);
KTBOOL  amMidiAllocateSequencePort		(AM_SEQUENCE_PTR theSequence);
KTBOOL	amMidiSetVolume					(AM_SEQUENCE_PTR theSequence,KTU32 newVolume);
KTBOOL	amMidiInstallCallback			(AM_SEQUENCE_PTR theSequence,AM_MIDI_CALLBACK theCallback);
KTBOOL	amMidiSetLoopFlag				(AM_SEQUENCE_PTR theSequence,KTBOOL onOrOff);
KTBOOL	amMidiSetTempo					(AM_SEQUENCE_PTR theSequence,KTS32 percentOfChange);

KTBOOL	amMidiNoteOff					(KTU32 midiPort,KTU32 midiChannel,KTU8  midiNoteNumber);
KTBOOL	amMidiNoteOn					(KTU32 midiPort,KTU32 midiChannel,KTU8  midiNoteNumber,KTU32 midiNoteOnVelocity);
KTBOOL	amMidiSetChannelProgram			(KTU32 midiPort,KTU32 midiChannel,KTU32 midiProgramNumber);
KTBOOL	amMidiSetChannelVolume			(KTU32 midiPort,KTU32 midiChannel,KTU32 midiVolume);
KTBOOL	amMidiSetChannelPan				(KTU32 midiPort,KTU32 midiChannel,KTU32 midiPan);
KTBOOL	amMidiAllocateSoundEffectPort	(KTU32 *voiceChannel,KTU32 *midiPort);
KTBOOL	amMidiPlayRaw					(KTU32 midiPort,KTS32 gmMode,KTU32 ticksPQN,KTU32 loop,KTU32 sequenceSize,KTU32 *sequenceAddress,KTU32 midiVolume,AM_MIDI_CALLBACK callback);

// amio.c, redirectable file system
KTBOOL	amFileRead						(ACFILE fd,KTU8 * buffer,KTU32 size);
KTBOOL	amFileOpen						(KTSTRING fileName,ACFILE *fd);
KTBOOL	amFileClose						(ACFILE fd);
KTBOOL	amFileGetSize					(KTSTRING fileName, KTU32 * size);
KTBOOL	amFileLoad						(KTSTRING fileName,KTU8 * buffer);
//void	amFileIoSystemInit					(void);
void	amFileInstallAlternateIoManager	(AM_IO_PROC ioProc);

// amBank.c, manage assets packed in katbanks
KTBOOL	amBankLoad					(KTSTRING fileName,AM_BANK_PTR buffer);
KTBOOL	amBankFetchAsset			(AM_BANK_PTR theBank,AM_BANK_FILE_UNION_PTR parameters,KTU32 assetNumber,KTU32 **theAsset,KTU32 *assetSize);
KTBOOL	amBankFetchAssetParameters	(AM_BANK_PTR theBank,KTU32 assetNumber,AM_BANK_FILE_UNION_PTR parameters);
KTBOOL	amBankFetchUnknownParameters(AM_BANK_PTR theBank,KTU32 assetNumber,KTU32 parameterNumber,KTS32 *parameterValue);
KTBOOL	amBankFetchWaveBitDepth		(AM_BANK_PTR theBank,KTU32 assetNumber,KTU32 *bitDepth);
KTBOOL	amBankFetchWaveSampleRate	(AM_BANK_PTR theBank,KTU32 assetNumber,KTU32 *sampleRate);
KTBOOL	amBankFetchWaveRandomPitch	(AM_BANK_PTR theBank,KTU32 assetNumber,KTU32 *randomPitchAmount);
KTBOOL	amBankFetchWaveLoopFlag		(AM_BANK_PTR theBank,KTU32 assetNumber,KTBOOL *loopFlag);
KTBOOL	amBankFetchMidiGmModeFlag	(AM_BANK_PTR theBank,KTU32 assetNumber,KTS32 *gmModeFlag);
KTBOOL	amBankFetchMidiVolume		(AM_BANK_PTR theBank,KTU32 assetNumber,KTU32 *masterVolume);
KTBOOL	amBankFetchMidiPpqn			(AM_BANK_PTR theBank,KTU32 assetNumber,KTU32 *ppqn);
KTBOOL	amBankFetchMidiLoop			(AM_BANK_PTR theBank,KTU32 assetNumber,KTU32 *loop);
KTBOOL	amBankFetchMidiUspqn		(AM_BANK_PTR theBank,KTU32 assetNumber,KTU32 *uspqn);
KTBOOL	amBankGetAssetSize			(AM_BANK_PTR theBank,KTU32 assetNumber,KTU32 *assetSize);
KTBOOL	amBankGetHeaderSize			(AM_BANK_PTR theBank,KTU32 *headerSize);
KTBOOL	amBankGetNumberOfAssets		(AM_BANK_PTR theBank,KTU32 *numberOfAssets);

// amstreamio.c, redirectable file system used by the stream API
void	amStreamIoInstallAlternateIoManager	(AM_STREAM_IO_PROC ioProc);
void 	amStreamIoSystemInit				(void);

// amstreamio.c internals
KTBOOL	_amStreamIoClose			(ACFILE sd);
KTBOOL	_amStreamIoOpen				(KTSTRING fileName,ACFILE *sd);
KTBOOL	_amStreamIoRead				(ACFILE sd,KTU8 * buffer,KTU32 size);
KTBOOL	_amStreamIoRewind			(ACFILE sd);
KTBOOL 	_amStreamIoGetSize			(KTSTRING fileName, KTU32 * size);

// amdma.c, DMA transfer calls used by the stream API
KTBOOL	amDmaMemCpy	(KTU32 *target, KTU32 *source, KTU32 size,KTU32 bytesPerTransfer,KTU32 dmaChannel);

// amHeap.c, audio memory management functionality
KTBOOL	amHeapInit			(volatile KTU32 *memoryStartAddress,KTU32 size);
void 	amHeapShutdown		(void);
KTBOOL	amHeapAlloc			(volatile KTU32 **buffer,KTU32 size,KTU32 alignment,AM_HEAP_MEMORY_TYPE memoryType,AM_MEMORY_CALLBACK callback);
KTBOOL	amHeapFree			(volatile KTU32 *buffer);
KTBOOL	amHeapClear			(AM_HEAP_MEMORY_TYPE memoryType);
KTBOOL	amHeapPurge			(KTU32 sizeNeeded);
KTBOOL	amHeapGetFree		(KTU32 *freeMemory);
KTBOOL	amHeapGetInfo		(volatile KTU32 **freeSoundMemory,KTU32 *size);
KTBOOL	amHeapGetMaxPurgable(KTU32 *maxPurgable);
KTBOOL	amHeapCheck			(void);
KTBOOL	_amHeapCheck		(void);
void	amHeapShow			(void);

// amDsp.c
KTBOOL amDspFetchProgramBank(AM_BANK_PTR theBank,KTU32 assetNumber);
KTBOOL amDspFetchOutputBank(AM_BANK_PTR theBank,KTU32 assetNumber);

// amInit.c
void 	amShutdown			(void);
KTBOOL 	amInit				(KTU32 *driverImage,KTU32 driverSize);
KTBOOL 	amInitSelectDriver	(AM_DRIVER_TYPE driverType);
char *	amInitGetDriverFileName(void);

// amVoice.c, allocates voices for MIDI and DA playback
void	amVoiceCheck						(void);
void	amVoiceInit							(void);
KTBOOL	amVoiceAllocate						(KTU32 * voiceChannel,AM_VOICE_TYPE voiceType,void * owner);

// amVoice.c internals
void	_amVoiceInstallUserCallback			(KTU32 voiceChannel,AM_USER_CALLBACK userCallbackProc);
void	_amVoiceSetChannelFlag				(KTU32 voiceChannel,KTBOOL trueOrFalse);
void	_amVoiceDefaultCallbackProc			(volatile KTU32 voiceChannel);
void	_amVoiceDefaultSystemCallbackProc	(KTU32 voiceChannel);
void	_amVoiceSetStreamIsr				(KTU32 voiceChannel,AM_STREAM_ISR streamIsr);
void	_amVoiceIssueOneShotSystemCallback	(KTU32 voiceChannel);
void	_amVoiceIssueOneShotUserCallback	(KTU32 voiceChannel);
void	_amVoiceClearChannel				(KTU32 voiceChannel);

// allows liberal use of audio heapCheck() function for debug.
#ifdef DEBUG
#define amHeapCheck()	_amHeapCheck()
#else
#define amHeapCheck()	((void)0)
#endif

#ifdef __cplusplus
}
#endif // end of #ifdef __cplusplus


#endif // end of #ifndef _AICA_MANAGER_H

// -------------------------------------------------------------------------
// end of file am.h
// -------------------------------------------------------------------------


