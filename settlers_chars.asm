// 10 sys2061
*=$0801
	.byte $0b, $08, $0a, $00, $9e, $32, $30, $36, $31, $00, $00, $00

*=$080d
	jmp start

	// this sub is allocating a temp char
	// jsr getTempChar
	//     lda tempCharID
	//     sta $400
	//     lda tempCharLow
	//     sta testChar+1
	//     lda tempCharHigh
	//     sta testChar+2
	//     lda #$ff
	// testChar: sta $ffff
 
	//.import source "getTempChar.asm"
	//.import source "copyChar.asm"
start:
	// set to 25 line text mode and turn on the screen
	lda #$1b
	sta $d011

	// disable shift-commodore
	lda #$80
	sta $0291

	// set screen memory ($0400) and charset bitmap offset ($2000)
	lda #$18
	sta $d018

	// set border color
	lda #$00
	sta $d020

	// set background color
	lda #$06
	sta $d021

	// set text color
	lda #$0e
	sta $0286

    //  clear the screen
       jsr $e544 //.c:670d  20 44 e5   
.var road = $2000
.var csp1 = $2000 + $30*8
.var csp1origin = $2000 + $ff*8  //the temp char for sp1 from where the csp started
.var csp1right  = $2000 + $fe*8  //the temp char for sp1 to where will go to the right

	jmp load_map
// copy the original to the tempCharLow/High address
// set theOriginalChar
// set theTempCharID
copyChar:				jmp copyTheOriginalChar
							copyTheOriginalCharBackupX:
							.byte 0
							copyTheOriginalCharBackupY:
							.byte 0
							theOriginalChar:
							.byte 0
	copyTheOriginalChar:    stx copyTheOriginalCharBackupX
							sty copyTheOriginalCharBackupY
						
							ldx theOriginalChar 
							lda default_charset_low_map,x
							sta copy+1
							lda default_charset_high_map,x
							sta copy+2

							ldx tempCharID
							lda default_charset_low_map,x
							sta paste+1
							lda default_charset_high_map,x
							sta paste+2
							ldy #0
	copyLoop:				
	copy:					lda $ffff,y
	paste:					sta $ffff,y
							iny
							cpy #8
							bne copyLoop
							ldx copyTheOriginalCharBackupX
							ldy copyTheOriginalCharBackupY
						rts


// cspdata
cspX:
.byte 30
cspY:
.byte 4
cspDirection:
.byte 2
cspOriginX:
.byte 0
cspOriginY:
.byte 0
cspTargetX:
.byte 0
cspTargetY:
.byte 0
cspStage:
.byte 2


// this loop loads the data for the csp loads the chars
//init
cspInit:
						lda cspDirection
						sta $fb

						// for test, draw a path
						lda cspX
						sta set_x+1
						lda cspY
						sta set_y+1
						lda #0
						sta set_char+1
						jsr setXYonScreen
						
	//					jsr getTempChar
						// lda tempCharLow
						// lda tempCharHigh

						// get the original character in the csp position
						lda cspX
						sta get_x+1
						lda cspY
						sta get_y+1
						jsr getXYonScreen
						sta theOriginalChar

						jsr copyChar
					
						// for test, draw the copied char
						lda #32
						sta set_x+1
						lda cspY
						sta set_y+1
						lda tempCharID
						sta set_char+1
						jsr setXYonScreen

						// the background char of the origin what will be merged with the settler
						lda #<road
						sta mergeSettlerWithOrigin+1
						lda #>road
						sta mergeSettlerWithOrigin+2
						
						// the background char of the target what will be merged with the settler
						lda #<road
						sta mergeSettlerWithTarget+1
						lda #>road
						sta mergeSettlerWithTarget+2
						
						//the bitmap address of the settler
						lda #<csp1
						sta charStageLoop+1
						lda #>csp1
						sta charStageLoop+2
						
						// storeToOriginTemp = the temp char assigned to the staging Origin
						lda #<csp1origin
						sta storeToOriginTemp+1
						lda #>csp1origin
						sta storeToOriginTemp+2

						// storeToTargetTemp = the temp char assigned to the staging Target
						lda #<csp1right
						sta storeToTargetTemp+1
						lda #>csp1right
						sta storeToTargetTemp+2
rts
cspStageLoop:  			ldx #$00
						

charStageLoop:			lda csp1 // load the settler bitmap line - I setup this value during the init phase and inc after a stage finished
						tay //y=the bitmap line number (0-7)	
