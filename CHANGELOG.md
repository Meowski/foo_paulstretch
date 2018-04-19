# Changelog

## [1.1] - 2018-04-19 
### Added
- Presets are now supported. This allows multiple conversions to run without interfering with each other and also saves settings between sessions.

### Changed
- Removed the static variable version of the algorithm. This forces playback to reset whenever any settings are changed. This behavior differs from the previous version's, which would attempt to blend the audio and avoid resetting playback. I don't believe this old behavior can be emulated when using presets.

### Fixed
- Removed the "wavy playback" bug caused by channels rapidly being swapped when certain stretch amounts and window sizes were used.

## [1.0] - 2017-02-20
Initial release.






