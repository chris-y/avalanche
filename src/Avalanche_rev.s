VERSION = 2
REVISION = 3

.macro DATE
.ascii "16.6.2023"
.endm

.macro VERS
.ascii "Avalanche 2.3"
.endm

.macro VSTRING
.ascii "Avalanche 2.3 (16.6.2023)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 2.3 (16.6.2023)"
.byte 0
.endm
