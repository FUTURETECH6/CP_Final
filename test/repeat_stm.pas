
program repeat_stm;

var 
  i,s : integer;

begin
  s:=1;
  i:=1;
  repeat
    s:=s*i;
    i:=i+2;
    until i > 8;
writeln(s);
end.