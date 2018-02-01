#include <sys/stat.h> 
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <vector> 
#include <algorithm> 
#include <iostream>
#include <sstream>
#include <pthread.h>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include "Consumer_impl.h"
#include "SimplePublisher.h"
#include "fitsio.h"


#define SECONDARY_HDU 2
#define HEIGHT 512
#define WIDTH 2048

using namespace std;
using namespace YAML;

class Forwarder {
    public:

    //Important 'per readout' values
    std::vector<string> visit_raft_list;
    std::vector<std::vector<string> > visit_raft_ccd_list;

    std::vector<string> current_image_work_list;
    std::vector<string> finished_image_work_list;

    std::string Session_ID = "";
    std::string Visit_ID = "";
    std::string Job_Num = "";
    std::string Target_Dir = "";
    std::string Daq_Addr = "";
    std::string Work_Dir = ""; 
    std::string Src_Dir = ""; 
    std::string Name = ""; //such as F1
    std::string Lower_Name; //such as f1
    std::string Component = ""; //such as FORWARDER_1
    int Num_Images = 0; 
   
    
    std::string consume_queue = "";
    std::string fetch_consume_queue = "";
    std::string format_consume_queue = "";
    std::string forward_consume_queue = "";

    //General Forwarder consumers
    Consumer *from_foreman_consumer;
    Consumer *from_fetch_consumer;
    Consumer *from_format_consumer;
    Consumer *from_forward_consumer;

    //Consumers in work threads
    Consumer *from_forwarder_to_fetch;
    Consumer *from_forwarder_to_format;
    Consumer *from_forwarder_to_forward;

    //Publishers
    SimplePublisher *FWDR_pub;
    SimplePublisher *FWDR_to_fetch_pub;
    SimplePublisher *FWDR_to_format_pub;
    SimplePublisher *FWDR_to_forward_pub;
    SimplePublisher *fetch_pub;
    SimplePublisher *fmt_pub;
    SimplePublisher *fwd_pub;
    
    string USER, PASSWD, BASE_BROKER_ADDR, FQN, HOSTNAME, IP_ADDR, CONSUME_QUEUE, USER_FORWARD_PUB, PASSWD_FORWARD_PUB;
    string USER_PUB, PASSWD_PUB, USER_FETCH_PUB, PASSWD_FETCH_PUB, USER_FORMAT_PUB, PASSWD_FORMAT_PUB;
    string FETCH_USER, FETCH_USER_PASSWD, FORMAT_USER, FORMAT_USER_PASSWD,  FORWARD_USER, FORWARD_USER_PASSWD;
    string FETCH_USER_PUB, FETCH_USER_PUB_PASSWD, FORMAT_USER_PUB, FORMAT_USER_PUB_PASSWD; 
    string FORWARD_USER_PUB, FORWARD_USER_PUB_PASSWD;

    vector<string> readout_img_ids; 
    map<string, string> header_info_dict; 
    map<string, string> take_img_done_msg; 

    Forwarder();
    ~Forwarder();
    void setup_consumers(string);
    void setup_publishers(string); 

    //Declarations for message callbacks
    void on_foreman_message(string body);
    void on_fetch_message(string body);
    void on_format_message(string body);
    void on_forward_message(string body);

    void on_forwarder_to_fetch_message(string body);
    void on_forwarder_to_format_message(string body);
    void on_forwarder_to_forward_message(string body);

    //Declarations message handlers within callbacks
    void process_new_visit(Node n);
    void process_health_check(Node n);
    void process_xfer_params(Node n);
    void process_take_images(Node n);
    void process_take_images_done(Node n);
    void process_end_readout(Node n);

    void process_fetch(Node n);
    void process_fetch_ack(Node n);
    void process_fetch_end_readout(Node n);
    void process_fetch_health_check(Node n);
    void process_fetch_health_check_ack(Node n);
    void process_format(Node n);
    void process_format_ack(Node n);
    void process_format_health_check(Node n);
    void process_format_health_check_ack(Node n);
    void process_forward(Node n);
    void process_forward_ack(Node n);
    void process_forward_health_check(Node n);
    void process_forward_health_check_ack(Node n);

    void run();
    static void *run_thread(void *);
    char* read_img_segment(const char*);
    unsigned char** assemble_pixels(char *);
    void write_img(std::string, std::string);
    void assemble_img(string, string);
    void send_completed_msg(std::string);
    vector<string> list_files(string); 

