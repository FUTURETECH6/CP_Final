
program quick_sort;


var
  a: array[0..9] of integer;
  I: integer;

procedure QuickSort(ALo, AHi: integer);
var
  Lo, Hi, Pivot, T: integer;
begin
  Lo := ALo;
  Hi := AHi;
  Pivot := a[(Lo + Hi) div 2];
  repeat
    while a[Lo] < Pivot do
      Lo:=Lo+1 ;
    while a[Hi] > Pivot do
      Hi:=Hi-1 ;
    if Lo <= Hi then
    begin
      T := a[Lo];
      a[Lo] := a[Hi];
      a[Hi] := T;
      Lo:=Lo+1 ;
      Hi:=Hi-1 ;
    end;
  until Lo > Hi;
  if Hi > ALo then
    QuickSort(ALo, Hi) ;
  if Lo < AHi then
    QuickSort(Lo, AHi) ;
end;
 


begin
  a[1] := 6;
  a[2] := 3;
  a[3] := 7;
  a[4] := 5;
  a[5] := 2;
  a[6] := 4;
  a[7] := 1;
  a[8] := 9;
  a[9] := 0;
  a[0] := 8;
  QuickSort(0, 9);
 
  for I := 0 to 9 do
    writeln(a[i]);
end.

