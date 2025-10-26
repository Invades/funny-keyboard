# funny keyboard
---
A simple reimagination of the [Tamam Keyboard](https://play.google.com/store/apps/details?id=com.ziipin.softkeyboard.saudi) app for android, now on windows.
Requires [Visual C++ redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-supported-redistributable-version) to run.
### Features
---
- [x] GLFW overlay powered by raylib
- [x] Supports all common image formats supported by [raylib](https://github.com/raysan5/raylib), plus [webp](https://github.com/webmproject/libwebp)
- [x] Sounds on keypress (default: the ones from that one stupid meme format)
- [x] Simple JSON config system
- [ ] DPI Scaling (soon™️, currently "optimized" for 100% scale)
- [ ] [Request a feature](https://github.com/invades/funny-keyboard/issues)
### Known "issues"
---
- No multi language support, it will draw the Latin characters corresponding to the pressed key regardless of what language you're using.
- Pixelated font, I chose to not use any anti-aliasing as I thought it was better suited for this type of program.
### Config
Should be straight forward, a default config will be generated on the first run if no config file is present. These are some of the assets included in every release.
#### Sounds
- `main.wav`
	Plays for all keys except backspace and enter
- `backspace.wav`
	Plays for backspace
- `enter.wav`
	Plays for enter
#### Images
The program defaults to a simple circle behind the text, affected by the "colorize" configuration option. These images are included in the release:
- `fire.webp`
- `fire2.webp`
- `fire3.webp`
### Build
---
VSCode is recommended as it will do everything for you.
1. Ensure you have clang and Ninja installed and in PATH.
2. Ensure Windows SDK is installed
---
Manual instructions:
1. Create a build directory and run CMake with Ninja generator.
```powershell
mkdir build
cmake -S . -B build
cmake --build build --config Debug
```
(Debug can be replaced with "Release" depending on the build target)
CPM will download all the libraries on the first configure (second command).
### Notes
- Exit with `ctrl+alt+f`
### Credits
- [FaceDev](https://youtu.be/ROMLBio1iCI?t=387) for the meme Linux distro that has this feature, I simply remade it for windows but better.
- All the stupid reels that used the [Tamam Keyboard](https://play.google.com/store/apps/details?id=com.ziipin.softkeyboard.saudi) app.
- [Tamam Keyboard](https://play.google.com/store/apps/details?id=com.ziipin.softkeyboard.saudi) itself.
### Libraries used
- [raylib](https://github.com/raysan5/raylib)
- GLFW, miniaudio, bundled with raylib
- [libwebp](https://github.com/webmproject/libwebp), for webp support