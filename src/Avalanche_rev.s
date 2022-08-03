VERSION = 1
REVISION = 7

.macro DATE
.ascii "3.8.2022"
.endm

.macro VERS
.ascii "Avalanche 1.7"
.endm

.macro VSTRING
.ascii "Avalanche 1.7 (3.8.2022)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 1.7 (3.8.2022)"
.byte 0
.endm
