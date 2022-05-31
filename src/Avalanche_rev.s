VERSION = 1
REVISION = 6

.macro DATE
.ascii "31.5.2022"
.endm

.macro VERS
.ascii "Avalanche 1.6"
.endm

.macro VSTRING
.ascii "Avalanche 1.6 (31.5.2022)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 1.6 (31.5.2022)"
.byte 0
.endm
