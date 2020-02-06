#pragma once
#include "ofMain.h"
#include "ofVbo.h"



class ParticleSystem
{
public:
    
    
    vector<glm::vec2> positions;
    vector<glm::vec2> velocities;
    vector<glm::vec2> origins;
    
    vector<float> pointSizes;
    vector<float> rotations;
    
    int count;
    ParticleSystem()
    {
        
    }
    void addParticle()
    {
        count++;
        glm::vec2 pos;
        glm::vec2 vel;
        glm::vec2 home;
        float pointSize;
        float rotation;
        
        pos.x = ofRandomWidth();
        pos.y = ofRandomHeight();
        vel.x = ofRandomf();
        vel.y = ofRandomf();
        home = pos;
        pointSize = ofRandom(2, 40);
        rotation = ofRandom(0, 360);
        
        
        positions.push_back(pos);
        pointSizes.push_back(pointSize);
        rotations.push_back(rotation);
        velocities.push_back(vel);
        origins.push_back(home);
    }
    
    
};

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    ofVbo vbo;
    ofShader shader;
    ofImage texture;
    
    ParticleSystem particleSystem;
};











