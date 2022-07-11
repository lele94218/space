Installation
------------

### MacOS

#### OpenGL Install

```
brew install glfw
brew install glog
```

Then, under the repo directory:
```
cmake .
make
./main
```

#### SDL2 Install

Download release version [SDL2](https://www.libsdl.org/download-2.0.php). Then
move folder `SDL2.framework` to
`/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks`
