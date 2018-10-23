#include "ofApp.h"

void ofApp::exit(){
    this->_eog_trigger->stopRecording();
    disconnectPhone();
    if (this->_android_tcp_server->isConnected()) {
        this->_android_tcp_server->close();
    }
    this->_vicon_receiver.stop();
}

//--------------------------------------------------------------
void ofApp::setup(){
    this->_my_ip = getIPhost();
    this->_client_ip = "000.000.000.000";

    // add gui
    this->_uiPanel.setup();
    this->_uiPanel.add(this->_phone_label.setup("PHONE",""));
    this->_uiPanel.add(this->_push_button_connect.setup("connect to phone"));
    this->_uiPanel.add(this->_push_button_disconnect.setup("disconnect from phone"));

    this->_uiPanel.add(this->_tracking_label.setup("TRACKING",""));
    this->_uiPanel.add(this->_reset_head_origin.setup("reset head origin"));
    this->_uiPanel.add(this->_toggle_button_eog.setup("record eog", false));

    this->_uiPanel.add(this->_presentation_label.setup("PRESENTATION",""));
    this->_uiPanel.add(this->_toggle_button_sound.setup("sound on", false));
    this->_uiPanel.add(this->_push_button_next.setup("next target"));
    this->_uiPanel.add(this->_push_button_previous.setup("previous target"));

    // set gui text and box colors
    this->_push_button_connect.setTextColor(ofColor::red);
    this->_push_button_disconnect.setFillColor(ofColor::black);
    this->_push_button_disconnect.setTextColor(ofColor::black);
    this->_toggle_button_eog.setTextColor(ofColor::red);
    this->_toggle_button_sound.setTextColor(ofColor::red);

    // add gui listeners
    this->_push_button_connect.addListener(this, &ofApp::connectPhone);
    this->_reset_head_origin.addListener(this, &ofApp::resetHeadOrigin);
    this->_toggle_button_eog.addListener(this, &ofApp::toggleRecording);
    this->_toggle_button_sound.addListener(this, &ofApp::toggleSound);
    this->_push_button_next.addListener(this, &ofApp::moveToNextTarget);
    this->_push_button_previous.addListener(this, &ofApp::moveToPreviousTarget);

    this->_min_distance = 0.5f;
    this->_max_distance = 2.5f;

    // reset head origin
    this->_x_origin = 0;
    this->_y_origin = 0;
    this->_z_origin = 0;
    this->_phi_origin = 0;
    this->_head_x = 0;
    this->_head_y = 0;
    this->_head_z = 0;
    this->_head_phi = 0;
    this->_old_head_x = 0;
    this->_old_head_y = 0;
    this->_old_head_z = 0;
    this->_old_head_phi = 0;

    // draw live feedback background
    ofSetCircleResolution(100);
    this->_ui_world_diameter = ofGetWindowHeight();
    this->_ui_center = ofVec2f(ofGetWindowWidth(), ofGetWindowHeight()) - this->_ui_world_diameter/2;
    this->_ui_world_start = this->_ui_center  - this->_ui_world_diameter/2;
    this->_ui_min_distance = (this->_ui_world_diameter/2)*0.30;
    this->_ui_max_distance = (this->_ui_world_diameter/2)*0.90;

    loadSettingsAndWriteDefaultIfNeeded();

    // create sound positions to be tested
    for (int i=0; i < this->_source_positions.size(); i++) {
        this->_source_instance.push_back(*new Blinky(this->_source_radius));
        ofVec2f pos = mapDistanceToPixel(this->_source_positions[i]);
        ofVec2f cartesian = convertPolarToCartesian(pos);
        this->_source_instance[i].setPosition(cartesian);
    }
    this->_current_target = 0;

    // create and connect to shimmer udp port
    this->_eog_trigger = new UdpTrigger(this->_eog_host);
    this->_eog_trigger->connectToHost();

    // create tcp server to connect to android
    if (this->_android_tcp_server == NULL) {
        this->_android_tcp_server = new ofxTCPServer();
        this->_android_tcp_server->setMessageDelimiter("");
    }

    // setup vicon receiver
    ofxUDPSettings settings;
    settings.receiveOn(this->_mocap_receive_port);
    settings.blocking = false;
    this->_vicon_receiver.setup(settings);
}

