// *=$1000
// 	lda #$18
// 	sta $d018
// jsr getTempChar
//     lda tempCharID
//     sta $400
//     lda tempCharLow
//     sta testChar+1
//     lda tempCharHigh
//     sta testChar+2
//     lda #$ff
    
// testChar: sta $ffff
// rts



// *=$2000
// .fill 2048, 0    


//////////////////////
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