charStageOrigin:				
	stageOrigin0:				lda cspStage
								cmp #0
								bne stageOrigin1
								lda $fb  
								cmp #2 //right
								bne stageOrigin0Left
		stageOrigin0Right:		tya
							jmp drawOrigin
		stageOrigin0Left:		cmp #1
								bne stageOrigin1
								tya
							jmp drawOrigin
	stageOrigin1:				lda cspStage
								cmp #1
								bne stageOrigin2
								lda $fb
								cmp #2
								bne stageOrigin1Left
		stageOrigin1Right:		tya
								lsr
							jmp drawOrigin
		stageOrigin1Left:		cmp #1
								bne stageOrigin2
								tya
								asl
							jmp drawOrigin						
	stageOrigin2:				lda cspStage
								cmp #2
								bne stageOrigin3
								lda $fb
								cmp #2
								bne stageOrigin2Left
		stageOrigin2Right:		tya
								lsr
								lsr
							jmp drawOrigin
		stageOrigin2Left:		cmp #1
								bne stageOrigin3
								tya
								asl
								asl
							jmp drawOrigin	
	stageOrigin3:				lda cspStage
								cmp #3
								bne stageOrigin4
								lda $fb
								cmp #2
								bne stageOrigin3Left
		stageOrigin3Right:		tya
								lsr
								lsr
								lsr
							jmp drawOrigin
		stageOrigin3Left:		cmp #1
								bne stageOrigin4
								tya
								asl
								asl
								asl
								jmp drawOrigin	
	stageOrigin4:				lda cspStage
								cmp #4
								bne stageOrigin5
								lda $fb
								cmp #2
								bne stageOrigin4Left
		stageOrigin4Right:		tya
								lsr
								lsr
								lsr
								lsr
							jmp drawOrigin
		stageOrigin4Left:		cmp #1
								bne stageOrigin5
								tya
								asl
								asl
								asl
								asl
							jmp drawOrigin	
	stageOrigin5:				lda cspStage
								cmp #5
								bne stageOrigin6
								lda $fb
								cmp #2
								bne stageOrigin5Left
		stageOrigin5Right:		tya
								lsr
								lsr
								lsr
								lsr
								lsr
							jmp drawOrigin
		stageOrigin5Left:		cmp #1
								bne stageOrigin6
								tya
								asl
								asl
								asl
								asl
								asl
							jmp drawOrigin	
	stageOrigin6:				lda cspStage
								cmp #6
								bne stageOrigin7
								lda $fb
								cmp #2
								bne stageOrigin6Left
		stageOrigin6Right:		tya
								lsr
								lsr
								lsr
								lsr
								lsr
								lsr
							jmp drawOrigin
		stageOrigin6Left:		cmp #1
								bne stageOrigin7
								tya
								asl
								asl
								asl
								asl
								asl
								asl
							jmp drawOrigin	
	stageOrigin7:				lda cspStage
								cmp #7
								bne drawOrigin
								lda $fb
								cmp #2
								bne stageOrigin7Left
		stageOrigin7Right:		tya
								lsr
								lsr
								lsr
								lsr
								lsr
								lsr
								lsr
							jmp drawOrigin
		stageOrigin7Left:		cmp #1
								bne drawOrigin
								tya
								asl
								asl
								asl
								asl
								asl
								asl
								asl
						jmp drawOrigin			
drawOrigin:
	// at this point a=the csp data rotated right stage_data times, x=0-7, y= original csp1
mergeSettlerWithOrigin:		ora road
storeToOriginTemp:			sta csp1origin

