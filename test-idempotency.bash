#!/bin/bash
WID=50
echo -n "installed fot, run once on input:  "
fot -w $WID $1 | md5sum -
echo -n "installed fot, run twice on input: "
fot -w $WID $1 | fot -w $WID | md5sum -
echo -n "cur. dir. fot, run once on input:  "
./fot -w $WID $1 | md5sum -
echo -n "cur. dir. fot, run twice on input: "
./fot -w $WID $1 | ./fot -w $WID | md5sum -
