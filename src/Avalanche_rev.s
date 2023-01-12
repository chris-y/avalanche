VERSION = 1
REVISION = 10

.macro DATE
.ascii "12.1.2023"
.endm

.macro VERS
.ascii "Avalanche 1.10"
.endm

.macro VSTRING
.ascii "Avalanche 1.10 (12.1.2023)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 1.10 (12.1.2023)"
.byte 0
.endm
