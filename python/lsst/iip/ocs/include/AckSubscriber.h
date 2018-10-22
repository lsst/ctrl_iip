#include "Consumer_impl.h"
#include <pthread.h>
#include "SAL_archiver.h" 
#include "SAL_catchuparchiver.h" 
#include "SAL_processingcluster.h" 
#include "SAL_atArchiver.h"

/** Rabbitmq subscriber class to ack back messages from OCS after processing */ 
class AckSubscriber : public OCS_Bridge { 
    public: 

        /** Consumer object to listen to messages from rabbitmq */ 
        Consumer* ack_consumer; 
        SAL_archiver ar; 
        SAL_catchuparchiver cu; 
        SAL_processingcluster pp; 
        SAL_atArchiver at; 
        pthread_t ack_t, telemetry_t; 

        /** constructor for Rabbitmq ack subscriber to OCS system */ 
	AckSubscriber(); 

        /** destructor of AckSubscriber object */ 
	~AckSubscriber(); 

        /** Set up rabbitmq consumer object */ 
	void setup_consumer(); 

        /** Run rabbitmq IOLoop to listen to messages */ 
	void run(); 
        void shutdown();

        /** Rabbitmq callback function to parse into Consumer object to listen to messages
          * @param string message body
          */ 
        void on_message(std::string); 
        void on_telemetry_message(std::string); 

	void process_ack(YAML::Node);  
        void process_summary_state(YAML::Node); 
        void process_recommended_settings_version(YAML::Node); 
        void process_settings_applied(YAML::Node); 
        void process_applied_settings_match_start(YAML::Node); 
        void process_error_code(YAML::Node); 
	void process_book_keeping(YAML::Node); 
	void process_resolve_ack(YAML::Node); 
        void process_telemetry(YAML::Node);

        static void *run_telemetry_consumer(void *); 
        static void *run_ack_consumer(void *); 

        // recover from fault
        void process_reset_from_fault_ack(YAML::Node); 
}; 

