/ pos=fail warn=2 error=3
#backread.dat,
C1 [inch,10.23,20.47,1234.56], C3 [m,10.23,20.47,1234.56],
C2 [n,10.23,20.47,1234.56], C3 [m,10.23,20.47,1234.56],
C3 [f,10.23,20.47,1234.56];
/ Test that * is ignored
*30;
%0.31;
/ Test that % is ignored
$30;
%0.31;
/ Test that ! is ignored
!ot;
/ Test an unknown command gives an error
?what;

/ Regression test for handling of blank lines


*30;

