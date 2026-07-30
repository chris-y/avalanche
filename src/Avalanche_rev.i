VERSION		EQU	4
REVISION	EQU	1

DATE	MACRO
		dc.b '30.7.2026'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 4.1'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 4.1 (30.7.2026)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 4.1 (30.7.2026)',0
		ENDM
