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

    // draw a path
    lda #$00
    ldx #$00
screenmem:    sta $0400
    inx
    inc screenmem+1
    cpx #$0a
    bne screenmem

// copy the background char to a temp space
    ldx #$0
chcopy1:    lda $2000
chcopy2:    sta $2800-8*1
    inc chcopy1+1
    inc chcopy2+1
    inx
    cpx #7
    bne chcopy1

// copy the background char to a temp space
    ldx #$0
chcopy3:    lda $2000
chcopy4:    sta $2800-8*2
    inc chcopy3+1
    inc chcopy4+1
    inx
    cpx #7
    bne chcopy3

// merge with a man
// steps needed :
// copy the original man to a temp
	
stage_loop:    ldx #$00
//init
						lda #<road
						sta load_background_char+1
						lda #>road
						sta load_background_char+2
						lda #<road
						sta load_background_char2+1
						lda #>road
						sta load_background_char2+2
						lda #<sp1
						sta load_sp_data+1
						lda #>sp1
						sta load_sp_data+2
						lda #<sp1temp
						sta store_to_temp+1
						lda #>sp1temp
						sta store_to_temp+2
						lda #<sp2temp
						sta store_to_temp2+1
						lda #>sp2temp
						sta store_to_temp2+2



load_sp_data:			lda sp1 
						tay
						lda stage_data
						cmp #0
						bne stage1
stage0:					tya
						jmp next
stage1:					cmp #1
						bne stage2
						tya
						lsr
						jmp next
stage2:					cmp #2
						bne stage3
						tya
						lsr
						lsr
						jmp next

stage3:					cmp #3
						bne stage4
						tya
						lsr
						lsr
						lsr
				
						jmp next

stage4:					cmp #4
						bne stage5
						tya
						lsr
						lsr
						lsr
						lsr
stage5:					cmp #5
						bne next
						tya
						lsr
						lsr
						lsr
						lsr		
						lsr				
next:
load_background_char:	ora road
store_to_temp:			sta sp1temp
						tya
						asl
						asl
						asl
						asl
					

load_background_char2:	ora road
store_to_temp2:			sta sp2temp

    				inc load_background_char+1
					inc load_background_char2+1
    				inc load_sp_data+1
    				inc store_to_temp+1
					inc store_to_temp2+1

    inx
    cpx #8
    bne load_sp_data



// main
    lda #$ff
    sta $0420
    lda #$fe
    sta $0421        
    lda #$30
    sta $0410
// waitkey:
// 	sta $fb
// 	stx $fc
// 	stx $fd
// internalwaitkey:	jsr $FFE4
// 	beq internalwaitkey
// 	lda $fb
// 	ldx $fc
// 	ldy $fd
	rts
stage_data: .byte $04
*=$0a00
new_char: .byte $aa
*=$5000

	// character bitmap definitions 2k
*=$2000
road:
	.byte	$00, $00, $00, $00, $00, $FF, $00, $FF
emptychar:
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
sp1:	.byte	$10, $18, $10, $38, $10, $28, $28, $00
sp2:	.byte	$00, $18, $1C, $0A, $1C, $28, $14, $14
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
	.byte	$00, $00, $00, $00, $00, $00, $00, $00
sp2temp:	.byte	$ee, $00, $00, $00, $00, $00, $00, $00
sp1temp:	.byte	$ee, $00, $00, $00, $00, $00, $00, $00