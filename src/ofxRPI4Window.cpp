#include "ofxRPI4Window.h"

#define __func__ __PRETTY_FUNCTION__

#define CASE_STR(x,y) case x: str = y; break

static string eglErrorString(EGLint err) {
    string str;
    switch (err) {
            CASE_STR(EGL_SUCCESS, "EGL_SUCCESS");
            CASE_STR(EGL_NOT_INITIALIZED, "EGL_NOT_INITIALIZED");
            CASE_STR(EGL_BAD_ACCESS, "EGL_BAD_ACCESS");
            CASE_STR(EGL_BAD_ALLOC, "EGL_BAD_ACCESS");
            CASE_STR(EGL_BAD_ATTRIBUTE, "EGL_BAD_ATTRIBUTE");
            CASE_STR(EGL_BAD_CONTEXT, "EGL_BAD_CONTEXT");
            CASE_STR(EGL_BAD_CONFIG, "EGL_BAD_CONFIG");
            CASE_STR(EGL_BAD_CURRENT_SURFACE, "EGL_BAD_CURRENT_SURFACE");
            CASE_STR(EGL_BAD_DISPLAY, "EGL_BAD_DISPLAY");
            CASE_STR(EGL_BAD_SURFACE, "EGL_BAD_SURFACE");
            CASE_STR(EGL_BAD_MATCH, "EGL_BAD_MATCH");
            CASE_STR(EGL_BAD_PARAMETER, "EGL_BAD_PARAMETER");
            CASE_STR(EGL_BAD_NATIVE_PIXMAP, "EGL_BAD_NATIVE_PIXMAP");
            CASE_STR(EGL_BAD_NATIVE_WINDOW, "EGL_BAD_NATIVE_WINDOW");
            CASE_STR(EGL_CONTEXT_LOST, "EGL_CONTEXT_LOST");
        default: str = "unknown error " + err; break;
    }
    return str;
}


ofxRPI4Window::ofxRPI4Window() {
    orientation = OF_ORIENTATION_DEFAULT;
    skipRender = false;
    previousBo = NULL;
    lastFrameTimeMillis = 0;
}
ofxRPI4Window::ofxRPI4Window(const ofGLESWindowSettings & settings) {
    ofLog() << "CTOR CALLED WITH settings";
    setup(settings);
}

int match_config_to_visual(EGLDisplay egl_display,
                           EGLint visual_id,
                           EGLConfig *configs,
                           int count)
{
    int i;
    
    for (i = 0; i < count; ++i) {
        EGLint id;
        
        if (!eglGetConfigAttrib(egl_display,
                                configs[i], EGL_NATIVE_VISUAL_ID,
                                &id))
            continue;
        
        if (id == visual_id)
            return i;
    }
    
    return -1;
}




