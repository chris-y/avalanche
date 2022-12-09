VERSION = 1
REVISION = 8

.macro DATE
.ascii "9.12.2022"
.endm

.macro VERS
.ascii "Avalanche 1.8"
.endm

.macro VSTRING
.ascii "Avalanche 1.8 (9.12.2022)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 1.8 (9.12.2022)"
.byte 0
.endm
