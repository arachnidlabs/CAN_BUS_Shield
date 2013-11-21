#include "uCAN.h"

uCAN_IMPL uCAN;

uint8_t uCAN_IMPL::begin(uint64_t hardware_id, uint8_t node_id) {
	this->hardware_id = hardware_id;
	this->node_id = node_id;

	this->pong_handler = NULL;
	this->address_change_handler = NULL;

	uint8_t ret = CAN.begin(CAN_125KBPS);
	this->ping(this->node_id, UCAN_PRIORITY_HIGH);
}

void uCAN_IMPL::send(MessageID id, uint8_t len, uint8_t *message) {
	CAN.sendMsgBuf(id.raw, 1, len, message);
}

void uCAN_IMPL::receive() {
	uint8_t rxbuf[8];
	uint8_t len = 0;
	MessageID id;

	CAN.readMsgBuf(&len, rxbuf);
	id.raw = CAN.getCanId();

	if(id.broadcast.broadcast) {
		switch(id.broadcast.protocol) {

		}
	} else {
		switch(id.unicast.protocol) {
		case 0: // YARP
			this->handleYARP(id, len, rxbuf);
			break;
		}
	}
}

void uCAN_IMPL::handleYARP(MessageID id, uint8_t len, uint8_t *data) {
	if((id.unicast.subfields & 0x38) == 0x38) {
		// Query response
		if(this->pong_handler) {
			this->pong_handler(0xFFFFFF & *(uint64_t*)data, id.unicast.sender);
		}
	} else if((id.unicast.subfields & 0x30) == 0x20) {
		// Query
		if(id.unicast.recipient != this->node_id && id.unicast.recipient != UCAN_BROADCAST_NODE_ID)
			// Not addressed to us
			return;
		if((id.unicast.subfields & 0x08) && this->hardware_id != *(uint64_t*)data)
			// Not addressed to our hardware ID
			return;

		MessageID return_id;
		return_id.unicast.priority = id.unicast.priority;
		return_id.unicast.protocol = 0;
		return_id.unicast.broadcast = 0;
		return_id.unicast.subfields = 0x38;
		return_id.unicast.recipient = id.unicast.sender;
		return_id.unicast.sender = this->node_id;
		this->send(return_id, 6, (uint8_t*)&this->hardware_id);
	} else if((id.unicast.subfields & 0x38) == 0x18) {
		// Address assignment
		if(this->hardware_id == (0xFFFFFF & *(uint64_t*)data)) {
			this->node_id = data[6];
			if(this->address_change_handler)
				this->address_change_handler(this->node_id);
		}
	}
}

void uCAN_IMPL::ping(uint8_t node_id) {
	this->ping(node_id, UCAN_PRIORITY_NORMAL);
}

void uCAN_IMPL::ping(uint8_t node_id, uint8_t priority) {
	MessageID id;
	id.unicast.priority = priority;
	id.unicast.protocol = 0;
	id.unicast.broadcast = 0;
	id.unicast.subfields = 0x20;
	id.unicast.recipient = node_id;
	id.unicast.sender = this->node_id;
	this->send(id, 0, NULL);
}

void uCAN_IMPL::pingHardwareID(uint64_t hardware_id) {
	this->pingHardwareID(hardware_id, UCAN_PRIORITY_NORMAL);
}

void uCAN_IMPL::pingHardwareID(uint64_t hardware_id, uint8_t priority) {
	MessageID id;
	id.unicast.priority = priority;
	id.unicast.protocol = 0;
	id.unicast.broadcast = 0;
	id.unicast.subfields = 0x28;
	id.unicast.recipient = node_id;
	id.unicast.sender = this->node_id;
	this->send(id, 6, (uint8_t*)&hardware_id);
}

void uCAN_IMPL::registerPongHandler(PongHandler handler) {
	this->pong_handler = handler;
}

void uCAN_IMPL::registerAddressChangeHandler(AddressChangeHandler handler) {
	this->address_change_handler = handler;
}

void uCAN_IMPL::setAddress(uint64_t hardware_id, uint8_t node_id) {
	MessageID id;
	uint8_t body[8];

	id.unicast.priority = UCAN_PRIORITY_HIGH;
	id.unicast.protocol = 0;
	id.unicast.broadcast = 0;
	id.unicast.subfields = 0x18;
	id.unicast.recipient = UCAN_BROADCAST_NODE_ID;
	id.unicast.sender = this->node_id;

	memcpy(body, (uint8_t*)hardware_id, 6);
	body[6] = node_id;
	this->send(id, 6, body);
}
