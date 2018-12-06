#include "ofApp.h"

void ofApp::exit(){
    this->_eog_trigger->stopRecording();
    disconnectPhone();
    /*if (this->_android_tcp_server->isConnected()) {
        this->_android_tcp_server->close();
    }*/
    this->_vicon_receiver.stop();
}

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetLogLevel(OF_LOG_SILENT); // for now keep output silent
    this->_isLogFileCreated = false;

    this->_my_ip = getIPhost();
    this->_client_ip = "000.000.000.000";

    // add gui
    this->_uiPanel.setup();
    this->_uiPanel.add(this->_phone_label.setup("PHONE",""));
    this->_uiPanel.add(this->_push_button_connect.setup("connect to tobii"));
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
    //this->_push_button_connect.addListener(this, &ofApp::connectPhone);
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
    this->_phi_origin = 0;
    this->_head_x = 0;
    this->_head_y = 0;
    this->_head_z = 0;
    this->_head_phi = 0;
    this->_old_head_x = 0;
    this->_old_head_y = 0;
    this->_old_head_z = 0;
    this->_old_head_phi = 0;
    this->_time = 0.0f;
    this->_logStartTime = 0.0f;

    // draw live feedback background
    ofSetCircleResolution(100);
    this->_ui_world_diameter = ofGetWindowHeight();
    this->_ui_center = ofVec2f(ofGetWindowWidth(), ofGetWindowHeight()) - this->_ui_world_diameter/2;
    this->_ui_world_start = this->_ui_center  - this->_ui_world_diameter/2;
    this->_ui_min_distance = (this->_ui_world_diameter/2)*0.30;
    this->_ui_max_distance = (this->_ui_world_diameter/2)*0.90;

    loadSettingsAndWriteDefaultIfNeeded();
    loadPositions();

    // create sound positions to be tested
    for (int i=0; i < this->_source_positions.size(); i++) {
        this->_source_instance.push_back(*new Blinky(this->_source_radius));
        ofVec2f pos = mapDistanceToPixel(this->_source_positions[i]);
        ofVec2f cartesian = convertPolarToCartesian(pos, 90); // rotate sources by 90 cw to have 0 at the bottom
        this->_source_instance[i].setPosition(cartesian);
    }
    this->_sound_on = false;
    this->_current_target = 0;
    this->_current_source_position = convertPolarToCartesian(this->_source_positions[this->_current_target], 90);

    // create and connect to shimmer udp port
    this->_eog_trigger = new UdpTrigger(this->_eog_host);
    this->_eog_trigger->connectToHost();

    // create tcp server to connect to android
    /*if (this->_android_tcp_server == NULL) {
        this->_incoming_message_len = 0;
        this->_android_tcp_server = new ofxTCPServer();
        this->_android_tcp_server->setMessageDelimiter("");
    }*/
    this->_ssr_osc = new ofxOscSender();
    this->_ssr_osc->setup(this->_android_ip, this->_android_port);

    this->_tobii_osc = new ofxOscSender();
    this->_tobii_osc->setup(this->_tobii_ip, this->_tobii_port);

    // setup vicon receiver
    ofxUDPSettings settings;
    settings.receiveOn(this->_mocap_receive_port);
    settings.blocking = false;
    this->_vicon_receiver.setup(settings);
}