//--------------------------------------------------------------
void ofApp::update(){
    if (this->_my_ip == "") {
        this->_my_ip = getIPhost();
    }
    if (this->_client_ip == "000.000.000.000") {
        if (this->_android_tcp_server->getNumClients() > 0) {
            this->_client_ip = this->_android_tcp_server->getClientIP(0);
            this->_push_button_connect.setTextColor(ofColor::green);
        } else {
            this->_push_button_connect.setTextColor(ofColor::red);
        }
    }
    for (int i=0; i < this->_source_positions.size(); i++) {
        if (i == this->_current_target) {
            if (!this->_source_instance[i].isBlinking()) {
                this->_source_instance[i].setBlinking(true);
            }
        } else {
            this->_source_instance[i].setBlinking(false);
        }
    }
    for (int i=0; i < this->_source_positions.size(); i++) {
        _source_instance[i].update();
    }
    this->_vicon_receiver.updateData();
    this->_head_data = this->_vicon_receiver.getLatestData();
    // keep a copy important for android and substract origin (x,y,z,phi)
    this->_old_head_x = this->_head_x;
    this->_old_head_y = this->_head_y;
    this->_old_head_z = this->_head_z;
    this->_old_head_phi = this->_head_phi;
    this->_head_x = (this->_head_data.x_position/1000) - this->_x_origin;
    this->_head_y = (this->_head_data.y_position/1000) - this->_y_origin;
    this->_head_z = (this->_head_data.z_position/1000) - this->_z_origin;
    this->_head_phi = fmod(this->_head_data.z_rot_avg - this->_phi_origin, 360.0f);

    if ((this->_old_head_x != this->_head_x) || (this->_old_head_y != this->_head_y) || (this->_old_head_z != this->_head_z) || (this->_old_head_phi != this->_head_phi)) {
        sendMessageToPhone(0, "POSITION/" + ofToString(this->_head_x) + "/" + ofToString(this->_head_y) + "/" + ofToString(this->_head_z) + "/" + ofToString(this->_head_phi));
    }
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

        // draw head
        ofPushMatrix();
        {
            ofVec2f pos = ofVec2f(-this->_head_y, -this->_head_x); // flip and invert axis for show only axes
            pos = mapPositionToPixel(pos);
            ofSetColor(ofColor::orange);
            ofTranslate(pos);
            ofDrawCircle(0, 0, this->_ui_head_radius);
            ofRotateDeg(180 - this->_head_phi);
            ofDrawCircle(0, this->_ui_head_radius, 3);
        }
        ofPopMatrix();

        for (int i=0; i < this->_source_positions.size(); i++) {
            _source_instance[i].draw();
        }
    }
    ofPopMatrix();

    ofSetColor(ofColor::white); // 55, 40, 25, 10
    ofDrawBitmapString("origin (x/y): " + ofToString(this->_x_origin) + "/" + ofToString(this->_y_origin), 10, ofGetWindowHeight()-100);
    ofDrawBitmapString("origin (phi): " + ofToString(this->_phi_origin), 10, ofGetWindowHeight()-85);
    ofDrawBitmapString("Target: " + ofToString(this->_current_target+1) + "/" + ofToString(this->_source_positions.size()), 10, ofGetWindowHeight()-70);
    ofDrawBitmapString("r[m]: " + ofToString(this->_source_positions[this->_current_target].x) + " phi[deg]: " + ofToString(this->_source_positions[this->_current_target].y), 10, ofGetWindowHeight()-55);

    ofDrawBitmapString("IP: " + this->_my_ip, 10, ofGetWindowHeight()-25);
    ofDrawBitmapString("connected to: " + this->_client_ip, 10, ofGetWindowHeight()-10);

    ofSetColor(ofColor::black);
    ofDrawBitmapString("/- window -\\", this->_ui_center.x - 45, 15);
    ofDrawBitmapString("door", ofGetWindowWidth() - 30, this->_ui_center.y);

    this->_uiPanel.draw();
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

void ofApp::moveToNextTarget() {
    if ((this->_current_target+1) < this->_source_instance.size()) {
        this->_current_target++;
    } else {
        this->_current_target = 0;
    }
}

void ofApp::moveToPreviousTarget() {
    if ((this->_current_target-1) >= 0) {
        this->_current_target--;
    } else {
        this->_current_target = this->_source_instance.size()-1;
    }
}

