G92 B0 ; Set position to 0


; homeb.g
; called to home the B axis

; increase Z
G91 ; relative positioning
G1 H2 Z10 ; move Z relative to current position to avoid dragging nozzle over the bed
G90 ; absolute positioning

; home B
G91 ; relative positioning
var maxTravel = move.axes[4].max - move.axes[4].min + 5 ; calculate how far B can travel plus 5deg
G1 H1 B{-var.maxTravel} F200 ; coarse home in the +B direction
G1 B3 F200 ; move back 3deg
G1 H1 B{-var.maxTravel} F20 ; fine home in the +B direction
G90 ; absolute positioning

G1 B0 F200;	move to level

; decrease Z again
G91 ; relative positioning
G1 H2 Z-10 ; move Z relative to current position
G90 ; absolute positioning
