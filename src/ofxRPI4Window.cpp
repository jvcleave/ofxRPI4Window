#include "ofxRPI4Window.h"

#include "ofGraphics.h" // used in runAppViaInfiniteLoop()
#include "ofAppRunner.h"
#include "ofUtils.h"
#include "ofFileUtils.h"
#include "ofGLProgrammableRenderer.h"
#include "ofGLRenderer.h"
#include "ofVectorMath.h"
#include <assert.h>


// x11
#include <X11/Xutil.h>
#include <EGL/egl.h>

// include includes for both native and X11 possibilities
#include <libudev.h>
#include <stdbool.h>
#include <stdio.h> // sprintf
#include <stdlib.h>  // malloc
#include <math.h>
#include <fcntl.h>  // open fcntl
#include <unistd.h> // read close
#include <linux/joystick.h>

#include "linux/kd.h"    // keyboard stuff...
#include "termios.h"
#include "sys/ioctl.h"

#include <string.h> // strlen

using namespace std;



#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <gbm.h>

//#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifndef EGL_KHR_platform_gbm
#define EGL_KHR_platform_gbm 1
#define EGL_PLATFORM_GBM_KHR              0x31D7
#endif /* EGL_KHR_platform_gbm */


typedef EGLDisplay (EGLAPIENTRYP PFNEGLGETPLATFORMDISPLAYEXTPROC) (EGLenum platform, void *native_display, const EGLint *attrib_list);

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
    
    int (*run)(const struct gbm *gbm, const struct egl *egl);
    
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
    
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
    PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
    PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
    PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
    PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
    
    bool modifiers_supported;
    
    void (*draw)(unsigned i);
    
    EGL()
    {
        
        display = NULL;
        config = NULL;
        context = NULL;
        surface = NULL;
        
        
        
        
    }
};

static bool has_ext(const char *extension_list, const char *ext)
{
    
    ofLog() << "extension_list: " << extension_list;
    ofLog() << "ext: " << ext;
    
    const char *ptr = extension_list;
    int len = strlen(ext);
    
    if (ptr == NULL || *ptr == '\0')
        return false;
    
    while (true) {
        ptr = strstr(ptr, ext);
        if (!ptr)
            return false;
        
        if (ptr[len] == ' ' || ptr[len] == '\0')
            return true;
        
        ptr += len;
    }
}

