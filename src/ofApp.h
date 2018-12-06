#pragma once

#include "ofMain.h"
#include "ofx_blinky.h"
#include "ofx_udp_trigger.h"
#include "ofxXmlSettings.h"
#include "ofxGui.h"
#include "vicon_receiver.h"
#include <ctime>
#include "ofxOsc.h"

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
    ofxXmlSettings *_settings, *_position_settings;
    const string _settings_filename = "settings.xml";
    string _position_settings_filename;
    void loadSettingsAndWriteDefaultIfNeeded();
    void loadPositions();
    void writeDefaultSettings();
    void writeDefaultPositionSettings();

    // ui settings
    float _line_width = 6.0f;
    float _ui_head_radius;
    float _ui_world_diameter;
    ofVec2f _ui_center, _ui_world_start;
    float _ui_min_distance, _ui_max_distance;
    ofVec2f mapDistanceToPixel(ofVec2f pos);
    ofVec2f mapPositionToPixel(ofVec2f pos);
    static ofVec2f convertPolarToCartesian(ofVec2f polar) {
        return convertPolarToCartesian(polar, 0);
    }
    static ofVec2f convertPolarToCartesian(ofVec2f polar, float piOffset) {
        float r   = polar.x;
        float phi = polar.y + piOffset;
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
    ofVec2f _current_source_position;
    bool _sound_on;

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
    float _old_head_x, _old_head_y, _old_head_z, _old_head_phi;

    // experimental control
    float _x_origin, _y_origin, _phi_origin;
    void resetHeadOrigin();
    void moveToNextTarget();
    void moveToPreviousTarget();
    void toggleRecording(const void *sender, bool &value);
    void toggleSound(const void *sender, bool &value);

    // network
    string _my_ip = "";
    string getIPhost();
    vector<string> getLocalIPs();
    //ofxTCPServer* _android_tcp_server;
    int _android_port;
    string _client_ip;
    //void connectPhone();
    void disconnectPhone();
    void sendMessageToPhone(int client, string message);
    float _time;
    //int _incoming_message_len;
    ofxOscSender *_ssr_osc;
    void connectToSSR(bool value);
    void loadSsrScene();
    void streamSSR(bool value);
    void updateSoundPos(float x, float y);
    void updatePos(float x, float y);
    void updateAngle(float phi);
    bool _ssr_running;

    // data logging
    string nowToString();
    bool _isLogFileCreated;
    float _logStartTime;
    string _username;
};
