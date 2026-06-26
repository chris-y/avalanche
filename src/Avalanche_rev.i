VERSION		EQU	3
REVISION	EQU	4

DATE	MACRO
		dc.b '26.6.2026'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 3.4'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 3.4 (26.6.2026)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 3.4 (26.6.2026)',0
		ENDM
