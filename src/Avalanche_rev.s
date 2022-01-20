VERSION = 1
REVISION = 2

.macro DATE
.ascii "20.1.2022"
.endm

.macro VERS
.ascii "Avalanche 1.2"
.endm

.macro VSTRING
.ascii "Avalanche 1.2 (20.1.2022)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 1.2 (20.1.2022)"
.byte 0
.endm
