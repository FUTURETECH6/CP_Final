
program for_sentence;

var 
  i,res : integer;
  sum : integer;


function add(n: integer): integer;
begin
  for i:=1 to n do
    sum := sum + i;
  add := sum;
end;

begin
  res := add(7);
  writeln(res);
end.
