#pragma once

#include "ofMain.h"
#include "ofx_blinky.h"
#include "ofx_udp_trigger.h"
#include "ofxXmlSettings.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{

	public:
		void exit();
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
    static ofVec2f convertPolarToCartesian(ofVec2f polar) {
        float r   = polar.x;
        float phi = polar.y - 90; // shift so 0 is up on the screen
        return ofVec2f(r * cos(phi/180*PI), r * sin(phi/180*PI));
    }
    ofxPanel _uiPanel;
    ofxToggle _toggle_button_sound;
    ofxButton _push_button_next, _push_button_previous;
    ofxButton _push_button_connect, _push_button_disconnect;

    // sound source specific settings
    float _source_height, _source_radius, _min_distance, _max_distance;
    vector<ofVec2f> _source_positions;
    vector<Blinky> _source_instance;
    int _current_target;

    // shimmer eog
    UdpTrigger *_eog_trigger;
    string _eog_host;

    // experimental control
    void moveToNextTarget();
    void moveToPreviousTarget();
    void toggleSound(const void *sender, bool &value);

    // network
    string _my_ip = "";
    string getIPhost();
    vector<string> getLocalIPs();
    ofxTCPServer* _android_tcp_server;
    int _android_port;
    string _client_ip;
    void connectPhone();
    void disconnectPhone();
    void sendMessageToPhone(int client, string message);
};
