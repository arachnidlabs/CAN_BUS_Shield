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

typedef union {
  struct {
    unsigned int priority : 2;
    unsigned int protocol : 4;
    unsigned int broadcast : 1;
    unsigned int subfields : 6;
    unsigned int recipient : 8;
    unsigned int sender : 8;
  } unicast;

  struct {
    unsigned int priority : 2;
    unsigned int protocol : 4;
    unsigned int broadcast : 1;
    unsigned int subfields : 14;
    unsigned int sender : 8;
  } broadcast;

  uint32_t raw;
} MessageID;


typedef void (*PongHandler)(uint64_t hardware_id, uint8_t node_id);
typedef void (*AddressChangeHandler)(uint8_t node_id);

class uCAN_IMPL {
private:
    uint8_t node_id;
    uint64_t hardware_id;
    PongHandler pong_handler;
    AddressChangeHandler address_change_handler;

protected:
    void handleYARP(MessageID id, uint8_t len, uint8_t *message);

public:
    uint8_t begin(uint64_t hardware_id, uint8_t node_id);
    void send(MessageID id, uint8_t len, uint8_t *message);
    void receive();

    // YARP methods
    void ping(uint8_t node_id);
    void ping(uint8_t node_id, uint8_t priority);
    void pingHardwareID(uint64_t hardware_id);
    void pingHardwareID(uint64_t hardware_id, uint8_t priority);
    void registerPongHandler(PongHandler handler);
    void registerAddressChangeHandler(AddressChangeHandler handler);
    void setAddress(uint64_t hardware_id, uint8_t node_id);
};
extern uCAN_IMPL uCAN;
