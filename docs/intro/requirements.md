---
layout: page
title: Tools
permalink: /about/tools/
nav_order: 3
---

# What tools/components do you need?

## Software
For Software-development:
- Windows 10 with WSL (Debian).
- Clion of C-development. You can in principle use anything.
- Visual Studio for some of the weird debugging (not needed here).
- STM32CubeMX.

For electronics:
- KiCAD
- ngspice

For Audio / MIDI-control:
- Studio One 4
<!--- Ableton Live 10 -->

## Equipment
Electronics:
- Leader 3060D scope (60Mhz analog / 40MIPS digital)
- Korad KA3305P power supply (but any two-channel bench supply will do; even a computer PSU albeit somewhat noisy)
- Some cheap multimeters
- Some cables (test leads, audio cables etc)

Music:
- Behringer U-Phoria UMC2 Audio interface (please don't use your computer)
- AKAI MPK249 keyboard (any MIDI-device will do though)

## Components
Passives:
- A set of E12 metal film resistors (1 ohm - 10 ohm)
- A set of E6 ceramic capacitors (10pF - 10uF)
- A set of electrolytic capacitors
- A set of inductors
- A set of WS2182-style potentiometers for knobs
- A set of 3296w-style trimmers
- A set of LDRs
- A set of NTC resistors.

Actives:
- Some BJTs (BC547 / BC557 and friends)
- Some MOSFETs (BS170) and JFETs (2N5457)
- Some Diodes (1N4001, 1N4148)
- Some Zener Diodes (a set, 2v2 to 30v)
- Some LEDS

ICs:
- 7805 and friends
- 74HCT and 74LS series, specifically
   - Basic gates (And/Not/Or/Nand/Nor)
   - Schmitt inverters
   - JK-flipflops
   - RS-flipflops

OpAmps / Comperators:
- LM386, TL06x, TL07x and TL08x series as general purpose opamps
- LM311 as comperator
- OP27G and UA741

Micro / dev boards:
- Nucleo144: STM32F767

# Why these tools?
I am quite familiar with the STM32 series ARM processor, and with their extremely
affordable development boards it seems like the best option by far. I passionately hate
the Arduino (or actually, the AVR), so I try to stay away from it. Furthermore,
it's development tools are quite neat and integrate with the IDE I use anyway.

My electronics equipment I generally have anyway, and the components
(specifically the opamps) are mostly ordered from AliExpress, which means that roughly
all the components needed to build the modules should total around €250. Given how also
not much equipment is needed, this should be quite acceptable as a beginner project.