---
layout: page
title: USB MIDI Descriptor
permalink: /modules/midi2cv/midi-descriptor
nav_order: 1
parent: MIDI 2 CV
grand_parent: Modules
---

# TLDR: USB
As you might know, as soon as you connect an USB device, it will notify the host
of its presence. After that, the host will quickly interrogate the device regarding
its capabilities and function. Armed with this information, your OS can then find a
matching driver and everything will be merry; assuming we're either a class-compliant
device or we have the appropriate drivers.

As writing drivers is incredibly not fun, and Windows / Linux / every imaginable OS /
actually comes with USB MIDI drivers out of the box, it is good practice to actually
use those drivers. Also, USB can really look scary, but once I started to grasp it
it is by far not as complicated as I always thought.

# Enumeration, Endpoints, Device descriptors and so on.
Enumeration is the practice of interrogating your device. Effectively, USB devices have
32 (16 in and 16 out) endpoints (EPs), which all have some flavour. The magic interrogation
endpoint is EP0 (in and out), which is of the control flavour.

Your host nicely asks the device to basically spill its insides, and then it knows
which other endpoints exist.

Unless you're absolutely insane and trying to implement a USB device library yourself,
most of this stuff should be taking care of for you. However, you do have to supply all
the other EPs yourself, and also provide your device description.

