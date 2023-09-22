VERSION = 2
REVISION = 4

.macro DATE
.ascii "22.9.2023"
.endm

.macro VERS
.ascii "Avalanche 2.4"
.endm

.macro VSTRING
.ascii "Avalanche 2.4 (22.9.2023)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 2.4 (22.9.2023)"
.byte 0
.endm
