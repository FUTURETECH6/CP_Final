
program gcd;

var 
  ans : integer;

function gcd(a, b : integer) : integer;
begin
  if b = 0 then
    begin
      gcd := a;
    end
  else
    begin
      gcd := gcd(b , a % b);
    end;
end;

begin
  writeln(gcd(9 , 36));
  writeln(gcd(15 , 27));
end.