void ofApp::toggleRecording(const void *sender, bool &value) {
    if (value == true) {
        this->_toggle_button_eog.setTextColor(ofColor::green);
        this->_eog_trigger->startRecording();
    } else {
        this->_eog_trigger->stopRecording();
        this->_toggle_button_eog.setTextColor(ofColor::red);
    }
}

void ofApp::toggleSound(const void *sender, bool &value) {
    if (value == true) {
        // sound source position
        ofVec2f pos = convertPolarToCartesian(this->_source_positions[this->_current_target]);
        sendMessageToPhone(0, "SRCPOS/" + ofToString(pos.x) + "/" + ofToString(pos.y));
        // update ui
        this->_push_button_next.removeListener(this, &ofApp::moveToNextTarget);
        this->_push_button_previous.removeListener(this, &ofApp::moveToPreviousTarget);
        this->_push_button_next.setFillColor(ofColor::black);
        this->_push_button_previous.setFillColor(ofColor::black);
        this->_push_button_next.setTextColor(ofColor::black);
        this->_push_button_previous.setTextColor(ofColor::black);
        this->_toggle_button_sound.setTextColor(ofColor::green);
        // send sound and eog-trigger message
        sendMessageToPhone(0, "PLAY/");
        this->_eog_trigger->sendTrigger("sound_on");
    } else {
        sendMessageToPhone(0, "STOP/");
        this->_eog_trigger->sendTrigger("sound_off");
        this->_toggle_button_sound.setTextColor(ofColor::red);
        this->_push_button_next.setTextColor(ofColor::white);
        this->_push_button_previous.setTextColor(ofColor::white);
        this->_push_button_next.setFillColor(ofColor::gray);
        this->_push_button_previous.setFillColor(ofColor(128));
        this->_push_button_next.addListener(this, &ofApp::moveToNextTarget);
        this->_push_button_previous.addListener(this, &ofApp::moveToPreviousTarget);
    }
}

void ofApp::resetHeadOrigin() {
    this->_x_origin = this->_head_data.x_position/1000;
    this->_y_origin = this->_head_data.y_position/1000;
    this->_z_origin = this->_head_data.z_position/1000;
    this->_phi_origin = this->_head_data.z_rot_avg;
}

ofVec2f ofApp::mapDistanceToPixel(ofVec2f pos) {
    pos.x -= this->_min_distance;
    pos.x /= (this->_max_distance - this->_min_distance);
    pos.x *= (this->_ui_max_distance - this->_ui_min_distance);
    pos.x += this->_ui_min_distance;
    return pos;
}

ofVec2f ofApp::mapPositionToPixel(ofVec2f pos) {
    float realWorld2PixelFactor = ((this->_ui_max_distance - this->_ui_min_distance) / (this->_max_distance - this->_min_distance));
    float sign;
    if (abs(pos.x)-this->_min_distance < 0) {
        // points closer than the minimum distance
        pos.x *= (this->_ui_min_distance/this->_min_distance);
    } else {
        // points between the minimum and the maximum distance
        if (pos.x < 0) {
            sign = -1.0f;
        } else {
            sign = 1.0f;
        }
        pos.x = pos.x + sign*this->_min_distance;
        pos.x *= realWorld2PixelFactor;
        pos.x = pos.x - sign*this->_min_distance;
    }
    if (abs(pos.y)-this->_min_distance < 0) {
        // points closer than the minimum distance
        pos.y *= (this->_ui_min_distance/this->_min_distance);
    } else {
        // points between the minimum and the maximum distance
        if (pos.y < 0) {
            sign = -1.0f;
        } else {
            sign = 1.0f;
        }
        pos.y = pos.y + sign*this->_min_distance;
        pos.y *= realWorld2PixelFactor;
        pos.y = pos.y - sign*this->_min_distance;
    }
    return pos;
}

void ofApp::sendMessageToPhone(int client, string message) {
    if (this->_android_tcp_server->isClientConnected(client) == true) {
        char messageLength[4];
        for (int i = 0; i < 4; i++) {
            messageLength[3 - i] = (message.length() >> (i * 8));
        }
        this->_android_tcp_server->sendRawBytes(client, messageLength, 4);
        this->_android_tcp_server->sendRawBytes(client, message.c_str(), message.length());
    }
}