    void process_formatted_img(Node); 
    void look_for_work(); 
};

using funcptr = void(Forwarder::*)(Node);

struct arg_struct {
    Consumer *consumer;
    Forwarder *forwarder;
    callback<Forwarder> funcptr;
};

//Primary Forwarder message actions
map<string, funcptr> on_foreman_message_actions = {
    { "AR_NEW_VISIT", &Forwarder::process_new_visit},
    { "PP_NEW_VISIT", &Forwarder::process_new_visit},
    { "SP_NEW_VISIT", &Forwarder::process_new_visit},
    { "AR_FWDR_HEALTH_CHECK", &Forwarder::process_health_check},
    { "PP_FWDR_HEALTH_CHECK", &Forwarder::process_health_check},
    { "SP_FWDR_HEALTH_CHECK", &Forwarder::process_health_check},
    { "AR_FWDR_XFER_PARAMS", &Forwarder::process_xfer_params},
    { "PP_FWDR_XFER_PARAMS", &Forwarder::process_xfer_params},
    { "SP_FWDR_XFER_PARAMS", &Forwarder::process_xfer_params},
    { "AR_TAKE_IMAGES", &Forwarder::process_take_images},
    { "PP_TAKE_IMAGES", &Forwarder::process_take_images},
    { "SP_TAKE_IMAGES", &Forwarder::process_take_images},
    { "AR_TAKE_IMAGES_DONE", &Forwarder::process_take_images_done},
    { "PP_TAKE_IMAGES_DONE", &Forwarder::process_take_images_done},
    { "SP_TAKE_IMAGES_DONE", &Forwarder::process_take_images_done},
    { "AR_END_READOUT", &Forwarder::process_end_readout},
    { "PP_END_READOUT", &Forwarder::process_end_readout},
    { "SP_END_READOUT", &Forwarder::process_end_readout}
};

//The next three handlers are essentially acks...
map<string, funcptr> on_fetch_message_actions = {
    { "FETCH_HEALTH_CHECK_ACK", &Forwarder::process_fetch_health_check_ack},
    { "AR_FETCH_ACK", &Forwarder::process_fetch_ack},
    { "PP_FETCH_ACK", &Forwarder::process_fetch_ack},
    { "SP_FETCH_ACK", &Forwarder::process_fetch_ack}

};

map<string, funcptr> on_format_message_actions = {
    { "FORMAT_HEALTH_CHECK_ACK", &Forwarder::process_format_health_check_ack},
    { "AR_FORMAT_ACK", &Forwarder::process_format_ack},
    { "PP_FORMAT_ACK", &Forwarder::process_format_ack},
    { "SP_FORMAT_ACK", &Forwarder::process_format_ack}

};

map<string, funcptr> on_forward_message_actions = {
    { "FORWARD_HEALTH_CHECK_ACK", &Forwarder::process_forward_health_check_ack},
    { "AR_FORWARD_ACK", &Forwarder::process_forward_ack},
    { "PP_FORWARD_ACK", &Forwarder::process_forward_ack},
    { "SP_FORWARD_ACK", &Forwarder::process_forward_ack},

};


//Forwarder Component message actions
//This handler is for messages from Primary Forwarder to fetch thread
map<string, funcptr> on_forwarder_to_fetch_message_actions = {
    { "FETCH_HEALTH_CHECK", &Forwarder::process_fetch_health_check},
    { "FETCH_END_READOUT", &Forwarder::process_fetch_end_readout},
    { "AR_FETCH", &Forwarder::process_fetch},
    { "PP_FETCH", &Forwarder::process_fetch},
    { "SP_FETCH", &Forwarder::process_fetch}
};

//This handler is for messages from Primary Forwarder to format thread
map<string, funcptr> on_forwarder_to_format_message_actions = {
    { "FORMAT_HEALTH_CHECK", &Forwarder::process_format_health_check},
    { "AR_FORMAT", &Forwarder::process_format},
    { "PP_FORMAT", &Forwarder::process_format},
    { "SP_FORMAT", &Forwarder::process_format}, 
};

