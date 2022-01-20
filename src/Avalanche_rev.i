VERSION		EQU	1
REVISION	EQU	2

DATE	MACRO
		dc.b '20.1.2022'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 1.2'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 1.2 (20.1.2022)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 1.2 (20.1.2022)',0
		ENDM
