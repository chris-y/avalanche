VERSION		EQU	1
REVISION	EQU	6

DATE	MACRO
		dc.b '31.5.2022'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 1.6'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 1.6 (31.5.2022)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 1.6 (31.5.2022)',0
		ENDM
