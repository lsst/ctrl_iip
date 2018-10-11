#include <string.h>
#include <iostream> 
#include <yaml-cpp/yaml.h>
#include "OCS_Bridge.h"
#include "AckSubscriber.h"
#include "Toolsmod.h"

using namespace std; 
using namespace YAML; 

using ack_funcptr = void (AckSubscriber::*)(Node); 

map<string, map<string, string>> ack_book_keeper; 

map<string, long> summary_states = { 
    {"DISABLE", atArchiver::summaryState_DisabledState}, 
    {"ENABLE", atArchiver::summaryState_EnabledState}, 
    {"FAULT", atArchiver::summaryState_FaultState}, 
    {"OFFLINE", atArchiver::summaryState_OfflineState}, 
    {"STANDBY", atArchiver::summaryState_StandbyState}, 
};

map<string, ack_funcptr> action_handler = { 
    {"START_ACK", &AckSubscriber::process_ack}, 
    {"STOP_ACK", &AckSubscriber::process_ack}, 
    {"ENABLE_ACK", &AckSubscriber::process_ack}, 
    {"DISABLE_ACK", &AckSubscriber::process_ack}, 
    {"ENTER_CONTROL_ACK", &AckSubscriber::process_ack}, 
    {"STANDBY_ACK", &AckSubscriber::process_ack}, 
    {"EXIT_CONTROL_ACK", &AckSubscriber::process_ack}, 
    {"ABORT_ACK", &AckSubscriber::process_ack}, 
    {"RESET_FROM_FAULT_ACK", &AckSubscriber::process_recover_from_fault_ack}, 
    {"SUMMARY_STATE_EVENT", &AckSubscriber::process_summary_state}, 
    {"RECOMMENDED_SETTINGS_VERSION_EVENT", &AckSubscriber::process_recommended_settings_version}, 
    {"SETTINGS_APPLIED_EVENT", &AckSubscriber::process_settings_applied}, 
    {"APPLIED_SETTINGS_MATCH_START_EVENT", &AckSubscriber::process_applied_settings_match_start}, 
    {"ERROR_CODE_EVENT", &AckSubscriber::process_error_code}, 
    {"BOOK_KEEPING", &AckSubscriber::process_book_keeping}, 
    {"RESOLVE_ACK", &AckSubscriber::process_resolve_ack}
}; 

template<typename T> 
class Command { 
    public: 
        typedef salReturn (T::*funcptr)(int, salLONG, salLONG, char *); 

        map<string, funcptr> action_handler = { 
            {"START_ACK", &T::ackCommand_start}, 
            {"ENABLE_ACK", &T::ackCommand_enable}, 
            {"DISABLE_ACK", &T::ackCommand_disable}, 
            {"ENTER_CONTROL_ACK", &T::ackCommand_enterControl}, 
            {"STANDBY_ACK", &T::ackCommand_standby}, 
            {"EXIT_CONTROL_ACK", &T::ackCommand_exitControl}, 
            {"ABORT_ACK", &T::ackCommand_abort},
        }; 
};  

AckSubscriber::AckSubscriber() : OCS_Bridge() { 
    setup_consumer(); 
} 

AckSubscriber::~AckSubscriber() { 
}

void AckSubscriber::setup_consumer() { 
    ack_consumer = new Consumer(base_broker_addr, OCS_CONSUME); 
    ar = SAL_archiver(); 
    cu = SAL_catchuparchiver(); 
    pp = SAL_processingcluster(); 
    at = SAL_atArchiver(); 
}

