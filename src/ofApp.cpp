#include "ofApp.h"

void ofApp::exit(){
    this->_eog_trigger->stopRecording();
    disconnectPhone();
    if (this->_android_tcp_server->isConnected()) {
        this->_android_tcp_server->close();
    }
}

//--------------------------------------------------------------
void ofApp::setup(){
    this->_my_ip = getIPhost();
    this->_client_ip = "000.000.000.000";

    this->_uiPanel.setup();
    this->_uiPanel.add(this->_toggle_button_eog.setup("record", false));
    this->_toggle_button_eog.addListener(this, &ofApp::toggleRecording);
    this->_uiPanel.add(this->_toggle_button_sound.setup("sound", false));
    this->_toggle_button_sound.addListener(this, &ofApp::toggleSound);
    this->_uiPanel.add(this->_push_button_next.setup("next"));
    this->_uiPanel.add(this->_push_button_previous.setup("previous"));
    this->_push_button_next.addListener(this, &ofApp::moveToNextTarget);
    this->_push_button_previous.addListener(this, &ofApp::moveToPreviousTarget);
    this->_uiPanel.add(this->_push_button_connect.setup("connect phone"));
    this->_uiPanel.add(this->_push_button_disconnect.setup("disconnect phone"));
    this->_push_button_connect.addListener(this, &ofApp::connectPhone);
    this->_uiPanel.add(this->_reset_head_origin.setup("reset origin"));
    this->_reset_head_origin.addListener(this, &ofApp::resetHeadOrigin);

    this->_min_distance = 0.5f;
    this->_max_distance = 2.5f;

    this->_x_origin = 0;
    this->_y_origin = 0;

    ofSetCircleResolution(100);
    this->_ui_world_diameter = ofGetWindowHeight();
    this->_ui_center = ofVec2f(ofGetWindowWidth(), ofGetWindowHeight()) - this->_ui_world_diameter/2;
    this->_ui_world_start = this->_ui_center  - this->_ui_world_diameter/2;
    this->_ui_min_distance = (this->_ui_world_diameter/2)*0.30;
    this->_ui_max_distance = (this->_ui_world_diameter/2)*0.90;

    loadSettingsAndWriteDefaultIfNeeded();

    for (int i=0; i < this->_source_positions.size(); i++) {
        this->_source_instance.push_back(*new Blinky(this->_source_radius));
        ofVec2f pos = mapDistanceToPixel(this->_source_positions[i]);
        ofVec2f cartesian = convertPolarToCartesian(pos);
        this->_source_instance[i].setPosition(cartesian);
    }
    this->_current_target = 0;

    this->_eog_trigger = new UdpTrigger(this->_eog_host);

    if (this->_android_tcp_server == NULL) {
        this->_android_tcp_server = new ofxTCPServer();
        this->_android_tcp_server->setMessageDelimiter("");
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    if (this->_my_ip == "") {
        this->_my_ip = getIPhost();
    }
    if (this->_client_ip == "000.000.000.000") {
        if (this->_android_tcp_server->getNumClients() > 0) {
            this->_client_ip = this->_android_tcp_server->getClientIP(0);
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

        for (int i=0; i < this->_source_positions.size(); i++) {
            _source_instance[i].draw();
        }
    }
    ofPopMatrix();

    ofSetColor(ofColor::white); // 55, 40, 25, 10
    ofDrawBitmapString("origin (x/y): " + ofToString(this->_x_origin) + "/" + ofToString(this->_y_origin), 10, ofGetWindowHeight()-85);
    ofDrawBitmapString("Target: " + ofToString(this->_current_target+1) + "/" + ofToString(this->_source_positions.size()), 10, ofGetWindowHeight()-70);
    ofDrawBitmapString("r[m]: " + ofToString(this->_source_positions[this->_current_target].x) + " phi[deg]: " + ofToString(this->_source_positions[this->_current_target].y), 10, ofGetWindowHeight()-55);

    ofDrawBitmapString("IP: " + this->_my_ip, 10, ofGetWindowHeight()-25);
    ofDrawBitmapString("connected to: " + this->_client_ip, 10, ofGetWindowHeight()-10);

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
        cout << "should start" << endl;
        this->_eog_trigger->startRecording();
    } else {
        cout << "should stop" << endl;
        this->_eog_trigger->stopRecording();
    }
}

void ofApp::toggleSound(const void *sender, bool &value) {
    if (value == true) {
        this->_toggle_button_sound.setFillColor(ofColor::green);
        this->_eog_trigger->sendTrigger("sound_on");
    } else {
        this->_toggle_button_sound.setFillColor(ofColor::red);
        this->_eog_trigger->sendTrigger("sound_off");
    }
}

void ofApp::resetHeadOrigin() {
    this->_x_origin = 1;
    this->_y_origin = 1;
}

ofVec2f ofApp::mapDistanceToPixel(ofVec2f pos) {
    pos.x -= this->_min_distance;
    pos.x /= (this->_max_distance - this->_min_distance);
    pos.x *= (this->_ui_max_distance - this->_ui_min_distance);
    pos.x += this->_ui_min_distance;
    return pos;
}

void ofApp::sendMessageToPhone(int client, string message) {
    if (this->_android_tcp_server->isClientConnected(client) == true) {
        this->_android_tcp_server->send(client, message);
    }
}

void ofApp::connectPhone() {
    if (this->_android_tcp_server->getNumClients() <= 0) {
        bool success = this->_android_tcp_server->setup(this->_android_port);
        if (success == true) {
            this->_push_button_connect.removeListener(this, &ofApp::connectPhone);
            this->_push_button_disconnect.addListener(this, &ofApp::disconnectPhone);
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
