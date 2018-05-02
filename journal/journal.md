### [Proposal](proposal.md) - [Timeline](timeline.md) - [Journal](journal.md) - [Bibliography](bibliography.md) - [Samples](samples.md)

# Journal

#### Spring

Week | What is completed
--- | ---
Week 1 | Added SRAM read/writing, Added readable Mode7 registers.  Started debugging why Castlevania 4 isn't working (looks like a CPU bug)
Week 2 | Continued debugging efforts.  Fixed a CPU bug that was causing indirect JMPs to use the wrong bank for address lookup.  But CV4 still isn't working.  Grrrrr.
Week 3 | Wrote some utilities to convert my CPU trace logs to a format that matches BSNES tracelogs so I can compare my emu output to the output of an emu that is working.  Though minor difference in timing (when IRQs trip, etc) still make a direct comparison difficult to find exactly where the bug is.  At this point I'm thinking of just redoing the CPU from scratch and then diffing my two CPUs to see where the problem was.
Week 4 | Rewrote entire CPU from scratch!  Just about.  I didn't rewrite the switch statement, or interrupts, but all the core addressing modes and instruction logic rewritten.  This fixed a lot of bugs!
Week 5 | Fixed Mode7 a bit.  Most Mode7 stuff now works, but there are some problems.  Also added HDMA support, and it appears to be working well.
Week 6 | ...



#### Winter

Week | What is completed
--- | ---
Week 2 | APU finished.  Can create a tracelog of the executed SPC code.  Can play actual music!  Wooo!
Week 3 | Main CPU finished, but is untested.  Can't create any tracelogs yet (tracer isn't written), and can't actually run code (no loader)
Week 4 | CPU tracer and loader are complete.  Fixed some CPU bugs in the process.  Super Mario World can load and run code, but it seems to get stuck in a loop shortly into the game, so there must be another CPU bug I'm missing.
Week 5 | CPU bugs fixed. SMW now loads and runs properly and is audible! Also started work on basic PPG/Graphics stuff and the DMA unit.
Week 6 | Some timing stuff touched up.  NMIs/IRQs now should work via an "event" system I have set up.  WAI instruction should also be working.  Started to build the PPU frame but haven't actually rendered anything yet.
Week 7 | Did some planning for how I wan to organize the frame, but didn't really get much coding done.  AAArrrggggg.
Week 8 | Tons of work done to make up for last week!  Timing for the frame all straightened out.  PPU is mostly working!  4 bpp and 2 bpp backgrounds are displayed properly.  Sprites are also displayed.  Color math implemented.  You can now watch the demo of Super Mario World!  Although for some reason Yoshi is invisible, still need to debug that.
Week 9 | Fixed the invisible Yoshi bug.  Started on Mode7 graphics (Pseudo-3D effects).  Have is sort of displaying, but it's very glitchy.  Also implemented PPU generated IRQs, a few readable registers, joypad input (so you can actually play the games!), and a basic frame for HiROM support, but no way to detect whether a game is HiROM or LoROM yet.
Week 10 | No time to do anything this week with finals and rushing to finish homework and labs.  But I'm a little ahead of where I thought I'd be by this point according to my timeline, so that's good.
