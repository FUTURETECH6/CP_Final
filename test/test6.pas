
program recursivegcd;

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
    end
  ;
end
;

begin
  ans := gcd(9 , 36) * gcd(4 , 12);
  writeln(ans);
end
.