void AckSubscriber::run() { 
    string devices[] = {"archiver", "catchuparchiver", "processingcluster", "atArchiver"};
    string commands[] = {"enable", "disable", "standby", "enterControl", "exitControl", "start", "stop", "abort"}; 
    string events[] = {"SummaryState", "AppliedSettingsMatchStart", "SettingVersion", "ErrorCode"};

    for (const string device: devices) { 
        for (const string command: commands) { 
            string topic = device + "_command_" + command;
            if (device == "archiver") { 
                ar.salProcessor(const_cast<char *>(topic.c_str())); 
            } 
            else if (device == "catchuparchiver") { 
                cu.salProcessor(const_cast<char *>(topic.c_str())); 
            } 
            else if (device == "processingcluster") { 
                pp.salProcessor(const_cast<char *>(topic.c_str())); 
            } 
            else if (device == "atArchiver") { 
                at.salProcessor(const_cast<char *>(topic.c_str())); 
            } 
        }
    } 

    for (const string device: devices) { 
        for (const string event: events) { 
            string topic = device + "_logevent_" + event;
            if (device == "archiver") { 
                ar.salEvent(const_cast<char *>(topic.c_str())); 
            } 
            else if (device == "catchuparchiver") { 
                cu.salEvent(const_cast<char *>(topic.c_str())); 
            } 
            else if (device == "processingcluster") { 
                pp.salEvent(const_cast<char *>(topic.c_str())); 
            } 
            else if (device == "atArchiver") { 
                at.salEventPub(const_cast<char *>(topic.c_str())); 
            } 
        }
    } 

    // These two events do not exist in other devices
    at.salProcessor(const_cast<char *>("atArchiver_command_recoverFromFault"));
    at.salEventPub(const_cast<char *>("atArchiver_logevent_settingsApplied")); 

    cout << "============> running CONSUMER <=============" << endl; 
    callback<AckSubscriber> on_msg = &AckSubscriber::on_message; 
    ack_consumer->run<AckSubscriber>(this, on_msg); 
} 

void AckSubscriber::on_message(string message) { 
    Node node = Load(message); 
    string message_value; 
    try { 
	message_value = node["MSG_TYPE"].as<string>(); 
	if (message_value != "RESOLVE_ACK"){
	    cout << "=== MSG: " << message << endl;
	    cout << "[" << message_value << "] ..." << endl; 
	}   
	ack_funcptr action = action_handler[message_value]; 
	(this->*action)(node); 
    } 
    catch (exception& e) { 
	cout << "WARNING: " << "In AckSubscriber -- on_message, cannot read fields from message." << endl; 
    } 
} 

void AckSubscriber::process_ack(Node n) {
    try { 
	string message_value = n["MSG_TYPE"].as<string>(); 
	long cmdId = stol(n["CMD_ID"].as<string>()); 
	string device = n["DEVICE"].as<string>(); 
	string ack_id = n["ACK_ID"].as<string>(); 
	string ack_bool = n["ACK_BOOL"].as<string>(); 
	string ack_statement = n["ACK_STATEMENT"].as<string>();

        salLONG error_code = (ack_bool == "true") ? 0: -302; 

        if (device == "AR") { 
            Command<SAL_archiver> sender; 
            Command<SAL_archiver>::funcptr action = sender.action_handler[message_value]; 
            (ar.*action)(cmdId, SAL__CMD_COMPLETE, error_code, const_cast<char *>(ack_statement.c_str())); 
        } 
        else if (device == "CU") { 
            Command<SAL_catchuparchiver> sender; 
            Command<SAL_catchuparchiver>::funcptr action = sender.action_handler[message_value]; 
            (cu.*action)(cmdId, SAL__CMD_COMPLETE, error_code, const_cast<char *>(ack_statement.c_str())); 
        } 
        else if (device == "PP"){ 
            Command<SAL_processingcluster> sender; 
            Command<SAL_processingcluster>::funcptr action = sender.action_handler[message_value]; 
            (pp.*action)(cmdId, SAL__CMD_COMPLETE, error_code, const_cast<char *>(ack_statement.c_str())); 
        }
        else if (device == "AT"){ 
            Command<SAL_atArchiver> sender; 
            Command<SAL_atArchiver>::funcptr action = sender.action_handler[message_value]; 
            (at.*action)(cmdId, SAL__CMD_COMPLETE, error_code, const_cast<char *>(ack_statement.c_str())); 
        }

	cout << "=== PROCESS_ACK: " << cmdId << "::" << device << "::" << ack_id 
             << "::" << message_value << "::" << ack_statement << endl; 

	ack_book_keeper[ack_id]["CHECKBOX"] = "true"; 
    } 
    catch (exception& e) { 
	cout << "WARNING: " << "In AckSubscriber -- process_ack, cannot read fields from message." << endl; 
    }  
}

