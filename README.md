Judge.c - A simple text-mode lane judge for Pinewood Derby

This software is licensed under the GPL. If you find it useful, I encourage
you to add features and fix bugs and send the patches back to me.

My son is no longer a cub scout, so I doubt I will run another derby any
time soon. I'm releasing this into the wild to encourage others to make
what they will of it.

It's so simple, there's no makefile: 

$ gcc -o judge judge.c

You'll need to have the 'toilet' package installed to do big printing on
the screen. If you don't have access to this, choose another big printer
like figlet and change the source, or just use printf and squint.

To use, plug your Derby Stick into a USB port. Wait a second and the look
at dmesg to see what device it to assigned to. Normally it will be
something like /dev/ttyS0 or maybe /dev/ttyUSB0. Then pass this value to
the judge program on the command line:

$ ./judge -p /dev/ttyS0

(Obviously, use YOUR devices dev entry, not mine.)

After this, follow the directions on the screen. The judge will tell you
which lane won after each race and list the times.

For our races, we used a big sheet of paper with a double-elimination
bracket printed on it. Then we would use a Sharpie to write down names and
times. We awarded prizes for first, second, third, and best time, which
wasn't necessarily one of the winners, as cars tend to slow down each
time they are run.

It would be nice to see this as a fancy GUI application with lots of bells
and whistles, but my expertise is device drivers, so I would have to learn
a whole new API for that. Feel free to do that. I'm happy to accept patches
and enhancements here.

Mitch Williams
December 2015
