fot
========

Wrap lines in (fortune cookie) text files, recognizing & preserving "`%`" separators on their own line, and refraining from breaking lines mid-word as GNU fold does.

The code is, I think, pure ANSI C99, so a simple `make` should build `fot` without any excitement.

Illustrative comparison to fold & fmt
-------------------------------------

```
$ fold -w 50 /usr/share/games/fortunes/literature | head
A banker is a fellow who lends you his umbrella wh
en the sun is shining
and wants it back the minute it begins to rain.
		-- Mark Twain
%
A classic is something that everyone wants to have
 read
and nobody wants to read.
		-- Mark Twain, "The Disappearance 
of Literature"
```

```
$ fmt -w 50 /usr/share/games/fortunes/literature | head
A banker is a fellow who lends you his umbrella
when the sun is shining and wants it back the
minute it begins to rain.
		-- Mark Twain
% A classic is something that everyone wants to
have read and nobody wants to read.
		-- Mark Twain, "The Disappearance
		of Literature"
% A horse!  A horse!  My kingdom for a horse!
		-- Wm. Shakespeare, "Richard III"
```

```
$ ./fot -w 50 /usr/share/games/fortunes/literature | head
A banker is a fellow who lends you his umbrella
when the sun is shining and wants it back the
minute it begins to rain.
		-- Mark Twain
%
A classic is something that everyone wants to have
read and nobody wants to read.
		-- Mark Twain, "The Disappearance
		of Literature"
%
```