static int
match_config_to_visual(EGLDisplay egl_display,
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

static bool
egl_choose_config(EGLDisplay egl_display, const EGLint *attribs,
                  EGLint visual_id, EGLConfig *config_out)
{
    EGLint count = 0;
    EGLint matched = 0;
    int config_index = -1;
    
    if (!eglGetConfigs(egl_display, NULL, 0, &count) || count < 1) {
        printf("No EGL configs to choose from.\n");
        return false;
    }
    
    EGLConfig configs[count];
    
    
    
    if (!eglChooseConfig(egl_display, attribs, configs,
                         count, &matched) || !matched) {
        printf("No EGL configs with appropriate attributes.\n");
        goto out;
    }
    
    if (!visual_id)
        config_index = 0;
    
    if (config_index == -1)
        config_index = match_config_to_visual(egl_display,
                                              visual_id,
                                              configs,
                                              matched);
    
    if (config_index != -1)
        *config_out = configs[config_index];
    
out:
    if (config_index == -1)
        return false;
    
    return true;
}



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
/*
 void get_proc_client(EGL* egl, const char *ext, const char *name)
 {
 if (has_ext(egl_exts_client, ext))
 {
 egl->name = (void *)eglGetProcAddress(name);
 }
 }*/
EGL init_egl(GBM& gbm, int samples)
{
    
    EGL egl;
    
    
    EGLint major, minor;
    
    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    
    const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SAMPLES, samples,
        EGL_NONE
    };
    const char *egl_exts_client, *egl_exts_dpy, *gl_exts;
    
    
    
    egl_exts_client = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    
    egl.modifiers_supported = has_ext(egl_exts_dpy,
                                      "EGL_EXT_image_dma_buf_import_modifiers");
    
    
    egl.eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    egl.eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    egl.eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    egl.glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    egl.eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC)eglGetProcAddress("eglCreateSyncKHR");
    egl.eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC)eglGetProcAddress("eglDestroySyncKHR");
    egl.eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC)eglGetProcAddress("eglClientWaitSyncKHR");
    
    
    egl.display = egl.eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, gbm.dev, NULL);
    
    if (!eglInitialize(egl.display, &major, &minor)) {
        ofLog(OF_LOG_VERBOSE, "failed to initialize\n");
    }
    
    egl_exts_dpy = eglQueryString(egl.display, EGL_EXTENSIONS);
    /*get_proc_dpy(EGL_KHR_image_base, eglCreateImageKHR);
     get_proc_dpy(EGL_KHR_image_base, eglDestroyImageKHR);
     get_proc_dpy(EGL_KHR_fence_sync, eglCreateSyncKHR);
     get_proc_dpy(EGL_KHR_fence_sync, eglDestroySyncKHR);
     get_proc_dpy(EGL_KHR_fence_sync, eglWaitSyncKHR);
     get_proc_dpy(EGL_KHR_fence_sync, eglClientWaitSyncKHR);
     get_proc_dpy(EGL_ANDROID_native_fence_sync, eglDupNativeFenceFDANDROID);*/
    
    egl.modifiers_supported = has_ext(egl_exts_dpy,
                                      "EGL_EXT_image_dma_buf_import_modifiers");
    
    
    ofLog() << "egl.modifiers_supported: " << egl.modifiers_supported;
    
    ofLog(OF_LOG_VERBOSE, "Using display %p with EGL version %d.%d\n",
          egl.display, major, minor);
    
    ofLog(OF_LOG_VERBOSE, "===================================\n");
    ofLog(OF_LOG_VERBOSE, "EGL information:\n");
    ofLog(OF_LOG_VERBOSE, "  version: \"%s\"\n", eglQueryString(egl.display, EGL_VERSION));
    ofLog(OF_LOG_VERBOSE, "  vendor: \"%s\"\n", eglQueryString(egl.display, EGL_VENDOR));
    ofLog(OF_LOG_VERBOSE, "  client extensions: \"%s\"\n", egl_exts_client);
    ofLog(OF_LOG_VERBOSE, "  display extensions: \"%s\"\n", egl_exts_dpy);
    ofLog(OF_LOG_VERBOSE, "===================================\n");
    
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        ofLog(OF_LOG_VERBOSE, "failed to bind api EGL_OPENGL_ES_API\n");
    }
    
    if (!egl_choose_config(egl.display, config_attribs, gbm.format,
                           &egl.config)) {
        ofLog(OF_LOG_VERBOSE, "failed to choose config\n");
    }else
    {
        ofLog() << "egl_choose_config PASS";
        
        
    }
    
    
    EGLint attribute_list_surface_context[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    
    egl.context = eglCreateContext(egl.display,
                                   egl.config,
                                   EGL_NO_CONTEXT,
                                   attribute_list_surface_context);
    
    
    
    if(egl.context == EGL_NO_CONTEXT) {
        EGLint error = eglGetError();
        if(error == EGL_BAD_CONFIG) {
            ofLogError() << "error creating context: EGL_BAD_CONFIG: " << eglErrorString(error);
            //return false;
        } else {
            ofLogError() << "error creating context: " << eglErrorString(error);
            //return false;
        }
    }else
    {
        ofLog() << "egl.context IS VALID: ", egl.context;
    }
    
    
    
    egl.surface = eglCreateWindowSurface(egl.display, egl.config,
                                         (EGLNativeWindowType)gbm.surface, NULL);
    if (egl.surface == EGL_NO_SURFACE) {
        EGLint error = eglGetError();
        ofLogError() << "error creating surface: " << eglErrorString(error) << " error: " << error;
        
    }else
    {
        ofLog() << "egl.surface IS VALID: ", egl.surface;
    }
    
    /* connect the context to the surface */
    
    
    return egl;
    
}

