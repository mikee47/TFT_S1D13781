TFT_S1D13781
============

Introduction
------------

This is a port of the driver from the
`Epson S1D13781 display controller <https://www.vdc.epson.com/display-controllers/lcd-controllers/s1d13781/shield-tft-board-for-arduino/s1d13781-shield-graphics-library
>`__ using the :library:`HardwareSPI` library.

Evaluation boards are inexpensive and is a useful way to evaluate display modules with TFT interfaces.

The `Newhaven NHD-5.0-800480TF-ATXL#-CTP <https://www.newhavendisplay.com/nhd50800480tfatxlctp-p-6062.html>`__
was used during development.

Notes
-----

This library was developed as a stepping stone to the
`Bridgetek FT81x series EVE <https://brtchip.com/ft81x/>`__ Embedded Video Engines.

These are more advanced and work by buffering graphics command primitives which are then executed in real
time to construct each displayed frame.

These devices are used in the `Gameduino <https://excamera.com/sphinx/gameduino/>`__.
The author has an `open source library <https://github.com/jamesbowman/gd2-lib>`__.

This library is not fully asynchronous, but has couple of key improvements over the original source:

Cached register values
   These do not need to be read for every operation. Considerable increase in performance.
   
Asynchronous writes
   The HardwareSPI library can execute requests asynchronously, and this is used to provide
   some improvement in performance where no response is required from the display controller.

The next stage of development would be to build a generic graphics library to support multiple
display controllers. Like the EVE controllers, it would incorporate a graphics instruction pipeline
so that the application can buffer drawing requests in a fully asynchronous manner, eliminating
any bottlenecks or wait states in the application.
