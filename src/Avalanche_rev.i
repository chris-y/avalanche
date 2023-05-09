VERSION		EQU	2
REVISION	EQU	2

DATE	MACRO
		dc.b '9.5.2023'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 2.2'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 2.2 (9.5.2023)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 2.2 (9.5.2023)',0
		ENDM
