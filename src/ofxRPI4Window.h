#pragma once

#include "ofConstants.h"
#include "ofAppBaseWindow.h"
#include "ofRectangle.h"
#include "ofGLProgrammableRenderer.h"

#include <stdio.h> // sprintf
#include <stdlib.h>  // malloc
#include <fcntl.h>  // open fcntl
#include <unistd.h> // read close
#include <string.h> // strlen
#include <sys/time.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <gbm.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>


#ifndef EGL_KHR_platform_gbm
#define EGL_KHR_platform_gbm 1
#define EGL_PLATFORM_GBM_KHR              0x31D7
#endif /* EGL_KHR_platform_gbm */



typedef EGLDisplay (EGLAPIENTRYP PFNEGLGETPLATFORMDISPLAYEXTPROC) (EGLenum platform, void *native_display, const EGLint *attrib_list);

using namespace std;


//Linux RPI4 4.19.75-v7l+ #1270 SMP Tue Sep 24 18:51:41 BST 2019 armv7l GNU/Linux 
class ofxRPI4Window : public ofAppBaseGLESWindow
{
public:
    

    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    
    int device;
    drmModeModeInfo mode;
    struct gbm_device* gbmDevice;
    struct gbm_surface* gbmSurface;
    drmModeCrtc *crtc;
    uint32_t connectorId;
    gbm_bo *previousBo;
    uint32_t previousFb;
    
    ofRectangle currentWindowRect;
    ofOrientation orientation;
    bool bEnableSetupScreen;
    int glesVersion;
    ofWindowMode windowMode;


    static bool allowsMultiWindow(){ return false; }
    static bool doesLoop(){ return false; }
    static void loop(){};
    static bool needsPolling(){ return true; }
    static void pollEvents();
    

    ofxRPI4Window();
    ofxRPI4Window(const ofGLESWindowSettings & settings);

   
    int getWidth() override;
    int getHeight() override;
    glm::vec2 getWindowSize() override;
    glm::vec2 getWindowPosition() override;
    glm::vec2 getScreenSize()  override;
    ofOrientation getOrientation() { return orientation; }
    void enableSetupScreen() override;
    void disableSetupScreen() override;
    void setWindowShape(int w, int h) override;
    void setVerticalSync(bool enabled) override;
    
    void update() override;
    void draw() override;
    void swapBuffers() override;
    
    void makeCurrent() override;
    void startRender() override;
    void finishRender() override;
    
    ofCoreEvents coreEvents;
    ofCoreEvents & events();
    std::shared_ptr<ofBaseRenderer> & renderer();
    void setup(const ofGLESWindowSettings & settings);
    
    std::shared_ptr<ofBaseRenderer> currentRenderer;
    
  
    EGLDisplay getEGLDisplay() override;
    EGLContext getEGLContext() override;
    EGLSurface getEGLSurface() override;
  
    virtual ~ofxRPI4Window();
    bool skipRender;
    struct timeval t0;
    struct timeval t1;
    float lastFrameTimeMillis;
    string getInfo();
    void gbmClean();
};
