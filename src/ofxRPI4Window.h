#pragma once
#define USING_DRM 1

#include "ofConstants.h"
#include "ofAppBaseWindow.h"
#include "ofThread.h"
#include "ofImage.h"
#include "ofEvents.h"
#include "ofRectangle.h"


#include <queue>
#include <map>
#include <X11/Xlib.h>



enum ofAppEGLWindowType {
    OF_APP_WINDOW_AUTO,
    OF_APP_WINDOW_NATIVE,
    OF_APP_WINDOW_X11
};

typedef std::map<EGLint,EGLint> ofEGLAttributeList;


struct ofxRPI4WindowSettings: public ofGLESWindowSettings {
public:
    ofAppEGLWindowType eglWindowPreference; ///< what window type is preferred?
    EGLint eglWindowOpacity; ///< 0-255 window alpha value
    
    ofEGLAttributeList frameBufferAttributes;
    // surface creation
    ofEGLAttributeList windowSurfaceAttributes;
    
    ofColor initialClearColor;
    
    int screenNum;
    int layer;
    
    ofxRPI4WindowSettings();
    ofxRPI4WindowSettings(const ofGLESWindowSettings & settings);
};

class ofxRPI4Window : public ofAppBaseGLESWindow, public ofThread {
public:
    
    static bool allowsMultiWindow(){ return false; }
    static bool doesLoop(){ return false; }
    static void loop(){};
    static bool needsPolling(){ return true; }
    static void pollEvents();
    
    
    ofxRPI4Window();
    ofxRPI4Window(const ofGLESWindowSettings & settings);

    void update();
    void draw();
    
    ofCoreEvents coreEvents;
    ofCoreEvents & events();
    std::shared_ptr<ofBaseRenderer> & renderer();
    void setup(const ofGLESWindowSettings & settings);
    
    std::shared_ptr<ofBaseRenderer> currentRenderer;
    
    virtual ~ofxRPI4Window();
    
    
};
