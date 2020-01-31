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


class CRTC
{
public:
    
    drmModeCrtc *crtc;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    
    CRTC()
    {
        crtc = NULL;
        props = NULL;
        props_info = NULL;
        
    }
    
};

class Connector {
public:
    
    drmModeConnector *connector;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    
    Connector()
    {
        connector = NULL;
        props = NULL;
        props_info = NULL;
    }
    
};

class DRM
{
public:
    
    int fd;
    
    
    CRTC crtc;
    Connector connector;
    int crtc_index;
    int kms_in_fence_fd;
    int kms_out_fence_fd;
    
    drmModeModeInfo *mode;
    uint32_t crtc_id;
    uint32_t connector_id;
        
    DRM()
    {
        fd = 0;
        mode = NULL;
    }
    
};

class GBM
{
public:
    
    struct gbm_device *dev;
    struct gbm_surface *surface;
    uint32_t format;
    int width, height;
    
    GBM()
    {
        
        format = 0;
        width = 0;
        height = 0;
        
    }
    
};






class EGL {
    
public:
    
    EGLDisplay display;
    EGLConfig config;
    EGLContext context;
    EGLSurface surface;
    EGLint eglVersionMajor;
    EGLint eglVersionMinor;
    
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
    PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
    PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
    PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
    PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
    
    bool modifiers_supported;
        
    EGL()
    {
        
        display = NULL;
        config = NULL;
        context = NULL;
        surface = NULL;
        eglVersionMajor = 0;
        eglVersionMinor = 0;

    }
};

typedef std::map<EGLint,EGLint> ofEGLAttributeList;



struct drm_fb {
    struct gbm_bo *bo;
    uint32_t fb_id;
};

class ofxRPI4Window : public ofAppBaseGLESWindow
{
public:
    
    EGL egl;
    DRM drm;
    GBM gbm;
    fd_set fds;
    drmEventContext evctx;
    gbm_bo* bufferObject;
    
    
    ofRectangle currentWindowRect;
    ofOrientation orientation;
    bool bEnableSetupScreen;
    int glesVersion;
    ofWindowMode windowMode;

    void init_gbm(int drm_fd, int w, int h, uint32_t format, uint64_t modifier);
    void init_drm(const char *device, const char *mode_str, unsigned int vrefresh);
    void init_egl(int samples);
    static bool allowsMultiWindow(){ return false; }
    static bool doesLoop(){ return false; }
    static void loop(){};
    static bool needsPolling(){ return true; }
    static void pollEvents();
    
    drm_fb* drm_fb_get_from_bo(gbm_bo* bo);

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

};