// FIX: only AtArchiver is summaryStateC, not AR, CU or pp
void AckSubscriber::process_summary_state(Node n) { 
    try { 
        string message_value = n["MSG_TYPE"].as<string>(); 
        string device = n["DEVICE"].as<string>(); 
        string summary_state = n["CURRENT_STATE"].as<string>(); 
        long priority = 0; 

        if (device == "AR") { 
            //archiver_logevent_SummaryStateC data; 
            //data.SummaryStateValue = summary_state; 
            //data.priority = priority; 
            //ar.logEvent_SummaryState(&data, priority); 
        }
        else if (device == "CU") {
            //catchuparchiver_logevent_SummaryStateC data; 
            //data.SummaryStateValue = summary_state; 
            //data.priority = priority; 
            //cu.logEvent_SummaryState(&data, priority); 
        }
        else if (device == "PP") { 
            //processingcluster_logevent_SummaryStateC data; 
            //data.SummaryStateValue = summary_state; 
            //data.priority = priority; 
            //pp.logEvent_SummaryState(&data, priority); 
        }
        else if (device == "AT") { 
            atArchiver_logevent_summaryStateC data; 
            data.summaryStateValue = summary_states[summary_state]; 
            data.priority = priority; 
            at.logEvent_summaryState(&data, priority); 
        }
    } 
    catch (exception& e) { 
	cout << "WARNING: " << "In AckSubscriber -- summary_state, cannot read fields from message." << endl; 
        cout << "The error is " << e.what() << endl; 
    }  
} 

// FIX: AT camelcase
void AckSubscriber::process_recommended_settings_version(Node n) { 
    try { 
        string message_value = n["MSG_TYPE"].as<string>(); 
        string device = n["DEVICE"].as<string>(); 
        string recommended_setting = n["CFG_KEY"].as<string>(); 
        long priority = 0; 

        if (device == "AR") { 
            archiver_logevent_SettingVersionsC data; 
            data.recommendedSettingVersion = recommended_setting; 
            data.priority = priority; 
            ar.logEvent_SettingVersions(&data, priority); 
        }
        else if (device == "CU") {
            catchuparchiver_logevent_SettingVersionsC data; 
            data.recommendedSettingVersion = recommended_setting; 
            data.priority = priority; 
            cu.logEvent_SettingVersions(&data, priority); 
        }
        else if (device == "PP") { 
            processingcluster_logevent_SettingVersionsC data; 
            data.recommendedSettingVersion = recommended_setting; 
            data.priority = priority; 
            pp.logEvent_SettingVersions(&data, priority); 
        }
        else if (device == "AT") { 
            atArchiver_logevent_settingVersionsC data; 
            data.recommendedSettingVersion = recommended_setting; 
            data.priority = priority; 
            at.logEvent_settingVersions(&data, priority); 
        }
    } 
    catch (exception& e) { 
	cout << "WARNING: " << "In AckSubscriber -- recommended_setting_versions, cannot read fields from message." << endl; 
        cout << "The error is " << e.what() << endl; 
    }  
}

