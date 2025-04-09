# Corne Keyboard with Integrated Trackpoint Build Guide

This guide documents how I integrated a Sprintek SK8707-01 trackpoint module into my Typeractive Corne keyboard. The Corne is a popular 36-key split ergonomic keyboard, and adding a trackpoint allows for mouse control without moving your hands away from the home row.

## Final Result

![Full Keyboard](images/picture%20of%20full%20keyboard.jpg)

## Parts List

- Typeractive Corne keyboard (36 keys split)
- Sprintek SK8707-01 trackpoint module (specifically the SK8707-01-002 integrated version)
- M2 screw kit with sockets and extenders (10mm extender used for the trackpoint stem)
- Super glue
- Sandpaper (for prepping surfaces)
- Wire for connections
- Drill for making holes in the bottom plate

![M2 Screw Kit](images/m2%20screw%20kit.jpg)

## Build Process

### Step 1: Preparing the Keyboard

I drilled holes in the bottom aluminum plate of the keyboard to mount the trackpoint module. These holes are positioned under one of the trackpoint tenting legs, which helps hide them while providing a sturdy mounting point.

![Back of Keyboard](images/back%20of%20keyboard.jpg)
![Close-up of Through Hole](images/close%20up%20picture%20of%20through%20hole%20in%20keyboard%20between%20yuhj%20keys%20with%20sanded%20down%20keys.jpg)

### Step 2: Preparing the Trackpoint Module

The Sprintek SK8707-01 trackpoint module comes with its own controller board. The trackpoint sensor connects to this board via castellated pins which need to be soldered.

**Important Note:** Make sure to order the integrated version (SK8707-01-002). The SK8707-01-001 version is 5V, which may not be compatible with your keyboard's power supply. You can always desolder components if needed.

![Trackpoint Module Disassembled](images/close%20up%20of%20trackpoint%20module%20disassembled%20with%20nub%20and%20m2%20stick.jpg)
![Close-up of Trackpoint Module](images/close%20up%20of%20trackpoint%20module.jpg)

### Step 3: Customizing the Trackpoint Nub

The trackpoint module comes with a plastic nub attached. I removed this with a flathead screwdriver and replaced it with an M2 screw:

1. Carefully pry off the original plastic nub
2. Sand the top of the M2 screw to create a rougher surface for better adhesion
3. Apply superglue to attach the M2 screw to the trackpoint module
4. I used a 10mm M2 extender as the stem, which allows me to adjust the height to the perfect level

![Trackpoint with Original Nub](images/close%20up%20of%20trackpoint%20module%20with%20the%20nub%20attached.jpg)
![Trackpoint with M2 Stick](images/trackpoint%20with%20m2%20stick.jpg)

### Step 4: Installing the Trackpoint Module

The trackpoint module needs to be mounted securely to the keyboard and wired to the controller.

1. Mount the module to the back plate using the through holes prepared earlier
2. Connect the necessary wires from the trackpoint controller to your keyboard's microcontroller

![Back Plate with Trackpoint Mounted](images/closer%20picture%20of%20backplate%20of%20my%20keyboard%20witht%20he%20trackpoint%20module%20mounted.jpg)
![Soldered Wires](images/soldered%20on%20wires%20on%20to%20keyboard.jpg)

### Step 5: Using Nice!View Sockets for Easy Connections

One of the tricks I used for connecting the trackpoint module was utilizing the Nice!View display sockets as convenient solder points. This approach has several advantages:

1. The sockets are already properly spaced and accessible
2. They provide a clean, reliable connection point
3. It reduces the need for additional wiring across the PCB

I had to modify the pin configuration in my `corne_tp_right.overlay` file to match where I connected the trackpoint module:

```
// SCL (Clock) pin configuration
#define MOUSE_PS2_PIN_SCL_PRO_MICRO <&pro_micro 2 GPIO_ACTIVE_HIGH> // P0.10

// SDA (Data) pin configuration
#define MOUSE_PS2_PIN_SDA_PRO_MICRO <&pro_micro 1 GPIO_ACTIVE_HIGH> // P0.09

// Reset pin configuration (if using a reset circuit)
#define MOUSE_PS2_PIN_RST_PRO_MICRO <&pro_micro 3 GPIO_ACTIVE_HIGH> // P0.20
```

When choosing pins, I tried to use high-frequency pins where possible to avoid Bluetooth interference. For the Nice!nano, these are typically the D pins marked in green on the pinout diagram.

The PS/2 protocol requires precise timing, so ensuring good connections at these points is critical for reliable operation.

### Step 6: Reassembling the Keyboard

Once everything is mounted and connected, reassemble the keyboard carefully.

![Taken Apart Keyboard](images/taken%20apart%20keyboard.jpg)

## Firmware Configuration

I'm using ZMK firmware with the PS2 mouse driver. In my configuration, I've enabled the press-to-select feature, which works well with this trackpoint. This allows you to tap the trackpoint itself to perform a click.

To enable this feature in your ZMK config, add these lines to your mouse_ps2 configuration:

```
&mouse_ps2 {
    tp-press-to-select;
    tp-press-to-select-threshold = <1>;
}
```

## Tips and Observations

- The black trackpoint cap that came with my Sprintek module has been very comfortable to use
- The press-to-select feature is useful but takes some adjustment. I'm still fine-tuning the sensitivity:
  - If set too low, it triggers accidentally while moving the cursor
  - If set too high, it requires too much force to click
  - You can adjust this in your config with the `tp-press-to-select-threshold` value
  - Values of 1-8 are typical, with lower numbers being more sensitive
  - You can also adjust it at runtime using the `MS_TP_PTS_THRESHOLD_INCR` and `MS_TP_PTS_THRESHOLD_DECR` keycodes
- You might prefer traditional mouse buttons initially while getting used to the trackpoint
- Using the M2 extender allows for easy height adjustment, which is important for comfortable use
- Mounting the trackpoint under one of the tenting legs helps conceal the modification while maintaining functionality
- If you find yourself accidentally clicking or not clicking when you want to, consider adjusting these settings:
  - `tp-sensitivity` - controls how fast the cursor moves (default: 128)
  - `tp-pts-threshold` - controls how hard you need to press to click (default: 8)
  - `tp-neg-inertia` - affects how responsive the trackpoint feels (default: 6)

## Conclusion

Integrating a trackpoint into the Corne keyboard has significantly improved my workflow by eliminating the need to move my hands between the keyboard and mouse. While this modification requires some precision work and soldering, the result is well worth the effort.

Feel free to reach out if you have any questions about this build!
