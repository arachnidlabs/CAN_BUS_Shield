/*
  uCAN.h
  2013 Copyright (c) Arachnid Labs Ltd.  All right reserved.

  Author:Nick Johnson
  2013-11-20
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-
  1301  USA
*/
#include "mcp_can.h"

#define UCAN_BROADCAST_NODE_ID 0xFF
#define UCAN_PRIORITY_EMERGENCY 0
#define UCAN_PRIORITY_HIGH 1
#define UCAN_PRIORITY_NORMAL 2
#define UCAN_PRIORITY_LOW 3
#define UCAN_NODE_NOT_FOUND -1

typedef union {
  struct {
    unsigned int sender : 8;
    unsigned int recipient : 8;
    unsigned int subfields : 6;
    unsigned int broadcast : 1;
    unsigned int protocol : 4;
    unsigned int priority : 2;
  } unicast;

  struct {
    unsigned int sender : 8;
    unsigned int subfields : 14;
    unsigned int broadcast : 1;
    unsigned int protocol : 4;
    unsigned int priority : 2;
  } broadcast;

  uint32_t raw;
} MessageID;

typedef struct {
  uint8_t address[6];
} HardwareID;

typedef struct {
  MessageID id;
  uint8_t len;
  uint8_t body[8];
} uCANMessage;

typedef int16_t NodeAddress;

typedef void (*PongHandler)(HardwareID hardware_id, uint8_t node_id);
typedef void (*AddressChangeHandler)(uint8_t node_id);

class uCAN_IMPL {
private:
    uint8_t node_id;
    HardwareID hardware_id;
    AddressChangeHandler address_change_handler;
    uint16_t timeout;

    bool tryReceive(uCANMessage *message);

protected:
    MessageID makeUnicastMessageID(uint8_t priority, uint8_t protocol, uint8_t subfields, uint8_t recipient);
    MessageID makeBroadcastMessageID(uint8_t priority, uint8_t protocol, uint16_t subfields);
    bool handleYARP(uCANMessage *message);
    void send(MessageID id, uint8_t len, uint8_t *message);

public:
    uCAN_IMPL();
    uint8_t begin(HardwareID hardware_id, uint8_t node_id);
    bool receive();
    void setTimeout(uint16_t timeout);

    // YARP methods
    NodeAddress getNodeFromNodeID(uint8_t node_id);
    NodeAddress getNodeFromHardwareID(HardwareID hardware_id);
    bool ping(NodeAddress node);
    bool ping(NodeAddress node, HardwareID *hardware_id);
    void registerAddressChangeHandler(AddressChangeHandler handler);
    void setAddress(HardwareID hardware_id, uint8_t node_id);
};
extern uCAN_IMPL uCAN;
