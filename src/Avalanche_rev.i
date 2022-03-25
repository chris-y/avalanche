VERSION		EQU	1
REVISION	EQU	5

DATE	MACRO
		dc.b '25.3.2022'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 1.5'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 1.5 (25.3.2022)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 1.5 (25.3.2022)',0
		ENDM
