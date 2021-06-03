program bumble_sort;


type 
  ar = array [1..8] of integer;

var
  a:ar;
  temp: integer;
  i,j:integer;

begin
  a[1] := 6;
  a[2] := 3;
  a[3] := 7;
  a[4] := 5;
  a[5] := 2;
  a[6] := 0;
  a[7] := 1;
  a[8] := 9;

  for i:=1 to 7 do 
    for j:=1 to 8-i do  
      if a[j]<a[j+1] then 
        begin
          temp:= a[j];
          a[j]:= a[j+1];
          a[j+1]:=temp;
        end;

  for i:=1 to 8 do   
    begin
    write(a[i]:4);
    if i mod 5=0
    then writeln
    end;
end.
