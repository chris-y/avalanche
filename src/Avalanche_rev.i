VERSION		EQU	2
REVISION	EQU	4

DATE	MACRO
		dc.b '22.9.2023'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 2.4'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 2.4 (22.9.2023)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 2.4 (22.9.2023)',0
		ENDM
