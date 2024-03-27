LINKSCAN_P         equ    0x80 ; port data base address
LINKSCAN_LINK      equ    0x0  ; link down/up
LINKSCAN_TX        equ    0x4  ; transmitted packet
LINKSCAN_RX        equ    0x5  ; received packet

PORTSTATUS_P       equ    0x00 ; port status base address
PORTSTATUS_RX      equ    0x0  ; received packet
PORTSTATUS_TX      equ    0x1  ; transmitted packet
PORTSTATUS_COLL    equ    0x2  ; collision
PORTSTATUS_SPEED_C equ    0x3  ; 100Mbps
PORTSTATUS_SPEED_M equ    0x4  ; 1Gbps
PORTSTATUS_DUPLEX  equ    0x5  ; half/full duplex
PORTSTATUS_FLOW    equ    0x6  ; flow control capable
PORTSTATUS_LINKUP  equ    0x7  ; link down/up
PORTSTATUS_LINKEN  equ    0x8  ; link disabled/enabled
PORTSTATUS_ZERO    equ    0xe  ; always 0
PORTSTATUS_ONE     equ    0xf  ; always 1

TICKS_PER_SEC      equ    30   ; LED program runs at 30Hz

TICKS_PER_BLINK    equ    3    ; 0 to alternate every cycle (15Hz)
NUM_LEDS           equ    72

BLINK_TICKS_P      equ    0xf0
BLINK_STATE_P      equ    0xf1

main:
        ld      a,1
        call    set_led_CelRedstoneXpSFPp
        ld      a,2
        call    set_led_CelRedstoneXpSFPp
        ld      a,3
        call    set_led_CelRedstoneXpSFPp
        ld      a,4
        call    set_led_CelRedstoneXpSFPp
        ld      a,5
        call    set_led_CelRedstoneXpSFPp
        ld      a,6
        call    set_led_CelRedstoneXpSFPp
        ld      a,7
        call    set_led_CelRedstoneXpSFPp
        ld      a,8
        call    set_led_CelRedstoneXpSFPp
        ld      a,9
        call    set_led_CelRedstoneXpSFPp
        ld      a,10
        call    set_led_CelRedstoneXpSFPp
        ld      a,11
        call    set_led_CelRedstoneXpSFPp
        ld      a,12
        call    set_led_CelRedstoneXpSFPp
        ld      a,13
        call    set_led_CelRedstoneXpQSFPp
        ld      a,14
        call    set_led_CelRedstoneXpQSFPp
        ld      a,15
        call    set_led_CelRedstoneXpQSFPp
        ld      a,16
        call    set_led_CelRedstoneXpQSFPp
        ld      a,17
        call    set_led_CelRedstoneXpQSFPp
        ld      a,18
        call    set_led_CelRedstoneXpQSFPp

        call blink_update_state
        send NUM_LEDS

blink_update_state:
        ld      a,(BLINK_TICKS_P)
        cmp     a,0
        jz      blink_toggle
        dec     a
        ld      (BLINK_TICKS_P),a
        ret
blink_toggle:
        ld      a,(BLINK_STATE_P)
        xor     a,0x01
        and     a,0x01
        ld      (BLINK_STATE_P),a
        ld      a,TICKS_PER_BLINK
        ld      (BLINK_TICKS_P),a
        ret

set_led_CelRedstoneXpQSFPp:
        call    set_any_led    ; set the first led green/off

        push    PORTSTATUS_ONE ; 1, 1 => led 2 off
        pack
        push    PORTSTATUS_ONE
        pack

        push    PORTSTATUS_ONE ; 1, 1 => led 3 off
        pack
        push    PORTSTATUS_ONE
        pack

        push    PORTSTATUS_ONE ; 1, 1 => led 4 off
        pack
        push    PORTSTATUS_ONE
        pack

        ret

set_led_CelRedstoneXpSFPp:
set_any_led:
        port a

        pushst  PORTSTATUS_RX
        pushst  PORTSTATUS_TX
        tor
        push    (BLINK_STATE_P)
        tand
        tinv                    ; flash off with activity
        pushst  PORTSTATUS_LINKEN
        tand
        tinv
        pack

        port 1
        pushst  PORTSTATUS_ZERO ; disable yellow LED
        pack

        ret


