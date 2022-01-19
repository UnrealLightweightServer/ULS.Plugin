// Fill out your copyright notice in the Description page of Project Settings.


#include "ULSFunctionLibrary.h"
#include "ULSWirePacket.h"

int32 UULSFunctionLibrary::GetPacketTypeByName(const FString& str)
{
    if (str == TEXT("WorldJoinRequest"))
    {
        return (int32)EWirePacketType::WorldJoinRequest;
    }
    else if (str == TEXT("WorldLeaveRequest"))
    {
        return (int32)EWirePacketType::WorldLeaveRequest;
    }
    else if (str == TEXT("WorldJoinComplete"))
    {
        return (int32)EWirePacketType::WorldJoinComplete;
    }
    else if (str == TEXT("WorldInvalid"))
    {
        return (int32)EWirePacketType::WorldInvalid;
    }
    else if (str == TEXT("WorldJoinSuccess"))
    {
        return (int32)EWirePacketType::WorldJoinSuccess;
    }
    else if (str == TEXT("MatchLeaveSuccess"))
    {
        return (int32)EWirePacketType::MatchLeaveSuccess;
    }
    else if (str == TEXT("WorldBegin"))
    {
        return (int32)EWirePacketType::WorldBegin;
    }
    else if (str == TEXT("WorldEnd"))
    {
        return (int32)EWirePacketType::WorldEnd;
    }
    else if (str == TEXT("RpcCall"))
    {
        return (int32)EWirePacketType::RpcCall;
    }
    else if (str == TEXT("RpcCallResponse"))
    {
        return (int32)EWirePacketType::RpcCallResponse;
    }
    else if (str == TEXT("Replication"))
    {
        return (int32)EWirePacketType::Replication;
    }
    else if (str == TEXT("SpawnActor"))
    {
        return (int32)EWirePacketType::SpawnActor;
    }
    else if (str == TEXT("DespawnActor"))
    {
        return (int32)EWirePacketType::DespawnActor;
    }
    else if (str == TEXT("SystemMessage"))
    {
        return (int32)EWirePacketType::SystemMessage;
    }
    else if (str == TEXT("ChatMessage"))
    {
        return (int32)EWirePacketType::ChatMessage;
    }

    return -1;
}

FString UULSFunctionLibrary::GetPacketNameByType(int32 type)
{
    EWirePacketType etype = (EWirePacketType)type;

    switch (etype)
    {
        case EWirePacketType::WorldJoinRequest: return TEXT("WorldJoinRequest");
        case EWirePacketType::WorldLeaveRequest: return TEXT("WorldLeaveRequest");
        case EWirePacketType::WorldJoinComplete: return TEXT("WorldJoinComplete");

        case EWirePacketType::WorldInvalid: return TEXT("WorldInvalid");
        case EWirePacketType::WorldJoinSuccess: return TEXT("WorldJoinSuccess");
        case EWirePacketType::MatchLeaveSuccess: return TEXT("MatchLeaveSuccess");
        case EWirePacketType::WorldBegin: return TEXT("WorldBegin");
        case EWirePacketType::WorldEnd: return TEXT("WorldEnd");

        case EWirePacketType::RpcCall: return TEXT("RpcCall");
        case EWirePacketType::RpcCallResponse: return TEXT("RpcCallResponse");

        case EWirePacketType::Replication: return TEXT("Replication");
        case EWirePacketType::SpawnActor: return TEXT("SpawnActor");
        case EWirePacketType::DespawnActor: return TEXT("DespawnActor");

        case EWirePacketType::SystemMessage: return TEXT("SystemMessage");
        case EWirePacketType::ChatMessage: return TEXT("ChatMessage");
    }

    return TEXT("");
}