charStageTarget:					
								lda cspStage
								cmp #0
								bne stageTarget1
	stageTarget0:				lda $fb
								cmp #2
								bne stageTarget0Left
		stageTarget0Right:    	lda #0
							jmp mergeSettlerWithTarget
		stageTarget0Left:		cmp #1
								bne stageTarget1
								lda #0
							jmp mergeSettlerWithTarget
	stageTarget1:
								lda cspStage
								cmp #1
								bne stageTarget2
								lda $fb
								cmp #2
								bne stageTarget1Left
		stageTarget1Right:		tya 
								asl
								asl
								asl
								asl
								asl
								asl
								asl
							jmp mergeSettlerWithTarget
		stageTarget1Left:		cmp #1
								bne stageTarget2
								tya
								lsr
								lsr
								lsr
								lsr
								lsr
								lsr
								lsr	
							jmp mergeSettlerWithTarget
	stageTarget2:
								lda cspStage
								cmp #2
								bne stageTarget3
								lda $fb
								cmp #2
								bne stageTarget2Left
		stageTarget2Right:	    tya 
								asl
								asl
								asl
								asl
								asl
								asl
							jmp mergeSettlerWithTarget
		stageTarget2Left:		cmp #1
								bne stageTarget3
								tya
								lsr
								lsr
								lsr
								lsr
								lsr
								lsr
							jmp mergeSettlerWithTarget
	stageTarget3:
								lda cspStage
								cmp #3
								bne stageTarget4
								lda $fb
								cmp #2
								bne stageTarget3Left
		stageTarget3Right:  	tya 
								asl
								asl
								asl
								asl
								asl
							jmp mergeSettlerWithTarget
		stageTarget3Left:		cmp #1
								bne stageTarget4
								tya
								lsr
								lsr
								lsr
								lsr
								lsr
							jmp mergeSettlerWithTarget
	stageTarget4:
								lda cspStage
								cmp #4
								bne stageTarget5
								lda $fb
								cmp #2
								bne stageTarget4Left
		stageTarget4Right:	 	tya 
								asl
								asl
								asl
								asl
							jmp mergeSettlerWithTarget
		stageTarget4Left:		cmp #1
								bne stageTarget5
								tya
								lsr
								lsr
								lsr
								lsr		
							jmp mergeSettlerWithTarget
	stageTarget5:
								lda cspStage
								cmp #5
								bne stageTarget6
								lda $fb
								cmp #2
								bne stageTarget5Left
		stageTarget5Right:  	tya 
								asl
								asl
								asl
							jmp mergeSettlerWithTarget
		stageTarget5Left:		cmp #1
								bne stageTarget6
								tya
								lsr 
								lsr
								lsr
							jmp mergeSettlerWithTarget
	stageTarget6:
								lda cspStage
								cmp #6
								bne stageTarget7
								lda $fb
								cmp #2
								bne stageTarget6Left
		stageTarget6Right:  	tya 
								asl
								asl
							jmp mergeSettlerWithTarget
		stageTarget6Left:		cmp #1
								bne stageTarget7
								tya
								lsr
								lsr
							jmp mergeSettlerWithTarget
	stageTarget7:
								lda cspStage
								cmp #7
								bne mergeSettlerWithTarget
								lda $fb
								cmp #2
								bne stageTarget7Left
		stageTarget7Right:  	tya 
								asl
								jmp mergeSettlerWithTarget
		stageTarget7Left:		cmp #1
								bne mergeSettlerWithTarget
								tya
								lsr
							jmp mergeSettlerWithTarget
drawTarget:
mergeSettlerWithTarget:	ora road
storeToTargetTemp:		sta csp1right

    				inc mergeSettlerWithOrigin+1
					inc mergeSettlerWithTarget+1
    				inc charStageLoop+1
    				inc storeToOriginTemp+1
					inc storeToTargetTemp+1

    				inx
    				cpx #8
					bne charStagingInProgress
goto_main:    		//
ldx #8    //char staging finished, all 8 bitmap lines are processed
					jmp mainLogic
charStagingInProgress:	jmp charStageLoop 

mainLogic:
			jsr cspInit
			lda cspStage
			cmp #8
			beq end   
			// csp became idle   
cspStageCheck:		cpx #8   //char stage loop finished, all bitmap lines are generated - x used for the charloop
			beq nextStage
			
			lda cspStage
			cmp #0
			bne cspStage1
cspStage0:	lda #$79
			sta $400	
			jmp cspStageLoop
cspStage1:		
			cmp #1
			bne cspStage2
			lda #$70
			sta $400
			jmp cspStageLoop
cspStage2:		
			cmp #2
			bne cspStage3
			lda #$71
			sta $400
			jmp cspStageLoop
cspStage3:		
			cmp #3
			bne cspStage4
			lda #$72
			sta $400
			jmp cspStageLoop
cspStage4:		
			cmp #4
			bne cspStage5
			lda #$73
			sta $400
			jmp cspStageLoop

cspStage5:		
			cmp #5
			bne cspStage6
			lda #$74
			sta $400
			jmp cspStageLoop

cspStage6:		
			cmp #6
			bne cspStage7
			lda #$75
			sta $400
			jmp cspStageLoop

cspStage7:		
			cmp #7
			bne nextStage
			lda #$76
			sta $400
			jmp cspStageLoop




nextStage:
			waitkey:
	//		jsr $FFE4
	//		beq waitkey
			jsr delay

 			inc cspStage
			inc $400
			jmp mainLogic


end:		lda #0
			sta cspStage
			jmp mainLogic
			rts

delay:
			ldx #0
     		lda #10
wait: 		cmp $d012
     		bne wait
     		inx
     		cpx #30
     		bne wait   
