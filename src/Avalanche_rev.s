VERSION = 1
REVISION = 5

.macro DATE
.ascii "25.3.2022"
.endm

.macro VERS
.ascii "Avalanche 1.5"
.endm

.macro VSTRING
.ascii "Avalanche 1.5 (25.3.2022)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 1.5 (25.3.2022)"
.byte 0
.endm
