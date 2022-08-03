VERSION		EQU	1
REVISION	EQU	7

DATE	MACRO
		dc.b '3.8.2022'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 1.7'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 1.7 (3.8.2022)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 1.7 (3.8.2022)',0
		ENDM
