VERSION		EQU	1
REVISION	EQU	8

DATE	MACRO
		dc.b '7.12.2022'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 1.8'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 1.8 (7.12.2022)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 1.8 (7.12.2022)',0
		ENDM
