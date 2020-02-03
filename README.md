# ofxRPI4Window

### STATUS
Very early, no Mouse/Keyboard support

### DESCRIPTION:   
This is an openFrameworks addon for the Raspberry Pi to allow rendering without X

### REQUIREMENTS:   
- openFrameworks 11
- Raspberry Pi 4
- Raspbian Buster
- KMS Driver enabled
- Change `openFrameworks/libs/openFrameworksCompiled/project/linuxarmv6l/config.linuxarmv6l.default.mk`

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

comment out `ofSetupOpenGL` in 
https://github.com/openframeworks/openFrameworks/blob/master/libs/openFrameworks/app/ofAppRunner.cpp#L31


### USAGE:   
Clone into openFrameworks/addons

### CREDITS:   
derived from 
https://gitlab.freedesktop.org/mesa/kmscube/tree/master
