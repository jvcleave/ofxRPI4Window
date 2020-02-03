#include "ofxRPI4Window.h"

#define __func__ __PRETTY_FUNCTION__



#define MAX_DRM_DEVICES 64

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


int refresh_rate(drmModeModeInfo *mode)
{
    int res = (mode->clock * 1000000LL / mode->htotal + mode->vtotal / 2) / mode->vtotal;
    
    if (mode->flags & DRM_MODE_FLAG_INTERLACE)
        res *= 2;
    
    if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
        res /= 2;
    
    if (mode->vscan > 1)
        res /= mode->vscan;
    
    return res;
}

bool has_ext(const char *extension_list, const char *ext)
{
    
    //ofLog() << "extension_list: " << extension_list;
    //ofLog() << "ext: " << ext;
    
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

bool egl_choose_config(EGLDisplay egl_display, const EGLint *attribs,
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

int get_resources(int fd, drmModeRes **resources)
{
    *resources = drmModeGetResources(fd);
    if (*resources == NULL)
        return -1;
    return 0;
}


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

static void
page_flip_handler(int fd, unsigned int frame,
                  unsigned int sec, unsigned int usec, void *data)
{
    /* suppress 'unused parameter' warnings */
    (void)fd, (void)frame, (void)sec, (void)usec;
    //ofLog() << __func__;
    
    int *waiting_for_flip = (int*)data;
    *waiting_for_flip = 0;
}

static void
drm_fb_destroy_callback(struct gbm_bo *bo, void *data)
{
    
    ofLog() << __func__;
    
    int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
    struct drm_fb *fb = (drm_fb*)data;
    
    if (fb->fb_id)
        drmModeRmFB(drm_fd, fb->fb_id);
    
    free(fb);
}




void ofxRPI4Window::init_egl(int samples)
{
    
    EGLint major, minor;
    
   
    
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
    const char *egl_exts_client = NULL;
    const char *egl_exts_dpy = NULL;
    const char *gl_exts = NULL;

    egl_exts_client = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    
    egl.eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    egl.eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    egl.eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    egl.glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    egl.eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC)eglGetProcAddress("eglCreateSyncKHR");
    egl.eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC)eglGetProcAddress("eglDestroySyncKHR");
    egl.eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC)eglGetProcAddress("eglClientWaitSyncKHR");

    ofLog() << "egl.eglClientWaitSyncKHR: " << egl.eglClientWaitSyncKHR;
    ofLog() << "egl.eglCreateImageKHR: " << egl.eglCreateImageKHR;

    egl.display = egl.eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, gbm.dev, NULL);
    
    if (!eglInitialize(egl.display, &egl.eglVersionMajor, &egl.eglVersionMinor)) {
        ofLog(OF_LOG_VERBOSE, "failed to initialize\n");
    }
    
    egl_exts_dpy = eglQueryString(egl.display, EGL_EXTENSIONS);
    egl.modifiers_supported = has_ext(egl_exts_dpy,
                                      "EGL_EXT_image_dma_buf_import_modifiers");
    
    
    


    ofLog() << "egl.modifiers_supported: " << egl.modifiers_supported;
    
    ofLog(OF_LOG_VERBOSE, "Using display %p with EGL version %d.%d\n", egl.display, egl.eglVersionMajor, egl.eglVersionMinor);
    
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
    
    if (!egl_choose_config(egl.display, config_attribs, gbm.format, &egl.config))
    {
        ofLog(OF_LOG_VERBOSE, "failed to choose config\n");
    }else
    {
        ofLog() << "egl_choose_config PASS";
    }
    

    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    
    egl.context = eglCreateContext(egl.display,
                                   egl.config,
                                   EGL_NO_CONTEXT,
                                   context_attribs);
    
    
    
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
        
}





ofxRPI4Window::ofxRPI4Window() {
    orientation = OF_ORIENTATION_DEFAULT;
    bufferObject1 = NULL;
    bufferObject2 = NULL;
    skipRender = false;
}
ofxRPI4Window::ofxRPI4Window(const ofGLESWindowSettings & settings) {
    ofLog() << "CTOR CALLED WITH settings";
    setup(settings);
}


