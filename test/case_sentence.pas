program case_sentence;

var
  s:integer;
  ch:integer;
begin
  s := 8;
  case s of
    10,9 : ch:=1;
    8 : ch:=2;
    7,6 : ch:=3;
    else ch:=4;
  end;
  writeln(ch);
end.