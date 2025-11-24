VERSION = 3
REVISION = 2

.macro DATE
.ascii "24.11.2025"
.endm

.macro VERS
.ascii "Avalanche 3.2"
.endm

.macro VSTRING
.ascii "Avalanche 3.2 (24.11.2025)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 3.2 (24.11.2025)"
.byte 0
.endm
