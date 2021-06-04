
program global_definition;

var 
  res : integer;

function gcd(a, b: integer): integer;

var
  res : integer;
begin
  if b <> 0 then
    begin
      gcd := gcd(b , a % b);
    end
  else begin
      gcd := a;
    end;
end;

begin
  writeln(gcd(28 , 7));
  writeln(gcd(88 , 11));
end
.