rts






load_map:
	
	ldx #0
	line1:
	lda map1,x
	sta $400,x
	inx
	cpx #0
	bne line1

	ldx #0
	line2:
	lda map1+$100,x
	sta $500,x
	inx
	cpx #0
	bne line2

	ldx #0
	line3:
	lda map1+$200,x
	sta $600,x
	inx
	cpx #0
	bne line3

	ldx #0
	line4:
	lda map1+$300,x
	sta $700,x
	inx
	cpx #0
	bne line4

    lda #$ff
    sta $400 + 2*40 + 8
    lda #$fe
    sta $400 + 2*40 + 9


    
	jmp mainLogic

// get the x and y in set_x+1 and set_y+1 and load set_char
setXYonScreen:
		sta convertxy_backupa
		stx convertxy_backupx
		sty convertxy_backupy
			clc
set_x: 		ldx #$ff
set_y: 		ldy #$ff
			lda x_data,y
			stx add_x+1
add_x: 		adc #0
			sta screen+1
			lda y_data,y
			sta screen+2
			bcc no_carry
			inc screen+2
no_carry:
			set_char: lda #$70
screen: 	sta $ffff
		lda convertxy_backupa
		ldx convertxy_backupx
		ldy convertxy_backupy
rts

getXYonScreen:

		stx convertxy_backupx
		sty convertxy_backupy
			clc
get_x: 		ldx #$ff
get_y: 		ldy #$ff
			lda x_data,y
			stx add_x2+1
add_x2: 		adc #0
			sta screen2+1
			lda y_data,y
			sta screen2+2
			bcc no_carry2
			inc screen2+2
no_carry2:
	//		set_char: lda #$70
screen2: 	lda $ffff

		ldx convertxy_backupx
		ldy convertxy_backupy
rts


convertxy_backupa: .byte 00
convertxy_backupx: .byte 00
convertxy_backupy: .byte 00

x_data: 
	.byte 1024+40*0 //00
	.byte 1024+40*1 //28
	.byte 1024+40*2 //50
	.byte 1024+40*3 //78
	.byte 1024+40*4 //a0
	.byte 1024+40*5 //c8
	.byte 1024+40*6 //d0
	.byte 1024+40*7 //18*
	.byte 1024+40*8 //40
	.byte 1024+40*9 //68

	.byte 1024+40*10 //90
	.byte 1024+40*11 //b8
	.byte 1024+40*12 //e0
	.byte 1024+40*13 //08*
	.byte 1024+40*14 //30
	.byte 1024+40*15 //58
	.byte 1024+40*16 //80
	.byte 1024+40*17 //a8
	.byte 1024+40*18 //d0
	.byte 1024+40*19 //f8

	.byte 1024+40*20 //20*
	.byte 1024+40*21 //48
	.byte 1024+40*22 //70
	.byte 1024+40*23 //98
	.byte 1024+40*24 //c0

y_data:
	.byte $04
	.byte $04
	.byte $04
	.byte $04
	.byte $04
	.byte $04
	.byte $04
	.byte $05
	.byte $05
	.byte $05

	.byte $05
	.byte $05
	.byte $05
	.byte $06
	.byte $06
	.byte $06
	.byte $06
	.byte $06
	.byte $06
	.byte $06

	.byte $07
	.byte $07
	.byte $07
	.byte $07
	.byte $07


// testing the getTempChar
	// jsr getTempChar
    // lda tempCharID
    // sta $401
    // lda tempCharLow
    // sta testChar+1
    // lda tempCharHigh
    // sta testChar+2
    // lda #$ff
	// testChar: sta $ffff
getTempChar:
    	sta getTempChar_store_a
    	stx getTempChar_store_x

    	ldx #100
	getcharloop:    
    	lda tempCharBuffer,x
    	cmp #0
    	beq emptycharfound
    	inx
    	jmp getcharloop

	emptycharfound:
   		stx tempCharID
    	lda #1
    	sta tempCharBuffer,x
    	lda default_charset_low_map,x
    	sta tempCharLow
    	lda default_charset_high_map,x
    	sta tempCharHigh

    	lda getTempChar_store_a
    	ldx getTempChar_store_x
    rts
	getTempChar_store_a:
	.byte 0
	getTempChar_store_x:
	.byte 0
	tempCharID:
	.byte 0
	tempCharLow:
	.byte 0
	tempCharHigh:
	.byte 0
	tempCharBuffer:
	.fill 255, 0    