int get_resources(int fd, drmModeRes **resources)
{
    *resources = drmModeGetResources(fd);
    if (*resources == NULL)
        return -1;
    return 0;
}

#define MAX_DRM_DEVICES 64

uint32_t find_crtc_for_encoder(const drmModeRes *resources,
                               const drmModeEncoder *encoder) {
    int i;
    
    for (i = 0; i < resources->count_crtcs; i++) {
        /* possible_crtcs is a bitmask as described here:
         * https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
         */
        const uint32_t crtc_mask = 1 << i;
        const uint32_t crtc_id = resources->crtcs[i];
        if (encoder->possible_crtcs & crtc_mask) {
            return crtc_id;
        }
    }
    
    /* no match found */
    return -1;
}


uint32_t find_crtc_for_connector(DRM& drm, drmModeRes *resources, drmModeConnector *connector) {
    int i;
    
    for (i = 0; i < connector->count_encoders; i++) {
        const uint32_t encoder_id = connector->encoders[i];
        drmModeEncoder *encoder = drmModeGetEncoder(drm.fd, encoder_id);
        
        if (encoder) {
            const uint32_t crtc_id = find_crtc_for_encoder(resources, encoder);
            
            drmModeFreeEncoder(encoder);
            if (crtc_id != 0) {
                return crtc_id;
            }
        }
    }
    
    /* no match found */
    return -1;
}


int find_drm_device(drmModeRes **resources)
{
    drmDevicePtr devices[MAX_DRM_DEVICES] = { NULL };
    int num_devices, fd = -1;
    
    num_devices = drmGetDevices2(0, devices, MAX_DRM_DEVICES);
    if (num_devices < 0) {
        ofLog(OF_LOG_VERBOSE, "drmGetDevices2 failed: %s\n", strerror(-num_devices));
        return -1;
    }
    ofLog() << "num_devices: " << num_devices;
    
    for (int i = 0; i < num_devices; i++) {
        drmDevicePtr device = devices[i];
        int ret;
        
        if (!(device->available_nodes & (1 << DRM_NODE_PRIMARY)))
            continue;
        /* OK, it's a primary device. If we can get the
         * drmModeResources, it means it's also a
         * KMS-capable device.
         */
        fd = open(device->nodes[DRM_NODE_PRIMARY], O_RDWR);
        if (fd < 0)
            continue;
        ret = get_resources(fd, resources);
        if (!ret)
            break;
        close(fd);
        fd = -1;
    }
    drmFreeDevices(devices, num_devices);
    
    if (fd < 0)
        ofLog(OF_LOG_VERBOSE, "no drm device found!\n");
    
    ofLog() << "fd: " << fd;
    
    
    return fd;
}

