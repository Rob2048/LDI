(contour2)
(T6 D=3.175 CR=0 - ZMIN=0 - flat end mill)
G90 G94
G17
G21
G28 G91 Z0
G90

(2D Contour2 3)
T6
(18 Flat-45 degree helix)
S18000 M3
G17 G90 G94
G54
G0 X23.199 Y53.564
Z30
G0 Z11.5
G1 Z11 F30
G2 X19.801 Y45.936 Z10.542 I-1.699 J-3.814 F1000
X23.199 Y53.564 Z10.084 I1.699 J3.814
X19.801 Y45.936 Z9.626 I-1.699 J-3.814
X23.199 Y53.564 Z9.168 I1.699 J3.814
X25.675 Y49.75 Z9 I-1.699 J-3.814
X17.325 Y49.75 I-4.175 J0
X25.675 Y49.75 I4.175 J0
X17.325 Y49.75 Z8.542 I-4.175 J0
X25.675 Y49.75 Z8.084 I4.175 J0
X25.002 Y47.477 Z8 I-4.175 J0
X17.998 Y52.024 I-3.502 J2.273
X25.002 Y47.477 I3.502 J-2.273
X17.998 Y52.024 Z7.542 I-3.502 J2.273
X25.002 Y47.477 Z7.084 I3.502 J-2.273
X23.199 Y45.936 Z7 I-3.502 J2.273
X19.801 Y53.564 I-1.699 J3.814
X23.199 Y45.936 I1.699 J-3.814
G0 Z20
X6.305 Y39.004
Z11.5
G1 Z11 F30
G2 X11.195 Y45.773 Z10.542 I2.445 J3.385 F1000
X6.305 Y39.004 Z10.084 I-2.445 J-3.385
X11.195 Y45.773 Z9.626 I2.445 J3.385
X6.305 Y39.004 Z9.168 I-2.445 J-3.385
X4.663 Y43.244 Z9 I2.445 J3.385
X12.837 Y41.534 I4.087 J-0.855
X4.663 Y43.244 I-4.087 J0.855
X12.837 Y41.534 Z8.542 I4.087 J-0.855
X4.663 Y43.244 Z8.084 I-4.087 J0.855
X5.788 Y45.331 Z8 I4.087 J-0.855
X11.712 Y39.447 I2.962 J-2.942
X5.788 Y45.331 I-2.962 J2.942
X11.712 Y39.447 Z7.542 I2.962 J-2.942
X5.788 Y45.331 Z7.084 I-2.962 J2.942
X7.867 Y46.469 Z7 I2.962 J-2.942
X9.633 Y38.308 I0.883 J-4.081
X7.867 Y46.469 I-0.883 J4.081
G0 Z20
Y53.031
Z11.5
G1 Z11 F30
G2 X9.633 Y61.192 Z10.542 I0.883 J4.081 F1000
X7.867 Y53.031 Z10.084 I-0.883 J-4.081
X9.633 Y61.192 Z9.626 I0.883 J4.081
X7.867 Y53.031 Z9.168 I-0.883 J-4.081
X4.664 Y56.256 Z9 I0.883 J4.081
X12.837 Y57.966 I4.087 J0.855
X4.664 Y56.256 I-4.087 J-0.855
X12.837 Y57.966 Z8.542 I4.087 J0.855
X4.664 Y56.256 Z8.084 I-4.087 J-0.855
X4.857 Y58.619 Z8 I4.087 J0.855
X12.643 Y55.603 I3.893 J-1.508
X4.857 Y58.619 I-3.893 J1.508
X12.643 Y55.603 Z7.542 I3.893 J-1.508
X4.857 Y58.619 Z7.084 I-3.893 J1.508
X6.306 Y60.496 Z7 I3.893 J-1.508
X11.195 Y53.727 I2.445 J-3.385
X6.306 Y60.496 I-2.445 J3.385
G0 Z30
X10.096 Y14.726
Z11.5
G1 Z11 F30
G3 X15.904 Y10.774 Z10.615 I2.904 J-1.976 F1000
X10.096 Y14.726 Z10.229 I-2.904 J1.976
X15.904 Y10.774 Z9.844 I2.904 J-1.976
X10.096 Y14.726 Z9.459 I-2.904 J1.976
X15.904 Y10.774 Z9.073 I2.904 J-1.976
X16.513 Y12.75 Z9 I-2.904 J1.976
X9.487 Y12.75 I-3.513 J0
X16.513 Y12.75 I3.513 J0
X9.487 Y12.75 Z8.615 I-3.513 J0
X16.512 Y12.75 Z8.229 I3.513 J0
X11.966 Y16.107 Z8 I-3.512 J0
X14.034 Y9.393 I1.034 J-3.357
X11.966 Y16.107 I-1.034 J3.357
X14.034 Y9.393 Z7.615 I1.034 J-3.357
X11.966 Y16.107 Z7.229 I-1.034 J3.357
X10.096 Y10.774 Z7 I1.034 J-3.357
X15.904 Y14.726 I2.904 J1.976
X10.096 Y10.774 I-2.904 J-1.976
X15.904 Y14.726 Z6.615 I2.904 J1.976
X10.096 Y10.774 Z6.229 I-2.904 J-1.976
X15.743 Y10.556 Z6 I2.904 J1.976
X10.257 Y14.944 I-2.743 J2.194
X15.743 Y10.556 I2.743 J-2.194
X10.257 Y14.944 Z5.615 I-2.743 J2.194
X15.743 Y10.556 Z5.229 I2.743 J-2.194
X14.29 Y16.017 Z5 I-2.743 J2.194
X11.71 Y9.483 I-1.29 J-3.267
X14.29 Y16.017 I1.29 J3.267
X11.71 Y9.483 Z4.615 I-1.29 J-3.267
X14.29 Y16.017 Z4.229 I1.29 J3.267
X9.498 Y13.021 Z4 I-1.29 J-3.267
X16.502 Y12.479 I3.502 J-0.271
X9.498 Y13.021 I-3.502 J0.271
G0 Z30
X10.096 Y51.726
Z11.5
G1 Z11 F30
G3 X15.904 Y47.774 Z10.615 I2.904 J-1.976 F1000
X10.096 Y51.726 Z10.229 I-2.904 J1.976
X15.904 Y47.774 Z9.844 I2.904 J-1.976
X10.096 Y51.726 Z9.459 I-2.904 J1.976
X15.904 Y47.774 Z9.073 I2.904 J-1.976
X16.513 Y49.75 Z9 I-2.904 J1.976
X9.487 Y49.75 I-3.513 J0
X16.513 Y49.75 I3.513 J0
X9.487 Y49.75 Z8.615 I-3.513 J0
X16.513 Y49.75 Z8.229 I3.513 J0
X11.966 Y53.107 Z8 I-3.513 J0
X14.034 Y46.393 I1.034 J-3.357
X11.966 Y53.107 I-1.034 J3.357
X14.034 Y46.393 Z7.615 I1.034 J-3.357
X11.966 Y53.107 Z7.229 I-1.034 J3.357
X10.096 Y47.774 Z7 I1.034 J-3.357
X15.904 Y51.726 I2.904 J1.976
X10.096 Y47.774 I-2.904 J-1.976
X15.904 Y51.726 Z6.615 I2.904 J1.976
X10.096 Y47.774 Z6.229 I-2.904 J-1.976
X15.743 Y47.556 Z6 I2.904 J1.976
X10.257 Y51.944 I-2.743 J2.194
X15.743 Y47.556 I2.743 J-2.194
X10.257 Y51.944 Z5.615 I-2.743 J2.194
X15.743 Y47.556 Z5.229 I2.743 J-2.194
X14.29 Y53.017 Z5 I-2.743 J2.194
X11.71 Y46.483 I-1.29 J-3.267
X14.29 Y53.017 I1.29 J3.267
X11.71 Y46.483 Z4.615 I-1.29 J-3.267
X14.29 Y53.017 Z4.229 I1.29 J3.267
X9.498 Y50.021 Z4 I-1.29 J-3.267
X16.502 Y49.479 I3.502 J-0.271
X9.498 Y50.021 I-3.502 J0.271
X16.502 Y49.479 Z3.615 I3.502 J-0.271
X9.498 Y50.021 Z3.229 I-3.502 J0.271
X13.771 Y46.323 Z3 I3.502 J-0.271
X12.229 Y53.177 I-0.771 J3.427
X13.771 Y46.323 I0.771 J-3.427
G0 Z30
X10.626 Y50.177
Z11.5
G1 Z11 F30
G3 X15.374 Y49.323 Z10.735 I2.374 J-0.427 F1000
X10.626 Y50.177 Z10.471 I-2.374 J0.427
X15.374 Y49.323 Z10.206 I2.374 J-0.427
X10.626 Y50.177 Z9.941 I-2.374 J0.427
X15.374 Y49.323 Z9.677 I2.374 J-0.427
X10.626 Y50.177 Z9.412 I-2.374 J0.427
X15.374 Y49.323 Z9.147 I2.374 J-0.427
X13 Y52.162 Z9 I-2.374 J0.427
X13 Y47.338 I0 J-2.412
X13 Y52.162 I0 J2.412
X13 Y47.338 Z8.735 I0 J-2.412
X13 Y52.162 Z8.471 I0 J2.412
X13 Y47.338 Z8.206 I0 J-2.412
X14.548 Y51.601 Z8 I0 J2.412
X11.452 Y47.899 I-1.548 J-1.851
X14.548 Y51.601 I1.548 J1.851
X11.452 Y47.899 Z7.735 I-1.548 J-1.851
X14.548 Y51.601 Z7.471 I1.548 J1.851
X11.452 Y47.899 Z7.206 I-1.548 J-1.851
X15.374 Y50.177 Z7 I1.548 J1.851
X10.626 Y49.323 I-2.374 J-0.427
X15.374 Y50.177 I2.374 J0.427
X10.626 Y49.323 Z6.735 I-2.374 J-0.427
X15.374 Y50.177 Z6.471 I2.374 J0.427
X10.626 Y49.323 Z6.206 I-2.374 J-0.427
X15.095 Y48.554 Z6 I2.374 J0.427
X10.905 Y50.946 I-2.095 J1.196
X15.095 Y48.554 I2.095 J-1.196
X10.905 Y50.946 Z5.735 I-2.095 J1.196
X15.095 Y48.554 Z5.471 I2.095 J-1.196
X10.905 Y50.946 Z5.206 I-2.095 J1.196
X13.841 Y47.489 Z5 I2.095 J-1.196
X12.159 Y52.011 I-0.841 J2.261
X13.841 Y47.489 I0.841 J-2.261
X12.159 Y52.011 Z4.735 I-0.841 J2.261
X13.841 Y47.489 Z4.471 I0.841 J-2.261
X12.159 Y52.011 Z4.206 I-0.841 J2.261
X12.194 Y47.476 Z4 I0.841 J-2.261
X13.806 Y52.024 I0.806 J2.274
X12.194 Y47.476 I-0.806 J-2.274
X13.806 Y52.024 Z3.735 I0.806 J2.274
X12.194 Y47.476 Z3.471 I-0.806 J-2.274
X13.806 Y52.024 Z3.206 I0.806 J2.274
X10.923 Y48.522 Z3 I-0.806 J-2.274
X15.077 Y50.978 I2.077 J1.228
X10.923 Y48.522 I-2.077 J-1.228
X15.077 Y50.978 Z2.735 I2.077 J1.228
X10.923 Y48.522 Z2.471 I-2.077 J-1.228
X15.077 Y50.978 Z2.206 I2.077 J1.228
X10.619 Y50.14 Z2 I-2.077 J-1.228
X15.381 Y49.36 I2.381 J-0.39
X10.619 Y50.14 I-2.381 J0.39
X15.381 Y49.36 Z1.735 I2.381 J-0.39
X10.619 Y50.14 Z1.471 I-2.381 J0.39
X15.381 Y49.36 Z1.206 I2.381 J-0.39
X11.424 Y51.577 Z1 I-2.381 J0.39
X14.576 Y47.923 I1.576 J-1.827
X11.424 Y51.577 I-1.576 J1.827
X14.576 Y47.923 Z0.735 I1.576 J-1.827
X11.424 Y51.577 Z0.471 I-1.576 J1.827
X14.576 Y47.923 Z0.206 I1.576 J-1.827
X12.963 Y52.162 Z0 I-1.576 J1.827
X13.037 Y47.338 I0.037 J-2.412
X12.963 Y52.162 I-0.037 J2.412
G0 Z20
X10.626 Y13.177
Z11.5
G1 Z11 F30
G3 X15.374 Y12.323 Z10.735 I2.374 J-0.427 F1000
X10.626 Y13.177 Z10.471 I-2.374 J0.427
X15.374 Y12.323 Z10.206 I2.374 J-0.427
X10.626 Y13.177 Z9.941 I-2.374 J0.427
X15.374 Y12.323 Z9.677 I2.374 J-0.427
X10.626 Y13.177 Z9.412 I-2.374 J0.427
X15.374 Y12.323 Z9.147 I2.374 J-0.427
X13 Y15.163 Z9 I-2.374 J0.427
X13 Y10.337 I0 J-2.413
X13 Y15.163 I0 J2.413
X13 Y10.337 Z8.735 I0 J-2.413
X13 Y15.163 Z8.471 I0 J2.413
X13 Y10.338 Z8.206 I0 J-2.413
X14.548 Y14.601 Z8 I0 J2.412
X11.452 Y10.899 I-1.548 J-1.851
X14.548 Y14.601 I1.548 J1.851
X11.452 Y10.899 Z7.735 I-1.548 J-1.851
X14.548 Y14.601 Z7.471 I1.548 J1.851
X11.452 Y10.899 Z7.206 I-1.548 J-1.851
X15.374 Y13.177 Z7 I1.548 J1.851
X10.626 Y12.323 I-2.374 J-0.427
X15.374 Y13.177 I2.374 J0.427
X10.626 Y12.323 Z6.735 I-2.374 J-0.427
X15.374 Y13.177 Z6.471 I2.374 J0.427
X10.626 Y12.323 Z6.206 I-2.374 J-0.427
X15.095 Y11.554 Z6 I2.374 J0.427
X10.905 Y13.946 I-2.095 J1.196
X15.095 Y11.554 I2.095 J-1.196
X10.905 Y13.946 Z5.735 I-2.095 J1.196
X15.095 Y11.554 Z5.471 I2.095 J-1.196
X10.905 Y13.946 Z5.206 I-2.095 J1.196
X13.841 Y10.489 Z5 I2.095 J-1.196
X12.159 Y15.011 I-0.841 J2.261
X13.841 Y10.489 I0.841 J-2.261
X12.159 Y15.011 Z4.735 I-0.841 J2.261
X13.841 Y10.489 Z4.471 I0.841 J-2.261
X12.159 Y15.011 Z4.206 I-0.841 J2.261
X12.194 Y10.476 Z4 I0.841 J-2.261
X13.806 Y15.024 I0.806 J2.274
X12.194 Y10.476 I-0.806 J-2.274
X13.806 Y15.024 Z3.735 I0.806 J2.274
X12.194 Y10.476 Z3.471 I-0.806 J-2.274
X13.806 Y15.024 Z3.206 I0.806 J2.274
X10.923 Y11.522 Z3 I-0.806 J-2.274
X15.077 Y13.978 I2.077 J1.228
X10.923 Y11.522 I-2.077 J-1.228
X15.077 Y13.978 Z2.735 I2.077 J1.228
X10.923 Y11.522 Z2.471 I-2.077 J-1.228
X15.077 Y13.978 Z2.206 I2.077 J1.228
X10.619 Y13.14 Z2 I-2.077 J-1.228
X15.381 Y12.36 I2.381 J-0.39
X10.619 Y13.14 I-2.381 J0.39
X15.381 Y12.36 Z1.735 I2.381 J-0.39
X10.619 Y13.14 Z1.471 I-2.381 J0.39
X15.381 Y12.36 Z1.206 I2.381 J-0.39
X11.424 Y14.577 Z1 I-2.381 J0.39
X14.576 Y10.923 I1.576 J-1.827
X11.424 Y14.577 I-1.576 J1.827
X14.576 Y10.923 Z0.735 I1.576 J-1.827
X11.424 Y14.577 Z0.471 I-1.576 J1.827
X14.576 Y10.923 Z0.206 I1.576 J-1.827
X12.963 Y15.162 Z0 I-1.576 J1.827
X13.037 Y10.338 I0.037 J-2.412
X12.963 Y15.162 I-0.037 J2.412
G0 Z30
G28 G91 Z0
G90
G28 G91 X0 Y0
G90
M5
M30