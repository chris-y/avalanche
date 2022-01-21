VERSION		EQU	1
REVISION	EQU	3

DATE	MACRO
		dc.b '21.1.2022'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 1.3'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 1.3 (21.1.2022)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 1.3 (21.1.2022)',0
		ENDM
