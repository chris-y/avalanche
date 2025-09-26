VERSION		EQU	3
REVISION	EQU	1

DATE	MACRO
		dc.b '26.9.2025'
		ENDM

VERS	MACRO
		dc.b 'Avalanche 3.1'
		ENDM

VSTRING	MACRO
		dc.b 'Avalanche 3.1 (26.9.2025)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: Avalanche 3.1 (26.9.2025)',0
		ENDM
