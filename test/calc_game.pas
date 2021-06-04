
program calc_game;

const 
  factor = 2;

var 
  res, k : integer;

function cal(b : integer): integer;
begin
  if b = 0 then cal := b + 3
  else
    begin
      if b = 4 then cal := b + 4
      else
        begin
          cal := cal(b - 1) - 1;
        end;
    end;
end;

begin
  res := cal(7);
  for k := 0 to 10 do
    begin
      res := res * factor;
    end;
  write(res);
end.