void ofApp::connectToSSR(bool value) {
    if (this->_ssr_osc != NULL) {
        ofxOscMessage msg = ofxOscMessage();
        msg.setAddress("/connect");
        msg.addStringArg("?");
        if (value == true) {
            msg.addIntArg(1);
        } else {
            msg.addIntArg(0);
        }
        this->_ssr_osc->sendMessage(msg);
    }
}
void ofApp::loadSsrScene() {
    if (this->_ssr_osc != NULL) {
        ofxOscMessage msg = ofxOscMessage();
        msg.setAddress("/load");
        msg.addStringArg("?");
        msg.addStringArg("?");
        this->_ssr_osc->sendMessage(msg);
    }
}
void ofApp::streamSSR(bool value) {
    if (this->_ssr_osc != NULL) {
        ofxOscMessage msg = ofxOscMessage();
        msg.setAddress("/stream");
        msg.addStringArg("?");
        if (value == true) {
            msg.addIntArg(1);
        } else {
            msg.addIntArg(0);
        }
        this->_ssr_osc->sendMessage(msg);
        if (value == true) {
            this->_ssr_running = true;
        } else {
            this->_ssr_running = false;
        }
    }
}
void ofApp::updateSoundPos(float x, float y) {
    if (this->_ssr_osc != NULL) {
        ofxOscMessage msg = ofxOscMessage();
        msg.setAddress("/soundpos");
        msg.addFloatArg(x);
        msg.addFloatArg(y);
        this->_ssr_osc->sendMessage(msg);
    }
}
void ofApp::updatePos(float x, float y) {
    if (this->_ssr_osc != NULL) {
        ofxOscMessage msg = ofxOscMessage();
        msg.setAddress("/pos");
        msg.addFloatArg(x);
        msg.addFloatArg(y);
        this->_ssr_osc->sendMessage(msg);
    }
}
void ofApp::updateAngle(float phi) {
    if (this->_ssr_osc != NULL) {
        ofxOscMessage msg = ofxOscMessage();
        msg.setAddress("/azimuth");
        msg.addStringArg("?");
        msg.addFloatArg(phi);
        this->_ssr_osc->sendMessage(msg);
    }
}

void ofApp::setupProjectEyeTracker() {
    ofxOscMessage msg = ofxOscMessage();
    // set project
    msg.setAddress("/set");
    msg.addStringArg("project");
    msg.addStringArg("eog_calibration");
    _tobii_osc->sendMessage(msg);
}

void ofApp::setupSubjectEyeTracker() {
    ofxOscMessage msg = ofxOscMessage();
    // set participant
    msg.setAddress("/set");
    msg.addStringArg("participant");
    msg.addStringArg(this->_username);
    _tobii_osc->sendMessage(msg);
}

void ofApp::connectEyeTracker() {
    ofxOscMessage msg = ofxOscMessage();
    // connect
    msg.setAddress("/connect");
    msg.addStringArg("?");
    msg.addIntArg(1);
    _tobii_osc->sendMessage(msg);
}

void ofApp::streamEyeTracker() {
    ofxOscMessage msg = ofxOscMessage();
    // start streaming / wake up cameras
    msg.setAddress("/stream");
    msg.addStringArg("?");
    msg.addIntArg(1);
    _tobii_osc->sendMessage(msg);
}

void ofApp::stopRecordingEyeTracker() {
    ofxOscMessage msg = ofxOscMessage();
    // stop recording
    msg.setAddress("/record");
    msg.addStringArg("?");
    msg.addIntArg(0);
    _tobii_osc->sendMessage(msg);
}

void ofApp::cleanupEyeTracker() {
    ofxOscMessage msg = ofxOscMessage();
    // stop streaming
    msg.setAddress("/stream");
    msg.addStringArg("?");
    msg.addIntArg(0);
    _tobii_osc->sendMessage(msg);

    // disconnect
    msg.setAddress("/connect");
    msg.addStringArg("?");
    msg.addIntArg(0);
    _tobii_osc->sendMessage(msg);
}

void ofApp::calibrateEyeTracker() {
    ofxOscMessage msg = ofxOscMessage();
    msg.setAddress("/set");
    msg.addStringArg("calibration");
    msg.addStringArg("?");
    _tobii_osc->sendMessage(msg);
}

void ofApp::recordEyeTracker() {
    ofxOscMessage msg = ofxOscMessage();
    msg.setAddress("/record");
    msg.addStringArg("?");
    msg.addIntArg(1);
    _tobii_osc->sendMessage(msg);
}