//This handler is for messages from Primary Forwarder to forward thread
map<string, funcptr> on_forwarder_to_forward_message_actions = {
    { "FORWARD_HEALTH_CHECK", &Forwarder::process_forward_health_check},
    { "AR_FORWARD", &Forwarder::process_forward},
    { "PP_FORWARD", &Forwarder::process_forward},
    { "SP_FORWARD", &Forwarder::process_forward}, 
    { "FORMAT_DONE", &Forwarder::process_formatted_img} 

};

Forwarder::Forwarder() {
    // Read config file
    Node config_file;
    try {
        config_file = LoadFile("./ForwarderCfg.yaml");
    }
    catch (YAML::BadFile& e) {
        // FIX better catch clause...at LEAST a log message
        cout << "Error reading ForwarderCfg.yaml file." << endl;
        exit(EXIT_FAILURE);
    }

    Node root;
    string NAME, BASE_BROKER_ADDR, FQN, HOSTNAME, IP_ADDR;
    try {
        root = config_file["ROOT"];
        this->Name = root["NAME"].as<string>();
        this->Component = root["FQN"].as<string>();
        this->Lower_Name = root["LOWER_NAME"].as<string>();
        USER = root["USER"].as<string>();
        PASSWD = root["PASSWD"].as<string>();
        USER_PUB = root["USER_PUB"].as<string>();
        PASSWD_PUB = root["PASSWD_PUB"].as<string>();
        USER_FETCH_PUB = root["USER_FETCH_PUB"].as<string>();
        PASSWD_FETCH_PUB = root["PASSWD_FETCH_PUB"].as<string>();
        USER_FORMAT_PUB = root["USER_FORMAT_PUB"].as<string>();
        PASSWD_FORMAT_PUB = root["PASSWD_FORMAT_PUB"].as<string>();
        USER_FORWARD_PUB = root["USER_FORWARD_PUB"].as<string>();
        PASSWD_FORWARD_PUB = root["PASSWD_FORWARD_PUB"].as<string>();
        BASE_BROKER_ADDR = root["BASE_BROKER_ADDR"].as<string>(); // @xxx.xxx.xxx.xxx:5672/%2fbunny
        HOSTNAME = root["HOSTNAME"].as<string>();
        IP_ADDR = root["IP_ADDR"].as<string>();
        this->Work_Dir = root["WORK_DIR"].as<string>();
        this->Src_Dir = root["SRC_DIR"].as<string>(); 
        this->consume_queue = root["CONSUME_QUEUE"].as<string>();
        this->fetch_consume_queue = root["FETCH_CONSUME_QUEUE"].as<string>();
        this->format_consume_queue = root["FORMAT_CONSUME_QUEUE"].as<string>();
        this->forward_consume_queue = root["FORWARD_CONSUME_QUEUE"].as<string>();
        FETCH_USER = root["FETCH_USER"].as<string>();
        FETCH_USER_PASSWD = root["FETCH_USER_PASSWD"].as<string>();
        FETCH_USER_PUB = root["FETCH_USER_PUB"].as<string>();
        FETCH_USER_PUB_PASSWD = root["FETCH_USER_PUB_PASSWD"].as<string>();

        FORMAT_USER = root["FORMAT_USER"].as<string>();
        FORMAT_USER_PASSWD = root["FORMAT_USER_PASSWD"].as<string>();
        FORMAT_USER_PUB = root["FORMAT_USER_PUB"].as<string>();
        FORMAT_USER_PUB_PASSWD = root["FORMAT_USER_PUB_PASSWD"].as<string>();

        FORWARD_USER = root["FORWARD_USER"].as<string>();
        FORWARD_USER_PASSWD = root["FORWARD_USER_PASSWD"].as<string>();
        FORWARD_USER_PUB = root["FORWARD_USER_PUB"].as<string>();
        FORWARD_USER_PUB_PASSWD = root["FORWARD_USER_PUB_PASSWD"].as<string>();
    }
    catch (YAML::TypedBadConversion<string>& e) {
        cout << "ERROR: In ForwarderCfg.yaml, cannot read required elements from this file." << endl;
    }

    //ostringstream full_broker_url;
    //full_broker_url << "amqp://" << user_name << ":" << passwd << basePbroker_addr from above...

    setup_consumers(BASE_BROKER_ADDR);
    setup_publishers(BASE_BROKER_ADDR);
    
}

