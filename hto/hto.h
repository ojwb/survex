/*	File:	hto.h		Hierarchical Tagged Objects library header
 *	Author:	Douglas P. Dotson
 *	Date:	31 May 1993
 *	Edit:	27 Aug 1993	2350	DPD
*/

/* Delimiters */

#define	OBJECT_START	'('
#define	OBJECT_END	')'
#define	SEPARATOR	':'

/* Token types */

#define TOKEN_EOF		0
#define	TOKEN_OBJECT_START	1
#define	TOKEN_OBJECT_END	2
#define	TOKEN_SEPARATOR		3
#define	TOKEN_STRING		4

/* Current token and token string buffer */

#define	TOKEN_BUFFER_SIZE	100

/* Object tags */

#define	TAG_SIZE	8
typedef	char	ObjectTag [TAG_SIZE+1];

/* Post callback function stack */

#define	CALLBACK_STACK_SIZE	100
#ifndef __TURBOC__
typedef	void	(*CallBackPtr) ();
#else
typedef	void	(*CallBackPtr) (struct htodesc *, char *);
#endif

/* Callback function list */

typedef struct cback {
    ObjectTag	Tag;
    CallBackPtr	PreFunction;
    CallBackPtr	PostFunction;
} CallBackEntry;
#define	NUM_CALLBACKS	100

/* Value stacks */

#define	NUM_VALUES	100
#define	NUM_LEVELS	50
typedef	char	*ValStack [NUM_LEVELS];

typedef struct vlentry {
    ObjectTag	Tag;
    ValStack	*VStack;
} VLEntry;
typedef	VLEntry	ValStackList [NUM_VALUES];

/* Hierarchical Tagged Object Descriptor */

typedef struct htodesc {
    FILE	*HTO_File;	/* Actual file containing HTO objects */
    CallBackEntry CallBackList [NUM_CALLBACKS];
    int		Num_CallBacks;
    CallBackPtr	GlobalPreCallBack;
    CallBackPtr	GlobalPostCallBack;
    CallBackPtr	NonTermPreCallBack;
    CallBackPtr	NonTermPostCallBack;
    CallBackPtr	TermPreCallBack;
    CallBackPtr	TermPostCallBack;
    CallBackPtr	CallBackStack [CALLBACK_STACK_SIZE];
    int		CallBackStackPtr;
    char	Token_Buffer [TOKEN_BUFFER_SIZE+1];
    int		CurrToken;
    int		NestLevel;
    ValStackList ValueStackList;
    int		Num_ValStacks;
} htodesc, *HTO;

extern	HTO	HTO_Open (char *);
extern	void	HTO_Close (HTO);
extern	void	HTO_Process (HTO);
extern	void	HTO_RegisterTagCallBack (HTO, char *, CallBackPtr, CallBackPtr);
extern	void	HTO_RegisterGlobalCallBack (HTO, CallBackPtr, CallBackPtr);
extern	void	HTO_RegisterNonTermCallBack (HTO, CallBackPtr, CallBackPtr);
extern	void	HTO_RegisterTermCallBack (HTO, CallBackPtr, CallBackPtr);
extern	void	HTO_NoCallBack (HTO, char *);
extern	char	*HTO_GetObjectValue (HTO, char *, char *);
extern	HTO	HTO_Create (char *);
extern	void	HTO_BeginObject (HTO, char *);
extern	void	HTO_EndObject (HTO);
extern	void	HTO_WriteValue (HTO, char *, char *);
extern	void	HTO_PutChar (HTO, char);
extern	void	HTO_PutStr (HTO, char *);
extern	void	HTO_NewLine (HTO);
extern	void	HTO_Finish (HTO);
