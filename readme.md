# foobar2000 Paulstrech Component

Adds [paulstretch](http://hypermammut.sourceforge.net/paulstretch/) playback support as a DSP plugin.

## Installation

Copy the foo_paulstretch.dll file from the binary folder in this repository into the components of your foobar2000 installation.  

## Usage

First, enable the Paulstretch DSP under Preferences > DSP Manager in foobar2000. Then, under the Playback menu is a paulstretch item. 
Settings for the stretch amount and window sampling size are there. I recommend creating a keyboard shortcut if you like tinkering 
with the stretch rate as much as I do.

## Notes

The source code depends on the foobar2000 sdk, the WTL library, and kissFFT. The algorithm is based on the python implementation 
provided by the creator of paulstretch on [github](https://github.com/paulnasca/paulstretch_python).

## Troubleshooting

The component was developed in visual studio community 2015 update 3, so you may need to install the Visual C++ 2015 redistributable 
separately. Also, as I only have stereo playback, only two channel playback was tested explicitly, though up to 64 channels should 
theoretically be supported.