void ofApp::sendEyeTrackerEvent(string message) {
    ofxOscMessage msg = ofxOscMessage();
    msg.setAddress("/set");
    msg.addStringArg("trigger");
    msg.addStringArg(message);
    _tobii_osc->sendMessage(msg);
}
//--------------------------------------------------------------
void ofApp::update(){
    float now = ofGetElapsedTimef();
    float dt = now - this->_time;
    if (this->_my_ip == "") {
        this->_my_ip = getIPhost();
    }
    /*if (this->_client_ip == "000.000.000.000") {
        if (this->_android_tcp_server->getNumClients() > 0) {
            this->_client_ip = this->_android_tcp_server->getClientIP(0);
            this->_push_button_connect.setTextColor(ofColor::green);
        } else {
            this->_push_button_connect.setTextColor(ofColor::red);
        }
    }*/
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
    if (0) {
        // convert position from milimeters to meters
        this->_head_x = (this->_head_data.x_position/1000) - this->_x_origin;
        this->_head_y = (this->_head_data.y_position/1000) - this->_y_origin;
        this->_head_z = (this->_head_data.z_position/1000);
        // invert rotation direction for screen and android
        this->_head_phi = fmod((360.0f - this->_head_data.z_rot_avg) - this->_phi_origin, 360.0f);
    } else {
        // round to milimeter accuracy
        this->_head_x = round(this->_head_data.x_position)/1000 - this->_x_origin;
        this->_head_y = round(this->_head_data.y_position)/1000 - this->_y_origin;
        this->_head_z = round(this->_head_data.z_position)/1000;
        // round to 0.1 degree and invert rotation direction for screen and android
        this->_head_phi = fmod((360.0f - round(this->_head_data.z_rot_avg*10)/10) - this->_phi_origin, 360.0f);
    }

    //if (dt > (1.0f/30.0f)) {
        if ((this->_old_head_x != this->_head_x) || (this->_old_head_y != this->_head_y) || (this->_old_head_z != this->_head_z) || (this->_old_head_phi != this->_head_phi)) {
            if (this->_ssr_running == true) {
                updatePos(-this->_head_y, this->_head_x);
                updateAngle(fmod((270 + round(this->_head_data.z_rot_avg*10)/10) - this->_phi_origin, 360.0f));
            }
            //sendMessageToPhone(0, "POSITION/" + ofToString(-this->_head_x) + "/" + ofToString(-this->_head_y) + "/" + ofToString(this->_head_z) + "/" + ofToString(this->_head_phi));
        }
        this->_time = now;
        ofLogNotice("UPDATE", "," + ofToString(now-this->_logStartTime) + "," + ofToString(-this->_head_x) + "," + ofToString(-this->_head_y) + "," + ofToString(this->_head_z) + "," + ofToString(this->_head_phi) + "," + ofToString(this->_source_positions[this->_current_target].x) + "," + ofToString(this->_source_positions[this->_current_target].y) + "," + ofToString(-this->_current_source_position.x) + "," + ofToString(this->_current_source_position.y) + "," + ofToString(this->_source_height) + "," + ofToString(this->_sound_on));
    //}
    /*this->_incoming_message_len = this->_android_tcp_server->getNumReceivedBytes(0);
    if (this->_incoming_message_len >= 4) {
        char *_inchar = new char[this->_incoming_message_len];
        this->_android_tcp_server->receiveRawBytes(0, _inchar, this->_incoming_message_len);
    TODO:        int bytes_to_read = (_inchar[0] << 24) + (_inchar[1] << 16) + (_inchar[2] << 8) + _inchar[3];
        _inchar = new char[bytes_to_read];
        this->_android_tcp_server->receiveRawBytes(0, _inchar, bytes_to_read);
        string s = _inchar;
        cout << s << endl;
    }*/
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
            //ofVec2f pos = ofVec2f(this->_head_x, -this->_head_y);
            ofVec2f pos = ofVec2f(this->_head_y, this->_head_x);
            pos = mapPositionToPixel(pos);
            ofTranslate(pos);
            ofSetColor(ofColor::orange);
            ofDrawCircle(0, 0, this->_ui_head_radius);
            //ofRotateDeg(this->_head_phi, 0, 0, 1); // of_v0.10
            ofRotateDeg(this->_head_phi); // of_v0.9
            ofSetColor(ofColor::red);
            ofDrawCircle(0, this->_ui_head_radius, 3);
        }
        ofPopMatrix();

        // draw source positions
        for (int i=0; i < this->_source_positions.size(); i++) {
            _source_instance[i].draw();
        }
    }
    ofPopMatrix();

    ofSetColor(ofColor::white); // 55, 40, 25, 10
    ofDrawBitmapString("head (x/y): " + ofToString(this->_head_x) + "/" + ofToString(this->_head_y), 10, ofGetWindowHeight()-145);
    ofDrawBitmapString("head (phi): " + ofToString(this->_head_phi), 10, ofGetWindowHeight()-130);

    ofDrawBitmapString("origin (x/y): " + ofToString(this->_x_origin) + "/" + ofToString(this->_y_origin), 10, ofGetWindowHeight()-100);
    ofDrawBitmapString("origin (phi): " + ofToString(this->_phi_origin), 10, ofGetWindowHeight()-85);
    ofDrawBitmapString("Target: " + ofToString(this->_current_target+1) + "/" + ofToString(this->_source_positions.size()), 10, ofGetWindowHeight()-70);
    ofDrawBitmapString("r[m]: " + ofToString(this->_source_positions[this->_current_target].x) + " phi[deg]: " + ofToString(this->_source_positions[this->_current_target].y), 10, ofGetWindowHeight()-55);

    ofDrawBitmapString("IP: " + this->_my_ip, 10, ofGetWindowHeight()-25);
    ofDrawBitmapString("connected to: " + this->_client_ip, 10, ofGetWindowHeight()-10);

    ofSetColor(ofColor::black);
    ofDrawBitmapString("\\- window -/", this->_ui_center.x - 45, ofGetWindowHeight()-15);
    ofDrawBitmapString("door", this->_ui_world_start.x, this->_ui_center.y);

    this->_uiPanel.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key == '1') {
        connectToSSR(true);
    }
    if (key == '2') {
        loadSsrScene();
    }
    if (key == '3') {
        streamSSR(true);
    }
    if (key == '4') {
        updateSoundPos(this->_head_x, this->_head_y);
    }
    if (key == '5') {
        updatePos(this->_head_x, this->_head_y);
    }
    if (key == '6') {
        streamSSR(false);
    }
    if (key == '7') {
        connectToSSR(false);
    }

    if (key == 's') {
        setupProjectEyeTracker();
        setupSubjectEyeTracker();
        streamEyeTracker();
    }
    if (key == 'c') {
        calibrateEyeTracker();
    }
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
        recordEyeTracker(); // this needs some time, so I do it first
        if (this->_isLogFileCreated == false) {
            this->_isLogFileCreated = true;
            ofLogToFile(this->_username + "_" + nowToString() + ".txt"); // set output filename
        }
        ofSetLogLevel(OF_LOG_NOTICE); // activate logging
        ofLogNotice("RECORD", ",TIME,HEAD_X,HEAD_Y,HEAD_HEIGHT,HEAD_PHI,SOUND_R,SOUND_PHI,SOUND_X,SOUND_Y,SOUND_HEIGHT,SOUND_ON"); // write header
        this->_logStartTime = ofGetElapsedTimef();
        this->_toggle_button_eog.setTextColor(ofColor::green);
        this->_eog_trigger->startRecording();
    } else {
        stopRecordingEyeTracker();
        this->_eog_trigger->stopRecording();
        this->_toggle_button_eog.setTextColor(ofColor::red);
        ofSetLogLevel(OF_LOG_SILENT); // deactivate logging
        this->_isLogFileCreated = false;
    }
}

