
program quick_sort;

var 
  a: array[0..9] of integer;
  i: integer;

procedure Quicksort(Left, Right: integer);

var 
  ptrLeft, ptrRight, Pivot, Temp: integer;
begin
  ptrLeft := Left;
  ptrRight := Right;
  Pivot := a[(Left + Right) / 2];

  repeat
    while (ptrLeft < Right) and (a[ptrLeft] < Pivot) do
      // while a[ptrLeft] < Pivot do
      ptrLeft := ptrLeft + 1;
    while (ptrRight > Left) and (a[ptrRight] > Pivot) do
      // while a[ptrRight] > Pivot do
      ptrRight := ptrRight - 1;
    if ptrLeft <= ptrRight then
      begin
        if ptrLeft < ptrRight then
          begin
            Temp := a[ptrLeft];
            a[ptrLeft] := a[ptrRight];
            a[ptrRight] := Temp;
          end;
        ptrLeft := ptrLeft + 1;
        ptrRight := ptrRight - 1;
      end;
  until ptrLeft > ptrRight;

  if ptrRight > Left then
    Quicksort(Left, ptrRight);
  if ptrLeft < Right then
    Quicksort(ptrLeft, Right);
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
  Quicksort(0, 9);

  for i := 0 to 9 do
    writeln(a[i]);
end.
