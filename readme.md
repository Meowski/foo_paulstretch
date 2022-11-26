# foobar2000 Paulstrech DSP Component

Adds [paulstretch](http://hypermammut.sourceforge.net/paulstretch/) playback support as a DSP plugin.

## Installation

Download the fb2k-component file from the releases tab and open it. For a portable install the file can be dragged into the File > Preferences > Components section.

## Usage

1. Move the Paulstretch DSP from the "Available DSPs" to the "Active DSPs" list under File > Preferences > Playback > DSP Manager.
2. Alter settings either in DSP Manager via File > Preferences > Playback or in View > DSP > Paulstretch from the main window.
3. In the settings dialog, check the "enabled" mark to turn it on. If step one was ignored, this will perform that step for you.
4. You can also toggle paulstretch on and off under Playback > Paulstretch Toggle.

## Settings

The component only supports the basic 'stretch' and 'window size' settings.

## FB2K Related Settings

The conversion checkbox should be toggled when running in a conversion preset chain to ensure proper processing. Without it, the ending may be chopped off by a few seconds.  However, it should be disabled during realtime playback to avoid unecessary delays when scrubbing / changing settings / switching songs.

Settings are applied during the following actions:
* When the apply button is pushed.
* When the scrollbar is released or moved with keyboard input.
* When a new dropdown box value is selected.

Sliders will round values to the specified precision, while the apply button does not. This is intentionally done so you may enter more precise values manually. Fractional digits are capped to 8 decimal places, at which point rounding applies.

## Known Issues

* Playback ticker gets out of sync sometimes. Not sure how to fix this, sorry.

## Troubleshooting

The component was developed in Visual Studio Community 2022, so you may need to install the Microsoft Visual C++ 2022 redistributable 
separately.

## Notes

The source code depends on the foobar2000 sdk, the WTL library, and kissFFT. The algorithm is based on the python implementation 
right [here](https://github.com/paulnasca/paulstretch_python) on github provided by the creator of paulstretch.