default_charset_low_map:
	.var default_charset = $2000
	.byte <default_charset+8*0
	.byte <default_charset+8*1
	.byte <default_charset+8*2
	.byte <default_charset+8*3
	.byte <default_charset+8*4
	.byte <default_charset+8*5
	.byte <default_charset+8*6
	.byte <default_charset+8*7
	.byte <default_charset+8*8
	.byte <default_charset+8*9

	.byte <default_charset+8*10
	.byte <default_charset+8*11
	.byte <default_charset+8*12
	.byte <default_charset+8*13
	.byte <default_charset+8*14
	.byte <default_charset+8*15
	.byte <default_charset+8*16
	.byte <default_charset+8*17
	.byte <default_charset+8*18
	.byte <default_charset+8*19

	.byte <default_charset+8*20
	.byte <default_charset+8*21
	.byte <default_charset+8*22
	.byte <default_charset+8*23
	.byte <default_charset+8*24
	.byte <default_charset+8*25
	.byte <default_charset+8*26
	.byte <default_charset+8*27
	.byte <default_charset+8*28
	.byte <default_charset+8*29

	.byte <default_charset+8*30
	.byte <default_charset+8*31
	.byte <default_charset+8*32
	.byte <default_charset+8*33
	.byte <default_charset+8*34
	.byte <default_charset+8*35
	.byte <default_charset+8*36
	.byte <default_charset+8*37
	.byte <default_charset+8*38
	.byte <default_charset+8*39

	.byte <default_charset+8*40
	.byte <default_charset+8*41
	.byte <default_charset+8*42
	.byte <default_charset+8*43
	.byte <default_charset+8*44
	.byte <default_charset+8*45
	.byte <default_charset+8*46
	.byte <default_charset+8*47
	.byte <default_charset+8*48
	.byte <default_charset+8*49

	.byte <default_charset+8*50
	.byte <default_charset+8*51
	.byte <default_charset+8*52
	.byte <default_charset+8*53
	.byte <default_charset+8*54
	.byte <default_charset+8*55
	.byte <default_charset+8*56
	.byte <default_charset+8*57
	.byte <default_charset+8*58
	.byte <default_charset+8*59

	.byte <default_charset+8*60
	.byte <default_charset+8*61
	.byte <default_charset+8*62
	.byte <default_charset+8*63
	.byte <default_charset+8*64
	.byte <default_charset+8*65
	.byte <default_charset+8*66
	.byte <default_charset+8*67
	.byte <default_charset+8*68
	.byte <default_charset+8*69

	.byte <default_charset+8*70
	.byte <default_charset+8*71
	.byte <default_charset+8*72
	.byte <default_charset+8*73
	.byte <default_charset+8*74
	.byte <default_charset+8*75
	.byte <default_charset+8*76
	.byte <default_charset+8*77
	.byte <default_charset+8*78
	.byte <default_charset+8*79

	.byte <default_charset+8*80
	.byte <default_charset+8*81
	.byte <default_charset+8*82
	.byte <default_charset+8*83
	.byte <default_charset+8*84
	.byte <default_charset+8*85
	.byte <default_charset+8*86
	.byte <default_charset+8*87
	.byte <default_charset+8*88
	.byte <default_charset+8*89

	.byte <default_charset+8*90
	.byte <default_charset+8*91
	.byte <default_charset+8*92
	.byte <default_charset+8*93
	.byte <default_charset+8*94
	.byte <default_charset+8*95
	.byte <default_charset+8*96
	.byte <default_charset+8*97
	.byte <default_charset+8*98
	.byte <default_charset+8*99

	.byte <default_charset+8*100
	.byte <default_charset+8*101
	.byte <default_charset+8*102
	.byte <default_charset+8*103
	.byte <default_charset+8*104
	.byte <default_charset+8*105
	.byte <default_charset+8*106
	.byte <default_charset+8*107
	.byte <default_charset+8*108
	.byte <default_charset+8*109

	.byte <default_charset+8*110
	.byte <default_charset+8*111
	.byte <default_charset+8*112
	.byte <default_charset+8*113
	.byte <default_charset+8*114
	.byte <default_charset+8*115
	.byte <default_charset+8*116
	.byte <default_charset+8*117
	.byte <default_charset+8*118
	.byte <default_charset+8*119

	.byte <default_charset+8*120
	.byte <default_charset+8*121
	.byte <default_charset+8*122
	.byte <default_charset+8*123
	.byte <default_charset+8*124
	.byte <default_charset+8*125
	.byte <default_charset+8*126
	.byte <default_charset+8*127
	.byte <default_charset+8*128
	.byte <default_charset+8*129

	.byte <default_charset+8*130
	.byte <default_charset+8*131
	.byte <default_charset+8*132
	.byte <default_charset+8*133
	.byte <default_charset+8*134
	.byte <default_charset+8*135
	.byte <default_charset+8*136
	.byte <default_charset+8*137
	.byte <default_charset+8*138
	.byte <default_charset+8*139

	.byte <default_charset+8*140
	.byte <default_charset+8*141
	.byte <default_charset+8*142
	.byte <default_charset+8*143
	.byte <default_charset+8*144
	.byte <default_charset+8*145
	.byte <default_charset+8*146
	.byte <default_charset+8*147
	.byte <default_charset+8*148
	.byte <default_charset+8*149

	.byte <default_charset+8*150
	.byte <default_charset+8*151
	.byte <default_charset+8*152
	.byte <default_charset+8*153
	.byte <default_charset+8*154
	.byte <default_charset+8*155
	.byte <default_charset+8*156
	.byte <default_charset+8*157
	.byte <default_charset+8*158
	.byte <default_charset+8*159

	.byte <default_charset+8*160
	.byte <default_charset+8*161
	.byte <default_charset+8*162
	.byte <default_charset+8*163
	.byte <default_charset+8*164
	.byte <default_charset+8*165
	.byte <default_charset+8*166
	.byte <default_charset+8*167
	.byte <default_charset+8*168
	.byte <default_charset+8*169

	.byte <default_charset+8*170
	.byte <default_charset+8*171
	.byte <default_charset+8*172
	.byte <default_charset+8*173
	.byte <default_charset+8*174
	.byte <default_charset+8*175
	.byte <default_charset+8*176
	.byte <default_charset+8*177
	.byte <default_charset+8*178
	.byte <default_charset+8*179

	.byte <default_charset+8*180
	.byte <default_charset+8*181
	.byte <default_charset+8*182
	.byte <default_charset+8*183
	.byte <default_charset+8*184
	.byte <default_charset+8*185
	.byte <default_charset+8*186
	.byte <default_charset+8*187
	.byte <default_charset+8*188
	.byte <default_charset+8*189
	
	.byte <default_charset+8*190
	.byte <default_charset+8*191
	.byte <default_charset+8*192
	.byte <default_charset+8*193
	.byte <default_charset+8*194
	.byte <default_charset+8*195
	.byte <default_charset+8*196
	.byte <default_charset+8*197
	.byte <default_charset+8*198
	.byte <default_charset+8*199
	
	.byte <default_charset+8*200
	.byte <default_charset+8*201
	.byte <default_charset+8*202
	.byte <default_charset+8*203
	.byte <default_charset+8*204
	.byte <default_charset+8*205
	.byte <default_charset+8*206
	.byte <default_charset+8*207
	.byte <default_charset+8*208
	.byte <default_charset+8*209
	
	.byte <default_charset+8*210
	.byte <default_charset+8*211
	.byte <default_charset+8*212
	.byte <default_charset+8*213
	.byte <default_charset+8*214
	.byte <default_charset+8*215
	.byte <default_charset+8*216
	.byte <default_charset+8*217
	.byte <default_charset+8*218
	.byte <default_charset+8*219
	
	.byte <default_charset+8*220
	.byte <default_charset+8*221
	.byte <default_charset+8*222
	.byte <default_charset+8*223
	.byte <default_charset+8*224
	.byte <default_charset+8*225
	.byte <default_charset+8*226
	.byte <default_charset+8*227
	.byte <default_charset+8*228
	.byte <default_charset+8*229
	
	.byte <default_charset+8*230
	.byte <default_charset+8*231
	.byte <default_charset+8*232
	.byte <default_charset+8*233
	.byte <default_charset+8*234
	.byte <default_charset+8*235
	.byte <default_charset+8*236
	.byte <default_charset+8*237
	.byte <default_charset+8*238
	.byte <default_charset+8*239
	
	.byte <default_charset+8*240
	.byte <default_charset+8*241
	.byte <default_charset+8*242
	.byte <default_charset+8*243
	.byte <default_charset+8*244
	.byte <default_charset+8*245
	.byte <default_charset+8*246
	.byte <default_charset+8*247
	.byte <default_charset+8*248
	.byte <default_charset+8*249
	
	.byte <default_charset+8*250
	.byte <default_charset+8*251
	.byte <default_charset+8*252
	.byte <default_charset+8*253
	.byte <default_charset+8*254
	.byte <default_charset+8*255

