; >  s.armrot
; Copyright (C) Olly Betts 1990,1993,1994,1995,1997
;
; 1990       Original written on expo for BASIC + ARM code version
; 1993       Added assembler 'glue' to allow calling from C
; 1993.08.12 pRaw_data -> pData
; 1994.03.17 added obvious speed-up
; 1994.03.19 added my fast line drawing routines
; 1994.03.22 fixed centring for any screen mode
; 1994.03.23 tidied up initialisation code
;            added crosses
;            added some crap clipping - seems to help
; 1994.03.24 better clipping added
; 1994.03.28 pData passed rather than global
;            added separate routines to plot crosses
;            added separate routines to plot labels
; 1994.03.29 fixed scaling on labels
; 1994.04.07 fixed do_translate
; 1994.04.16 throw away labels off left and bottom (commented out)
; 1994.04.17 properly throw away labels off all sides
; 1994.09.11 recoded for 1bpp modes
; 1994.09.15 fixed point pos changed from 16 to 18
; 1994.09.23 implemented sliding point
; 1995.03.18 tweaked plot_label
; 1995.03.23 eig shifts are now done at the last moment
; 1997.05.29 STOP option is now 0 not -1
;
;***************************************************************************
;

        GET     stdh.RegNames

        GET     stdh.SWInames

; Area name C$$code advisable if wanted to link with C output

        AREA    |C$$code|, CODE
;, READONLY
; and pray the linker doesn't take that READONLY part seriously... !HACK!

; Export global symbols

        EXPORT  |plot|,  |plot_no_tilt|,  |plot_plan|
        EXPORT  |splot|, |splot_no_tilt|, |splot_plan|
        EXPORT  |lplot|, |lplot_no_tilt|, |lplot_plan|
        EXPORT  |fastline_init|, |do_trans|
        EXPORT  |really_plot|
        EXPORT  |ol_setcol|

        IMPORT  |fancy_label|
; some reg names

opt RN 0
XS  RN 1
YS  RN 2
x1  RN 3
x2  RN 4
y1  RN 5
y2  RN 6
y3  RN 7
x   RN 8
y   RN 9
z   RN 10
ptr RN 11

; r0 - r3 can be altered
; Since these are leaf fns (ie calls no others), don't set up stack frames

; The following bytes contain the name of the following procedure to
; make stack backtracing work, eg. when an address exception occurs!

        DCB     "plot", 0
        DCB     &00, &00, &00              ;align
        DCD     &ff000008

