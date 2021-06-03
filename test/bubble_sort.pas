
program bumble_sort;

type 
  ar = array [0..9] of integer;

var 
  a: ar;
  temp: integer;
  i, j: integer;

begin
  a[0] := 8;
  a[1] := 6;
  a[2] := 3;
  a[3] := 7;
  a[4] := 5;
  a[5] := 2;
  a[6] := 4;
  a[7] := 1;
  a[8] := 9;
  a[9] := 0;

  for i := 0 to 9 do
    for j := 0 to 8 do
      if a[j] > a[j+1] then
        begin
          temp := a[j];
          a[j] := a[j+1];
          a[j+1] := temp;
        end;

  for i := 0 to 9 do
    write(a[i]);

end.
