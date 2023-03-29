VERSION		EQU	2
REVISION	EQU	1

DATE	MACRO
		dc.b '29.3.2023'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 2.1'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 2.1 (29.3.2023)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 2.1 (29.3.2023)',0
		ENDM
