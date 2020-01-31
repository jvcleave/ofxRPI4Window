#include "ofMain.h"
#include "ofApp.h"
#include "ofxRPI4Window.h"

//========================================================================
int main( ){
    
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    ofGLESWindowSettings settings;
    settings.setSize(1280, 720);
    settings.setGLESVersion(2);
    auto window = std::make_shared<ofxRPI4Window>(settings);
    
    auto app = std::make_shared<ofApp>();
    ofRunApp(window, app);
    ofRunMainLoop();
}
