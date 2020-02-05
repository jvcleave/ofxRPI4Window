#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofxRPI4Window* winptr = static_cast<ofxRPI4Window*>(ofGetWindowPtr());
    winptr->skipRender = false;
}

//--------------------------------------------------------------
void ofApp::update(){

}

bool hasPrinted = false;
//--------------------------------------------------------------
void ofApp::draw(){
    
    ofxRPI4Window* winptr = static_cast<ofxRPI4Window*>(ofGetWindowPtr());
    if(winptr->skipRender)
    {
        testCube.draw();
        //ofLog() << "ofGetFrameNum: " << ofGetFrameNum();
        //ofLog() << "ofGetLastFrameTime: " << ofGetLastFrameTime();
    }else
    {
        auto randomColor = ofColor(ofRandom(255), ofRandom(255), ofRandom(255));
        //ofSetColor(randomColor);
        //ofBackground(randomColor);
        //ofBackgroundGradient(randomColor, ofColor::black, OF_GRADIENT_CIRCULAR);
        
        drawGraphicsExample();

        string info = winptr->getInfo();
        
        if (!hasPrinted)
        {
            
            ofLog() << "info: " << info;
            hasPrinted = true;
        }
        ofDrawBitmapStringHighlight(info, 20, 20);
    }
    
   
    

}


void ofApp::drawGraphicsExample()
{
    
    int counter = ofGetFrameNum();

    //--------------------------- circles
    //let's draw a circle:
    ofSetColor(255,130,0);
    float radius = 50 + 10 * sin(counter);
    ofFill();        // draw "filled shapes"
    ofDrawCircle(100,400,radius);
    
    // now just an outline
    ofNoFill();
    ofSetHexColor(0xCCCCCC);
    ofDrawCircle(100,400,80);
    
    // use the bitMap type
    // note, this can be slow on some graphics cards
    // because it is using glDrawPixels which varies in
    // speed from system to system.  try using ofTrueTypeFont
    // if this bitMap type slows you down.
    ofSetHexColor(0x000000);
    ofDrawBitmapString("circle", 75,500);
    
    
    //--------------------------- rectangles
    ofFill();
    for (int i = 0; i < 200; i++){
        ofSetColor((int)ofRandom(0,255),(int)ofRandom(0,255),(int)ofRandom(0,255));
        ofDrawRectangle(ofRandom(250,350),ofRandom(350,450),ofRandom(10,20),ofRandom(10,20));
    }
    ofSetHexColor(0x000000);
    ofDrawBitmapString("rectangles", 275,500);
    
    //---------------------------  transparency
    ofSetHexColor(0x00FF33);
    ofDrawRectangle(400,350,100,100);
    // alpha is usually turned off - for speed puposes.  let's turn it on!
    ofEnableAlphaBlending();
    ofSetColor(255,0,0,127);   // red, 50% transparent
    ofDrawRectangle(450,430,100,33);
    ofSetColor(255,0,0,(int)(counter * 10.0f) % 255);   // red, variable transparent
    ofDrawRectangle(450,370,100,33);
    ofDisableAlphaBlending();
    
    ofSetHexColor(0x000000);
    ofDrawBitmapString("transparency", 410,500);
    
    //---------------------------  lines
    // a bunch of red lines, make them smooth if the flag is set
    
    ofSetHexColor(0xFF0000);
    for (int i = 0; i < 20; i++){
        ofDrawLine(600,300 + (i*5),800, 250 + (i*10));
    }
    
    ofSetHexColor(0x000000);
    ofDrawBitmapString("lines\npress 's' to toggle smoothness", 600,500);
}




//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
