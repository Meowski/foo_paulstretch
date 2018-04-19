# foobar2000 Paulstrech Component

Adds [paulstretch](http://hypermammut.sourceforge.net/paulstretch/) playback support as a DSP plugin.

## Installation

Copy the foo_paulstretch.dll file from the binary folder in this repository into the components of your foobar2000 installation.  

## Usage

1. Move the Paulstretch DSP from the "Available DSPs" to the "Active DSPs" list under File > Preferences > Playback > DSP Manager.
2. Alter settings as desired either  in DSP Manager using "configure selected" or under Playback > Paulstretch Settings in the main window.
3. In the settings dialog, check the "enabled" mark to turn it on.
4. You can also toggle paulstretch on and off under Playback > Paulstretch Toggle.


The reason for having the toggle and settings menus under Playback is to allow for
keyboard shortcuts to be set. If there is a better way to do this I'd love to know. 

## Notes

The source code depends on the foobar2000 sdk, the WTL library, and kissFFT. The algorithm is based on the python implementation 
right [here](https://github.com/paulnasca/paulstretch_python) on github provided by the creator of paulstretch.

## Troubleshooting

The component was developed in Visual Studio Community 2017, so you may need to install the Microsoft Visual C++ 2017 redistributable 
separately.