void ofxRPI4Window::setup(const ofGLESWindowSettings & settings)
{
    
    bEnableSetupScreen = true;
    windowMode = OF_WINDOW;
    glesVersion = settings.glesVersion;
    device = open("/dev/dri/card1", O_RDWR | O_CLOEXEC);
    drmModeRes* resources = drmModeGetResources(device);
    bool passed = false;
    if (resources == NULL)
    {
        ofLogError() << "Unable to get DRM resources";
    }
    
    drmModeConnector* connector = NULL;
    
    for (int i = 0; i < resources->count_connectors; i++)
    {
        drmModeConnector* modeConnector = drmModeGetConnector(device, resources->connectors[i]);
        if (modeConnector->connection == DRM_MODE_CONNECTED)
        {
            connector = modeConnector;
            break;
        }
        drmModeFreeConnector(connector);
    }
    
    
    if (connector == NULL)
    {
        ofLogError() << "Unable to get connector";
        drmModeFreeResources(resources);
    }
    
    connectorId = connector->connector_id;
    mode = connector->modes[0];
    
    currentWindowRect = ofRectangle(0, 0, mode.hdisplay, mode.vdisplay);
    ofLog() << "currentWindowRect: " << currentWindowRect;
    
    drmModeEncoder* encoder = NULL;
    
    if (connector->encoder_id)
    {
        encoder = drmModeGetEncoder(device, connector->encoder_id);
    }
    
    
    if (encoder == NULL)
    {
        ofLogError() << "Unable to get encoder";
        
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
    }else
    {
        passed = true;
    }
    
    if(passed)
    {
        crtc = drmModeGetCrtc(device, encoder->crtc_id);
        drmModeFreeEncoder(encoder);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        gbmDevice = gbm_create_device(device);
        gbmSurface = gbm_surface_create(gbmDevice, mode.hdisplay, mode.vdisplay, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
        display = eglGetDisplay(gbmDevice);
        if (!display)
        {
            auto error = eglGetError();
            ofLogError() << "display ERROR: " << eglErrorString(error);
        }
        
        int major, minor;
        eglInitialize(display, &major, &minor);
        //eglBindAPI(EGL_OPENGL_API);
        eglBindAPI(EGL_OPENGL_ES_API);
        
        EGLint count = 0;
        EGLint matched = 0;
        int config_index = -1;
        
        if (!eglGetConfigs(display, NULL, 0, &count) || count < 1)
        {
            ofLogError() << "No EGL configs to choose from";
        }
        
        EGLConfig configs[count];
        EGLint configAttribs[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_DEPTH_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE};
        
        EGLint visualId = GBM_FORMAT_XRGB8888;
        EGLConfig config = NULL;
        
        
        
        
        
        if (!eglChooseConfig(display, configAttribs, configs,
                             count, &matched) || !matched) {
            printf("No EGL configs with appropriate attributes.\n");
        }
        
        if (config_index == -1)
        {
            config_index = match_config_to_visual(display,
                                                  visualId,
                                                  configs,
                                                  matched);
        }
        
        if (config_index != -1)
        {
            config = configs[config_index];
        }
        
        
        
        const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE};
        
        if(config)
        {
            context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
            if (!context)
            {
                auto error = eglGetError();
                ofLogError() << "context ERROR: " << eglErrorString(error);
            }
            
            surface = eglCreateWindowSurface(display, config, gbmSurface, NULL);
            if (!surface)
            {
                auto error = eglGetError();
                ofLogError() << "surface ERROR: " << eglErrorString(error);
            }
            
            currentRenderer = make_shared<ofGLProgrammableRenderer>(this);
            makeCurrent();
            static_cast<ofGLProgrammableRenderer*>(currentRenderer.get())->setup(2,0);
            
#if 1
            
            
            ofLog() << "-----EGL-----";
            //ofLog() << "EGL_VERSION_MAJOR = " << eglVersionMajor;
            //ofLog() << "EGL_VERSION_MINOR = " << eglVersionMinor;
            ofLog() << "EGL_CLIENT_APIS = " << eglQueryString(getEGLDisplay(), EGL_CLIENT_APIS);
            ofLog() << "EGL_VENDOR = "  << eglQueryString(getEGLDisplay(), EGL_VENDOR);
            ofLog() << "EGL_VERSION = " << eglQueryString(getEGLDisplay(), EGL_VERSION);
            ofLog() << "EGL_EXTENSIONS = " << eglQueryString(getEGLDisplay(), EGL_EXTENSIONS);
            ofLog() << "GL_SHADING_LANGUAGE_VERSION   = " << glGetString(GL_SHADING_LANGUAGE_VERSION);
            ofLog() << "GL_RENDERER = " << glGetString(GL_RENDERER);
            ofLog() << "GL_VERSION  = " << glGetString(GL_VERSION);
            ofLog() << "GL_VENDOR   = " << glGetString(GL_VENDOR);
            ofLog() << "-------------";
            
            auto gl_exts = (char *) glGetString(GL_EXTENSIONS);
            ofLog(OF_LOG_VERBOSE, "GL INFO");
            ofLog(OF_LOG_VERBOSE, "  version: \"%s\"", glGetString(GL_VERSION));
            ofLog(OF_LOG_VERBOSE, "  shading language version: \"%s\"", glGetString(GL_SHADING_LANGUAGE_VERSION));
            ofLog(OF_LOG_VERBOSE, "  vendor: \"%s\"", glGetString(GL_VENDOR));
            ofLog(OF_LOG_VERBOSE, "  renderer: \"%s\"", glGetString(GL_RENDERER));
            ofLog(OF_LOG_VERBOSE, "  extensions: \"%s\"", gl_exts);
            ofLog(OF_LOG_VERBOSE, "===================================\n");
            //get_proc_gl(GL_OES_EGL_image, glEGLImageTargetTexture2DOES);
            
#endif
        }else
        {
            ofLog() << "RIP";
        }
        
        
        
        
    }
    
    
    
    
    
    
    
}










void ofxRPI4Window::makeCurrent()
{
    eglMakeCurrent(display, surface, surface, context);
    
}

void ofxRPI4Window::update()
{
    //ofLog() << "update";
    coreEvents.notifyUpdate();
    
}

int ofxRPI4Window::getWidth()
{
    //ofLog() << __func__ << currentWindowRect.width;
    return currentWindowRect.width;
}

int ofxRPI4Window::getHeight()
{
    //ofLog() << __func__ << currentWindowRect.height;
    
    return currentWindowRect.height;
}

glm::vec2 ofxRPI4Window::getScreenSize()
{
    
    //ofLog() << __func__;
    return {currentWindowRect.getWidth(), currentWindowRect.getHeight()};
}

glm::vec2 ofxRPI4Window::getWindowSize()
{
    //ofLog() << __func__;
    return {currentWindowRect.width, currentWindowRect.height};
}

