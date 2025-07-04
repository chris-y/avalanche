VERSION = 2
REVISION = 5

.macro DATE
.ascii "4.7.2025"
.endm

.macro VERS
.ascii "Avalanche 2.5"
.endm

.macro VSTRING
.ascii "Avalanche 2.5 (4.7.2025)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 2.5 (4.7.2025)"
.byte 0
.endm
