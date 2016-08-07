---
title:  "Portability Pointers: POPCNT"
categories: 
  - "Portability Pointers"
tags:
  - POPCNT
---

## The POPCNT Instruction: Not Portable Unless It Is, And Then Only Sometimes

Oh boy, Gentle Reader, fair warning: this is going to be a long one, and I don't have all the details worked out yet.  Such a mess for such a simple instruction....

Ok, so around the time SSE4.2 was introduced, the x86-64 ISA finally got the simple, very useful instruction we all had been clamoring for for years: [POPCNT](https://software.intel.com/sites/landingpage/IntrinsicsGuide/#expand=1786,1785,5432,5437,5473,4048,1152,2010,3835,4512,4515,4048,4047&othertechs=POPCNT).  _Finally_, we were able to count the number of bits set in an word without all the bit-twiddling and/or loops and/or [general complete nightmare](http://chessprogramming.wikispaces.com/Population+Count) that it is without the dozen-or-so gates it takes to implement this operation in hardware.  Need the number of set bits in that word?  POPCNT, BAM!, done!  21st century for the win!

...yeah, except not.

### The Tragedy of POPCNT

Because (as I understand it) AMD beat Intel to the punch in delivering the POPCNT instruction, [POPCNT is not considered part of SSE4.2 proper](https://en.wikipedia.org/wiki/SSE4#POPCNT_and_LZCNT).  You can't just compile with `-msse4.2` and use POPCNT, you have to compile with `-mpopcnt` to get POPCNT.

Except you don't.  Give `gcc` (6.1.1) the `-msse4.2` flag and it spits out POPCNTs just fine.  Why?  I can't figure it out; as far as I can tell from extensive study of The Internet(tm) it shouldn't (q.v. the Intel and Wikipedia links above, [this StackOverflow answer from some random guy on the internet](http://stackoverflow.com/a/11130642), etc.)  The closest I can come to an explanation is that it appears that there's no `-march=` supported by `gcc` which supports SSE4.2 but does _not_ support POPCNT (or as AMD would have you call it, ABM); [But you don't have to take my word for it](https://gcc.gnu.org/onlinedocs/gcc-6.1.0/gcc/x86-Options.html#x86-Options).

### It Gets Worse At Runtime

Ok, I can see I'm losing you (believe me, you'll thank me later for all the time and frustration this article saved you).  "Big whoop," you say, "give `gcc` a `-msse4.2` and be done with it."  But as the heading three up there says, things only get worse at runtime.  Bizarrely worse.

So portability: Imagine you're like me, toiling away trying to deliver a portable program where performance is important.  That means SSE2 is your baseline, and _if available at runtime_, you call certain critical functions which have been separately-compiled to use SSE4.2 and POPCNT.

Further, assume you're rockin' an Intel Core i7-960 monster as your dev machine (don't laugh), running Windows 7 (don't laugh), running Fedora 24 in a VirtualBox VM to do your Linux development (ok you can laugh now).  Two things:

1. The Intel Core i7-960 supports SSE4.2 and POPCNT.
2. The self-same Intel Core i7-960 as presented to Linux by VirtualBox supports SSE4.2.  But not POPCNT.

Don't believe me?  Here's what the CPUID instruction has to say from inside the VM (via `cpuid`):

```
$ cpuid
CPU 0:
   vendor_id = "GenuineIntel"
   version information (1/eax):
      [...]
      (simple synth)  = Intel Core i7-900 (Bloomfield D0) [...]
   miscellaneous (1/ebx):
      [...]
   brand id = 0x00 (0): unknown
   feature information (1/edx):
      [...]
   feature information (1/ecx):
      PNI/SSE3: Prescott New Instructions     = true
      [...]
      SSSE3 extensions                        = true
      [...]
      SSE4.1 extensions                       = true
      SSE4.2 extensions                       = true
      [...]
      POPCNT instruction                      = false
```

Is your mind blown yet?  My guess is no.  Well hold on to your hat my friend:

**POPCNT actually works fine in the VM, even though it's advertised as not supported.**

And I just blew your mind.

> Editor's Note: Similar but even worse things happen to these bits under [Valgrind](http://valgrind.org/), but that's an article for a different day.

### POPCNT Situation Summary

Ok, so we have:

1. `gcc` will generate POPCNTs when given `-msse4.2`
2. There is at least one platform in the wild which indicates it supports SSE4.2, but not POPCNT

    2a. That platform actually does support the POPCNT instruction, meaning that its claim of non-support is erroneous.

3. Ergo, we can't trust a CPUID indication of POPCNT non-support.
4. Except if we want to write portable code, we have no other choice but to trust it, and not use POPCNT if CPUID says it doesn't exist.
5. **But see #1.  You'll get them even if you don't ask for them.**

### POPCNT Portability Solutions

Like I said at the top, I don't have all this worked out yet, because like you, my mind is also still blown from all this conflicting weirdness.  My Provisional POPCNT Portability Pointer(tm) at the moment is the following:

**Take the conservative route: Don't use POPCNT if CPUID claims that it doesn't exist, even if it very likely does exist and works correctly.**

To do that, if you otherwise want to use SSE4.2 instructions, you'll need to do the following:

1. If you're compiling a separate module with `-msse4.2`, split it into two: One compiled with `-msse4.2 -mno-popcnt`, one compiled with `-msse4.2 -mpopcnt`.
2. Using your favorite flavor of CPU Dispatching/[Function Multiversioning](https://gcc.gnu.org/onlinedocs/gcc-6.1.0/gcc/Function-Multiversioning.html#Function-Multiversioning) (oh man, yet another future artice), choose between the two implementations at load- or run-time based on what CPUID says about POPCNT support.

Complete and should-be-unnecessary hassle, but that's what my research to date indicates should keep you portable.  One last time: **I don't have a complete understanding of this issue at this time.**  I've verified that `gcc` 6.1.1 with `-msse4.2 -mno-popcnt` does in fact not generate POPCNTs, even when provoked by using `__builtin_popcountll()`.  The advice above matches what both Intel's and AMD's manuals say you should do, as quoted in [the StackOverflow post noted above](http://stackoverflow.com/a/11130642).  But no guarantees on this one.  And I have no idea what happens when we get into AVX territory.

Word to the wise.  QED.