void Forwarder::setup_consumers(string BASE_BROKER_ADDR){
    //Consumers for Primary Forwarder
    ostringstream full_broker_url;
    full_broker_url << "amqp://" << USER << ":" << PASSWD << BASE_BROKER_ADDR ;
    from_foreman_consumer = new Consumer(full_broker_url.str(), this->consume_queue);

    ostringstream consume_queue1;
    consume_queue1 << this->consume_queue << "_from_fetch";
    from_fetch_consumer = new Consumer(full_broker_url.str(), consume_queue1.str());

    ostringstream consume_queue2;
    consume_queue2 << this->consume_queue << "_from_format";
    from_format_consumer = new Consumer(full_broker_url.str(), consume_queue2.str());

    ostringstream consume_queue3;
    consume_queue3 << this->consume_queue << "_from_forward";
    ostringstream from_fwd_broker;
    from_fwd_broker << "amqp://" << USER << ":" << PASSWD << BASE_BROKER_ADDR ;
    from_forward_consumer = new Consumer(full_broker_url.str(), consume_queue3.str());

    //Consumers for sub-components
    //ostringstream consume_queue;

    ostringstream full_broker_url2;
    full_broker_url2 << "amqp://" << FETCH_USER << ":" << FETCH_USER_PASSWD << BASE_BROKER_ADDR ;
    from_forwarder_to_fetch = new Consumer(full_broker_url.str(), this->fetch_consume_queue);

    ostringstream full_broker_url3;
    full_broker_url3 << "amqp://" << FORMAT_USER << ":" << FORMAT_USER_PASSWD << BASE_BROKER_ADDR ;
    from_forwarder_to_format = new Consumer(full_broker_url.str(), this->format_consume_queue);
    cout << this->format_consume_queue << endl; 

    ostringstream full_broker_url4;
    full_broker_url4 << "amqp://" << FORWARD_USER << ":" << FORWARD_USER_PASSWD << BASE_BROKER_ADDR ;
    from_forwarder_to_forward = new Consumer(full_broker_url.str(), this->forward_consume_queue);

}

void Forwarder::run() {
    //Set up argument structs for all seven consumer threads
    //Primary Forwarder consumer threads
    arg_struct *args1 = new arg_struct;
    args1->consumer = from_foreman_consumer;
    args1->forwarder = this;
    args1->funcptr = &Forwarder::on_foreman_message;

    arg_struct *args2 = new arg_struct;
    args2->consumer = from_fetch_consumer;
    args2->forwarder = this;
    args2->funcptr = &Forwarder::on_fetch_message;

    arg_struct *args3 = new arg_struct;
    args3->consumer = from_format_consumer;
    args3->forwarder = this;
    args3->funcptr = &Forwarder::on_format_message;

    arg_struct *args4 = new arg_struct;
    args4->consumer = from_forward_consumer;
    args4->forwarder = this;
    args4->funcptr = &Forwarder::on_forward_message;

    //Subcomponent consumer threads
    arg_struct *args5 = new arg_struct;
    args5->consumer = from_forwarder_to_fetch;
    args5->forwarder = this;
    args5->funcptr = &Forwarder::on_forwarder_to_fetch_message;

    arg_struct *args6 = new arg_struct;
    args6->consumer = from_forwarder_to_format;
    args6->forwarder = this;
    args6->funcptr = &Forwarder::on_forwarder_to_format_message;

    arg_struct *args7 = new arg_struct;
    args7->consumer = from_forwarder_to_forward;
    args7->forwarder = this;
    args7->funcptr = &Forwarder::on_forwarder_to_forward_message;

    //Create then run threads
    pthread_t t1;
    pthread_create(&t1, NULL, &Forwarder::run_thread, args1);

    pthread_t t2;
    pthread_create(&t2, NULL, &Forwarder::run_thread, args2);

    pthread_t t3;
    pthread_create(&t3, NULL, &Forwarder::run_thread, args3);

    pthread_t t4;
    pthread_create(&t4, NULL, &Forwarder::run_thread, args4);

    pthread_t t5;
    pthread_create(&t5, NULL, &Forwarder::run_thread, args5);

    pthread_t t6;
    pthread_create(&t6, NULL, &Forwarder::run_thread, args6);

    pthread_t t7;
    pthread_create(&t7, NULL, &Forwarder::run_thread, args7);

}

