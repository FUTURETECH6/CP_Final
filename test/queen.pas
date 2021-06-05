
program queen;

var 
  a: array [1..8] of integer;
  b,c,d: array [0..23] of integer;
  t,k: integer;

procedure print;
begin
  t := t+1;
  write(t);
  write(' ');
  for k:=1 to 8 do
    begin
      write(a[k]);
      write(' ');
    end;
  writeln;
end;

procedure 
try
  (i:integer);

  var 
    j: integer;
  begin
    for j:=1 to 8 do
      if (b[j+7]=0) and (c[i+j+7]=0) and (d[i-j+7]=0) then
        begin
          a[i] := j;
          b[j+7] := 1;
          c[i+j+7] := 1;
          d[i-j+7] := 1;
          if i<8 then
            try
              (i+1)
              else print;
              b[j+7] := 0;
              c[i+j+7] := 0;
              d[i-j+7] := 0;
        end;
  end;
  begin
    for k:=-7 to 16 do
      begin
        b[k+7] := 0;
        c[k+7] := 0;
        d[k+7] := 0;
      end;
    try
      (1);
  end.
