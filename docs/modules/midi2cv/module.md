---
layout: page
title: MIDI 2 CV
permalink: /modules/midi2cv/
nav_order: 1
parent: Modules
has_children: yes
has_toc: false
---
# A MIDI-2-CV module

One of the first modules we're going to need is a MIDI 2 CV module.
We could technically do without, but if we actually want to sample anything,
play anything or what not, such a module is required.

Furthermore, I have not found a lot of proper USB MIDI resources and the whole
standard is quite intimidating if you're not used to it.

## The Plan
So, as a microcontroller I will use a [STM32F767 Nucleo144 board](https://www.st.com/en/evaluation-tools/nucleo-f767zi.html).
These are quite cheap and offer a ton of options.

I will use the User USB port as a Full Speed Device, the ADC as the CV and some GPIOs
as gate and triggers. Microcontrollers generally don't like high voltages and this little
racing machine is not any different in that respect: Some pins are 5V tolerant, but most
pins can go between 0V and 3.3V. (Negative voltages are a big no-no!)

In order to get up to our desired CV range, I will simply use two op-amps to amplify and
offset the voltage. The gate is created by using an op-amp in [schmitt-trigger](https://en.wikipedia.org/wiki/Schmitt_trigger) topology.
The trigger will be generated using an RC-circuit with a rectifier, so that we only have to
toggle the GPIO pin.

