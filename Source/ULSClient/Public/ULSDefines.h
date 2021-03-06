#pragma once

#define BEGIN_RPC_BP_EVENTS_FROM_SERVER // BEGIN_RPC_BP_EVENTS_FROM_SERVER
#define END_RPC_BP_EVENTS_FROM_SERVER // END_RPC_BP_EVENTS_FROM_SERVER

#define BEGIN_RPC_BP_EVENTS_TO_SERVER // BEGIN_RPC_BP_EVENTS_TO_SERVER
#define END_RPC_BP_EVENTS_TO_SERVER // END_RPC_BP_EVENTS_TO_SERVER

#define BEGIN_RPC_BP_EVENTS_FROM_SERVER_CALL // BEGIN_RPC_BP_EVENTS_FROM_SERVER_CALL
#define END_RPC_BP_EVENTS_FROM_SERVER_CALL // END_RPC_BP_EVENTS_FROM_SERVER_CALL

#define BEGIN_RPC_BP_EVENTS_TO_SERVER_CALL // BEGIN_RPC_BP_EVENTS_TO_SERVER_CALL
#define END_RPC_BP_EVENTS_TO_SERVER_CALL // END_RPC_BP_EVENTS_TO_SERVER_CALL

#define CALL_RPC_FUNCTION_0P(name) if (methodName == TEXT(#name)) { name(); }
#define CALL_RPC_FUNCTION_1P(name, r1, p1) if (methodName == TEXT(#name)) { name(dataField->r1(#p1)); }
#define CALL_RPC_FUNCTION_2P(name, r1, p1, r2, p2) if (methodName == TEXT(#name)) { name(dataField->r1(#p1), dataField->r2(#p2)); }