void *Forwarder::run_thread(void *pargs) {

    arg_struct *params = ((arg_struct *) pargs);
    Consumer *consumer = params->consumer;
    Forwarder *forwarder = params->forwarder;
    callback<Forwarder> on_msg = params->funcptr;

    consumer->run<Forwarder>(forwarder, on_msg);
}


void Forwarder::setup_publishers(string BASE_BROKER_ADDR){
    //Publishers
    ostringstream full_broker_url;
    full_broker_url << "amqp://" << USER_PUB << ":" << PASSWD_PUB << BASE_BROKER_ADDR;
    //cout << "Broker url is: " << full_broker_url.str();
    FWDR_pub = new SimplePublisher(full_broker_url.str());

    ostringstream full_broker_url1;
    full_broker_url1 << "amqp://" << USER_FETCH_PUB << ":" << PASSWD_FETCH_PUB << BASE_BROKER_ADDR;
    //cout << "Broker url is: " << full_broker_url1.str();
    FWDR_to_fetch_pub = new SimplePublisher(full_broker_url1.str());

    ostringstream full_broker_url2;
    full_broker_url2 << "amqp://" << USER_FORMAT_PUB << ":" << PASSWD_FORMAT_PUB << BASE_BROKER_ADDR;
    //cout << "Broker url is: " << full_broker_url2.str();
    FWDR_to_format_pub = new SimplePublisher(full_broker_url2.str());

    ostringstream full_broker_url3;
    full_broker_url3 << "amqp://" << USER_FORWARD_PUB << ":" << PASSWD_FORWARD_PUB << BASE_BROKER_ADDR;
    //cout << "Broker url is: " << full_broker_url3.str();
    FWDR_to_forward_pub = new SimplePublisher(full_broker_url3.str());

    ostringstream full_broker_url4;
    full_broker_url4 << "amqp://" << FETCH_USER_PUB << ":" << FETCH_USER_PUB_PASSWD << BASE_BROKER_ADDR;
    //cout << "Broker url is: " << full_broker_url4.str();
    fetch_pub = new SimplePublisher(full_broker_url4.str());

    ostringstream full_broker_url5;
    full_broker_url5 << "amqp://" << FORMAT_USER_PUB << ":" << FORMAT_USER_PUB_PASSWD << BASE_BROKER_ADDR;
    //cout << "Broker url is: " << full_broker_url5.str();
    fmt_pub = new SimplePublisher(full_broker_url5.str());

    ostringstream full_broker_url6;
    full_broker_url6 << "amqp://" << FORWARD_USER_PUB << ":" << FORWARD_USER_PUB_PASSWD << BASE_BROKER_ADDR;
    fwd_pub = new SimplePublisher(full_broker_url6.str());
}



//Messages received by Primary Forwarder from Foreman
void Forwarder::on_foreman_message(string body) {
    Node node = Load(body);
    string message_type = node["MSG_TYPE"].as<string>();
    funcptr action = on_foreman_message_actions[message_type];
    (this->*action)(node);
}

//Messages received by Primary Forwarder from fetch thread
void Forwarder::on_fetch_message(string body) {
    cout << "ON_FETCH: " << body << endl;
    Node node = Load(body);
    string message_type = node["MSG_TYPE"].as<string>();
    funcptr action = on_fetch_message_actions[message_type];
    (this->*action)(node);
}

//Messages received by Primary Forwarder from format thread
void Forwarder::on_format_message(string body) {
    Node node = Load(body);
    string message_type = node["MSG_TYPE"].as<string>();
    funcptr action = on_format_message_actions[message_type];
    (this->*action)(node);
}

//Messages received by Primary Forwarder from forwardthread
void Forwarder::on_forward_message(string body) {
    Node node = Load(body);
    string message_type = node["MSG_TYPE"].as<string>();
    funcptr action = on_forward_message_actions[message_type];
    (this->*action)(node);
}



//Messages received by the fetch, format, and forward threads
void Forwarder::on_forwarder_to_fetch_message(string body) {
    Node node = Load(body);
    string message_type = node["MSG_TYPE"].as<string>();
    funcptr action = on_forwarder_to_fetch_message_actions[message_type];
    (this->*action)(node);
}

