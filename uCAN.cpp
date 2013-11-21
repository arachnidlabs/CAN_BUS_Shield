#include <Arduino.h>
#include "uCAN.h"

uCAN_IMPL uCAN;

uCAN_IMPL::uCAN_IMPL() {
	this->address_change_handler = NULL;
	this->timeout = 1000;
}

MessageID uCAN_IMPL::makeUnicastMessageID(uint8_t priority, uint8_t protocol, uint8_t subfields, uint8_t recipient) {
	MessageID ret;
	ret.unicast.broadcast = 0;
	ret.unicast.priority = priority;
	ret.unicast.protocol = protocol;
	ret.unicast.subfields = subfields;
	ret.unicast.recipient = recipient;
	ret.unicast.sender = this->node_id;
	return ret;
}

MessageID uCAN_IMPL::makeBroadcastMessageID(uint8_t priority, uint8_t protocol, uint16_t subfields) {
	MessageID ret;
	ret.broadcast.broadcast = 1;
	ret.broadcast.priority = priority;
	ret.broadcast.protocol = protocol;
	ret.broadcast.subfields = subfields;
	ret.broadcast.sender = this->node_id;
	return ret;
}

uint8_t uCAN_IMPL::begin(HardwareID hardware_id, uint8_t node_id) {
	this->hardware_id = hardware_id;
	this->node_id = node_id;

	uint8_t ret = CAN.begin(CAN_125KBPS);
	this->ping(this->node_id);

	//TODO: Proper address assignment
}

void uCAN_IMPL::send(MessageID id, uint8_t len, uint8_t *message) {
	CAN.sendMsgBuf(id.raw, 1, len, message);
}

bool uCAN_IMPL::tryReceive(uCANMessage *message) {
	CAN.readMsgBuf(&message->len, message->body);
	message->id.raw = CAN.getCanId();

	if(message->id.broadcast.broadcast) {
		switch(message->id.broadcast.protocol) {

		}
	} else {
		switch(message->id.unicast.protocol) {
		case 0: // YARP
			return this->handleYARP(message);
		}
	}
	return true;
}


bool uCAN_IMPL::receive() {
	if(CAN.checkReceive() != CAN_MSGAVAIL)
		return false;

	uCANMessage message;
	this->tryReceive(&message);
	return true;
}

void uCAN_IMPL::setTimeout(uint16_t timeout) {
	this->timeout = timeout;
}

bool uCAN_IMPL::handleYARP(uCANMessage *message) {
	if((message->id.unicast.subfields & 0x30) == 0x20) {
		// Query
		if(message->id.unicast.recipient != this->node_id && message->id.unicast.recipient != UCAN_BROADCAST_NODE_ID)
			// Not addressed to us
			return false;
		if((message->id.unicast.subfields & 0x08) && memcmp(&this->hardware_id, message->body, sizeof(HardwareID)) != 0)
			// Not addressed to our hardware ID
			return false;

		this->send(
			this->makeUnicastMessageID(message->id.unicast.priority, 0, 0x38, message->id.unicast.sender),
			6, this->hardware_id.address);
		return true;
	} else if((message->id.unicast.subfields & 0x38) == 0x18) {
		// Address assignment
		if(memcmp(&this->hardware_id, message->body, sizeof(HardwareID)) == 0) {
			this->node_id = message->body[6];
			if(this->address_change_handler)
				this->address_change_handler(this->node_id);
		}
		return true;
	}
	return false;
}

NodeAddress uCAN_IMPL::getNodeFromNodeID(uint8_t node_id) {
	return node_id;
}

NodeAddress uCAN_IMPL::getNodeFromHardwareID(HardwareID hardware_id) {
	this->send(
		this->makeUnicastMessageID(UCAN_PRIORITY_NORMAL, 0, 0x28, 0xFF),
		sizeof(HardwareID), hardware_id.address);

	uint16_t start = millis();
	while(millis() - start < this->timeout) {
		if(CAN.checkReceive() == CAN_MSGAVAIL) {
			uCANMessage message;
			if(this->tryReceive(&message)) {
				if(message.id.unicast.broadcast == 0 && message.id.unicast.protocol == 0 &&
				   message.id.unicast.subfields == 0x38 && message.id.unicast.recipient == this->node_id) {
					// Ping response to us!
					if(memcmp(message.body, hardware_id.address, sizeof(HardwareID)) == 0) {
						// From the node we queried
						return message.id.unicast.sender;
					}
				}
			}
		}
	}
	return UCAN_NODE_NOT_FOUND;
}

bool uCAN_IMPL::ping(NodeAddress node) {
	return this->ping(node, NULL);
}

bool uCAN_IMPL::ping(NodeAddress node, HardwareID *hardware_id) {
	this->send(this->makeUnicastMessageID(UCAN_PRIORITY_NORMAL, 0, 0x20, node), 0, NULL);

	uint16_t start = millis();
	while(millis() - start < this->timeout) {
		if(CAN.checkReceive() == CAN_MSGAVAIL) {
			uCANMessage message;
			if(this->tryReceive(&message)) {
				if(message.id.unicast.broadcast == 0 && message.id.unicast.protocol == 0 &&
				   message.id.unicast.subfields == 0x38 && message.id.unicast.recipient == this->node_id) {
					// Ping response to us!
					if(message.id.unicast.sender == node) {
						// From the node we queried
						if(hardware_id)
							memcpy(hardware_id->address, message.body, sizeof(HardwareID));
						return true;
					}
				}
			}
		}
	}
	return false;
}

void uCAN_IMPL::registerAddressChangeHandler(AddressChangeHandler handler) {
	this->address_change_handler = handler;
}

void uCAN_IMPL::setAddress(HardwareID hardware_id, uint8_t node_id) {
	MessageID id;
	uint8_t body[8];

	id.unicast.priority = UCAN_PRIORITY_HIGH;
	id.unicast.protocol = 0;
	id.unicast.broadcast = 0;
	id.unicast.subfields = 0x18;
	id.unicast.recipient = UCAN_BROADCAST_NODE_ID;
	id.unicast.sender = this->node_id;

	memcpy(body, &hardware_id.address, 6);
	body[6] = node_id;
	this->send(id, 6, body);
}
