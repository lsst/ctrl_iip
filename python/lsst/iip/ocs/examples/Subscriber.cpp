#include <iostream> 
#include <pthread.h>
#include <yaml-cpp/yaml.h>
#include "Consumer_impl.h" 

using namespace std; 
using namespace YAML; 

// header part 
class Subscriber { 
    public: 
    Consumer *consumer; 

    Subscriber(); 
    ~Subscriber(); 
    void setup_consumer(); 
    void on_message(string body); 
    void do_something(Node n); 
    void run();
    static void *run_thread(void *);
}; 

// implementation starts here
using funcptr = void (Subscriber::*)(Node); 

struct arg_struct { 
    Consumer *consumer;
    Subscriber *subscriber; 
};

map<string, funcptr> action_handler = { 
    { "DO_SOMETHING", &Subscriber::do_something}, 
}; 

Subscriber::Subscriber() {
    setup_consumer();
} 

Subscriber::~Subscriber() {} 

void Subscriber::setup_consumer() { 
    consumer = new Consumer("amqp://AFM:AFM@141.142.208.160/%2fbunny", "ocs_dmcs_consume"); 
} 

void Subscriber::on_message(string body) { 
    Node node = Load(body); 
    string message_type = node["MSG_TYPE"].as<string>(); 
    funcptr action = action_handler[message_type]; 
    (this->*action)(node); 
} 

// IMPLEMENT ME 
void Subscriber::do_something(Node n) { 
    string device = n["DEVICE"].as<string>(); 
    cout << "[x] I did something and device is " << device << endl; 
} 

void Subscriber::run() { 
    cout << "Consumer running ..." << endl;

    arg_struct *args = new arg_struct; 
    args->consumer = consumer; 
    args->subscriber = this; 

    pthread_t t;
    pthread_create(&t, NULL, &Subscriber::run_thread, args); 
} 

void *Subscriber::run_thread(void *pargs) {
    arg_struct *params = ((arg_struct *) pargs); 
    Consumer *consumer = params->consumer;
    Subscriber *subscriber = params->subscriber;

    callback<Subscriber> on_msg = &Subscriber::on_message;
    consumer->run<Subscriber>(subscriber, on_msg); 
} 

int main() { 
    Subscriber *sub = new Subscriber(); 
    sub->run(); 
    
    pthread_exit(NULL); 
    return 0;
} 