void Forwarder::on_forwarder_to_format_message(string body) {
    Node node = Load(body);
    string message_type = node["MSG_TYPE"].as<string>();
    if (message_type == "FORMAT_END_READOUT") { 
        this->readout_img_ids.push_back(node["IMAGE_ID"].as<string>()); 
        this->look_for_work(); 
    } 
    else if (message_type == "HEADER_READY") { 
        this->header_info_dict[node["IMAGE_ID"].as<string>()] = node["FILENAME"].as<string>(); 
        this->look_for_work(); 
    } 
    else if (message_type == "FORMAT_TAKE_IMAGES_DONE") { 
        this->look_for_work(); 
    } 
    else { 
        funcptr action = on_forwarder_to_format_message_actions[message_type];
        (this->*action)(node);
    }
}

void Forwarder::on_forwarder_to_forward_message(string body) {
    Node node = Load(body);
    string message_type = node["MSG_TYPE"].as<string>();
    funcptr action = on_forwarder_to_forward_message_actions[message_type];
    (this->*action)(node);
}


//Message action handler methods...
void Forwarder::process_new_visit(Node n) {
    cout << "New Visit Message" << endl;
    return;
}

void Forwarder::process_health_check(Node n) {
    string ack_id = n["ACK_ID"].as<string>();
    string reply_queue = n["REPLY_QUEUE"].as<string>();

    string message_type = "AR_FWDR_HEALTH_CHECK_ACK";
    //string component = "AR";
    string ack_bool = "false";

    ostringstream message;
    message << "{ MSG_TYPE: " << message_type
            << ", COMPONENT: " << this->Name
            << ", ACK_ID: " << ack_id
            << ", ACK_BOOL: " << ack_bool << "}";

    FWDR_pub->publish_message(reply_queue, message.str());
    cout << "Health Check request Message" << endl;
    return;
}

void Forwarder::process_xfer_params(Node n) {
    this->visit_raft_list.clear();
    this->visit_raft_list = n["RAFT_LIST"].as<std::vector<string>>();
    this->visit_raft_ccd_list.clear();
    this->visit_raft_ccd_list = n["RAFT_CCD_LIST"].as<std::vector<std::vector<string>>>();

    this->Session_ID = n["SESSION_ID"].as<string>();
    this->Visit_ID = n["VISIT_ID"].as<string>();
    this->Job_Num = n["JOB_NUM"].as<string>();
    this->Target_Dir = n["TARGET_LOCATION"].as<string>();
    this->Daq_Addr = n["DAQ_ADDR"].as<string>();

    string ack_id = n["ACK_ID"].as<string>();
    string reply_queue = n["REPLY_QUEUE"].as<string>();

    string message_type = "AR_FWDR_XFER_PARAMS_ACK";
    //string component = "AR";
    string ack_bool = "false";

    ostringstream message;
    message << "{ MSG_TYPE: " << message_type
            << ", COMPONENT: " << this->Name
            << ", ACK_ID: " << ack_id
            << ", ACK_BOOL: " << ack_bool << "}";

    FWDR_pub->publish_message(reply_queue, message.str());
    return;
}

void Forwarder::process_take_images(Node n) {
    this->Num_Images = n["NUM_IMAGES"].as<int>();;
    cout << "Take Image Message...should be some tasty params here" << endl;
    return;
}

void Forwarder::process_take_images_done(Node n) {
    string ack_id = n["ACK_ID"].as<string>();
    // 1) Message fetch, format, and forwarder to clear all when work queue is complete
    // 2) forward thread must generate report
    // 3) 
    cout << "Take Image Message...should be some tasty params here" << endl;
    return;
}

void Forwarder::process_end_readout(Node n) {
    // Send IMAGE_ID to fetch thread...use message broker queue as work queue
    //If ForwarderCfg.yaml DAQ val == 'API', draw from actual DAQ emulator,
    //else, DAQ val will equal a path where files can be found.

    //If DAQ == 'API':  pass manifold into new fetch_and_reassemble class
    string image_id = n["IMAGE_ID"].as<string>();
    string msg_type = "FETCH_END_READOUT";
    ostringstream message;
    message << "{MSG_TYPE: " << msg_type
            << ", IMAGE_ID: " << image_id << "}";
    this->FWDR_to_fetch_pub->publish_message(this->fetch_consume_queue, message.str());
    return;
}