// FIX: camelCase
void AckSubscriber::process_settings_applied(Node n) {
    try { 
        string message_value = n["MSG_TYPE"].as<string>(); 
        string device = n["DEVICE"].as<string>(); 
        string ts_sal = n["TS_SAL_VERSION"].as<string>(); 
        string ts_xml = n["TS_XML_VERSION"].as<string>(); 
        string l1_tag = n["L1_DM_REPO_TAG"].as<string>(); 
	string settings = n["SETTINGS"].as<string>();
        long priority = 0; 

        if (device == "AR") { 
            // archiver_logevent_dSettingsMatchStartC data; 
            // data.appliedSettingsMatchStartIsTrue = settings_applied; 
            // data.priority = priority; 
            // ar.logEvent_AppliedSettingsMatchStart(&data, priority); 
        }
        else if (device == "CU") {
            // catchuparchiver_logevent_AppliedSettingsMatchStartC data; 
            // data.appliedSettingsMatchStartIsTrue = settings_applied; 
            // data.priority = priority; 
            // cu.logEvent_AppliedSettingsMatchStart(&data, priority); 
        }
        else if (device == "PP") { 
            // processingcluster_logevent_AppliedSettingsMatchStartC data; 
            // data.appliedSettingsMatchStartIsTrue = settings_applied; 
            // data.priority = priority; 
            // pp.logEvent_AppliedSettingsMatchStart(&data, priority); 
        }
        else if (device == "AT") { 
            atArchiver_logevent_settingsAppliedC data; 
	    data.settings = settings;
            data.tsSALVersion = ts_sal; 
            data.tsXMLVersion = ts_xml; 
            data.l1dmRepoTag = l1_tag; 
            at.logEvent_settingsApplied(&data, priority); 
        }
    } 
    catch (exception& e) { 
	cout << "WARNING: " << "In AckSubscriber -- settings_applied, cannot read fields from message." << endl; 
        cout << "The error is " << e.what() << endl; 
    }  
}

// FIX: camelCase
void AckSubscriber::process_applied_settings_match_start(Node n) {
    try { 
        string message_value = n["MSG_TYPE"].as<string>(); 
        string device = n["DEVICE"].as<string>(); 
        bool settings_applied = n["APPLIED"].as<bool>(); 
        long priority = 0; 

        if (device == "AR") { 
            archiver_logevent_AppliedSettingsMatchStartC data; 
            data.appliedSettingsMatchStartIsTrue = settings_applied; 
            data.priority = priority; 
            ar.logEvent_AppliedSettingsMatchStart(&data, priority); 
        }
        else if (device == "CU") {
            catchuparchiver_logevent_AppliedSettingsMatchStartC data; 
            data.appliedSettingsMatchStartIsTrue = settings_applied; 
            data.priority = priority; 
            cu.logEvent_AppliedSettingsMatchStart(&data, priority); 
        }
        else if (device == "PP") { 
            processingcluster_logevent_AppliedSettingsMatchStartC data; 
            data.appliedSettingsMatchStartIsTrue = settings_applied; 
            data.priority = priority; 
            pp.logEvent_AppliedSettingsMatchStart(&data, priority); 
        }
        else if (device == "AT") { 
            atArchiver_logevent_appliedSettingsMatchStartC data; 
            data.appliedSettingsMatchStartIsTrue = settings_applied; 
            data.priority = priority; 
            at.logEvent_appliedSettingsMatchStart(&data, priority); 
        }
    }
    catch (exception& e) { 
	cout << "WARNING: " << "In AckSubscriber -- applied_settings_match_start, cannot read fields from message." << endl; 
        cout << "The error is " << e.what() << endl; 
    }  
}

// FIX: camelCase
void AckSubscriber::process_error_code(Node n) {
    try {
        string message_value = n["MSG_TYPE"].as<string>(); 
        string device = n["DEVICE"].as<string>(); 
        long error_code = n["ERROR_CODE"].as<long>(); 
        long priority = 0; 

        if (device == "AR") { 
            archiver_logevent_ErrorCodeC data; 
            data.errorCode = error_code; 
            data.priority = priority; 
            ar.logEvent_ErrorCode(&data, priority); 
        }
        else if (device == "CU") {
            catchuparchiver_logevent_ErrorCodeC data; 
            data.errorCode = error_code; 
            data.priority = priority; 
            cu.logEvent_ErrorCode(&data, priority); 
        }
        else if (device == "PP") { 
            processingcluster_logevent_ErrorCodeC data; 
            data.errorCode = error_code; 
            data.priority = priority; 
            pp.logEvent_ErrorCode(&data, priority); 
        }
        else if (device == "AT") { 
            atArchiver_logevent_errorCodeC data; 
            data.errorCode = error_code; 
            data.priority = priority; 
            at.logEvent_errorCode(&data, priority); 
        }
    }
    catch (exception& e) { 
	cout << "WARNING: " << "In AckSubscriber -- error_code, cannot read fields from message." << endl; 
        cout << "The error is " << e.what() << endl; 
    }  
} 