int init_drm(DRM& drm, const char *device, const char *mode_str, unsigned int vrefresh)
{
    drmModeRes *resources;
    drmModeConnector *connector = NULL;
    drmModeEncoder *encoder = NULL;
    int i, ret, area;
    
    if (device) {
        drm.fd = open(device, O_RDWR);
        ret = get_resources(drm.fd, &resources);
        if (ret < 0 && errno == EOPNOTSUPP)
            ofLog(OF_LOG_VERBOSE, "%s does not look like a modeset device\n", device);
    } else {
        drm.fd = find_drm_device(&resources);
    }
    
    if (drm.fd < 0) {
        ofLog(OF_LOG_VERBOSE, "could not open drm device\n");
        return -1;
    }
    
    if (!resources) {
        ofLog(OF_LOG_VERBOSE, "drmModeGetResources failed: %s\n", strerror(errno));
        return -1;
    }
    
    /* find a connected connector: */
    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) {
            /* it's connected, let's use this! */
            ofLog() << "FOUND CONNECTION";
            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }
    
    if (!connector) {
        /* we could be fancy and listen for hotplug events and wait for
         * a connector..
         */
        ofLog(OF_LOG_VERBOSE, "no connected connector!\n");
        return -1;
    }
    
    /* find user requested mode: */
    /*
     if (mode_str && *mode_str) {
     for (i = 0; i < connector->count_modes; i++) {
     drmModeModeInfo *current_mode = &connector->modes[i];
     
     if (strcmp(current_mode->name, mode_str) == 0) {
     if (vrefresh == 0 || current_mode->vrefresh == vrefresh) {
     drm.mode = current_mode;
     break;
     }
     }
     }
     if (!drm.mode)
     {
     ofLog(OF_LOG_VERBOSE, "requested mode not found, using default mode!\n");
     }else
     {
     ofLog() << "FOUND drm.mode: " << drm.mode;
     
     }
     }*/
    
    /* find preferred mode or the highest resolution mode: */
    if (!drm.mode) {
        for (i = 0, area = 0; i < connector->count_modes; i++) {
            drmModeModeInfo *current_mode = &connector->modes[i];
            
            if (current_mode->type & DRM_MODE_TYPE_PREFERRED) {
                drm.mode = current_mode;
                if(current_mode->hdisplay)
                {
                    ofLog() << "current_mode hdisplay: " << current_mode->hdisplay;
                    
                }else
                {
                    ofLog() << "NO current_mode hdisplay";
                    
                }
                break;
            }
            
            int current_area = current_mode->hdisplay * current_mode->vdisplay;
            if (current_area > area) {
                drm.mode = current_mode;
                area = current_area;
            }
            ofLog() << "current_area: " << current_area;
        }
    }
    
    if (!drm.mode) {
        ofLog(OF_LOG_VERBOSE, "could not find mode!\n");
        return -1;
    }
    
    /* find encoder: */
    for (i = 0; i < resources->count_encoders; i++) {
        encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
        if (encoder->encoder_id == connector->encoder_id)
            break;
        drmModeFreeEncoder(encoder);
        encoder = NULL;
    }
    
    if (encoder) {
        drm.crtc_id = encoder->crtc_id;
    } else {
        uint32_t crtc_id = find_crtc_for_connector(drm, resources, connector);
        if (crtc_id == 0) {
            ofLog(OF_LOG_VERBOSE, "no crtc found!\n");
            return -1;
        }else
        {
            ofLog() << "FOUND crtc_id: " << crtc_id;
        }
        
        drm.crtc_id = crtc_id;
    }
    
    for (i = 0; i < resources->count_crtcs; i++) {
        if (resources->crtcs[i] == drm.crtc_id) {
            drm.crtc_index = i;
            break;
        }
    }
    
    drmModeFreeResources(resources);
    
    drm.connector_id = connector->connector_id;
    ofLog() << "FOUND drm.connector_id: " << drm.connector_id;
    
    return 0;
}


DRM init_drm_legacy(const char *device, const char *mode_str, unsigned int vrefresh)
{
    
    DRM drm;
    
    auto ret = init_drm(drm, device, mode_str, vrefresh);
    
    ofLog() << "ret: " << ret;
    
    return drm;
    
}