void Forwarder::process_fetch_end_readout(Node n) {
    //Make dir using image_id as name under work_dir
    //Fetch data from DAQ or copy from local drive
    //Send message to Format thread with image_id
    string image_id = n["IMAGE_ID"].as<string>();
    ostringstream cmd;
    ostringstream filepath;
    filepath << this->Work_Dir << "/" << image_id;
    cmd << "mkdir " << filepath.str();
    const std::string tmpstr = cmd.str();
    const char* cmdstr = tmpstr.c_str();
    system(cmdstr);

    if (this->Daq_Addr == "API") {

    } 
    else {
        string src_path = Src_Dir + image_id + "/*"; 

        ostringstream cp_cmd; 
        cp_cmd << "cp " << src_path << " " << filepath << "/"; 
        cout << cp_cmd.str() << endl; 
        system(cp_cmd.str().c_str()); 
    }
    string msg_type = "FORMAT_END_READOUT";
    ostringstream message;
    message << "{MSG_TYPE: " << msg_type
            << ", IMAGE_ID: " << image_id << "}";
    this->FWDR_to_format_pub->publish_message(this->format_consume_queue, message.str());
    return;
}

void Forwarder::process_fetch(Node n) {
    cout << "process_fetch" << endl;
    return;
}

void Forwarder::process_fetch_ack(Node n) {
    cout << "fetch ack being processed" << endl;
    return;
}

void Forwarder::process_fetch_health_check(Node n) {
    cout << "Send helth check to just fetch queue" << endl;
    return;
}

void Forwarder::process_fetch_health_check_ack(Node n) {
    cout << "Health check ack for fetch" << endl;
    return;
}

void Forwarder::process_format(Node n) {
    cout << "processing format message" << endl;
    return;
}

void Forwarder::process_format_ack(Node n) {
    cout << "processing format ack message" << endl;
    return;
}

void Forwarder::process_format_health_check(Node n) {
    cout << " Health check sent to Format only" << endl;
    return;
}

void Forwarder::process_format_health_check_ack(Node n) {
    cout << "Processing format health check ack message" << endl;
    return;
}

void Forwarder::process_forward(Node n) {
    cout << "Processing forward message" << endl;
    return;
}

void Forwarder::process_forward_ack(Node n) {
    cout << "Processing forward ack message" << endl;
    return;
}

void Forwarder::process_forward_health_check(Node n) {
    cout << "Processing forward health check message" << endl;
    return;
}

void Forwarder::process_forward_health_check_ack(Node n) {
    cout << "Processing forward health check ack message" << endl;
    return;
}

void Forwarder::assemble_img(string header, string img) {
    // create dir  /mnt/ram/FITS/IMG_10
    string fits_dir = Work_Dir + "FITS"; 
    const int dir = mkdir(fits_dir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR); 
    write_img(img, header);
}


char* Forwarder::read_img_segment(const char *file_path) { 
    fstream img_file(file_path, fstream::in | fstream::binary); 
    long len = WIDTH * HEIGHT; 
    char *buffer = new char[len]; 
    img_file.seekg(0, ios::beg); 
    img_file.read(buffer, len); 
    img_file.close();
    return buffer;
} 

unsigned char** Forwarder::assemble_pixels(char *buffer) { 
    unsigned char **array = new unsigned char*[HEIGHT]; 
    array[0] = (unsigned char *) malloc( WIDTH * HEIGHT * sizeof(unsigned char)); 

    for (int i = 1; i < HEIGHT; i++) { 
        array[i] = array[i-1] + WIDTH; 
    } 

    for (int j = 0; j < HEIGHT; j++) {
        for (int i = 0; i < WIDTH; i++) {
            array[j][i]= buffer[i+j]; 
        } 
    }
    return array;
} 