void ofApp::connectPhone() {
    if (this->_android_tcp_server->getNumClients() <= 0) {
        bool success = this->_android_tcp_server->setup(this->_android_port);
        if (success == true) {
            this->_push_button_connect.removeListener(this, &ofApp::connectPhone);
            this->_push_button_disconnect.addListener(this, &ofApp::disconnectPhone);
            this->_push_button_connect.setFillColor(ofColor::black);
            this->_push_button_disconnect.setFillColor(ofColor::gray);
            this->_push_button_disconnect.setTextColor(ofColor::white);
        }
    }
}

void ofApp::disconnectPhone() {
    if (this->_android_tcp_server->isConnected()) {
        for (int clientID = 0; clientID < this->_android_tcp_server->getNumClients(); clientID++) {
            sendMessageToPhone(clientID, "END/");
            this->_android_tcp_server->disconnectClient(clientID);
        }
        this->_push_button_disconnect.removeListener(this, &ofApp::disconnectPhone);
        this->_push_button_connect.addListener(this, &ofApp::connectPhone);
        this->_push_button_connect.setTextColor(ofColor::red);
        this->_push_button_connect.setFillColor(ofColor::gray);
        this->_push_button_disconnect.setFillColor(ofColor::black);
        this->_push_button_disconnect.setTextColor(ofColor::black);
    }
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
            this->_ui_head_radius = this->_settings->getValue("ui_radius", 2.0f);
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
        this->_settings->pushTag("network");
        {
            this->_settings->pushTag("android");
            {
                this->_android_port = this->_settings->getValue("port", -1);
            }
            this->_settings->popTag();
            this->_settings->pushTag("eog");
            {
                this->_eog_host = this->_settings->getValue("host", "");
            }
            this->_settings->popTag();
            this->_settings->pushTag("mocap");
            {
                this->_use_vicon = this->_settings->getValue("use_vicon", false);
                this->_mocap_ip = this->_settings->getValue("host", "");
                this->_settings->pushTag("port");
                {
                    this->_mocap_receive_port = this->_settings->getValue("in",  -1);
                    this->_mocap_send_port = this->_settings->getValue("out", -1);
                }
                this->_settings->popTag();
            }
            this->_settings->popTag();
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
        this->_settings->addTag("network");
        this->_settings->pushTag("network");
        {
            this->_settings->addTag("android");
            this->_settings->pushTag("android");
            {
                this->_settings->addValue("port", 12345);
            }
            this->_settings->popTag();
            this->_settings->addTag("eog");
            this->_settings->pushTag("eog");
            {
                this->_settings->addValue("host", "192.168.1.1");
            }
            this->_settings->popTag();
            this->_settings->addTag("mocap"); // maybe the eog trigger can be broadcasted, then mocap has to listen on port 65500 as well
            this->_settings->pushTag("mocap");
            {
                this->_settings->setValue("use_vicon", true);
                this->_settings->addValue("host", "192.168.1.1");
                this->_settings->addTag("port");
                this->_settings->pushTag("port");
                {
                    this->_settings->addValue("in",  18403);
                    this->_settings->addValue("out", 18404);
                }
                this->_settings->popTag();
            }
            this->_settings->popTag();
        }
        this->_settings->popTag();
    }
    this->_settings->popTag();
    this->_settings->saveFile();
}

vector<string> ofApp::getLocalIPs() {
    vector<string> result;
#ifdef TARGET_WIN32
    string commandResult = ofSystem("ipconfig");
    for (int pos = 0; pos >= 0; ) {
        pos = commandResult.find("IPv4", pos);
        if (pos >= 0) {
            pos = commandResult.find(":", pos) + 2;
            int pos2 = commandResult.find("\n", pos);
            string ip = commandResult.substr(pos, pos2 - pos);
            pos = pos2;
            if (ip.substr(0, 3) != "127") { // let's skip loopback addresses
                result.push_back(ip);
            }
        }
    }
#else
    string commandResult = ofSystem("ifconfig");
    for (int pos = 0; pos >= 0; ) {
        pos = commandResult.find("inet ", pos);
        if (pos >= 0) {
            int pos2 = commandResult.find("netmask", pos);
            string ip = commandResult.substr(pos + 5, pos2 - pos - 6);
            pos = pos2;
            if (ip.substr(0, 3) != "127") { // let's skip loopback addresses
                result.push_back(ip);
            }
        }
    }
#endif
    return result;
}
string ofApp::getIPhost() {
    string ad = string();
    vector<string> list = getLocalIPs();
    if (!list.empty()) {
        ad = list[0];
    }
    return ad;
}
