(drill1)
(T2  D=5 CR=0 TAPER=120deg - ZMIN=9 - center drill)
G90 G94
G17
G21
G28 G91 Z0
G90

(Drill1 4)
T2
S5000 M3
G54
G0 X-14.023 Y-9.25
Z35
Z25
Z20
Z12
G1 Z9.5 F100.3
G0 Z20
Z11.5
G1 Z9 F100.3
G0 Z25
X2 Y-18.501
Z20
Z12
G1 Z9.5 F100.3
G0 Z20
Z11.5
G1 Z9 F100.3
G0 Z25
X18.022 Y-9.251
Z20
Z12
G1 Z9.5 F100.3
G0 Z20
Z11.5
G1 Z9 F100.3
G0 Z25
X18.023 Y9.25
Z20
Z12
G1 Z9.5 F100.3
G0 Z20
Z11.5
G1 Z9 F100.3
G0 Z25
X2 Y18.501
Z20
Z12
G1 Z9.5 F100.3
G0 Z20
Z11.5
G1 Z9 F100.3
G0 Z25
X-14.022 Y9.251
Z20
Z12
G1 Z9.5 F100.3
G0 Z20
Z11.5
G1 Z9 F100.3
G0 Z25
Z35
G28 G91 Z0
G90
G28 G91 X0 Y0
G90
M5
M30