void Forwarder::write_img(string img, string header) { 
    long len = WIDTH * HEIGHT;
    int bitpix = BYTE_IMG; 
    long naxis = 2;
    long naxes[2] = { WIDTH, HEIGHT }; 
    long fpixel = 1; 
    long nelements = len; 
    int status = 0; 
    int hdunum = 2;
    int nkeys; 
    char card[FLEN_CARD]; 
    fitsfile *iptr, *optr; 

    // /mnt/ram/IMG_31
    string img_path = Work_Dir + img;
    string header_path = Work_Dir + "header/" + header;
    string destination = Work_Dir + "FITS/" + img;

    fits_open_file(&iptr, header_path.c_str(), READONLY, &status); 
    fits_create_file(&optr, destination.c_str(), &status); 
    fits_copy_hdu(iptr, optr, 0, &status); 

    vector<string> file_names = list_files(img_path); 
    vector<string>::iterator it; 
    for (it = file_names.begin(); it != file_names.end(); it++) { 
        string img_segment = img_path + "/" + *it; 
        char *img_buffer = read_img_segment(img_segment.c_str());
        unsigned char **array = assemble_pixels(img_buffer); 

        fits_movabs_hdu(iptr, hdunum, NULL, &status); 
        fits_create_img(optr, bitpix, naxis, naxes, &status); 
        fits_write_img(optr, TBYTE, fpixel, nelements, array[0], &status); 

        fits_get_hdrspace(iptr, &nkeys, NULL, &status); 
        for (int i = 1; i <= nkeys; i++) { 
            fits_read_record(iptr, i, card, &status); 
            fits_write_record(optr, card, &status); 
        }
        hdunum++;
    } 
    fits_close_file(iptr, &status); 
    fits_close_file(optr, &status); 

    send_completed_msg(img);
} 

vector<string> Forwarder::list_files(string path) { 
    struct dirent *entry; 
    DIR *dir  = opendir(path.c_str()); 
    vector<string> file_names; 
    while (entry  = readdir(dir)) { 
        string name = entry->d_name;
        if (strcmp(name.c_str(), ".") && strcmp(name.c_str(), "..")) { 
            file_names.push_back(name); 
        }
    } 

    sort(file_names.begin(), file_names.end()); 
    closedir(dir);
    return file_names; 
} 

void Forwarder::send_completed_msg(string img_id) { 
    ostringstream msg; 
    msg << "{ MSG_TYPE: FORWARD_END_READOUT" 
        << ", IMG_ID: " << img_id << "}"; 
    fmt_pub->publish_message(this->forward_consume_queue, msg.str()); 
} 

void Forwarder::look_for_work() { 
    vector<string>::iterator it;
    map<string, string>::iterator mit;  
    map<string, string>::iterator tid; 
    if (this->readout_img_ids.size() != 0) { 
        for (it = this->readout_img_ids.begin(); it != this->readout_img_ids.end(); it++) { 
            string img_id = *it; 
            mit = this->header_info_dict.find(img_id); 
            if (mit != this->header_info_dict.end()) { 
                this->readout_img_ids.erase(it); 
                string header_filename = this->header_info_dict[img_id]; 
                this->header_info_dict.erase(mit); 

                // do the work 
                assemble_img(header_filename, img_id); 
            } 
        } 
    } 

    if (this->readout_img_ids.size() == 0 && this->take_img_done_msg.size() == 0) { 
        return; 
    } 

    if (this->readout_img_ids.size() == 0 && this->take_img_done_msg.size() == 0) { 
        Emitter msg; 
        msg << BeginMap; 

        for (tid = this->take_img_done_msg.begin(); tid != this->take_img_done_msg.end(); tid++) { 
            string key = tid->first; 
            string val = tid->second; 
            if (key == "MSG_TYPE") { 
                val = "FORWARD_TAKE_IMAGES_DONE"; 
            } 

            msg << Key << key; 
            msg << Value << val; 
        } 
        msg << EndMap; 
        fmt_pub->publish_message(this->forward_consume_queue, msg.c_str()); 
    } 
} 

///////////////////////////////////////////////////////////////////////////////
// Forward part 
///////////////////////////////////////////////////////////////////////////////

void Forwarder::process_formatted_img(Node n) { 
    string img_id = n["IMG_ID"].as<string>(); 
    string img_path = this->Work_Dir + "FITS/" + img_id; 
    string dest_path = this->Target_Dir + img_id; 
    
    // use bbcp to send file 
    ostringstream bbcp_cmd; 
    bbcp_cmd << "bbcp "
             << img_path
             << " " 
             << dest_path; 
    cout << bbcp_cmd.str() << endl; 
    system(bbcp_cmd.str().c_str()); 
    this->finished_image_work_list.push_back(img_id);
} 

int main() {
    Forwarder *fwdr = new Forwarder();
    fwdr->run();
    while(1) {
    }
}

