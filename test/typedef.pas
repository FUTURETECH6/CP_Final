
program typedef;

const 
  a = 3;

type 
  ar = array [0..10] of integer;

var 
  c : ar;
  i : integer;
  j : integer;

begin
  c[0] := 1;
  c[1] := 2;
  c[2] := 3;
  c[3] := 4;
  c[4] := 5;
  c[5] := 6;
  c[6] := 7;
  c[7] := 8;
  c[8] := 9;
  c[9] := 10;
  c[10] := 11;
  for i := 0 to 10 do
    begin
      c[i] := c[i] * c[i];
      writeln(c[i]);
    end;
end.