//------------------------------------------------------------
glm::vec2 ofxRPI4Window::getWindowPosition(){
    //ofLog() << __func__;
    return glm::vec2(currentWindowRect.getPosition());
}


void ofxRPI4Window::swapBuffers()
{
    // ofLog() << __func__;
    
    EGLBoolean success = eglSwapBuffers(display, surface);
    if(!success) {
        GLint error = eglGetError();
        ofLog() << "eglSwapBuffers failed: " << eglErrorString(error);
    }
    struct gbm_bo *bo = gbm_surface_lock_front_buffer(gbmSurface);
    uint32_t handle = gbm_bo_get_handle(bo).u32;
    uint32_t pitch = gbm_bo_get_stride(bo);
    uint32_t fb;
    drmModeAddFB(device, mode.hdisplay, mode.vdisplay, 24, 32, pitch, handle, &fb);
    drmModeSetCrtc(device, crtc->crtc_id, fb, 0, 0, &connectorId, 1, &mode);
    
    if (previousBo)
    {
        drmModeRmFB(device, previousFb);
        gbm_surface_release_buffer(gbmSurface, previousBo);
    }
    previousBo = bo;
    previousFb = fb;
    
}

void ofxRPI4Window::startRender()
{
    //ofLog() << __func__;
    
    renderer()->startRender();
}

void ofxRPI4Window::finishRender()
{
    //ofLog() << __func__;
    renderer()->finishRender();
}


float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

void ofxRPI4Window::draw()
{
    
    
    
    int waiting_for_flip = 1;
    auto startFrame = ofGetElapsedTimeMillis();
    gettimeofday(&t0, 0);
    
    
    if(skipRender)
    {
        
        coreEvents.notifyDraw();
        
        swapBuffers();
        
    }else
    {
        currentRenderer->startRender();
        if( bEnableSetupScreen )
        {
            currentRenderer->setupScreen();
            //bEnableSetupScreen = false;
        }
        
        
        coreEvents.notifyDraw();
        currentRenderer->finishRender();
        swapBuffers();
    }
    
    
    gettimeofday(&t1, 0);
    
    lastFrameTimeMillis = timedifference_msec(t0, t1);
    
    //printf("Code executed in %f milliseconds.\n", elapsed);
    
    
    
    
}

void ofxRPI4Window::setWindowShape(int w, int h)
{
    currentWindowRect = ofRectangle(currentWindowRect.x,currentWindowRect.y, w, h);
}

void ofxRPI4Window::pollEvents()
{
    //ofLog() << "pollEvents";
    
}

ofCoreEvents & ofxRPI4Window::events(){
    return coreEvents;
}


void ofxRPI4Window::enableSetupScreen(){
    bEnableSetupScreen = true;
}

//------------------------------------------------------------
void ofxRPI4Window::disableSetupScreen(){
    bEnableSetupScreen = false;
}

void ofxRPI4Window::setVerticalSync(bool enabled)
{
    eglSwapInterval(display, enabled ? 1 : 0);
}

EGLDisplay ofxRPI4Window::getEGLDisplay()
{
    //ofLog() << __func__;
    return display;
}

EGLContext ofxRPI4Window::getEGLContext()
{
    //ofLog() << __func__;
    
    return context;
}

EGLSurface ofxRPI4Window::getEGLSurface()
{
    //ofLog() << __func__;
    
    return surface;
}

shared_ptr<ofBaseRenderer> & ofxRPI4Window::renderer(){
    
    //ofLog() << __func__;
    
    return currentRenderer;
}

string ofxRPI4Window::getInfo()
{
    
    
    stringstream info;
    
    info << "ofGetFrameRate(): " << ofGetFrameRate() << endl;
    info << "ofGetLastFrameTime(): " << ofGetLastFrameTime() << endl;
    info << "lastFrameTimeMillis: " << lastFrameTimeMillis << endl;

    info << "ofGetWidth(): " << ofGetWidth() << endl;
    info << " ofGetHeight(): " << ofGetHeight()<< endl;
    info << " ofGetScreenHeight(): " << ofGetScreenHeight()<< endl;
    info << " ofGetScreenWidth(): " << ofGetScreenWidth()<< endl;
    info << " ofGetWindowWidth(): " << ofGetWindowWidth()<< endl;
    info << " ofGetWindowHeight(): " << ofGetWindowHeight()<< endl;
    info << " ofGetWindowPositionX(): " << ofGetWindowPositionX()<< endl;
    info << " ofGetWindowPositionY(): " << ofGetWindowPositionY()<< endl;
    info << " ofGetWindowRect(): " << ofGetWindowRect()<< endl;
    
    return info.str();
}
ofxRPI4Window::~ofxRPI4Window()
{
    
}



