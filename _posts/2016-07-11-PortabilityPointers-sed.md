---
title:  "Portability Pointers: sed"
categories:
  - "Portability Pointers"
tags:
  - sed
  - 21st Century
---

## sed: Not Portable

So you want to use something like `sed -r -n 's/^.*([0-9]+\.[0-9]+\.[0-9]+).*$/\1/p'` to extract the version of some program in your Makefile and put it into a variable?  Well I hope you like walled gardens, because that, my friend, is NON-PORTABLE!  Works fine on Linux and Cygwin, luring you in with its false sense of security, then BAM, it totally collapses on a *BSD or OSX system.  And the worst part about it is, the problem is not what you think: the backref is totally fine.

It's the `-r`.  Meaning "use extended regular expressions instead of the 'basic' regexes that nobody would ever use because they make an already impossible-to-read expression even less possible to read, if that's even possible", at least to [GNU `sed`](http://www.gnu.org/software/sed/).  For reasons known only to... well, nobody..., the non-GNU `sed` on *BSDs and OSXes uses `-E` for this purpose instead.  True story.

Yep, in the year 200-ought-16, 21st century, [extended regular expressions are not portable](http://pubs.opengroup.org/onlinepubs/9699919799/utilities/sed.html):

> Regular Expressions in sed
> 
> The sed utility shall support the BREs [Basic Regular Expressions] described in XBD Basic Regular Expressions,[...]

Well, at least not in `sed` anyway.  Thanks [Open Group Base Specifications Issue 7](http://pubs.opengroup.org/onlinepubs/9699919799/).  If that is your real name.

### sed Portability Solutions

"So what do we do Gary!?!?!??!?", you ask?  Well, you only have three options:

1. Learn to read and write Basic Regular Expressions

    This is what Open Group would have you do.  Yep, me neither, I only list it for completeness.

2. Do `autoconf`'s job for it, and write an `M4` macro to find an installed `sed`, then determine the correct command-line param... and I lost you at `M4`.  No prob, this next one is what I do currently:

3. Cheat: Use `sed -E`

Fun fact: GNU `sed` accepts `-E` as a synonym for `-r`.  It's not in the `--help`, nor the `man` page, but it's there and it works - on at least the Linuxes, Cygwins, PC-BSDs (FreeBSDs), and even OSXs I've tried.  Number 3 for the win!  You.  Are.  Welcome!

> Editor's Note: Of course, #2 is what you should do in a production context, but that doesn't read as well.
