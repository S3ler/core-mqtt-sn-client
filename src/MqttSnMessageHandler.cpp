//
// Created by bele on 08.04.17.
//

#include "MqttSnMessageHandler.h"
#include "mqttsn_messages.h"

bool MqttSnMessageHandler::begin() {
    if (socket != nullptr && core != nullptr) {
        return socket->begin();
    }
    return false;
}

void MqttSnMessageHandler::setSocket(SocketInterface *socket) {
    this->socket = socket;
}

void MqttSnMessageHandler::setCore(Client *core) {
    this->core = core;
}

void MqttSnMessageHandler::receiveData(device_address *address, uint8_t *bytes) {
    message_header *header = (message_header *) bytes;
    if (header->length < 2) {
        return;
    }
    if (header->type == MQTTSN_ADVERTISE) {
        //TODO
        return;
    }
    if (!core->is_mqttsn_connected()) {
        if (header->type == MQTTSN_CONNACK) {
            parse_connack(address, bytes);
        }
        return;
    }
    if (!core->is_gateway_address(address)) {
        return;
    }
    switch (header->type) {
        case MQTTSN_PINGREQ:
            parse_pingreq(address, bytes);
            break;
        case MQTTSN_PINGRESP:
            parse_pingresp(address, bytes);
            break;
        case MQTTSN_CONNACK:
            parse_connack(address, bytes);
            break;
        case MQTTSN_SUBACK:
            parse_suback(address, bytes);
            break;
        case MQTTSN_REGACK:
            // TODO
        case MQTTSN_PUBLISH:
            parse_publish(address, bytes);
            break;
        default:
            break;
    }
}

void MqttSnMessageHandler::parse_pingreq(device_address *address, uint8_t *bytes) {
    msg_pingreq *msg = (msg_pingreq *) bytes;
    if (bytes[0] == 2 && bytes[1] == MQTTSN_PINGREQ) {
        handle_pingreq(address);
    }
}

void MqttSnMessageHandler::handle_pingreq(device_address *address) {
    if (core->is_gateway_address(address)) {
        if (core->get_await_message() == MQTTSN_PINGREQ) {
            send_pingresp(address);
        }
        // TODO disconnect
    }
}

void MqttSnMessageHandler::send_pingresp(device_address *address) {
    message_header to_send;
    to_send.length = 2;
    to_send.type = MQTTSN_PINGRESP;
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(message_header))) {
        core->notify_socket_disconnected();
    }
}

void MqttSnMessageHandler::send_connect(device_address *address, const char *client_id, uint16_t duration) {
    msg_connect to_send(false, true, PROTOCOL_ID, duration, client_id);
    if (!socket->send(address, (uint8_t *) &to_send, to_send.length)) {
        core->notify_socket_disconnected();
    }
}

void MqttSnMessageHandler::parse_connack(device_address *pAddress, uint8_t *bytes) {
    msg_connack *msg = (msg_connack *) bytes;
    // TODO check values
    if (bytes[0] == 3 && bytes[1] == MQTTSN_CONNACK) {
        if (msg->type == core->get_await_message()) {
            if (msg->return_code == ACCEPTED) {
                core->set_await_message(MQTTSN_PINGREQ);
                core->set_mqttsn_connected();
                return;
            } else if (msg->return_code == REJECTED_CONGESTION) {
                return;
            }
        }
        send_disconnect(pAddress);
        core->set_await_message(MQTTSN_DISCONNECT);
    }
}

void MqttSnMessageHandler::send_disconnect(device_address *address) {
    message_header to_send;
    to_send.to_disconnect();
    if (!socket->send(address, (uint8_t *) &to_send, sizeof(message_header))) {
        core->notify_socket_disconnected();
    }
}

void MqttSnMessageHandler::notify_socket_connected() {
    core->notify_socket_connected();
}

void MqttSnMessageHandler::notify_socket_disconnected() {
    core->notify_socket_disconnected();
}

void MqttSnMessageHandler::send_pingreq(device_address *address) {
    message_header to_send;
    to_send.to_pingreq();
    if (!socket->send(address, (uint8_t *) &to_send, (uint16_t) to_send.length)) {
        core->notify_socket_disconnected();
    }
}

