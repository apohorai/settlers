// 10 sys2061
*=$0801
	.byte $0b, $08, $0a, $00, $9e, $32, $30, $36, $31, $00, $00, $00

*=$080d
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
stage_loop:    ldx #$00
//init
						lda stage_direction
						sta $fb
						lda #<road
						sta load_origin_background_char+1
						lda #>road
						sta load_origin_background_char+2
						lda #<road
						sta load_right_background_char+1
						lda #>road
						sta load_right_background_char+2
						lda #<csp1
						sta csp_load_loop+1
						lda #>csp1
						sta csp_load_loop+2
						lda #<csp1origin
						sta store_to_origin_temp+1
						lda #>csp1origin
						sta store_to_origin_temp+2
						lda #<csp1right
						sta store_to_right_temp+1
						lda #>csp1right
						sta store_to_right_temp+2

csp_load_loop:			lda csp1 
						tay //y=the data of the csp1,x
//stage 0 origin						
stage0_origin:			lda stage_data
						cmp #0
						bne stage1_origin
						lda $fb  
						cmp #2 //right
						bne stage_origin_to_left0
stage_origin_to_right0:	tya
						jmp draw_origin
stage_origin_to_left0:	cmp #1
						bne stage1_origin
						tya
						jmp draw_origin

//stage 1 origin
stage1_origin:			lda stage_data
						cmp #1
						bne stage2_origin
						lda $fb
						cmp #2
						bne stage_origin_to_left1
stage_origin_to_right1: tya
						lsr
						jmp draw_origin
stage_origin_to_left1:	cmp #1
						bne stage2_origin
						tya
						asl
						jmp draw_origin						

//stage 2 origin
stage2_origin:			lda stage_data
						cmp #2
						bne stage3_origin
						lda $fb
						cmp #2
						bne stage_origin_to_left2
stage_origin_to_right2: tya
						lsr
						lsr
						jmp draw_origin
stage_origin_to_left2:	cmp #1
						bne stage3_origin
						tya
						asl
						asl
						jmp draw_origin	

//stage 3 origin
stage3_origin:			lda stage_data
						cmp #3
						bne stage4_origin
						lda $fb
						cmp #2
						bne stage_origin_to_left3
stage_origin_to_right3: tya
						lsr
						lsr
						lsr
						jmp draw_origin
stage_origin_to_left3:	cmp #1
						bne stage4_origin
						tya
						asl
						asl
						asl
						jmp draw_origin	


stage4_origin:			lda stage_data
						cmp #4
						bne stage5_origin
						lda $fb
						cmp #2
						bne stage_origin_to_left4
stage_origin_to_right4: tya
						lsr
						lsr
						lsr
						lsr
						jmp draw_origin
stage_origin_to_left4:	cmp #1
						bne stage4_origin
						tya
						asl
						asl
						asl
						asl
						jmp draw_origin	
//stage 5 origin
stage5_origin:			lda stage_data
						cmp #5
						bne stage6_origin
						lda $fb
						cmp #2
						bne stage_origin_to_left5
stage_origin_to_right5: tya
						lsr
						lsr
						lsr
						lsr
						lsr
						jmp draw_origin
stage_origin_to_left5:	cmp #1
						bne stage6_origin
						tya
						asl
						asl
						asl
						asl
						asl
						jmp draw_origin	
//stage 6 origin
stage6_origin:			lda stage_data
						cmp #6
						bne stage7_origin
						lda $fb
						cmp #2
						bne stage_origin_to_left6
stage_origin_to_right6: tya
						lsr
						lsr
						lsr
						lsr
						lsr
						lsr
						jmp draw_origin
stage_origin_to_left6:	cmp #1
						bne stage7_origin
						tya
						asl
						asl
						asl
						asl
						asl
						asl
						jmp draw_origin	

stage7_origin:			lda stage_data
						cmp #7
						bne draw_origin
						lda $fb
						cmp #2
						bne stage_origin_to_left7
stage_origin_to_right7: tya
						lsr
						lsr
						lsr
						lsr
						lsr
						lsr
						lsr
						jmp draw_origin
stage_origin_to_left7:	cmp #1
						bne draw_origin
						tya
						asl
						asl
						asl
						asl
						asl
						asl
						asl
						jmp draw_origin			
draw_origin:
// at this point a=the csp data rotated right stage_data times, x=0-7, y= original csp1
load_origin_background_char:	ora road
store_to_origin_temp:			sta csp1origin

move_temps:					
						lda stage_data
						cmp #0
						bne stage_temps1
stage_temps0:			lda $fb
						cmp #2
						bne stage_temps0_left
stage_temps0_right:     lda #0
						jmp load_right_background_char
