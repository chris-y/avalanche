VERSION = 1
REVISION = 11

.macro DATE
.ascii "11.2.2023"
.endm

.macro VERS
.ascii "Avalanche 1.11"
.endm

.macro VSTRING
.ascii "Avalanche 1.11 (11.2.2023)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 1.11 (11.2.2023)"
.byte 0
.endm
