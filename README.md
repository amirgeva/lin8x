# Lin8x

Pronounced: Lin Aches

A retro vibe computing environment.

Although the look and feel is usually what people mean when this goal is mentioned, in this case, the intent is much deeper.  Anyone who has owned an early micro-computer knows the experience of booting your machine into a practically empty environment, but with potential to run / write code that will do amazing things.  There was also a real sense of control of what is running on the machine.  In contrast to this, if you log in into the console of a very minimal Linux distro and list the running processes, you will get a list of dozens if not hundreds of processes doing stuff you don't really know.

The guiding design rules therefore, are thus:
1. Anything running on user space was explicitly started or configured by the user.
2. All programs are statically linked (no shared libraries / objects)
3. The system comes with the following minimal set of tools:
   1.  Simple shell (Custom)
   2.  Simple editor (Custom)
   3.  C compiler (Example, binary of  https://github.com/larmel/lacc)
   4.  Linker (Example, binary of https://github.com/rui314/mold)
   5.  C Library (Example, headers and library files of https://www.musl-libc.org/)
   6.  A small set of development libraries (Custom)

Further design decisions are pending, but do not expect them to follow Unix traditions just because they are traditions.

***

More details in the [Wiki](https:///github.com/amirgeva/lin8x/wiki)