void ofApp::toggleSound(const void *sender, bool &value) {
    this->_sound_on = value;
    if (value == true) {
        // sound source position
        this->_current_source_position = convertPolarToCartesian(this->_source_positions[this->_current_target], 90);
        sendMessageToPhone(0, "SRCPOS/" + ofToString(-_current_source_position.x) + "/" + ofToString(_current_source_position.y) + "/" + ofToString(this->_source_height));
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
        sendEyeTrackerEvent("sound_on");
    } else {
        sendEyeTrackerEvent("sound_off");
        //sendMessageToPhone(0, "STOP/");
        streamSSR(false);
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
    if (0) {
        this->_x_origin = this->_head_data.x_position/1000;
        this->_y_origin = this->_head_data.y_position/1000;
        this->_phi_origin = 360.0f - this->_head_data.z_rot_avg;
    } else {
        // round to milimeter accuracy
        this->_x_origin = round(this->_head_data.x_position)/1000;
        this->_y_origin = round(this->_head_data.y_position)/1000;
        // round to 0.1 degree
        this->_phi_origin = 360.0f - round(this->_head_data.z_rot_avg*10)/10;
    }
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
    /*if (this->_android_tcp_server->isClientConnected(client) == true) {
        char messageLength[4];
        for (int i = 0; i < 4; i++) {
            messageLength[3 - i] = (message.length() >> (i * 8));
        }
        this->_android_tcp_server->sendRawBytes(client, messageLength, 4);
        this->_android_tcp_server->sendRawBytes(client, message.c_str(), message.length());
    }*/
}

void ofApp::connectPhone() {
    connectEyeTracker();
    /*if (this->_android_tcp_server->getNumClients() <= 0) {
        bool success = this->_android_tcp_server->setup(this->_android_port);
        if (success == true) {*/
            this->_push_button_connect.removeListener(this, &ofApp::connectPhone);
            this->_push_button_connect.setFillColor(ofColor::black);
            /*this->_push_button_disconnect.addListener(this, &ofApp::disconnectPhone);
            this->_push_button_disconnect.setFillColor(ofColor::gray);
            this->_push_button_disconnect.setTextColor(ofColor::white);
        }
    }*/
}

void ofApp::disconnectPhone() {
    connectToSSR(false);
    cleanupEyeTracker();
    /*if (this->_android_tcp_server->isConnected()) {
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
    }*/
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
            this->_username = this->_settings->getValue("name", "USER");
            this->_position_settings_filename = this->_username + ".xml";
            this->_source_height = this->_settings->getValue("ear_height", 1.60f);
            this->_ui_head_radius = this->_settings->getValue("ui_radius", 2.0f);
        }
        this->_settings->popTag();
        this->_source_radius = this->_settings->getValue("ui_radius", 1.0f);
        this->_settings->pushTag("network");
        {
            this->_settings->pushTag("tobii");
            {
                this->_tobii_port = this->_settings->getValue("port", 8000);
                this->_tobii_ip = this->_settings->getValue("ip", "192.168.1.1");
            }
            this->_settings->popTag();
            this->_settings->pushTag("android");
            {
                this->_android_ip = this->_settings->getValue("ip", "192.168.1.1");
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

void ofApp::loadPositions() {
    this->_position_settings = new ofxXmlSettings();
    if (this->_position_settings->loadFile(this->_position_settings_filename) == false) {
        writeDefaultPositionSettings();
        this->_position_settings->loadFile(this->_position_settings_filename);
    }
    this->_position_settings->pushTag("position_settings");
    {
        int number_of_positions = this->_position_settings->getValue("number_of_positions", 0);
        this->_position_settings->pushTag("positions");
        {
            for (int i=0; i < number_of_positions; i++) {
                this->_position_settings->pushTag("position", i);
                {
                    float phi = this->_position_settings->getValue("phi", 0);  // 0 - 359.9 degrees
                    float r   = this->_position_settings->getValue("r", 0.0f); // 0.5 - 3.5 meters
                    this->_source_positions.push_back(ofVec2f(r, phi));
                }
                this->_position_settings->popTag();
            }
        }
        this->_position_settings->popTag();
    }
    this->_position_settings->popTag();
}

void ofApp::writeDefaultSettings() {
    this->_settings->addTag("settings");
    this->_settings->pushTag("settings");
    {
        this->_settings->addTag("subject");
        this->_settings->pushTag("subject");
        {
            this->_settings->addValue("name", "xx00xx00");
            this->_settings->addValue("ear_height", 1.60f);
            this->_settings->addValue("ui_radius", 15.0f);
        }
        this->_settings->popTag();
        this->_settings->addValue("ui_radius", 10.0f);
        this->_settings->addTag("network");
        this->_settings->pushTag("network");
        {
            this->_settings->addTag("tobii");
            this->_settings->pushTag("tobii");
            {
                this->_settings->addValue("port", 8000);
                this->_settings->addValue("ip", "192.168.1.1");
            }
            this->_settings->popTag();
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

void ofApp::writeDefaultPositionSettings() {
    this->_position_settings->addTag("position_settings");
    this->_position_settings->pushTag("position_settings");
    {
        this->_position_settings->addValue("number_of_positions", 1);
        this->_position_settings->addTag("positions");
        this->_position_settings->pushTag("positions");
        {
            this->_position_settings->addTag("position");
            this->_position_settings->pushTag("position");
            {
                this->_position_settings->addValue("phi", 0);
                this->_position_settings->addValue("r", 0.5f);
            }
            this->_position_settings->popTag();
        }
        this->_position_settings->popTag();
    }
    this->_position_settings->popTag();
    this->_position_settings->saveFile();
}

string ofApp::nowToString() {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer,sizeof(buffer),"%d-%m-%Y_%H-%M-%S",timeinfo);
    std::string str(buffer);
    return str;
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
