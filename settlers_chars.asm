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

	jmp main_loop
stage_loop:    ldx #$00
//init
.var road = $2000
.var csp1 = $2000 + $30*8
.var csp1origin = $2000 + $ff*8  //the temp char for sp1 from where the csp started
.var csp1right  = $2000 + $fe*8  //the temp char for sp1 to where will go to the right


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
						bne stage_origin1
stage_origin0:			tya
						jmp move_right
stage_origin1:			cmp #1
						bne stage_origin2
						tya
						lsr
						jmp move_right
stage_origin2:			cmp #2
						bne stage_origin3
						tya
						lsr
						lsr
						jmp move_right
stage_origin3:			cmp #3
						bne stage_origin4
						tya
						lsr
						lsr
						lsr
						jmp move_right
stage_origin4:			cmp #4
						bne stage_origin5
						tya
						lsr
						lsr
						lsr
						lsr
						jmp move_right
stage_origin5:			cmp #5
						bne stage_origin6
						tya
						lsr
						lsr
						lsr
						lsr		
						lsr				
						jmp move_right
stage_origin6:			cmp #6
						bne stage_origin7
						tya
						lsr
						lsr
						lsr
						lsr		
						lsr	
						lsr
						jmp move_right
stage_origin7:			cmp #7
						bne move_right
						tya
						lsr
						lsr
						lsr
						lsr		
						lsr
						lsr
						lsr					
move_right:
// at this point a=the csp data rotated right stage_data times, x=0-7, y= original csp1
load_origin_background_char:	ora road
store_to_origin_temp:			sta csp1origin
						lda stage_data
						cmp #0
						bne stage_right1
stage_right0:			
						jmp load_right_background_char
stage_right1:			cmp #1
						bne stage_right2
						tya 
						asl
						asl
						asl
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_right2:			cmp #2
						bne stage_right3
						tya
						asl
						asl
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_right3:			cmp #3
						bne stage_right4
						asl
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_right4:			cmp #4
						bne stage_right5
						tya
						asl
						asl
						asl
						asl
						jmp load_right_background_char
stage_right5:			cmp #5
						bne stage_right6
						tya
						asl
						asl
						asl
						jmp load_right_background_char			
stage_right6:			cmp #6
						bne stage_right7
						tya
						asl
						asl
						jmp load_right_background_char
stage_right7:			cmp #7
						bne load_right_background_char
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
    beq main_loop
csp_load_loop_still_enable:		jmp csp_load_loop


main_loop:
// main
    lda #$ff
    sta $0420
    lda #$fe
    sta $0421        
    lda #$30
    sta $0410


			jmp stage_loop
end:		rts
stage_data: .byte $05

	// character bitmap definitions 2k
*=$2000
.import source "chars.asm"