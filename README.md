# DRIPS
### Decentralized Relative Inertial Positioning System
_Daniele Di Sarli, Diego Giorgini_

This proof of concept implements a detection, communication and coordination system between small-scaled cars in a crossroad without any centralized infrastructure.

Each car is able to:

 * detect other vehicles around it
 * communicate with them in order to compute the precedence according to road rules, or to signal an emergency situation

The system has been realized by relying only on these four basic components:

 * Arduino UNO
 * IR emitters
 * IR receivers
 * Radio card

[Watch the slides explaining how the system works](docs/slides-2017-06-04.pdf)

The following is a video of my colleague showing the results of the project:

[![Watch the video](resources/video-preview.jpg)](https://youtu.be/jqtYrXdcc9c)

You can find the protocol specification in [docs/RFC-CCS.md](docs/RFC-CCS.md), or you can
read the source code at [hwcontroller/](hwcontroller/).

This project has been realized in collaboration with [Diego Giorgini](https://github.com/diegobit).