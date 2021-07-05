# ofxRPI4Window

### STATUS
Very early, no Mouse/Keyboard support

### DESCRIPTION:   
This is an openFrameworks addon for the Raspberry Pi to allow rendering without X

### REQUIREMENTS:   
- openFrameworks 11
- Raspberry Pi 4 or 3B+ (previous ones might work too)
- Raspbian Buster (or Stretch on Rpi <= 3)
- KMS Driver enabled
- Modify your openframeworks installation:   

 #### Manual Option  
Change `openFrameworks/libs/openFrameworksCompiled/project/linuxarmv6l/config.linuxarmv6l.default.mk`   

```
    ifeq ($(USE_PI_LEGACY), 1)
    	PLATFORM_DEFINES += TARGET_RASPBERRY_PI_LEGACY
        $(info using legacy build)
    else
    	# comment this for older EGL windowing. Has no effect if USE_PI_LEGACY is enabled
    	# GLFW seems to provide a more robust window on newer Raspbian releases
	#USE_GLFW_WINDOW = 1
    endif
```
    
Comment out `ofSetupOpenGL` in 
https://github.com/openframeworks/openFrameworks/blob/master/libs/openFrameworks/app/ofAppRunner.cpp#L31

#### Automatic Option  
Run `./patchOF.sh`

### USAGE:   
Clone into openFrameworks/addons  
If needed, `sudo apt-get install libgbm-dev`.

### RUNNING
Use the classic oF way to Launch your application. (eg: `make RunDebug`)  
*Note: Quitting is not yet properly supported, use Ctrl+C to interrupt your ofApp.*

### CREDITS:   
derived from 
https://github.com/matusnovak/rpi-opengl-without-x

https://gitlab.freedesktop.org/mesa/kmscube/tree/master
