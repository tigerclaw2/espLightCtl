LEDSCRIPT documentation v0.2 [Work in progress]

The behavior presented below may not be fully implemented or may change at any time!

N<int> (number of channels) is a one time definition in the header/meta section of a
script that defines the number of channels the script (expects) to control
all functions in the script which rely on reading multiple parameters (such as S)
will adapt to follow the definition

F<int> (fade interval) sets the fade interval duration used by the controller from this
point onwards. The value can be changed multiple times during the execution

S<n int><int> (explicit set) sets the light to an explicit value in one instruction.
This is the equivalent of calling C for each channel
This function expects n+1 ints as parameters, the first n ints in range 0-255 and the
last int in range 0-1024. The first n ints represent the exact value each channel will
be set to, the last one being ch0 (general brightness)
Examples:
```
N3;
S1024,0,0;    		// Sets channels 1,2,3 to 0 and brightness to max
S256,128,128,0; 	// Sets channels 1 and 2 to 50% and brightness to about 25%
S1024,255,255,255;	// Turns on all channels, full brightness
```

C<int>,<int> (channel set) sets the specified channel to a specified value
The first value represents the channel and must be <= N and the second value represents
the value to set.

P<int> (program) creates a subroutine further called by the provided value.
The line number +1 at which the definition was found gets stored in a key-value
like structure. Note that, by default a program runs on loop, indefinitely.

Q<int> (end program) defines the end of the specified program.
This function simply resets the cursor at the line stored by the P definition of the
specified int

L<int> (loop define) creates a loop repeating <int> times
On the backend, this function simply stores the line number +1 at which it was found
and the number of iterations it has to "reset" to that line number

O (end loop) defines the end of a loop sending the read cursor to the last line L was
found incrementing the "reset" counter

W<int> (wait) defines a period of time in which the script execution is suspended.
The value is multiplied by 10 so the minimum wait time is about 10ms.
The script expects to be resumed at approximately <int>*10ms.
This is not an exact timer!
While the backend runs at full speed in this timeframe, delays will most likely happen
as the resume condition is simply: if(current_millis >= stop_millis + wait*10)

R<int>,<int>,<int> (random set) sets the channel specified in the first value to a random
number in the interval specified by the following 2 values.

Example script with comments:
```
N4;				// 4 channels or RGBW [Ignored but still required]
F2000;			// fade set 2000ms
S0,0,0,0,100;	// explicit set ch1 value 0 ch2 value 0 ch3 value 0 ch4 value 0 ch0 value 100

C1,16;			// ch1 set value 16

P1;				// program "1" (acts like a void in C++)
L10;			// loop 10 times

W100;			// wait 1000ms (value * 10)
R1,10,255;		// random set ch1 value between 10 and 255
W10;			// wait 100ms
R2,30,168;		// random set ch2 value between 30 and 168

O;				// end loop
C1,128;			// ch1 set value 128
W100;			// wait 1000ms;
Q1;				// end program "1"
```