// Fill out your copyright notice in the Description page of Project Settings.


#include "ULSFunctionLibrary.h"
#include "ULSWirePacket.h"

int32 UULSFunctionLibrary::GetPacketTypeByName(const FString& str)
{
    // Basic connection setup
    if (str == TEXT("ConnectionRequest"))
    {
        return (int32)EWirePacketType::ConnectionRequest;
    }
    else if (str == TEXT("ConnectionResponse"))
    {
        return (int32)EWirePacketType::ConnectionResponse;
    }
    else if (str == TEXT("ConnectionEnd"))
    {
        return (int32)EWirePacketType::ConnectionEnd;
    }
    // Runtime messages
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
    else if (str == TEXT("CreateObject"))
    {
        return (int32)EWirePacketType::CreateObject;
    }
    else if (str == TEXT("DestroyObject"))
    {
        return (int32)EWirePacketType::DestroyObject;
    }
    else if (str == TEXT("RpcCall"))
    {
        return (int32)EWirePacketType::RpcCall;
    }
    else if (str == TEXT("RpcCallResponse"))
    {
        return (int32)EWirePacketType::RpcCallResponse;
    }
    // Custom
    else if (str == TEXT("Custom"))
    {
        return (int32)EWirePacketType::Custom;
    }

    return -1;
}

FString UULSFunctionLibrary::GetPacketNameByType(int32 type)
{
    EWirePacketType etype = (EWirePacketType)type;

    switch (etype)
    {
        // Basic connection setup
        case EWirePacketType::ConnectionRequest: return TEXT("ConnectionRequest");
        case EWirePacketType::ConnectionResponse: return TEXT("ConnectionResponse");
        case EWirePacketType::ConnectionEnd: return TEXT("ConnectionEnd");

        // Runtime messages
        case EWirePacketType::Replication: return TEXT("Replication");
        case EWirePacketType::SpawnActor: return TEXT("SpawnActor");
        case EWirePacketType::DespawnActor: return TEXT("DespawnActor");
        case EWirePacketType::CreateObject: return TEXT("CreateObject");
        case EWirePacketType::DestroyObject: return TEXT("DestroyObject");
        case EWirePacketType::RpcCall: return TEXT("RpcCall");
        case EWirePacketType::RpcCallResponse: return TEXT("RpcCallResponse");

        // Custom
        case EWirePacketType::Custom: return TEXT("Custom");
    }

    return TEXT("");
}