void ofxRPI4Window::setup(const ofGLESWindowSettings & settings)
{
 
    bEnableSetupScreen = true;
    windowMode = OF_WINDOW;
    glesVersion = settings.glesVersion;

    //ofAppBaseGLESWindow::setup(settings);
    
    ofLog() << "setup ofGLESWindowSettings";
    ofLog() << "DRM_DISPLAY_MODE_LEN: " << DRM_DISPLAY_MODE_LEN;
    char mode_str[DRM_DISPLAY_MODE_LEN] = "";
    
    //char *device = "/dev/dri/card1";
    char *device = NULL;

    uint32_t format = DRM_FORMAT_XRGB8888;
    uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
    
    unsigned int vrefresh = 0;
    init_drm(device, mode_str, vrefresh);
    ofLog() << "device: " << device;
    ofLog() << "vrefresh: " << vrefresh;

    //bool canAtomic = (drmSetClientCap(drm.fd, DRM_CLIENT_CAP_ATOMIC, 1) == 0);
    //ofLog() << "canAtomic:" << canAtomic;

    
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
        init_gbm(drm.fd, drm.mode->hdisplay, drm.mode->vdisplay, format, modifier);
        int samples = 0;
        init_egl(samples);
        
    
        
      
        currentRenderer = make_shared<ofGLProgrammableRenderer>(this);
        makeCurrent();
        static_cast<ofGLProgrammableRenderer*>(currentRenderer.get())->setup(2,0);

        evctx = {
            .version = 2,
            .page_flip_handler = page_flip_handler,
        };
        
        swapBuffers();
        
        drm_fb_get_from_bo(bufferObject1);

        glClearColor(255.0f,
                     255.0f,
                     255.0f,
                     255.0f);
        glClear( GL_COLOR_BUFFER_BIT );
        glClear( GL_DEPTH_BUFFER_BIT );
        
#if 1
        auto gl_exts = (char *) glGetString(GL_EXTENSIONS);
        ofLog(OF_LOG_VERBOSE, "GL INFO");
        ofLog(OF_LOG_VERBOSE, "  version: \"%s\"", glGetString(GL_VERSION));
        ofLog(OF_LOG_VERBOSE, "  shading language version: \"%s\"", glGetString(GL_SHADING_LANGUAGE_VERSION));
        ofLog(OF_LOG_VERBOSE, "  vendor: \"%s\"", glGetString(GL_VENDOR));
        ofLog(OF_LOG_VERBOSE, "  renderer: \"%s\"", glGetString(GL_RENDERER));
        ofLog(OF_LOG_VERBOSE, "  extensions: \"%s\"", gl_exts);
        ofLog(OF_LOG_VERBOSE, "===================================\n");
        //get_proc_gl(GL_OES_EGL_image, glEGLImageTargetTexture2DOES);
        
        ofLog() << "-----EGL-----";
        //ofLog() << "EGL_VERSION_MAJOR = " << eglVersionMajor;
        //ofLog() << "EGL_VERSION_MINOR = " << eglVersionMinor;
        ofLog() << "EGL_CLIENT_APIS = " << eglQueryString(getEGLDisplay(), EGL_CLIENT_APIS);
        ofLog() << "EGL_VENDOR = "  << eglQueryString(getEGLDisplay(), EGL_VENDOR);
        ofLog() << "EGL_VERSION = " << eglQueryString(getEGLDisplay(), EGL_VERSION);
        ofLog() << "EGL_EXTENSIONS = " << eglQueryString(getEGLDisplay(), EGL_EXTENSIONS);
        ofLog() << "GL_RENDERER = " << glGetString(GL_RENDERER);
        ofLog() << "GL_VERSION  = " << glGetString(GL_VERSION);
        ofLog() << "GL_VENDOR   = " << glGetString(GL_VENDOR);
        ofLog() << "-------------";
#endif
    }
    
    
    
}

void ofxRPI4Window::init_drm(const char *device, const char *mode_str, unsigned int vrefresh)
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
        ofLog(OF_LOG_ERROR, "could not open drm device\n");
    }else
    {
        ofLog() << "drm.fd: " << drm.fd;
    }
    
    if (!resources) {
        ofLog(OF_LOG_ERROR, "drmModeGetResources failed: %s\n", strerror(errno));
    }
    
    /* find a connected connector: */
    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) {
            /* it's connected, let's use this! */
            ofLog() << "FOUND CONNECTION";
            ofLog() << "connector: " << resources->connectors[i];

            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }
    
    if (!connector) {
        /* we could be fancy and listen for hotplug events and wait for
         * a connector..
         */
        ofLog(OF_LOG_ERROR, "no connected connector!\n");
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
        ofLog() << "FOUND encoder: " << encoder;

        drm.crtc_id = encoder->crtc_id;
    } else {
        uint32_t crtc_id = find_crtc_for_connector(drm, resources, connector);
        if (crtc_id == 0) {
            ofLog(OF_LOG_VERBOSE, "no crtc found!\n");
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
    
}



void ofxRPI4Window::init_gbm(int drm_fd, int w, int h, uint32_t format, uint64_t modifier)
{
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
    
    currentWindowRect = ofRectangle(0, 0, gbm.width, gbm.height);
    ofLog() << "currentWindowRect: " << currentWindowRect;
    
}

drm_fb* ofxRPI4Window::drm_fb_get_from_bo(gbm_bo* bo)
{
    
    //ofLog() << __func__;
    if(bo == NULL)
    {
        bo = gbm_surface_lock_front_buffer(gbm.surface);

    }
    
    int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
    drm_fb* fb = (drm_fb*)gbm_bo_get_user_data(bo);
    uint32_t width, height, format,
    strides[4] = {0}, handles[4] = {0},
    offsets[4] = {0}, flags = 0;
    int ret = -1;
    
    if (!fb)
    {
        ofLog() << "fb IS NULL";
        fb = (drm_fb*)calloc(1, sizeof *fb);
        fb->bo = bo;
        
        width = gbm_bo_get_width(bo);
        height = gbm_bo_get_height(bo);
        format = gbm_bo_get_format(bo);
        ofLog() << "width: " << width;
        ofLog() << "height: " << height;
        ofLog() << "format: " << format;
        
        if (gbm_bo_get_modifier && gbm_bo_get_plane_count &&
            gbm_bo_get_stride_for_plane && gbm_bo_get_offset) {
            
            uint64_t modifiers[4] = {0};
            modifiers[0] = gbm_bo_get_modifier(bo);
            const int num_planes = gbm_bo_get_plane_count(bo);
            for (int i = 0; i < num_planes; i++) {
                strides[i] = gbm_bo_get_stride_for_plane(bo, i);
                handles[i] = gbm_bo_get_handle(bo).u32;
                offsets[i] = gbm_bo_get_offset(bo, i);
                modifiers[i] = modifiers[0];
            }
            
            if (modifiers[0]) {
                flags = DRM_MODE_FB_MODIFIERS;
                printf("Using modifier %" PRIx64 "\n", modifiers[0]);
            }
            
            ret = drmModeAddFB2WithModifiers(drm_fd, width, height,
                                             format, handles, strides, offsets,
                                             modifiers, &fb->fb_id, flags);
            
            ofLog() << "drmModeAddFB2WithModifiers: " << ret;
            
        }
    }else
    {
        //ofLog() << "FB IS VALID";
    }
    
    
    if(fb)
    {
        auto result = drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0, &drm.connector_id, 1, drm.mode);
        //ofLog() << "drmModeSetCrtc result: " << result;
    }
    
    
    
    
    gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);
    return fb;
}






