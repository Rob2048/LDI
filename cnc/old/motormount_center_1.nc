(motormount_center_1)
(T2  D=5 CR=0 TAPER=120deg - ZMIN=2 - center drill)
G90 G94
G17
G21
G28 G91 Z0
G90

(Drill1)
T2
S5000 M3
G54
G0 X-2 Y-22
Z25.2
Z15.2
Z11
Z8
G1 Z5.5 F100.3
G0 Z11
Z7.5
G1 Z5 F100.3
G0 Z11
Z7
G1 Z4.5 F100.3
G0 Z11
Z6.5
G1 Z4 F100.3
G0 Z11
Z6
G1 Z3.5 F100.3
G0 Z11
Z5.5
G1 Z3 F100.3
G0 Z11
Z5
G1 Z2.5 F100.3
G0 Z11
Z4.5
G1 Z2 F100.3
G0 Z15.2
Y22
Z11
Z8
G1 Z5.5 F100.3
G0 Z11
Z7.5
G1 Z5 F100.3
G0 Z11
Z7
G1 Z4.5 F100.3
G0 Z11
Z6.5
G1 Z4 F100.3
G0 Z11
Z6
G1 Z3.5 F100.3
G0 Z11
Z5.5
G1 Z3 F100.3
G0 Z11
Z5
G1 Z2.5 F100.3
G0 Z11
Z4.5
G1 Z2 F100.3
G0 Z15.2
Z25.2
G28 G91 Z0
G90
G28 G91 X0 Y0
G90
M5
M30
