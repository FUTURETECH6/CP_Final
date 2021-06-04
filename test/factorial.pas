
program factorial;

const 
  a = 3;

var 
  i,j : integer;
  ar : array [0..5] of integer;


begin  
  ar[5] := 6;
  ar[4] := 5;
  ar[3] := 4;
  ar[2] := 3;
  ar[1] := 2;
  ar[0] := 1;

  for i := 0 to 5 do
    begin
      if i = 0 then ar[i] := ar[i] * a
      else
        begin
          ar[i] := ar[i] * ar[i - 1];
        end;
      writeln(ar[i]);
    end;
end.
