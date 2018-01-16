# Proposal

## Project Overview

    I want to make a function SNES emulator for Windows, and if time permits, a
simple GUI and some debugging functionality.  This project is very interesting
to me because I've always been fascinated with emulation, and it will be very
exciting to see a program I make run a game that someone else has written.

    The emulator would draw on many concepts covered in OS, including virtual
memory, address translation, and potentially some synchronization issues.  I'll
effectively be recreating all the hardware for the SNES in software, so it will
require a deep understanding of low level system architecture and behavior.
By actually creating a working implementation of these OS concepts, it will
greatly help solidify my understanding.



## Hopeful schedule

    I'm scheduling for 18 weeks even though we have more time than that, in case
some things take longer than expected.  If I finish early there are extra tasks
I can work on.

Complete By         Task                    Deliverable
-------------------+-----------------------+------------------------------------
Week 3, Winter     |APU Processor,         |Be able to run ‘SPC’ files (APU
                   |Tracelogger, and       |subunit snapshots) and view a
                   |virtual memory         |tracelog of the code executed.
-------------------+-----------------------+------------------------------------
Week 4, Winter     |APU Audio generation   |Run SPC files and hear the actual
                   |                       |music being played.
-------------------+-----------------------+------------------------------------
Week 7, Winter     |Main CPU, memory,      |Load games and run them, and be
                   |and traceloggeer       |able to view a tracelog of the
                   |                       |executed CPU instructions.
-------------------+-----------------------+------------------------------------
Week 9, Winter     |GPU Simple backgrounds |See a title screen for a game
                   |& Scrolling            |
-------------------+-----------------------+------------------------------------
Week 10, Winter    |GPU sprites            |See an intro demo for a game.
-------------------+-----------------------+------------------------------------
Week 1, Spring     |Joypad controls and    |Actually be able to play a game
                   |tying CPU+APU together |and see/hear the game’s output.
-------------------+-----------------------+------------------------------------
Week 2, Spring     |GPU color math         |See the "translucent" video effects
                   |                       |in some games.
-------------------+-----------------------+------------------------------------
Week 3, Spring     |Misc CPU raster effects|See these effects in games that
                   |(Windows/Mosaic)       | use them.
-------------------+-----------------------+------------------------------------
Week 5, Spring     |More complicated GPU   |See these modes (Mode7, offset-
                   |background modes.      |by-tile) in games that use them.
-------------------+-----------------------+------------------------------------
Week 6, Spring     |Other GPU render modes |See these effects in games that
                   |(Hi-res, interleaved,  | use them.
                   |overscan)              |        
-------------------+-----------------------+------------------------------------
Week 8, Spring     |Improve game           |Take a bunch of games that were 
                   |compatibility          |broken in the emulator and fix them!
-------------------+-----------------------+------------------------------------
(If I have time)   |Simple GUI             |Be able to demo the emu with a GUI
-------------------+-----------------------+------------------------------------
(If I have time)   |Controller Support     |Be able to play games with a PC
                   |                       |controller
-------------------+-----------------------+------------------------------------
(probably won’t get|Interactive Debugger   |Pause CPU execution and step
to this)           |                       |through code.


