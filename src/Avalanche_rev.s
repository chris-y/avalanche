VERSION = 1
REVISION = 3

.macro DATE
.ascii "21.1.2022"
.endm

.macro VERS
.ascii "Avalanche 1.3"
.endm

.macro VSTRING
.ascii "Avalanche 1.3 (21.1.2022)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 1.3 (21.1.2022)"
.byte 0
.endm
