# doom-nano
A 3d raycast engine for Arduino

![](/screen-1.jpg?raw=true)

See it in action --->
https://youtu.be/FX7PQtUAkHk

Just experimenting with an Arduino Nano I bought some time ago. This chip is very limited in process and memory.
Probably can be optimized and structured much better. Still learning.
Will try to keep learning and improve it.

To be clear. This is not an actual Doom game, just picked some sprites from it (and simplified a lot), But the rendering engine is more like a Wolfeistein 3D. The Doom idea came because I started building the fancy melt-screen effect, included here.

Hardware I used:
- Protoboard
- Arduino nano V3 (ATmega328P)
- OLED Display (i2c 128x64)
- 4 buttons
- 4 10k ohms resistors

Resources:
- Sprites from https://www.spriters-resource.com
- Much thanks to https://lodev.org/cgtutor for so wonderful resource about raycasting engines

To do:
- ~~Make z-buffer work~~
- ~~Make work picking up medikits and keys~~
- Some UI
- Build some basic IA for enemies
- Make possible kill enemies
- Doors and locked doors
- A game over screen
- Textures? Very performance expensive. I don't think so.
- Sound/Music? 

More screens:
![](/screen-2.jpg?raw=true)

![](/screen-3.jpg?raw=true)
