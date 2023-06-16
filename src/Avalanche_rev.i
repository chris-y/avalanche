VERSION		EQU	2
REVISION	EQU	3

DATE	MACRO
		dc.b '16.6.2023'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 2.3'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 2.3 (16.6.2023)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 2.3 (16.6.2023)',0
		ENDM