|plot|
        STMFD   sp!,{r4-r12,r14}
        BL      fl_init
        MOV     ptr,r0
        MOV     y1,r3
        MOV     x2,r2
        MOV     x1,r1
        LDR     y2,[sp,#40]
        LDR     y3,[sp,#44]
        LDR     r12,[sp,#48]
plot_loop
        LDMIA   ptr!,{opt,x,y,z}
        TEQ     opt,#0 ; exit if option is 0
        MULNE   XS,x,x1
        MLANE   XS,y,x2,XS
        MULNE   YS,x,y1
        MLANE   YS,y,y2,YS
        MLANE   YS,z,y3,YS
        MOVNE   XS,XS,ASR r12
        MOVNE   YS,YS,ASR r12
        BLNE    ol_plot
        BNE     plot_loop
        LDMFD   sp!,{r4-r12,pc}^

|splot|
        STMFD   sp!,{r4-r12,r14}
        BL      fl_init
        MOV     ptr,r0
        MOV     y1,r3
        MOV     x2,r2
        MOV     x1,r1
        LDR     y2,[sp,#40]
        LDR     y3,[sp,#44]
        LDR     r12,[sp,#48]
splot_loop
        LDMIA   ptr!,{opt,x,y,z}
        TEQ     opt,#0 ; exit if option is 0
        MULNE   XS,x,x1
        MLANE   XS,y,x2,XS
        MULNE   YS,x,y1
        MLANE   YS,y,y2,YS
        MLANE   YS,z,y3,YS
        MOVNE   XS,XS,ASR r12
        MOVNE   YS,YS,ASR r12
        BLNE    plot_cross
        BNE     splot_loop
        LDMFD   sp!,{r4-r12,pc}^

|lplot|
        STMFD   sp!,{r4-r12,r14}
        BL      fl_init
        MOV     ptr,r0
        MOV     y1,r3
        MOV     x2,r2
        MOV     x1,r1
        LDR     y2,[sp,#40]
        LDR     y3,[sp,#44]
        LDR     r12,[sp,#48]
;        LDR     r2,xeig
;        MOV     x1,x1,ASL r2
;        MOV     x2,x2,ASL r2
;        LDR     r2,yeig
;        MOV     y1,y1,ASL r2
;        MOV     y2,y2,ASL r2
;        MOV     y3,y3,ASL r2
lplot_loop
        LDMIA   ptr!,{opt,x,y,z}
        TEQ     opt,#0 ; exit if option is 0
        MULNE   XS,x,x1
        MLANE   XS,y,x2,XS
        MULNE   YS,x,y1
        MLANE   YS,y,y2,YS
        MLANE   YS,z,y3,YS
        MOVNE   XS,XS,ASR r12
        MOVNE   YS,YS,ASR r12
        BLNE    plot_label
        BNE     lplot_loop
        LDMFD   sp!,{r4-r12,pc}^

        DCB     "plot_no_tilt", 0
        DCB     &00, &00, &00              ;align
        DCD     &ff000010

|plot_no_tilt|
        STMFD   sp!,{r4-r12,r14}
        BL      fl_init
        MOV     ptr,r0
        MOV     y3,r3
        MOV     x2,r2
        MOV     x1,r1
        LDR     r12,[sp,#40]
plot_no_tilt_loop
        LDMIA   ptr!,{opt,x,y,z}
        TEQ     opt,#0 ; exit if option is 0
        MULNE   XS,x,x1
        MLANE   XS,y,x2,XS
        MULNE   YS,z,y3
        MOVNE   XS,XS,ASR r12
        MOVNE   YS,YS,ASR r12
        BLNE    ol_plot
        BNE     plot_no_tilt_loop
        LDMFD   sp!,{r4-r12,pc}^

|splot_no_tilt|
        STMFD   sp!,{r4-r12,r14}
        BL      fl_init
        MOV     ptr,r0
        MOV     y3,r3
        MOV     x2,r2
        MOV     x1,r1
        LDR     r12,[sp,#40]
splot_no_tilt_loop
        LDMIA   ptr!,{opt,x,y,z}
        TEQ     opt,#0 ; exit if option is 0
        MULNE   XS,x,x1
        MLANE   XS,y,x2,XS
        MULNE   YS,z,y3
        MOVNE   XS,XS,ASR r12
        MOVNE   YS,YS,ASR r12
        BLNE    plot_cross
        BNE     splot_no_tilt_loop
        LDMFD   sp!,{r4-r12,pc}^

|lplot_no_tilt|
        STMFD   sp!,{r4-r12,r14}
        BL      fl_init
        MOV     ptr,r0
        MOV     y3,r3
        MOV     x2,r2
        MOV     x1,r1
        LDR     r12,[sp,#40]
;        LDR     r2,xeig
;        MOV     x1,x1,ASL r2
;        MOV     x2,x2,ASL r2
;        LDR     r2,yeig
;        MOV     y3,y3,ASL r2
lplot_no_tilt_loop
        LDMIA   ptr!,{opt,x,y,z}
        TEQ     opt,#0 ; exit if option is 0
        MULNE   XS,x,x1
        MLANE   XS,y,x2,XS
        MULNE   YS,z,y3
        MOVNE   XS,XS,ASR r12
        MOVNE   YS,YS,ASR r12
        BLNE    plot_label
        BNE     lplot_no_tilt_loop
        LDMFD   sp!,{r4-r12,pc}^

        DCB     "plot_plan", 0
        DCB     &00, &00              ;align
        DCD     &ff00000c

|plot_plan|
        STMFD   sp!,{r4-r12,r14}
        BL      fl_init
        MOV     ptr,r0
        MOV     y1,r3
        MOV     x2,r2
        MOV     x1,r1
        LDR     y2,[sp,#40]
        LDR     r12,[sp,#44]
plot_plan_loop
        LDMIA   ptr!,{opt,x,y,z}
        TEQ     opt,#0 ; exit if option is 0
        MULNE   XS,x,x1
        MLANE   XS,y,x2,XS
        MULNE   YS,x,y1
        MLANE   YS,y,y2,YS
        MOVNE   XS,XS,ASR r12
        MOVNE   YS,YS,ASR r12
        BLNE    ol_plot
        BNE     plot_plan_loop
        LDMFD   sp!,{r4-r12,pc}^

|splot_plan|
        STMFD   sp!,{r4-r12,r14}
        BL      fl_init
        MOV     ptr,r0
        MOV     y1,r3
        MOV     x2,r2
        MOV     x1,r1
        LDR     y2,[sp,#40]
        LDR     r12,[sp,#44]
splot_plan_loop
        LDMIA   ptr!,{opt,x,y,z}
        TEQ     opt,#0 ; exit if option is 0
        MULNE   XS,x,x1
        MLANE   XS,y,x2,XS
        MULNE   YS,x,y1
        MLANE   YS,y,y2,YS
        MOVNE   XS,XS,ASR r12
        MOVNE   YS,YS,ASR r12
        BLNE    plot_cross
        BNE     splot_plan_loop
        LDMFD   sp!,{r4-r12,pc}^

|lplot_plan|
        STMFD   sp!,{r4-r12,r14}
        BL      fl_init
        MOV     ptr,r0
        MOV     y1,r3
        MOV     x2,r2
        MOV     x1,r1
        LDR     y2,[sp,#40]
        LDR     r12,[sp,#44]
;        LDR     r2,xeig
;        MOV     x1,x1,ASL r2
;        MOV     x2,x2,ASL r2
;        LDR     r2,yeig
;        MOV     y1,y1,ASL r2
;        MOV     y2,y2,ASL r2
lplot_plan_loop
        LDMIA   ptr!,{opt,x,y,z}
        TEQ     opt,#0 ; exit if option is 0
        MULNE   XS,x,x1
        MLANE   XS,y,x2,XS
        MULNE   YS,x,y1
        MLANE   YS,y,y2,YS
        MOVNE   XS,XS,ASR r12
        MOVNE   YS,YS,ASR r12
        BLNE    plot_label
        BNE     lplot_plan_loop
        LDMFD   sp!,{r4-r12,pc}^

        DCB     "do_trans", 0
        DCB     &00, &00, &00              ;align
        DCD     &ff00000C

|do_trans|
        STMFD   sp!,{x,y,z,r14}
        MOV     r14,r0
do_trans_loop
        LDMIA   r14,{r0,x,y,z}
        TEQ     r0,#0 ; exit if option is 0 (which NULL pointer on ARM)
        ADDNE   x,x,r1
        ADDNE   y,y,r2
        ADDNE   z,z,r3
        ADDNE   r14,r14,#4 ; option is unchanged, so skip it
        STMNEIA r14!,{x,y,z}
        BNE     do_trans_loop
        LDMFD   sp!,{x,y,z,pc}^

;ppData
;        IMPORT  pData
;        DCD     pData

X0   RN 0
Y0   RN 1
X1   RN 2
Y1   RN 3
col  RN 4
dX   RN 5
dY   RN 6
D    RN 7
c    RN 8
llen RN 9
Ymin RN 10
Ymax RN 11
Xmax RN 12

blk_in
        DCD   6
        DCD 148
        DCD   7
        DCD  11
        DCD  12
        DCD   9
        DCD   0
        DCD   4
        DCD   5
        DCD  -1

blk_out
LLen     DCD 0 ; llen  r9
pScrn    DCD 0 ; Ymin r10
ScrnLen  DCD 0 ; Ymax r11 (becomes pScrnMax = ptr to bottom right pixel)
Width    DCD 0 ; Xmax r12
Depth    DCD 0 ;      r14
Log2BPP  DCD 0
ModeFlgs DCD 0
xeig     DCD 0
yeig     DCD 0

xclabmax DCD 0
yclabmax DCD 0

; initialisation code
|fastline_init|
; may corrupt r0-r3
        MOV     r3,r14
        BL      fl_init
; make sure we're in a 256 colour graphics mode...
        LDR     r14,ModeFlgs
        TST     r14,#7 ;%111
        LDREQ   r14,Log2BPP
        TEQEQ   r14,#3 ; ie 8bpp
        MOVEQ   pc,r3
        ADR     r0,badmode
        SWI     OS_GenerateError

badmode DCD 0
        = "Sorry, not in a 256 colour graphics mode",0
        ALIGN

fl_init
        STMFD   r13!,{r0-r1,r14}
        ADR     r0,blk_in
        ADR     r1,blk_out
        SWI     OS_ReadVduVariables
; change screen bank size entry to be a ptr to the bottom-right pixel
        LDR     r0,pScrn
        LDR     r1,ScrnLen
        ADD     r0,r0,r1
        SUB     r0,r0,#1
        STR     r0,ScrnLen
        LDR     r0,Width
;        LDR     r1,xeig
        ADD     r0,r0,#1
;        MOV     r0,r0,ASL r1
        MOV     r0,r0,LSR#1
        STR     r0,xclabmax
        LDR     r0,Depth
;        LDR     r1,yeig
        ADD     r0,r0,#1
;        MOV     r0,r0,ASL r1
        MOV     r0,r0,LSR#1
        STR     r0,yclabmax
        LDMFD   r13!,{r0-r1,pc}

last_x  DCD 0
last_y  DCD 0

plot_cross
        STMFD   r13!,{r1-r2,r14}
        SUB     r1,r1,#2
        SUB     r2,r2,#2
        STR     r1,last_x
        STR     r2,last_y
        ADD     r1,r1,#4
        ADD     r2,r2,#4
        BL      ol_draw
        SUB     r1,r1,#4
        STR     r1,last_x
        STR     r2,last_y
        ADD     r1,r1,#4
        SUB     r2,r2,#4
        BL      ol_draw
        SUB     r1,r1,#2
        ADD     r2,r2,#2 ; and fall thru'
        STR     r1,last_x
        STR     r2,last_y
        LDMFD   r13!,{r1-r2,pc}^

; /E r0 -> label (or NULL); r1,r2 are coordinates - (0,0) is centre of screen
; /X corrupts r0-r2,r8-r10 (opt, XS, YS, x, y, z)
plot_label
        MOV     r8,r14
        MOVS    r14,r1
        RSBMI   r14,r14,#0    ; r14=ABS(X)
        LDR     r9,xclabmax
        CMP     r14,r9
        MOVGTS  pc,r8
        MOVS    r14,r2
        RSBMI   r14,r14,#0    ; r14=ABS(Y)
        LDR     r9,yclabmax
        CMP     r14,r9
        MOV     r10,r12
        MOV     r9,r3
        BLLT    fancy_label
        MOV     r3,r9
        MOV     r12,r10
;        STR     r1,last_x
;        STR     r2,last_y
;        MOVNE   r1,r1,ASL #2 ; !HACK!
;        MOVNE   r2,r2,ASL #2 ; !HACK!
        MOVS    pc,r8

really_plot
        MOVS    r12,r0
        LDRNE   r0,xeig
        MOVNE   r1,r1,ASL r0
        LDRNE   r0,yeig
        MOVNE   r2,r2,ASL r0
        MOVNE   r0,#4
        SWINE   OS_Plot
        MOVNE   r0,r12
        SWINE   OS_Write0
        MOVS    pc,r14

fastmove
        STR     r1,last_x
        STR     r2,last_y
        MOVS    pc,r14
;
label
        = "Label",0
        ALIGN

ol_setcol
        STR r0,colour
        MOVS pc,r14

colour
        DCD 31
;
ol_plot
;        TEQ     r0,#69
;        BEQ     plot_cross
        TEQ     r0,#4
        BEQ     fastmove
ol_draw
        STMFD   r13!,{r1-r7,r11,r12,r14}
        MOV     r0,r1
        MOV     r1,r2
        LDR     r2,last_x
        LDR     r3,last_y
        STR     r0,last_x
        STR     r1,last_y

        ADR     llen,LLen
        LDMIA   llen,{llen,Ymin,Ymax,Xmax,r14} ; r14 is the width in pixels-1

        ADD     r2,r2,Xmax,LSR #1 ; add on half screen width
        ADDS    r0,r0,Xmax,LSR #1 ; add on half screen width
;        CMP     X0,#0
        CMPLT   X1,#0
        LDMLTFD r13!,{r1-r7,r11,r12,pc}^ ; off the bottom

        RSB     r3,r3,r14,LSR #1 ; subtract from half screen depth
        RSBS    r1,r1,r14,LSR #1 ; subtract from half screen depth
;        CMP     Y0,#0
        CMPLT   Y1,#0
        LDMLTFD r13!,{r1-r7,r11,r12,pc}^ ; off the left

        CMP     X0,Xmax
        CMPGT   X1,Xmax
        LDMGTFD r13!,{r1-r7,r11,r12,pc}^ ; off the top

        CMP     Y0,r14
        CMPGT   Y1,r14
        LDMGTFD r13!,{r1-r7,r11,r12,pc}^ ; off the right

        LDR     col,colour
;
        SUBS    dX,X1,X0
        RSBMI   dX,dX,#0    ; dX=abs(dX)
        MVNMI   X1,#0 ;NOT(-1) ; X1 is now step for X
        MOVPL   X1,#1
;
        SUBS    dY,Y1,Y0
        RSBMI   dY,dY,#0    ; dY=abs(dY)
        RSBMI   Y1,llen,#0  ; Y1 is now step for Y address
        MOVPL   Y1,llen
        MLA     c,Y0,llen,Ymin
        MOV     Y0,c ; sigh
;
        CMP     dX,dY
        BLT     steepline   ; branch if line > 45 degrees
;
        RSB     D,dX,#0     ; D=-dX
        SUB     c,dX,#1     ; counter
;
; first loop counts until line is on screen
loop
        CMP     X0,#0
        CMPGE   Y0,Ymin
        CMPGE   Xmax,X0
        CMPGE   Ymax,Y0
        BGE     on
;
        ADDS    D,D,dY,ASL #1
        SUBGE   D,D,dX,ASL #1
        ADDGE   Y0,Y0,Y1
        ADD     X0,X0,X1
;
        SUBS    c,c,#1
        BPL     loop
        LDMFD   r13!,{r1-r7,r11,r12,pc}^

;when we get to on, D = [ dX + |dist moved|*2.dY ] mod 2.dX - 2.dX
; or -dX if we start on screen

; line is now on screen - loop until end of line or line goes off screen
on
        STRB    col,[Y0,X0]
loop1
        ADDS    D,D,dY,ASL #1
        SUBGE   D,D,dX,ASL #1
        ADDGE   Y0,Y0,Y1
        ADDS    X0,X0,X1

        ; CMP     X0,#0 ; unneeded
        CMPGE   Y0,Ymin
        CMPGE   Xmax,X0
        CMPGE   Ymax,Y0

        STRGEB  col,[Y0,X0]

        SUBGES  c,c,#1
        BGE     loop1
        LDMFD   r13!,{r1-r7,r11,r12,pc}^

steepline
        RSB     D,dY,#0    ; D=-dY
        SUB     c,dY,#1    ; counter

; first loop counts until line is on screen
stloop
        CMP     X0,#0
        CMPGE   Y0,Ymin
        CMPGE   Xmax,X0
        CMPGE   Ymax,Y0
        BGE     ston

        ADDS    D,D,dX,ASL #1
        SUBGE   D,D,dY,ASL #1
        ADDGE   X0,X0,X1
        ADD     Y0,Y0,Y1

        SUBS    c,c,#1
        BPL     stloop
        LDMFD   r13!,{r1-r7,r11,r12,pc}^

; line is now on screen - loop until end of line or line goes off screen
ston
        STRB    col,[Y0,X0]
stloop1
        ADDS    D,D,dX,ASL #1
        SUBGE   D,D,dY,ASL #1
        ADD     Y0,Y0,Y1
        ADDGE   X0,X0,X1

        CMP     X0,#0
        CMPGE   Y0,Ymin
        CMPGE   Xmax,X0
        CMPGE   Ymax,Y0

        STRGEB  col,[Y0,X0]

        SUBGES  c,c,#1
        BGE     stloop1
        LDMFD   r13!,{r1-r7,r11,r12,pc}^

        END
