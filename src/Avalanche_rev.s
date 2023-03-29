VERSION = 2
REVISION = 1

.macro DATE
.ascii "29.3.2023"
.endm

.macro VERS
.ascii "Avalanche 2.1"
.endm

.macro VSTRING
.ascii "Avalanche 2.1 (29.3.2023)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: Avalanche 2.1 (29.3.2023)"
.byte 0
.endm
