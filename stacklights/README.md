# Team Stack Lights

The Team Stack lights are a 2-Layer stack light that uses the specifiction defined in the [FMS Whitepaper](https://fms-manual.readthedocs.io/en/latest/fms-whitepaper/fms-whitepaper.html#field-status-indicators).

There is an Amber light at the bottom of the stack that illuminates when the EStop or AStop is enabled.  The Team (Red or Blue) light at the top illuminates the robot communication state.

|          | Alliance Color                                | Amber Color                                          |
| :------- | :-------------------------------------------- | :--------------------------------------------------- |
| Flashing | No connection to robot or bypassed            | Astop pressed/enabled (during or before auto period) |
| Solid    | Robot enabled (before the match starts)       | Estop pressed/enabled                                |
| Off      | Connection estabished to robot (during match) | Estop not pressed/disabled                           |

Normal state during play is Off, Off
Normal state right before the match is started is Blue/Red Solid, Amber Off

## Team Stack Light Build and Assembly

Each stack light is 3D Printed using the included STL files.  The lenses are printed using transparent PETG and the other pieces can be printed in any material or color.  This was modified from NilsR's great make [here](https://www.printables.com/model/700641-modular-ws2812-neopixel-stack-light-andon). I've included the ZIP of his work here as well.

Modifications and additions to print design:
- Bottom cap piece is slotted to allow for LED clearance and includes holes for the bottom base.
- Bottom base created to house the 3.5mm headphone sockets and wires.  The base also has a wide flange to allow for attachment options.
  - When printing the base, print the sides of the box solid and then drill holes for the headphone sockets.
- A clip is included to allow hanging from the FRC arena top rail.

Materials used:
- [12 Volt WS2811 RGG+IC (3-wire) LED Strip](https://www.printables.com/model/700641-modular-ws2812-neopixel-stack-light-andon)
- [3.5mm 4-pole Headphone Sockets](https://www.printables.com/model/700641-modular-ws2812-neopixel-stack-light-andon)
- 12v 3A Power Supply

## Wiring Diagram

The stack lights are wired together in a single array.  The LED strip used here, has 3 LED's per address, so they are illustrated as one lights.

![Wiring Diagram](team-stacklights.drawio.svg "Wiring Diagram")