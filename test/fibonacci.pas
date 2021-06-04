
program fibonacci;


function fib(a : integer): integer;
begin
  if a = 1 then
    begin
      fib := 1;
    end
  else
    begin
      if a = 2 then
        begin
          fib := 1;
        end
      else
        begin
          fib := fib(a - 1) + fib(a - 2);
        end;
    end;
end;

begin
  writeln(fib(10));
end.
