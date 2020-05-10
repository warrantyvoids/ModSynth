## Design Goals

So, I plan to design a modular synthesizer. Let's try to be sane, keep all the voltages
safe and be slightly compatible with Eurorack, even though there doesn't seem to be much
of a standard.

###Electrical specs
- -5V to 5V
- 1V/oct
- Fed by -12V/ground/+12V
- Should be operable by drunks:
    - Indefinite short-times for outputs
    - All inputs are -5v to 5v tolerant
- All connectors are mono 3.5mm jacks
- DIN MIDI-jacks
- USB B-plugs for USB
- 6.35 TS un-balanced or XLR balanced outputs for audio.

###Philosophy
There is first of all no reason to being a purist. Nothing wrong with solving 
things in a 'modern' fashion, with a bunch of microcontrollers and DSPs and so.

However, one of the major points is to learn things about analog electronics, and
the old-skool way of doing it. So, a few rules which I will try to stick to:

1. Use discrete components where possible.
2. Avoid digital solutions or non-jellybean components where possible.
3. Don't build things without thoroughly explaining what is going on.
4. Don't copy designs, only be inspired by old designs.