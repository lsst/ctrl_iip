#rabbitmqctl -p /bunny purge_queue ocs_dmcs_consume
#rabbitmqctl -p /bunny purge_queue dmcs_ack_consume
#rabbitmqctl -p /bunny purge_queue dmcs_ocs_publish
#rabbitmqctl -p /bunny purge_queue pp_foreman_consume
#rabbitmqctl -p /bunny purge_queue ar_foreman_consume
#rabbitmqctl -p /bunny purge_queue cu_foreman_consume
#rabbitmqctl -p /bunny purge_queue f_consume
#rabbitmqctl -p /bunny purge_queue archive_ctrl_consume
#rabbitmqctl -p /bunny purge_queue archive_ctrl_publish
#rabbitmqctl -p /bunny purge_queue ar_foreman_ack_publish
#rabbitmqctl -p /bunny purge_queue pp_foreman_ack_publish
#rabbitmqctl -p /bunny purge_queue audit_consume
#rabbitmqctl -p /bunny purge_queue f1_consume
#rabbitmqctl -p /bunny purge_queue f2_consume
#rabbitmqctl -p /bunny purge_queue f3_consume
#rabbitmqctl -p /bunny purge_queue f4_consume
#rabbitmqctl -p /bunny purge_queue ncsa_consume
#rabbitmqctl -p /bunny purge_queue audit_consume
#rabbitmqctl -p /bunny purge_queue dmcs_consume
#rabbitmqctl -p /bunny purge_queue event_dmcs_consume

rabbitmqctl -p /test purge_queue ocs_dmcs_consume
rabbitmqctl -p /bunny purge_queue audit_consume
rabbitmqctl -p /test purge_queue dmcs_ack_consume
rabbitmqctl -p /test purge_queue dmcs_ocs_publish
rabbitmqctl -p /test purge_queue pp_foreman_consume
rabbitmqctl -p /test purge_queue ar_foreman_consume
rabbitmqctl -p /test purge_queue cu_foreman_consume
rabbitmqctl -p /test purge_queue archive_ctrl_consume
rabbitmqctl -p /test purge_queue archive_ctrl_publish
rabbitmqctl -p /test purge_queue ar_forwarder_publish
rabbitmqctl -p /test purge_queue ar_foreman_ack_publish
rabbitmqctl -p /test purge_queue pp_foreman_ack_publish
rabbitmqctl -p /test purge_queue audit_consume
rabbitmqctl -p /test purge_queue f1_consume
rabbitmqctl -p /test purge_queue f2_consume
rabbitmqctl -p /test purge_queue f3_consume
rabbitmqctl -p /test purge_queue f4_consume
rabbitmqctl -p /test purge_queue ncsa_consume
rabbitmqctl -p /test purge_queue audit_consume
rabbitmqctl -p /test purge_queue dmcs_consume
rabbitmqctl -p /test purge_queue event_dmcs_consume

rm ./logs/*.log

