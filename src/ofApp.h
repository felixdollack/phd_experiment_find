#pragma once

#include "ofMain.h"
#include "ofx_blinky.h"
#include "ofx_udp_trigger.h"
#include "ofxXmlSettings.h"
#include "ofxGui.h"
#include "vicon_receiver.h"

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
    float _ui_head_radius = 15.0f;
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
    ofxLabel _phone_label, _tracking_label, _presentation_label;
    ofxToggle _toggle_button_eog, _toggle_button_sound;
    ofxButton _push_button_next, _push_button_previous;
    ofxButton _push_button_connect, _push_button_disconnect;
    ofxButton _reset_head_origin;

    // sound source specific settings
    float _source_height, _source_radius, _min_distance, _max_distance;
    vector<ofVec2f> _source_positions;
    vector<Blinky> _source_instance;
    int _current_target;

    // shimmer eog
    UdpTrigger *_eog_trigger;
    string _eog_host;

    // motion capture
    int _mocap_receive_port, _mocap_send_port;
    string _mocap_ip;
    bool _use_vicon;
    ViconReceiver _vicon_receiver;
    HeadPositionAndRotation _head_data;
    float _head_x, _head_y, _head_z, _head_phi;

    // experimental control
    float _x_origin, _y_origin, _z_origin, _phi_origin;
    void resetHeadOrigin();
    void moveToNextTarget();
    void moveToPreviousTarget();
    void toggleRecording(const void *sender, bool &value);
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