Such a description in our case is split in a few files: [usbd_conf.c](https://github.com/warrantyvoids/ModSynth/blob/master/firmware/Src/usbd_desc.c)
contains the first 'generic' part, and [usbd_midi.c](https://github.com/warrantyvoids/ModSynth/blob/master/firmware/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_midi.c)
contains the MIDI-specific part.

## The Generic Part
```c 
/** USB standard device descriptor. */
__ALIGN_BEGIN uint8_t USBD_FS_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END =
{
  0x12,                       /*bLength */
  USB_DESC_TYPE_DEVICE,       /*bDescriptorType*/
#if (USBD_LPM_ENABLED == 1)
  0x01,                       /*bcdUSB */ /* changed to USB version 2.01
                                             in order to support LPM L1 suspend
                                             resume test of USBCV3.0*/
#else
  0x00,                       /*bcdUSB */
#endif /* (USBD_LPM_ENABLED == 1) */
  0x02,
  0x00,                       /*bDeviceClass*/
  0x00,                       /*bDeviceSubClass*/
  0x00,                       /*bDeviceProtocol*/
  USB_MAX_EP0_SIZE,           /*bMaxPacketSize*/
  LOBYTE(USBD_VID),           /*idVendor*/
  HIBYTE(USBD_VID),           /*idVendor*/
  LOBYTE(USBD_PID_FS),        /*idProduct*/
  HIBYTE(USBD_PID_FS),        /*idProduct*/
  0x00,                       /*bcdDevice rel. 2.00*/
  0x02,
  USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
  USBD_IDX_PRODUCT_STR,       /*Index of product string*/
  USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
  USBD_MAX_NUM_CONFIGURATION  /*bNumConfigurations*/
};
```
No, Wait! It's not that scary. Also, this is generated for us anyway, so no complaining here.

The structure of this descriptor is pretty much common everywhere: First we state how long we are,
then we state what we are (0x01 - e.g. a USB Device Descriptor). We have some preprocessor-statements
which we're going to ignore, that deals with low power modes, and then we have some things
describing the class, subclass and so on.

All these values are fine as is normally, except the `bDeviceClass`, which we
fix at 0x00, to flag that some other header will provide the real class.

## The ~~scary~~ MIDI part
So, the next part is a bit more unintuitive, but not so bad. Everything is based on
the [USB MIDI](https://www.usb.org/sites/default/files/midi10.pdf) and [USB AUDIO](https://www.usb.org/sites/default/files/audio10.pdf)
specifications. You can design these yourself, but it is quite a pain in the ass.

```c
//Standard Descriptor
0x09,        // bLength
0x02,        // bDescriptorType (Configuration)
0x65, 0x00,  // wTotalLength 101
0x02,        // bNumInterfaces 2
0x01,        // bConfigurationValue
0x00,        // iConfiguration (String Index)
0xC0,        // bmAttributes (Self Powered)
0x05,        // bMaxPower 10mA
```

So, once again,we tell what we are, and we tell the total length (of this header and all other following headers).
We describe the amount of interfaces, which is fixed at at least 2 for MIDI. `bConfiguration`
tells which configuration this is (well, 1); and `iConfiguration` - as all i-prefixed names,
tells us which text belongs to it (in this case, no text).

We then have to tell the other side that we're 'Self Powered' -- because we cannot
power this device from the USB Device port, remember? Therefore we can also set the
`bMaxPower` (in units of 2mA) quite low.

So, that was not so hard.

```c
//Audio interface
0x09,        // bLength
0x04,        // bDescriptorType (Interface)
0x00,        // bInterfaceNumber 0
0x00,        // bAlternateSetting
0x00,        // bNumEndpoints 0
0x01,        // bInterfaceClass (Audio)
0x01,        // bInterfaceSubClass (Audio control)
0x00,        // bInterfaceProtocol
0x00,        // iInterface (String Index)

//Class Specific Audio Interface
0x09,        // bLength
0x24,        // bDescriptorType (See Next Line)
0x01,        // bDescriptorSubtype (CS_INTERFACE -> HEADER)
0x00, 0x01,  // bcdADC 1.00
0x09, 0x00,  // wTotalLength 9
0x01,        // binCollection 0x01
0x01,        // baInterfaceNr 1
```

So, now we tell that we belong to the Audio interfaces. There is literally no choice here,
this is just required by the USB MIDI spec. And once again -- it literally follows
the same specification. Notice that the `bInterfaceSubClass` ('Audio control') is not
an error, unless we're talking about the specification itself.

Now, for the interesting part:
```c
//MIDI streaming interface
0x09,        // bLength
0x04,        // bDescriptorType (Interface)
0x01,        // bInterfaceNumber 1
0x00,        // bAlternateSetting
0x02,        // bNumEndpoints 2
0x01,        // bInterfaceClass (Audio)
0x03,        // bInterfaceSubClass (MIDI Streaming)
0x00,        // bInterfaceProtocol
USBD_IDX_INTERFACE_STR, // iInterface (String Index)

//MIDI Interface Header Descriptor
0x07,        // bLength
0x24,        // bDescriptorType (See Next Line)
0x01,        // bDescriptorSubtype (CS_INTERFACE -> MS_HEADER)
0x00, 0x01,  // bcdMSC 1.00
0x41, 0x00,  // wTotalLength 41
```

So, nothing weird here; The Subclass for a MIDI streaming device is 0x03; and the
`USBD_IDX_INTERFACE_STR` simply refers to a constant provided by the STM32 Libraries,
which I've set to 'Generic MIDI'. Does it work without? Yes; at least in my tests.

## The ~~bad part~~ MIDI Jacks
Now, the part which objectively took me the longest to figure out.

```c
//MIDI In Jack 1
0x06,        // bLength
0x24,        // bDescriptorType (See Next Line)
0x02,        // bDescriptorSubtype (CS_INTERFACE -> MIDI_IN_JACK)
0x01,        // bJackType (EMBEDDED)
MIDI_JACK_USB_OUT, // bJackID
0x00,        // iJack

//MIDI In Jack 2
0x06,        // bLength
0x24,        // bDescriptorType (See Next Line)
0x02,        // bDescriptorSubtype (CS_INTERFACE -> MIDI_IN_JACK)
0x02,        // bJackType (EXTERNAL)
MIDI_JACK_DEV_OUT, // bJackID
0x00,        // iJack

//MIDI Out Jack 1
0x09,        // bLength
0x24,        // bDescriptorType (See Next Line)
0x03,        // bDescriptorSubtype (CS_INTERFACE -> MIDI_OUT_JACK)
0x01,        // bJackType (EMBEDDED)
MIDI_JACK_USB_IN, // bJackID
0x01,        // bNrInputPins
MIDI_JACK_DEV_OUT, // baSourceID(1)
0x00,        // baSourcePin(1)
0x00,        // iJack

//MIDI Out Jack 2
0x09,        // bLength
0x24,        // bDescriptorType (See Next Line)
0x03,        // bDescriptorSubtype (CS_INTERFACE -> MIDI_OUT_JACK)
0x02,        // bJackType (EXTERNAL)
MIDI_JACK_DEV_IN, // bJackID
0x01,        // bNrInputPins
MIDI_JACK_USB_OUT, // baSourceID(1)
0x00,        // baSourcePin(1)
0x00,        // iJack
```

This is basically an over-definition of how our world looks. We tell by USB descriptor
that we do bi-directional USB. (e.g. we can send MIDI and receive MIDI). We also have
some real parts connected to the USB things; and for reasons which are not entirely
clear to me, they should always come in pairs of `EXTERNAL` to `EMBEDDED`. If somebody
could explain this to me, please do.

Anyway: The jacks come in four flavours: They're either `EMBEDDED` - e.g. USB MIDI,
or `EXTERNAL` - e.g. MIDI over a DIN cable. and then there are the `MIDI IN` and `MIDI OUT` options.

In this case, we say that we have a USB to external connection (MIDI IN Jack 1 to MIDI Out Jack 2)
and one external to USB connection (MIDI IN Jack 2 to MIDI Out jack 1). MIDI In headers only
tell they exist, and MIDI out headers actually link those together. Pins in this case
are simply further divisions of ports.

## Endpoint descriptors
```c
//OUT Endpoint descriptor
0x09,        // bLength
0x05,        // bDescriptorType (See Next Line)
MIDI_OUT_EP, // bEndpointAddress (OUT/H2D)
0x02,        // bmAttributes (Bulk)
0x40, 0x00,  // wMaxPacketSize 64
0x00,        // bInterval 0 (Ignored for bulk)
0x00,        // bRefresh
0x00,        // bSyncAddress

//Class-specific MS Bulk OUT Descriptor
0x05,        // bLength
0x25,        // bDescriptorType (See Next Line)
0x01,        // bDescriptorSubtype (CS_ENDPOINT -> MS_GENERAL)
0x01,        // bNumEmbMIDIJack (num of MIDI **IN** Jacks)
MIDI_JACK_USB_OUT, // BaAssocJackID(1) 1

//IN Endpoint descriptor
0x09,        // bLength
0x05,        // bDescriptorType (See Next Line)
MIDI_IN_EP,  // bEndpointAddress (OUT/H2D)
0x02,        // bmAttributes (Bulk)
0x40, 0x00,  // wMaxPacketSize 64
0x00,        // bInterval  0 (Ignored for bulk)
0x00,        // bRefresh
0x00,        // bSyncAddress

//Class-specific MS Bulk IN Descriptor
0x05,        // bLength
0x25,        // bDescriptorType (See Next Line)
0x01,        // bDescriptorSubtype (CS_ENDPOINT -> MS_GENERAL)
0x01,        // bNumEmbMIDIJack(1) (num of MIDI **OUT** Jacks)
MIDI_JACK_USB_IN, // BaAssocJackID(1) 1
```

And finally, we need to describe how many endpoints we have, and how to correlate
to our MIDI jacks. We follow the same structure as always, and mention EP1 (`MIDI_OUT_EP = 0x01`)
as an OUT endpoint (of the 'bulk' type). Effectively, all our parameters are specified
by the USB standard from here on. We might change `wMaxPacketSize` to 4, as that is what
the USB specification also tells us. We then have the part which says how many, and which
(embedded) jacks communicate over this EP. Of course, we can only have USB EMBEDDED IN jacks connected
to our OUT EP, and the USB EMBEDDED OUT JACK to our IN EP.

We then repeat this story for the OUT version.

## Debugging USB problems
So far, I have seen a few issues. If everything fucks up, get the USB Viewer tooling.
Until then, if the device at least shows up in the Device Manager, you can generally
tell what goes wrong. If it enumerates, you will see it under 'Sound, video and game controllers'.
If the USB driver itself disagrees with you, it will tell you that the device cannot start.
It also has the effect of putting the USB device into PowerStateD3, effectively suspending the
device. This is a symptom, not a cause.

In case you dun goofed, stash your work, flash to a known working state and make sure
there is not a USB-HUB, or something else playing up. I have literally lost hours
being terribly confused until I realised it was not my device not being able to enumerate,
but another device (which I probably confused though).

## Further reading
Do you still like USB? You maniac.

[USB MIDI spec](https://www.usb.org/sites/default/files/midi10.pdf)
[USB AUDIO spec](https://www.usb.org/sites/default/files/audio10.pdf)
[Sensible website](https://www.beyondlogic.org/usbnutshell/usb5.shtml)