#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void windowResized(int w, int h);
		void gotMessage(ofMessage msg);
		
private:
    ofxXmlSettings *_settings;
    const string _settings_filename = "settings.xml";
    void loadSettingsAndWriteDefaultIfNeeded();
    void writeDefaultSettings();

    // sound source specific settings
    float _source_height, _source_radius;
    vector<ofVec2f> _source_positions;
};
