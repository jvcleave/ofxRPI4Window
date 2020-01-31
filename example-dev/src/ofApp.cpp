#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    image.load("test.jpg");
    image.update();
}

//--------------------------------------------------------------
void ofApp::update(){

}

bool hasPrinted = false;
//--------------------------------------------------------------
void ofApp::draw(){
    
    auto randomColor = ofColor(ofRandom(255), ofRandom(255), ofRandom(255));
    ofSetColor(randomColor);
    ofBackground(randomColor);
    
    for (size_t i=0; i<100; i++)
    {
        ofDrawCircle(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()), 100);
    }
    
    if (!hasPrinted)
    {
        stringstream info;
        
        info << "ofGetWidth(): " << ofGetWidth();
        info << " ofGetHeight(): " << ofGetHeight();
        ofLog() << "info: " << info.str();
        hasPrinted = true;
    }
    //image.draw(0, 0, 100, 100);
    
    //ofLog() << "draw";
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
