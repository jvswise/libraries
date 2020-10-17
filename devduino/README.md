# DevDuino core library

I have updated this library to work properly on a linux platform.

All examples have been tested, but the Bluetooth and wifi functionality was not tested.

## What is the purpose of this library ?
This library is intended to ease the programming of your project using a [DevDuino](http://devduino.cc).

It serves the same goal as the [DevDuino](http://devduino.cc): Simplifying and accelerating the development of your prototypes.

It implements all the functionnalities you need to communicate with your [DevDuino](http://devduino.cc) core components (display, buzzer, RTC and temperature).

In addition to offer an API to simplify communication with each component, it adds higher level classes to simplify debugging of your program, like a console like API or a very simple "spreadsheet" implementation.

## What this library is not intended for ?
This library is not intended to be used on an final product (even if you can do so).

If you need APIs to be production ready and assuming that your final product is an Arduino compatible board, then you should only use the API of each of the module of your DevDuino on this same github repository.

## Is this library Arduino compatible ?
### Based on Arduino API
This library uses Arduino core libraries and is therefore compatible with it.

### Arduino IDE
This library also defines configuration files for arduino IDE, especially to propose syntax highlighting.

## How to use it ?
The best way is to go to http://devduino.cc to the download section and to unzip the library found on website with the other libraries of your Arduino IDE.

By doing so, the library and all examples will be pre-packaged together and will appear directly in your arduino IDE.

If you choose to download source code of this library directly on github, you will also have to download source code of each example in this github organisation. This is not the recommended way.

A complete tutorial on how to install and use this library can be found on [DevDuino](http://devduino.cc) web site.
