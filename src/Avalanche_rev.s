VERSION = 2
REVISION = 2

.macro DATE
.ascii "9.5.2023"
.endm

.macro VERS
.ascii "Avalanche 2.2"
.endm

.macro VSTRING
.ascii "Avalanche 2.2 (9.5.2023)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 2.2 (9.5.2023)"
.byte 0
.endm
