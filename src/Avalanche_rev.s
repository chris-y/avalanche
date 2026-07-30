VERSION = 4
REVISION = 1

.macro DATE
.ascii "30.7.2026"
.endm

.macro VERS
.ascii "Avalanche 4.1"
.endm

.macro VSTRING
.ascii "Avalanche 4.1 (30.7.2026)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 4.1 (30.7.2026)"
.byte 0
.endm
