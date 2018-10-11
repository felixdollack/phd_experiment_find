#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    this->_min_distance = 0.5f;
    this->_max_distance = 2.5f;

    ofSetCircleResolution(100);
    this->_ui_world_diameter = ofGetWindowHeight();
    this->_ui_center = ofVec2f(ofGetWindowWidth(), ofGetWindowHeight()) - this->_ui_world_diameter/2;
    this->_ui_world_start = this->_ui_center  - this->_ui_world_diameter/2;
    this->_ui_min_distance = (this->_ui_world_diameter/2)*0.30;
    this->_ui_max_distance = (this->_ui_world_diameter/2)*0.90;

    loadSettingsAndWriteDefaultIfNeeded();
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
    ofClear(0);

    ofSetColor(ofColor::white);
    ofDrawRectangle(this->_ui_world_start.x, this->_ui_world_start.y, this->_ui_world_diameter, this->_ui_world_diameter);
    ofPushMatrix();
    {
        ofTranslate(this->_ui_center);
        ofSetColor(ofColor::red);
        ofDrawCircle(0, 0, this->_ui_max_distance + this->_line_width);
        ofSetColor(ofColor::white);
        ofDrawCircle(0, 0, this->_ui_max_distance);
        ofSetColor(ofColor::blue);
        ofDrawCircle(0, 0, this->_ui_min_distance + this->_line_width);
        ofSetColor(ofColor::white);
        ofDrawCircle(0, 0, this->_ui_min_distance);
    }
    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

ofVec2f ofApp::mapDistanceToPixel(ofVec2f pos) {
    pos.x -= this->_min_distance;
    pos.x /= (this->_max_distance - this->_min_distance);
    pos.x *= (this->_ui_max_distance - this->_ui_min_distance);
    pos.x += this->_ui_min_distance;
    return pos;
}

void ofApp::loadSettingsAndWriteDefaultIfNeeded() {
    this->_settings = new ofxXmlSettings();
    if (this->_settings->loadFile(this->_settings_filename) == false) {
        writeDefaultSettings();
        this->_settings->loadFile(this->_settings_filename);
    }
    this->_settings->pushTag("settings");
    {
        this->_settings->pushTag("subject");
        {
            this->_source_height = this->_settings->getValue("ear_height", 175.0f);
            this->_settings->getValue("ui_radius", 2.0f);
        }
        this->_settings->popTag();
        this->_source_radius = this->_settings->getValue("ui_radius", 1.0f);
        int number_of_positions = this->_settings->getValue("number_of_positions", 0);
        this->_settings->pushTag("positions");
        {
            for (int i=0; i < number_of_positions; i++) {
                this->_settings->pushTag("position", i);
                {
                    float phi = this->_settings->getValue("phi", 0);  // 0 - 359.9 degrees
                    float r   = this->_settings->getValue("r", 0.0f); // 0.5 - 3.5 meters
                    this->_source_positions.push_back(ofVec2f(r, phi));
                }
                this->_settings->popTag();
            }
        }
        this->_settings->popTag();
    }
    this->_settings->popTag();
}

void ofApp::writeDefaultSettings() {
    this->_settings->addTag("settings");
    this->_settings->pushTag("settings");
    {
        this->_settings->addTag("subject");
        this->_settings->pushTag("subject");
        {
            this->_settings->addValue("ear_height", 175.0f);
            this->_settings->addValue("ui_radius", 15.0f);
        }
        this->_settings->popTag();
        this->_settings->addValue("ui_radius", 10.0f);
        this->_settings->addValue("number_of_positions", 1);
        this->_settings->addTag("positions");
        this->_settings->pushTag("positions");
        {
            this->_settings->addTag("position");
            this->_settings->pushTag("position");
            {
                this->_settings->addValue("phi", 0);
                this->_settings->addValue("r", 0.5f);
            }
            this->_settings->popTag();
        }
        this->_settings->popTag();
    }
    this->_settings->popTag();
    this->_settings->saveFile();
}