void MqttSnMessageHandler::parse_pingresp(device_address *pAddress, uint8_t *bytes) {
    msg_pingreq *msg = (msg_pingreq *) bytes;
    if (bytes[0] == 2 && bytes[1] == MQTTSN_PINGRESP) {
        if (core->get_await_message() == MQTTSN_PINGRESP) {
            core->norify_pingresponse_arrived();
            core->set_await_message(MQTTSN_PINGREQ);
        }
        //TODO disconnect
    }
}

void MqttSnMessageHandler::send_subscribe(device_address *address, const char *topic_name, uint8_t qos) {
    msg_subscribe_topicname to_send(topic_name, core->increment_and_get_msg_id_counter(), qos, false);
    if (!socket->send(address, (uint8_t *) &to_send, (uint16_t) to_send.length)) {
        core->notify_socket_disconnected();
    }

}

void MqttSnMessageHandler::parse_suback(device_address *pAddress, uint8_t *bytes) {
    msg_suback *msg = (msg_suback *) bytes;
    if (msg->length == 8 && msg->type == MQTTSN_SUBACK && msg->message_id == core->get_await_message_id()) {
        if (msg->return_code == ACCEPTED) {
            int8_t granted_qos = 0;
            if ((msg->flags & FLAG_QOS_M1) == FLAG_QOS_M1) {
                granted_qos = -1;
            } else if ((msg->flags & FLAG_QOS_2) == FLAG_QOS_2) {
                granted_qos = 2;
            } else if ((msg->flags & FLAG_QOS_1) == FLAG_QOS_1) {
                granted_qos = 1;
            } else {
                granted_qos = 0;
            }
            if (granted_qos == -1) {
                //TODO disconnect
            }
            if (core->await_topic_id) {
                core->set_await_topic_id(msg->topic_id);
            }
            core->set_granted_qos(granted_qos);
            core->set_await_message(MQTTSN_PINGREQ);
            return;
        } else if (msg->return_code == REJECTED_CONGESTION) {
            // TODO try again later
        } else if (msg->return_code == REJECTED_INVALID_TOPIC_ID) {
            // TODO register topic name again!
        }
        /*else if (msg->return_code == REJECTED_NOT_SUPPORTED) {
            // disconnect
        }*/
    }
    //TODO disconnect
}

void MqttSnMessageHandler::parse_publish(device_address *address, uint8_t *bytes) {
    msg_publish *msg = (msg_publish *) bytes;
    if (bytes[0] > 7 && bytes[1] == MQTTSN_PUBLISH) { // 7 bytes header + at least 1 byte data
        bool dup = (msg->flags & FLAG_DUP) != 0;

        int8_t qos = 0;
        if ((msg->flags & FLAG_QOS_M1) == FLAG_QOS_M1) {
            qos = -1;
        } else if ((msg->flags & FLAG_QOS_2) == FLAG_QOS_2) {
            qos = 2;
        } else if ((msg->flags & FLAG_QOS_1) == FLAG_QOS_1) {
            qos = 1;
        } else {
            qos = 0;
        }

        bool retain = (msg->flags & FLAG_RETAIN) != 0;
        bool short_topic = (msg->flags & FLAG_TOPIC_SHORT_NAME) != 0;
        uint16_t data_len = bytes[0] - (uint8_t) 7;
        if (((qos == 0) || (qos == -1)) && msg->message_id != 0x0000) {
            // this can be too strict
            // we can also ignore the message_id for Qos 0 and -1
            return;
        }

        //if (!short_topic && !(msg->flags & FLAG_TOPIC_PREDEFINED_ID != 0)) { // TODO what does this so?! WTF?!
        //    core->handle_publish(address, msg->data, data_len, msg->message_id, msg->topic_id, short_topic, retain, qos, dup);
        //}
        //msg id
        // short topic
        core->handle_publish(address, msg->data, data_len, msg->topic_id, retain, qos);
        return;
    }
    // TODO disconnect
}

void MqttSnMessageHandler::send_publish(device_address *address, uint8_t *data, uint8_t data_len, uint16_t msg_id,
                                        uint16_t topic_id, bool short_topic, bool retain, uint8_t qos, bool dup) {
    msg_publish to_send(dup, qos, retain, short_topic, topic_id, msg_id, data, data_len);
    if (!socket->send(address, (uint8_t *) &to_send, (uint16_t) to_send.length)) {
        core->notify_socket_disconnected();
    }
}
