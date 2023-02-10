//#ifndef __MQTT_CLIENT_ERROR_H__
//#define __MQTT_CLIENT_ERROR_H__

DEF_MQTT_ERROR(NONE,"None")
DEF_MQTT_ERROR(TCP,"Tcp Error")
DEF_MQTT_ERROR(TCP_CONNECT,"Tcp Connect Error")
DEF_MQTT_ERROR(CONNECT_EMPTY,"Connect Empty")
DEF_MQTT_ERROR(CONNACK_INVALID,"Connack Invalid")
DEF_MQTT_ERROR(RECV_TASK,"Recv Task Error")
DEF_MQTT_ERROR(PACKETID_INVALID,"PacketID Invalid")
DEF_MQTT_ERROR(PUBLISH_RESPONSE,"Publish Response")
DEF_MQTT_ERROR(TCP_NO_RESPONSE,"Tcp No Response")
DEF_MQTT_ERROR(INFO, "Info Error")
DEF_MQTT_ERROR(TX_SEMPHR,"Tx Semphr Error")
DEF_MQTT_ERROR(S_CONNECT, "S Connect Error")
DEF_MQTT_ERROR(S_DISCONNECT,"S Disconnect Error")
DEF_MQTT_ERROR(S_PUBLISH,"S Publish Error")
DEF_MQTT_ERROR(S_PUBREL,"S Pubrel Error")
DEF_MQTT_ERROR(S_SUBSCRIBE,"S Subscribe Error")
DEF_MQTT_ERROR(S_UNSUBSCRIBE,"S Unsubscribe Error")
DEF_MQTT_ERROR(S_PUBLISH_RESPONSE,"S Publish Response Error")
DEF_MQTT_ERROR(ACK_INVALID,"Ack Invalid Error")
DEF_MQTT_ERROR(MEMORY_SUBSCRIBE,"Memory Subscribe Error")
DEF_MQTT_ERROR(SUBACK_UNGRANTED,"Suback Ungranted Error")
DEF_MQTT_ERROR(SUBACK_DESERIALIZE,"Suback Deserialize error")
DEF_MQTT_ERROR(CONNACK_PROTOCOL,"Connack Protocol Error")
DEF_MQTT_ERROR(CONNACK_ID_REJECT,"Connack ID Reject Error")
DEF_MQTT_ERROR(CONNACK_SERVER_FAIL,"Connack Server Fail")
DEF_MQTT_ERROR(CONNACK_CREDETIALS,"Connack Credetrals Error")
DEF_MQTT_ERROR(CONNACK_UNATHORIZED,"Connack Unathorized Error")
DEF_MQTT_ERROR(CONNACK_UNDEFINED,"Connack Undefined Error")
//#endif