GBM init_gbm(int drm_fd, int w, int h, uint32_t format, uint64_t modifier)
{
    GBM gbm;
    ofLog() << "drm_fd: " << drm_fd;
    
    gbm.dev = gbm_create_device(drm_fd);
    gbm.format = format;
    gbm.surface = NULL;
    
    gbm.surface = gbm_surface_create_with_modifiers(gbm.dev, w, h,
                                                    gbm.format,
                                                    &modifier, 1);
    
    if (!gbm.surface)
    {
        if (modifier != DRM_FORMAT_MOD_LINEAR)
        {
            fprintf(stderr, "Modifiers requested but support isn't available\n");
        }else
        {
            gbm.surface = gbm_surface_create(gbm.dev, w, h,
                                             gbm.format,
                                             GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
        }
        
        
    }
    
    if (!gbm.surface) {
        ofLog(OF_LOG_VERBOSE, "failed to create gbm surface\n");
    }else
    {
        ofLog() << "gbm SURFACE PASS";
    }
    
    gbm.width = w;
    gbm.height = h;
    ofLog() << "gbm.width : " << gbm.width;
    ofLog() << "gbm.height : " << gbm.height;
    return gbm;
}


ofxRPI4Window::ofxRPI4Window() {
    
}
ofxRPI4Window::ofxRPI4Window(const ofGLESWindowSettings & settings) {
    ofLog() << "CTOR CALLED WITH settings";
    setup(settings);
}

void ofxRPI4Window::setup(const ofGLESWindowSettings & settings)
{
 
    
    //ofAppBaseGLESWindow::setup(settings);
    
    ofLog() << "setup ofGLESWindowSettings";
    ofLog() << "DRM_DISPLAY_MODE_LEN: " << DRM_DISPLAY_MODE_LEN;
    char mode_str[DRM_DISPLAY_MODE_LEN] = "";
    
    char *device = NULL;
    uint32_t format = DRM_FORMAT_XRGB8888;
    uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
    
    unsigned int vrefresh = 0;
    auto drm = init_drm_legacy(device, mode_str, vrefresh);
    
    if(drm.mode)
    {
        ofLog() << "setup drm.mode: " << drm.mode;
        if(drm.mode->hdisplay)
        {
            ofLog() << "hdisplay: " << drm.mode->hdisplay;
            
        }else
        {
            ofLog() << "NO hdisplay";
        }
        
        if(drm.mode->vdisplay)
        {
            ofLog() << "vdisplay: " << drm.mode->vdisplay;
            
        }else
        {
            ofLog() << "NO vdisplay";
        }
        
        
    }
    if(drm.mode->hdisplay && drm.mode->vdisplay)
    {
        auto gbm = init_gbm(drm.fd, drm.mode->hdisplay, drm.mode->vdisplay, format, modifier);
        int samples = 0;
        auto egl = init_egl(gbm, samples);
        currentRenderer = make_shared<ofGLProgrammableRenderer>(this);
        eglMakeCurrent(egl.display, egl.surface, egl.surface, egl.context);
        
        auto gl_exts = (char *) glGetString(GL_EXTENSIONS);
        ofLog(OF_LOG_VERBOSE, "OpenGL ES 2.x information:\n");
        ofLog(OF_LOG_VERBOSE, "  version: \"%s\"\n", glGetString(GL_VERSION));
        ofLog(OF_LOG_VERBOSE, "  shading language version: \"%s\"\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
        ofLog(OF_LOG_VERBOSE, "  vendor: \"%s\"\n", glGetString(GL_VENDOR));
        ofLog(OF_LOG_VERBOSE, "  renderer: \"%s\"\n", glGetString(GL_RENDERER));
        ofLog(OF_LOG_VERBOSE, "  extensions: \"%s\"\n", gl_exts);
        ofLog(OF_LOG_VERBOSE, "===================================\n");
        
        //get_proc_gl(GL_OES_EGL_image, glEGLImageTargetTexture2DOES);
    }
    
    
    
}

void ofxRPI4Window::update()
{
    coreEvents.notifyUpdate();
    
}


void ofxRPI4Window::draw()
{
    ofLog() << "draw";
    
    
}

void ofxRPI4Window::pollEvents()
{
    ofLog() << "pollEvents";

}

ofCoreEvents & ofxRPI4Window::events(){
    return coreEvents;
}

ofxRPI4Window::~ofxRPI4Window()
{
    
}

shared_ptr<ofBaseRenderer> & ofxRPI4Window::renderer(){
    return currentRenderer;
}
