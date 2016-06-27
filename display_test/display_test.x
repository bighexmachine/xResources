

val mcount = 10000;

val framebuff = #7FF0;

proc main() is
  while true do
  { horizontal();
    waitsec();
    vertical();
    waitsec()
  }

proc horizontal() is
  var i;
{ i := 0;
  while i < 16 do
  { framebuff[i] := #FFFF;
    framebuff[i+1] := 0;
    i := i + 2
  }
}

proc vertical() is
  var i;
{ i := 0;
  while i < 16 do
  { framebuff[i] := #5555;
    i := i + 1
  }
}

proc waitmsec() is
  var i;
{ i := 0;
  while i < mcount do 
    i := i + 1
}

proc waitsec() is
  var i;
{ i := 0;
  while i < 1000 do
  { waitmsec();
    i := i + 1
  }
}


