VERSION = 1
REVISION = 1

.macro DATE
.ascii "11.1.2022"
.endm

.macro VERS
.ascii "Avalanche 1.1"
.endm

.macro VSTRING
.ascii "Avalanche 1.1 (11.1.2022)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 1.1 (11.1.2022)"
.byte 0
.endm
