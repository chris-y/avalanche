VERSION = 3
REVISION = 1

.macro DATE
.ascii "26.9.2025"
.endm

.macro VERS
.ascii "Avalanche 3.1"
.endm

.macro VSTRING
.ascii "Avalanche 3.1 (26.9.2025)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 3.1 (26.9.2025)"
.byte 0
.endm