void AckSubscriber::process_book_keeping(Node n) { 
    cout << "=== BOOK_KEEPING received" << endl;
    try { 
	string ack_id = n["ACK_ID"].as<string>(); 
	string time = n["TIME"].as<string>(); 
	string checkbox = n["CHECKBOX"].as<string>(); 
	string cmdId = n["CMD_ID"].as<string>(); 
	string device = n["DEVICE"].as<string>(); 
        string sub_type = n["SUB_TYPE"].as<string>(); 

	map<string, string> innerdict; 
	innerdict["TIME"] = time; 
	innerdict["CHECKBOX"] = checkbox; 
	innerdict["CMD_ID"] = cmdId; 
	innerdict["DEVICE"] = device; 
        innerdict["SUB_TYPE"] = sub_type; 

	ack_book_keeper[ack_id] = innerdict; 
    } 
    catch (exception& e) { 
	cout << "WARNING: " << "In AckSubscriber -- process__book_keeping, cannot read fields from message.";  
    } 
} 

void AckSubscriber::process_resolve_ack(Node n) { 
    for (auto &ack_dict : ack_book_keeper) { 
	string check_box = ack_dict.second.find("CHECKBOX")->second; 
	string device = ack_dict.second.find("DEVICE")->second; 
	string ack_id = ack_dict.first; 

	if (check_box == "false") { 
	    string dict_time = ack_dict.second.find("TIME")->second; 
	    int ack_delay = 2; 

	    int time_delta = get_time_delta(dict_time); 
	    bool timeout_result = time_delta > ack_delay ? true : false; 

	    if (timeout_result) { 
		string cmd_id = ack_dict.second.find("CMD_ID")->second; 
		long command_id = stol(cmd_id); 
                string command = ack_dict.second.find("SUB_TYPE")->second + "_ACK"; 

		
		cout << "### RESOLVE_NAME: " << command  << "::" << device << "::" << command_id << endl; 
                Node new_msg; 
                new_msg["MSG_TYPE"] = command; 
                new_msg["CMD_ID"] = command_id; 
                new_msg["DEVICE"] = device; 
                new_msg["ACK_ID"] = ack_id; 
                new_msg["ACK_BOOL"] = "false";  
                new_msg["ACK_STATEMENT"] = "nack"; 
                
		ack_book_keeper[ack_id]["CHECKBOX"] = "true";
                process_ack(new_msg); 

	    }  	
	}  
    }
}
// ****************************************************************************
// RESET_FROM_FAULT_ACK
// ****************************************************************************
void AckSubscriber::process_recover_from_fault_ack(Node n) {
    try { 
	string message_value = n["MSG_TYPE"].as<string>(); 
	long cmdId = stol(n["CMD_ID"].as<string>()); 
	string device = n["DEVICE"].as<string>(); 
	string ack_id = n["ACK_ID"].as<string>(); 
	string ack_bool = n["ACK_BOOL"].as<string>(); 
	string ack_statement = n["ACK_STATEMENT"].as<string>();

        salLONG error_code = (ack_bool == "true") ? 0: -302; 

        Command<SAL_atArchiver> sender; 
        at.ackCommand_resetFromFault(cmdId, SAL__CMD_COMPLETE, error_code, const_cast<char *>(ack_statement.c_str())); 

	cout << "=== PROCESS_ACK: " << cmdId << "::" << device << "::" << ack_id 
             << "::" << message_value << "::" << ack_statement << endl; 

	ack_book_keeper[ack_id]["CHECKBOX"] = "true"; 
    } 
    catch (exception& e) { 
	cout << "WARNING: " << "In AckSubscriber -- process_ack, cannot read fields from message." << endl; 
    }  
}

int main() { 
    AckSubscriber ack; 
    ack.run(); 
    return 0; 
} 
