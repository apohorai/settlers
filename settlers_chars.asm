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
	lda #$06
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
			//			inc stage_data
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
						lda stage_data
						cmp #0
						bne stage1_origin

//stage 0 origin
						lda $fb
						cmp #1 //right
						bne stage_origin_to_left0
stage_origin_to_right0:	tya
						// inc stage_data
						jmp move_temps
stage_origin_to_left0:	cmp #2
						bne stage1_origin
						tya
						jmp move_temps
//stage 1 origin
stage1_origin:			tya
						cmp #1
						bne stage2_origin
						lda $fb
						cmp #1
						bne stage_origin_to_left1
stage_origin_to_right1: tya
						lsr
						jmp move_temps
stage_origin_to_left1:	cmp #2
						jmp move_temps						

//stage 2 origin
stage2_origin:			cmp #2
						bne stage3_origin
						lda $fb
						cmp #1
						bne stage_origin_to_left2
stage_origin_to_right2: tya
						lsr
						lsr
						jmp move_temps
stage_origin_to_left2:	cmp #2
						jmp move_temps	



//stage 3 origin
stage3_origin:			cmp #3
						bne stage4_origin
						lda $fb
						cmp #1
						bne stage_origin_to_left3
stage_origin_to_right3: tya
						lsr
						lsr
						lsr
						jmp move_temps
stage_origin_to_left3:	cmp #2
						jmp move_temps	


stage4_origin:			cmp #4
						bne stage5_origin
						lda $fb
						cmp #1
						bne stage_origin_to_left4
stage_origin_to_right4: tya
						lsr
						lsr
						lsr
						lsr
						jmp move_temps
stage_origin_to_left4:	cmp #2
						jmp move_temps	

stage5_origin:			cmp #5
						bne stage6_origin
						lda $fb
						cmp #1
						bne stage_origin_to_left5
stage_origin_to_right5: tya
						lsr
						lsr
						lsr
						lsr
						lsr
						jmp move_temps
stage_origin_to_left5:	cmp #2
						jmp move_temps	

stage6_origin:			cmp #6
						bne stage7_origin
						lda $fb
						cmp #1
						bne stage_origin_to_left6
stage_origin_to_right6: tya
						lsr
						lsr
						lsr
						lsr
						lsr
						lsr
						jmp move_temps
stage_origin_to_left6:	cmp #2
						jmp move_temps	

stage7_origin:			cmp #7
						bne move_temps
						lda $fb
						cmp #1
						bne move_temps
stage_origin_to_right7: tya
						lsr
						lsr
						lsr
						lsr
						lsr
						lsr
						lsr
						jmp move_temps
stage_origin_to_left7:	cmp #2
						jmp move_temps			
move_temps:
// at this point a=the csp data rotated right stage_data times, x=0-7, y= original csp1
load_origin_background_char:	ora road
store_to_origin_temp:			sta csp1origin
						lda stage_data
						cmp #0
						bne stage_right_to_right1
stage_right_to_right0:			
						jmp load_right_background_char
stage_right_to_right1:			cmp #1
						bne stage_right_to_right2
						tya 
						asl
						asl
						asl
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_right_to_right2:			cmp #2
						bne stage_right_to_right3
						tya
						asl
						asl
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_right_to_right3:			cmp #3
						bne stage_right_to_right4
						tya
						asl
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_right_to_right4:			cmp #4
						bne stage_right_to_right5
						tya
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_right_to_right5:			cmp #5
						bne stage_right_to_right6
						tya
						asl
						asl
						asl
						jmp load_right_background_char			
stage_right_to_right6:			cmp #6
						bne stage_right_to_right7
						tya
						asl
						asl
						jmp load_right_background_char
stage_right_to_right7:			cmp #7
						bne goto_main
						tya
						asl
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
					jmp main_logic
csp_load_loop_still_enable:		jmp csp_load_loop
stage_data: .byte $00
stage_direction: .byte $02 //right




main_logic:
	
			lda stage_data
			cmp #5
			beq end      
main:		cpx #8   //stage_loop finished
			beq next_stage
			lda stage_data
			cmp #0
			bne main_1
main_0:		lda #$79
			sta $400	
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
			bne next_stage
			lda #$73
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
	lda #0
	sta $400 + 4*40 + 2
	lda #1
	sta $400 + 4*40 + 3
	lda #2
	sta $400 + 3*40 + 3
	lda #0
	sta $400 + 3*40 + 4
	lda #0
	sta $400 + 3*40 + 5
	lda #0
	sta $400 + 3*40 + 6
	lda #0
	sta $400 + 3*40 + 7
	lda #0
	sta $400 + 3*40 + 7
    lda #$ff
    sta $400 + 3*40 + 8
    lda #$fe
    sta $400 + 3*40 + 9
	// house
	lda #$24
    sta $400 + 3*40 + 10
	lda #$25
    sta $400 + 3*40 + 11
	lda #$14
    sta $400 + 2*40 + 10
	lda #$15
    sta $400 + 2*40 + 11
	//mine
	lda #$26
    sta $400 + 3*40 + 12
	lda #$27
    sta $400 + 3*40 + 13
	lda #$16
    sta $400 + 2*40 + 12
	lda #$17
    sta $400 + 2*40 + 13
	jmp main_logic
	// character bitmap definitions 2k
*=$2000
.import source "chars.asm"