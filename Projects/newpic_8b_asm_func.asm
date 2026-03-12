;-------------------------------
; Title: TempController
;-------------------------------
; Purpose: Measure the temperature
;          and adjust the heating/cooling system
;          base on a reference temperature
; Dependencies: <xc.inc>
; Compiler: MPLAB v6.30
; Author: DUY PHAM
; OUTPUTS: LED1 (cooling system) connected to PORTD1,
;          LED2 (heating system) connected to PORTD2
; INPUTS: measureTemp from sensor and refTemp from keypad
; Versions:
;   V1.0: 03/06/2026 - First version
;-------------------------------
    
processor 18F47K42
#include <xc.inc>
    
;----------------
; PROGRAM INPUTS
;----------------
;The DEFINE directive is used to create macros or symbolic names for values.
;It is more flexible and can be used to define complex expressions or sequences of instructions.
;It is processed by the preprocessor before the assembly begins.

#define  measuredTempInput 	45 ; this is the input value
#define  refTempInput 		25 ; this is the input value

;---------------------
; Definitions
;---------------------
#define SWITCH    LATD,2  
#define LED0      LATD,0
#define LED1	  LATD,1
    
 
;---------------------
; Program Constants
;---------------------
; The EQU (Equals) directive is used to assign a constant value to a symbolic name or label.
; It is simpler and is typically used for straightforward assignments.
;It directly substitutes the defined value into the code during the assembly process.
    
REG10   equ     10h   ; in HEX
REG11   equ     11h
REG01   equ     1h


; Program registers

refTemp      equ 20h
measuredTemp equ 21h
contReg      equ 22h
count        equ 30h
number       equ 31h

; Required BCD output registers
REF_ONES     equ 60h
REF_TENS     equ 61h
REF_HUND     equ 62h
MEA_ONES     equ 70h
MEA_TENS     equ 71h
MEA_HUND     equ 72h


; Reset Vector

    org 0x20
    goto main

;Main Program
    
main:
    ; PORTD output setup
    banksel TRISD
    clrf TRISD
    banksel LATD
    clrf LATD

    ;arbitrary values required by assignment
    movlw 0x05
    movwf refTemp
    movlw 0x06
    movwf measuredTemp


; Compare measured vs ref
; (Image-style: use BZ / BC)

    movf refTemp, W
    subwf measuredTemp, W     ; W = measured - ref
    
    bz SET_EQUAL          ; if measured == ref
    bc SET_COOL            ; if measured > ref (since not equal, and carry=1 => meas>=ref)
    goto SET_HOT           ; else measured < ref

SET_EQUAL:
    clrf contReg
    goto LED_OFF

SET_HOT:
    movlw 0x01
    movwf contReg
    goto LED_HOT

SET_COOL:
    movlw 0x02
    movwf contReg
    goto LED_COOL


; Step 3: LED actions (PORTD)

LED_HOT:
    ; Turn ON hotAir (LED1), OFF coolAir (LED0)
    banksel LATD
    bcf LATD,0
    bsf LATD,1
    goto HEX_TO_BCD

LED_COOL:
    ; Turn ON coolAir (LED0), OFF hotAir (LED1)
    banksel LATD
    bsf LATD,0
    bcf LATD,1
    goto HEX_TO_BCD

LED_OFF:
    ; Turn OFF all
    banksel LATD
    bcf LATD,0
    bcf LATD,1
    goto HEX_TO_BCD


; HEX -> DEC (BCD digits) using subtraction method (matches image)
; Store:
;   ref  -> REG60-62
;   meas -> REG70-72

HEX_TO_BCD:

;REF TEMP: refTempInput -> 0x60..0x62 
    clrf count
    movlw refTempInput
    movwf number
    movlw 100

Loop100sRef:
    incf count, F
    subwf number, F      ; F=F-W
    bc Loop100sRef    ; keep on subtracting while carry=1 (no borrow)
    decf count, F
    addwf number, F	    ;add 100 back once 
    movff count, REF_HUND ; assign to 0x062

    clrf count
    movlw 10         ; set up for the ten place

Loop10sRef:
    incf count, F
    subwf number, F
    bc Loop10sRef
    decf count, F
    addwf number, F	     ; add 10 back once
    movff count, REF_TENS  ; 0x61
    movff number, REF_ONES ; 0x60

; MEASURED TEMP: measuredTempInput -> 0x70..0x72 
    clrf count
    movlw measuredTempInput
    movwf number
    movlw 100

Loop100sMeas:
    incf count, F
    subwf number, F	    ;F=F-W
    bc Loop100sMeas    ; keep subtracting until  carry=0 (borrow)
    decf count, F
    addwf number, F
    movff count, MEA_HUND  ; 0x72

    clrf count
    movlw 10		; set up for tens places

Loop10sMeas:
    incf count, F
    subwf number, F
    bc Loop10sMeas
    decf count, F
    addwf number, F
    movff count, MEA_TENS  ;0x71
    movff number, MEA_ONES ;0x70 
 
 goto $
 
    