void ofxRPI4Window::update()
{
    //ofLog() << "update";
    coreEvents.notifyUpdate();
    
}

void ofxRPI4Window::draw()
{
    
    int waiting_for_flip = 1;
    
    if(skipRender)
    {
        
        coreEvents.notifyDraw();
        
        swapBuffers();
        
    }else
    {
        currentRenderer->startRender();
        if( bEnableSetupScreen )
        {
            //currentRenderer->setupScreen();
            //bEnableSetupScreen = false;
        }
        coreEvents.notifyDraw();


        currentRenderer->finishRender();
        swapBuffers();

    }
    
    
    
    
    bufferObject2 = gbm_surface_lock_front_buffer(gbm.surface);
    
    //ofLog () << "next_bo: " << next_bo;
    drm_fb* fb = drm_fb_get_from_bo(bufferObject2);
    if (!fb) {
        fprintf(stderr, "Failed to get a new framebuffer BO\n");
    }
    
    /*
     * Here you could also update drm plane layers if you want
     * hw composition
     */
    
    int ret = drmModePageFlip(drm.fd, drm.crtc_id, fb->fb_id,
                              DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
    if (ret) {
        printf("failed to queue page flip: %s\n", strerror(errno));
    }
    
    while (waiting_for_flip) {
        //ofLog() << "waiting_for_flip";
        
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(drm.fd, &fds);
        
        ret = select(drm.fd + 1, &fds, NULL, NULL, NULL);
        if (ret < 0) {
            printf("select err: %s\n", strerror(errno));
        } else if (ret == 0) {
            printf("select timeout!\n");
        } else if (FD_ISSET(0, &fds)) {
            printf("user interrupted!\n");
        }
        drmHandleEvent(drm.fd, &evctx);
    }
    
    /* release last buffer to render on again: */
    gbm_surface_release_buffer(gbm.surface, bufferObject1);
    bufferObject1 = bufferObject2;
    
}

void ofxRPI4Window::drawAtomic()
{

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

void ofxRPI4Window::makeCurrent()
{
    eglMakeCurrent(egl.display, egl.surface, egl.surface, egl.context);
    
}

void ofxRPI4Window::swapBuffers()
{
   // ofLog() << __func__;
    
    EGLBoolean success = eglSwapBuffers(egl.display, egl.surface);
    if(!success) {
        GLint error = eglGetError();
        ofLog() << "eglSwapBuffers failed: " << eglErrorString(error);
    }
    
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

void ofxRPI4Window::setVerticalSync(bool enabled)
{
    eglSwapInterval(egl.display, enabled ? 1 : 0);
}

void ofxRPI4Window::enableSetupScreen()
{
    bEnableSetupScreen = true;
}

void ofxRPI4Window::disableSetupScreen()
{
    bEnableSetupScreen = false;
}

EGLDisplay ofxRPI4Window::getEGLDisplay()
{
    //ofLog() << __func__;
    return egl.display;
}

EGLContext ofxRPI4Window::getEGLContext()
{
    //ofLog() << __func__;

    return egl.context;
}

EGLSurface ofxRPI4Window::getEGLSurface()
{
    //ofLog() << __func__;

    return egl.surface;
}

shared_ptr<ofBaseRenderer> & ofxRPI4Window::renderer(){
    
    //ofLog() << __func__;
    
    return currentRenderer;
}

ofxRPI4Window::~ofxRPI4Window()
{
    
}


