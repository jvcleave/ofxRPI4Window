#include "ofApp.h"
#include "ofxRPI4Window.h"

int xPosition = 0;
int yPosition = 0;
int previousX = 0;
int previousY = 0;
bool useMouse = false;

//--------------------------------------------------------------
void ofApp::setup() {
    ofBackground(0);
    ofSetVerticalSync(true);
    
    
    // billboard particles
    
    
    
    if(ofIsGLProgrammableRenderer()){
        shader.load("shaderGL3/Billboard");
    }else{
        shader.load("shaderGL2/Billboard");
    }
    
    ofDisableArbTex();
    texture.load("snow.png");
    
    // we are getting the location of the point size attribute
    // we then set the pointSizes to the vertex attritbute
    // we then bind and then enable
    
}

//--------------------------------------------------------------
void ofApp::update() {
    
    if (ofGetFrameRate() > 30)
    {
        particleSystem.addParticle();
        
        shader.begin();
        // set the vertex data
        vbo.setVertexData(&particleSystem.positions[0], particleSystem.count, GL_DYNAMIC_DRAW);
        int pointAttLoc = shader.getAttributeLocation("pointSize");
        vbo.setAttributeData(pointAttLoc, &particleSystem.pointSizes[0], 1, particleSystem.count, GL_DYNAMIC_DRAW);
        
        // rotate the snow based on the velocity
        int angleLoc = shader.getAttributeLocation("angle");
        vbo.setAttributeData(angleLoc, &particleSystem.rotations[0], 1, particleSystem.count, GL_DYNAMIC_DRAW);
        shader.end();
    }
    
    if(useMouse)
    {
        xPosition = ofGetMouseX();
        yPosition = ofGetMouseY();
        previousX = ofGetPreviousMouseX();
        previousY = ofGetPreviousMouseY();
    }else
    {
        previousX = xPosition;
        previousY = yPosition;
        xPosition = ofGetFrameNum() % ofGetWidth();
        yPosition = ofGetFrameNum() % ofGetHeight();
        
        if(ofGetFrameNum() % 60)
        {
            previousX = (int)ofRandom(ofGetWidth());
        }
        if(ofGetFrameNum() % 90)
        {
            previousY = (int)ofRandom(ofGetHeight());
        }
    }
    
    glm::vec2 mouse(xPosition, yPosition);
    glm::vec2 mouseVec(previousX-xPosition, previousY-yPosition);
    glm::clamp(mouseVec, 0.0f, 10.0f);
    
    
    for (int i=0; i<particleSystem.count; i++)
    {
        ofSeedRandom(i);
        if(glm::distance(mouse, particleSystem.positions[i]) < ofRandom(100, 200)) {
            particleSystem.velocities[i] -= mouseVec;
        }
        
        particleSystem.positions[i] += particleSystem.velocities[i];
        particleSystem.velocities[i] *= 0.84f;
        
        if(particleSystem.positions[i].x < 0) particleSystem.positions[i].x = ofGetWidth();
        if(particleSystem.positions[i].x > ofGetWidth()) particleSystem.positions[i].x = 0;
        if(particleSystem.positions[i].y < 0) particleSystem.positions[i].y = ofGetHeight();
        if(particleSystem.positions[i].y > ofGetHeight()) particleSystem.positions[i].y = 0;
        
        glm::vec2 center(ofGetWidth()/2, ofGetHeight()/2);
        glm::vec2 frc = particleSystem.origins[i] - particleSystem.positions[i];
        if(glm::length(frc) > 20.0) {
            frc = glm::normalize(frc);
            frc *= 0.84;
            particleSystem.velocities[i] += frc;
        }
        
        // get the 2d heading
        float angle = (float)atan2(-particleSystem.velocities[i].y, particleSystem.velocities[i].x) + PI;
        particleSystem.rotations[i] = (angle * -1.0);
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofEnableAlphaBlending();
    ofSetColor(255);
    
    shader.begin();
    ofEnablePointSprites();
    
    
    texture.getTexture().bind();
    vbo.updateVertexData(&particleSystem.positions[0], particleSystem.count);
    
    // rotate the snow based on the velocity
    int angleLoc = shader.getAttributeLocation("angle");
    vbo.updateAttributeData(angleLoc, &particleSystem.rotations[0], particleSystem.count);
    
    vbo.draw(GL_POINTS, 0, particleSystem.count);
    texture.getTexture().unbind();
    
    ofDisablePointSprites();
    shader.end();
    
    ofxRPI4Window* winptr = static_cast<ofxRPI4Window*>(ofGetWindowPtr());
    
    string info = winptr->getInfo();
    info += "\n count: " + ofToString(particleSystem.count);
    
    ofDrawBitmapStringHighlight(info, 20, 20);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if(key == 'f') {
        ofToggleFullscreen();
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
    
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
