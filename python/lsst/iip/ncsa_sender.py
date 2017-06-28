from SimplePublisher import *
import time as t

sp = SimplePublisher("amqp://OCS:OCS@141.142.208.191:5672/%2fbunny")

msg = {} 
msg["MSG_TYPE"] = "NCSA_RESOURCES_QUERY"
msg["JOB_NUM"] = 6
msg["ACK_ID"] = "NCSA_000001"
msg["FORWARDERS"] = { "FORWARDER_1": 1 }
sp.publish_message("ncsa_consume", msg)

t.sleep(10)
msg = {}
msg["MSG_TYPE"] = "NCSA_READOUT"
msg["JOB_NUM"] = 6
msg["ACK_ID"] = "READOUT_0000001"
sp.publish_message("ncsa_consume", msg)