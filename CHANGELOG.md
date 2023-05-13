# Changelog

## [2.0.1]
### Added
- Dark Mode support.
- UI Element support. 
    - Allows resizable & embeddable settings window. 
    - Font changes are respected in UI element windows (when embedded in foobar2000 or opened from View > DSP > Paulstretch). Other windows use the default system font.
    - The 'enabled' checkmark is updated across UI elements. This includes using the `Paulstretch Toggle` menu command. Other settings do not sync; this is intentional.
- Added min and max value dropdowns for the stretch/window size values.
- Precision dropdown menus allow you to specify decimal place values when using the slider bar.
- Shorten mode support by selecting the 0.5 minimum stretch option. The scaling factor is equal to `1 / stretch`. For example, if you wanted to speed up a song by 10%, you would want a stretch value of 0.909090...
- Support new DSP api call that allows settings to be changed without destroying the dsp chain.  Practically, this means changing the stretch amount should be smooth without any crossfades necessary.

### Changed
- Moved the settings dialog menu command from `Playback > Paulstretch Settings` to `View > DSP > Paulstretch`.

### Fixed
- There was a bug in the FFT window size optimization which degraded performance by nearly an order of magnitude (`O(n^2)` vs `O(nlog(n)`) in certain cases. Particularly if you used a window size of 0.21 seconds.

## [1.1] - 2018-04-19 
### Added
- Presets are now supported. This allows multiple conversions to run without interfering with each other and also saves settings between sessions.

### Changed
- Removed the static variable version of the algorithm. This forces playback to reset whenever any settings are changed. This behavior differs from the previous version's, which would attempt to blend the audio and avoid resetting playback. I don't believe this old behavior can be emulated when using presets.

### Fixed
- Removed the "wavy playback" bug caused by channels rapidly being swapped when certain stretch amounts and window sizes were used.

## [1.0] - 2017-02-20
Initial release.