stage_temps0_left:		cmp #1
						bne stage_temps1
						lda #0
						jmp load_right_background_char

stage_temps1:
						lda stage_data
						cmp #1
						bne stage_temps2
						lda $fb
						cmp #2
						bne stage_temps1_left
stage_temps1_right:     tya 
						asl
						asl
						asl
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_temps1_left:		cmp #1
						bne stage_temps2
						tya
						lsr
						lsr
						lsr
						lsr
						lsr
						lsr
						lsr
						
						jmp load_right_background_char
 

stage_temps2:
						lda stage_data
						cmp #2
						bne stage_temps3
						lda $fb
						cmp #2
						bne stage_temps2_left
stage_temps2_right:     tya 
						asl
						asl
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_temps2_left:		cmp #1
						bne stage_temps3

						tya
						lsr
						lsr
						lsr
						lsr
						lsr
						lsr
						
						jmp load_right_background_char


stage_temps3:
						lda stage_data
						cmp #3
						bne stage_temps4
						lda $fb
						cmp #2
						bne stage_temps3_left
stage_temps3_right:     tya 
						asl
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_temps3_left:		cmp #1
						bne stage_temps4
						tya
						lsr
						lsr
						lsr
						lsr
						lsr
						

						jmp load_right_background_char

stage_temps4:
						lda stage_data
						cmp #4
						bne stage_temps5
						lda $fb
						cmp #2
						bne stage_temps4_left
stage_temps4_right:     tya 
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_temps4_left:		cmp #1
						bne stage_temps5
						tya
						lsr
						lsr
						lsr
						lsr		
						jmp load_right_background_char

stage_temps5:
						lda stage_data
						cmp #5
						bne stage_temps6
						lda $fb
						cmp #2
						bne stage_temps5_left
stage_temps5_right:     tya 
						asl
						asl
						asl
						jmp load_right_background_char
stage_temps5_left:		cmp #1
						bne stage_temps6
						tya
						lsr 
						lsr
						lsr
						jmp load_right_background_char


stage_temps6:
						lda stage_data
						cmp #6
						bne stage_temps7
						lda $fb
						cmp #2
						bne stage_temps6_left
stage_temps6_right:     tya 
						asl
						asl
						jmp load_right_background_char
stage_temps6_left:		cmp #1
						bne stage_temps7
						tya
						lsr
						lsr
						jmp load_right_background_char
stage_temps7:
						lda stage_data
						cmp #7
						bne load_right_background_char
						lda $fb
						cmp #2
						bne stage_temps7_left
stage_temps7_right:     tya 
						asl
						jmp load_right_background_char
stage_temps7_left:		cmp #1
						bne load_right_background_char
						tya
						lsr
						jmp load_right_background_char


load_right_background_char:	ora road
store_to_right_temp:	sta csp1right

    				inc load_origin_background_char+1
					inc load_right_background_char+1
    				inc csp_load_loop+1
    				inc store_to_origin_temp+1
					inc store_to_right_temp+1

    inx
    cpx #8
	bne csp_load_loop_still_enable
goto_main:    		ldx #8
		//			inc stage_data 
					jmp main_logic
csp_load_loop_still_enable:		jmp csp_load_loop
stage_data: .byte $00
stage_direction: .byte $02 //right




main_logic:
	
			lda stage_data
			cmp #8
			beq end      
main:		cpx #8   //stage_loop finished
			beq next_stage
			
			lda stage_data
			cmp #0
			bne main_1
main_0:		lda #$79
	//		sta $400	
			jmp stage_loop
main_1:		
			cmp #1
			bne main_2
			lda #$70
			sta $400
			jmp stage_loop
main_2:		
			cmp #2
			bne main_3
			lda #$71
			sta $400
			jmp stage_loop
main_3:		
			cmp #3
			bne main_4
			lda #$72
			sta $400
			jmp stage_loop
main_4:		
			cmp #4
			bne main_5
			lda #$73
			sta $400
			jmp stage_loop

main_5:		
			cmp #5
			bne main_6
			lda #$74
			sta $400
			jmp stage_loop

main_6:		
			cmp #6
			bne main_7
			lda #$75
			sta $400
			jmp stage_loop

main_7:		
			cmp #7
			bne next_stage
			lda #$76
			sta $400
			jmp stage_loop




next_stage:
			waitkey:
			jsr $FFE4
			beq waitkey 
 			inc stage_data
			jmp main_logic


end:		rts

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


	jmp main_logic

// get the x and y in set_x+1 and set_y+1 and load set_char
convertxy:
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


	// character bitmap definitions 2k
*=$2000
.import source "chars.asm"

*=$4000
map1:
.import source "map1.asm"


