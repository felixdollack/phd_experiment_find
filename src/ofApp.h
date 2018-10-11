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

    // ui settings
    float _line_width = 6.0f;
    float _ui_world_diameter;
    ofVec2f _ui_center, _ui_world_start;
    float _ui_min_distance, _ui_max_distance;
    ofVec2f mapDistanceToPixel(ofVec2f pos);

    // sound source specific settings
    float _source_height, _source_radius;
    vector<ofVec2f> _source_positions;
};
