VERSION = 3
REVISION = 4

.macro DATE
.ascii "26.6.2026"
.endm

.macro VERS
.ascii "Avalanche 3.4"
.endm

.macro VSTRING
.ascii "Avalanche 3.4 (26.6.2026)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 3.4 (26.6.2026)"
.byte 0
.endm