//highbytes
default_charset_high_map:
	.byte >default_charset+8*0
	.byte >default_charset+8*1
	.byte >default_charset+8*2
	.byte >default_charset+8*3
	.byte >default_charset+8*4
	.byte >default_charset+8*5
	.byte >default_charset+8*6
	.byte >default_charset+8*7
	.byte >default_charset+8*8
	.byte >default_charset+8*9

	.byte >default_charset+8*10
	.byte >default_charset+8*11
	.byte >default_charset+8*12
	.byte >default_charset+8*13
	.byte >default_charset+8*14
	.byte >default_charset+8*15
	.byte >default_charset+8*16
	.byte >default_charset+8*17
	.byte >default_charset+8*18
	.byte >default_charset+8*19

	.byte >default_charset+8*20
	.byte >default_charset+8*21
	.byte >default_charset+8*22
	.byte >default_charset+8*23
	.byte >default_charset+8*24
	.byte >default_charset+8*25
	.byte >default_charset+8*26
	.byte >default_charset+8*27
	.byte >default_charset+8*28
	.byte >default_charset+8*29

	.byte >default_charset+8*30
	.byte >default_charset+8*31
	.byte >default_charset+8*32
	.byte >default_charset+8*33
	.byte >default_charset+8*34
	.byte >default_charset+8*35
	.byte >default_charset+8*36
	.byte >default_charset+8*37
	.byte >default_charset+8*38
	.byte >default_charset+8*39

	.byte >default_charset+8*40
	.byte >default_charset+8*41
	.byte >default_charset+8*42
	.byte >default_charset+8*43
	.byte >default_charset+8*44
	.byte >default_charset+8*45
	.byte >default_charset+8*46
	.byte >default_charset+8*47
	.byte >default_charset+8*48
	.byte >default_charset+8*49

	.byte >default_charset+8*50
	.byte >default_charset+8*51
	.byte >default_charset+8*52
	.byte >default_charset+8*53
	.byte >default_charset+8*54
	.byte >default_charset+8*55
	.byte >default_charset+8*56
	.byte >default_charset+8*57
	.byte >default_charset+8*58
	.byte >default_charset+8*59

	.byte >default_charset+8*60
	.byte >default_charset+8*61
	.byte >default_charset+8*62
	.byte >default_charset+8*63
	.byte >default_charset+8*64
	.byte >default_charset+8*65
	.byte >default_charset+8*66
	.byte >default_charset+8*67
	.byte >default_charset+8*68
	.byte >default_charset+8*69

	.byte >default_charset+8*70
	.byte >default_charset+8*71
	.byte >default_charset+8*72
	.byte >default_charset+8*73
	.byte >default_charset+8*74
	.byte >default_charset+8*75
	.byte >default_charset+8*76
	.byte >default_charset+8*77
	.byte >default_charset+8*78
	.byte >default_charset+8*79

	.byte >default_charset+8*80
	.byte >default_charset+8*81
	.byte >default_charset+8*82
	.byte >default_charset+8*83
	.byte >default_charset+8*84
	.byte >default_charset+8*85
	.byte >default_charset+8*86
	.byte >default_charset+8*87
	.byte >default_charset+8*88
	.byte >default_charset+8*89

	.byte >default_charset+8*90
	.byte >default_charset+8*91
	.byte >default_charset+8*92
	.byte >default_charset+8*93
	.byte >default_charset+8*94
	.byte >default_charset+8*95
	.byte >default_charset+8*96
	.byte >default_charset+8*97
	.byte >default_charset+8*98
	.byte >default_charset+8*99

	.byte >default_charset+8*100
	.byte >default_charset+8*101
	.byte >default_charset+8*102
	.byte >default_charset+8*103
	.byte >default_charset+8*104
	.byte >default_charset+8*105
	.byte >default_charset+8*106
	.byte >default_charset+8*107
	.byte >default_charset+8*108
	.byte >default_charset+8*109

	.byte >default_charset+8*110
	.byte >default_charset+8*111
	.byte >default_charset+8*112
	.byte >default_charset+8*113
	.byte >default_charset+8*114
	.byte >default_charset+8*115
	.byte >default_charset+8*116
	.byte >default_charset+8*117
	.byte >default_charset+8*118
	.byte >default_charset+8*119

	.byte >default_charset+8*120
	.byte >default_charset+8*121
	.byte >default_charset+8*122
	.byte >default_charset+8*123
	.byte >default_charset+8*124
	.byte >default_charset+8*125
	.byte >default_charset+8*126
	.byte >default_charset+8*127
	.byte >default_charset+8*128
	.byte >default_charset+8*129

	.byte >default_charset+8*130
	.byte >default_charset+8*131
	.byte >default_charset+8*132
	.byte >default_charset+8*133
	.byte >default_charset+8*134
	.byte >default_charset+8*135
	.byte >default_charset+8*136
	.byte >default_charset+8*137
	.byte >default_charset+8*138
	.byte >default_charset+8*139

	.byte >default_charset+8*140
	.byte >default_charset+8*141
	.byte >default_charset+8*142
	.byte >default_charset+8*143
	.byte >default_charset+8*144
	.byte >default_charset+8*145
	.byte >default_charset+8*146
	.byte >default_charset+8*147
	.byte >default_charset+8*148
	.byte >default_charset+8*149

	.byte >default_charset+8*150
	.byte >default_charset+8*151
	.byte >default_charset+8*152
	.byte >default_charset+8*153
	.byte >default_charset+8*154
	.byte >default_charset+8*155
	.byte >default_charset+8*156
	.byte >default_charset+8*157
	.byte >default_charset+8*158
	.byte >default_charset+8*159

	.byte >default_charset+8*160
	.byte >default_charset+8*161
	.byte >default_charset+8*162
	.byte >default_charset+8*163
	.byte >default_charset+8*164
	.byte >default_charset+8*165
	.byte >default_charset+8*166
	.byte >default_charset+8*167
	.byte >default_charset+8*168
	.byte >default_charset+8*169

	.byte >default_charset+8*170
	.byte >default_charset+8*171
	.byte >default_charset+8*172
	.byte >default_charset+8*173
	.byte >default_charset+8*174
	.byte >default_charset+8*175
	.byte >default_charset+8*176
	.byte >default_charset+8*177
	.byte >default_charset+8*178
	.byte >default_charset+8*179

	.byte >default_charset+8*180
	.byte >default_charset+8*181
	.byte >default_charset+8*182
	.byte >default_charset+8*183
	.byte >default_charset+8*184
	.byte >default_charset+8*185
	.byte >default_charset+8*186
	.byte >default_charset+8*187
	.byte >default_charset+8*188
	.byte >default_charset+8*189

	.byte >default_charset+8*190
	.byte >default_charset+8*191
	.byte >default_charset+8*192
	.byte >default_charset+8*193
	.byte >default_charset+8*194
	.byte >default_charset+8*195
	.byte >default_charset+8*196
	.byte >default_charset+8*197
	.byte >default_charset+8*198
	.byte >default_charset+8*199

	.byte >default_charset+8*200
	.byte >default_charset+8*201
	.byte >default_charset+8*202
	.byte >default_charset+8*203
	.byte >default_charset+8*204
	.byte >default_charset+8*205
	.byte >default_charset+8*206
	.byte >default_charset+8*207
	.byte >default_charset+8*208
	.byte >default_charset+8*209

	.byte >default_charset+8*210
	.byte >default_charset+8*211
	.byte >default_charset+8*212
	.byte >default_charset+8*213
	.byte >default_charset+8*214
	.byte >default_charset+8*215
	.byte >default_charset+8*216
	.byte >default_charset+8*217
	.byte >default_charset+8*218
	.byte >default_charset+8*219

	.byte >default_charset+8*220
	.byte >default_charset+8*221
	.byte >default_charset+8*222
	.byte >default_charset+8*223
	.byte >default_charset+8*224
	.byte >default_charset+8*225
	.byte >default_charset+8*226
	.byte >default_charset+8*227
	.byte >default_charset+8*228
	.byte >default_charset+8*229

	.byte >default_charset+8*230
	.byte >default_charset+8*231
	.byte >default_charset+8*232
	.byte >default_charset+8*233
	.byte >default_charset+8*234
	.byte >default_charset+8*235
	.byte >default_charset+8*236
	.byte >default_charset+8*237
	.byte >default_charset+8*238
	.byte >default_charset+8*239

	.byte >default_charset+8*240
	.byte >default_charset+8*241
	.byte >default_charset+8*242
	.byte >default_charset+8*243
	.byte >default_charset+8*244
	.byte >default_charset+8*245
	.byte >default_charset+8*246
	.byte >default_charset+8*247
	.byte >default_charset+8*248
	.byte >default_charset+8*249

	.byte >default_charset+8*250
	.byte >default_charset+8*251
	.byte >default_charset+8*252
	.byte >default_charset+8*253
	.byte >default_charset+8*254
	.byte >default_charset+8*255

	// character bitmap definitions 2k
*=$2000
.import source "chars.asm"

*=$4000
map1:
.import source "map1.asm"


