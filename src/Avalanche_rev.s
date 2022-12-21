VERSION = 1
REVISION = 9

.macro DATE
.ascii "21.12.2022"
.endm

.macro VERS
.ascii "Avalanche 1.9"
.endm

.macro VSTRING
.ascii "Avalanche 1.9 (21.12.2022)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 1.9 (21.12.2022)"
.byte 0
.endm
