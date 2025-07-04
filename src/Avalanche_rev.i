VERSION		EQU	2
REVISION	EQU	5

DATE	MACRO
		dc.b '4.7.2025'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 2.5'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 2.5 (4.7.2025)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 2.5 (4.7.2025)',0
		ENDM
