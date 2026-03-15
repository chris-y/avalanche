VERSION = 3
REVISION = 3

.macro DATE
.ascii "15.3.2026"
.endm

.macro VERS
.ascii "Avalanche 3.3"
.endm

.macro VSTRING
.ascii "Avalanche 3.3 (15.3.2026)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 3.3 (15.3.2026)"
.byte 0
.endm
