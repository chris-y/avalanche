VERSION		EQU	3
REVISION	EQU	3

DATE	MACRO
		dc.b '15.3.2026'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 3.3'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 3.3 (15.3.2026)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 3.3 (15.3.2026)',0
		ENDM
