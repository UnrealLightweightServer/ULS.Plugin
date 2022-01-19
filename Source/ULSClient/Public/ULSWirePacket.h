// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ULSWirePacket.generated.h"

enum EWirePacketType : int
{
    WorldJoinRequest = 0,           // Sent by player. Request to join a match. Returns WorldInvalid or WorldJoinSuccess
    WorldLeaveRequest = 1,          // Sent by player. Request to leave match. Returns WorldLeaveSuccess
    WorldJoinComplete = 2,          // Sent by player. Player has entered world and is ready for start.

    WorldInvalid = 10,              // Sent by server, if WorldJoinRequest is invalid
    WorldJoinSuccess = 11,          // Sent by server, if WorldJoinRequest is valid
    MatchLeaveSuccess = 12,         // Sent by server, if WorldLeaveRequest is valid
    WorldBegin = 13,                // Sent by server. World enters running state
    WorldEnd = 14,                  // Sent by server. World has shut down

    Replication = 110,              // Replication message. Sent by the server only.
    SpawnActor = 111,               // Spawns a new network actor on the client. Sent by the server only.
    DespawnActor = 112,             // Despawns a network actor on the client. Sent by the server only.
    CreateObject = 113,             // Creates a new UObject based object on the client. Sent by the server only.
    DestroyObject = 114,            // Destroy a UObject based object on the client. Sent by the server only.
    RpcCall = 115,                  // Serialized RpcCall. Can be sent by both parties.
    RpcCallResponse = 116,          // Serialized response to an RpcCall. Can be sent by both parties.

    SystemMessage = 200,
    ChatMessage,
};

/**
 * 
 */
UCLASS(Blueprintable)
class ULSCLIENT_API UULSWirePacket : public UObject
{
	GENERATED_BODY()
	
public:
    UULSWirePacket();

    UPROPERTY(BlueprintReadWrite)
        int32 PacketType;

    UPROPERTY(BlueprintReadWrite)
        TArray<uint8> Payload;

    UFUNCTION()
        bool ParseFromBytes(const TArray<uint8>& bytes);

    UFUNCTION()
        TArray<uint8> SerializeToBytes() const;

    UFUNCTION()
        int8 ReadInt8(int index, int& advancedPosition) const;

    UFUNCTION()
        int16 ReadInt16(int index, int& advancedPosition) const;

    UFUNCTION()
        int32 ReadInt32(int index, int& advancedPosition) const;

    UFUNCTION()
        float ReadFloat32(int index, int& advancedPosition) const;

    UFUNCTION()
        int64 ReadInt64(int index, int& advancedPosition) const;

    UFUNCTION()
        FString ReadString(int index, int& advancedPosition) const;

    UFUNCTION()
        void PutInt8(int8 value, int index, int& advancedPosition);
    UFUNCTION()
        void PutInt16(int16 value, int index, int& advancedPosition);
    UFUNCTION()
        void PutUInt16(uint16 value, int index, int& advancedPosition);
    UFUNCTION()
        void PutInt32(int32 value, int index, int& advancedPosition);
    UFUNCTION()
        void PutFloat32(float value, int index, int& advancedPosition);
    UFUNCTION()
        void PutUInt32(uint32 value, int index, int& advancedPosition);
    UFUNCTION()
        void PutInt64(int64 value, int index, int& advancedPosition);
    UFUNCTION()
        void PutUInt64(uint64 value, int index, int& advancedPosition);
    UFUNCTION()
        void PutString(FString value, int index, int& advancedPosition);
    UFUNCTION()
        void PutArray(TArray<uint8> bytes, int index, int& advancedPosition);

private:
    static inline uint32 EndianSwap(uint32 value) { return (value << 24) | ((value & 0xff00) << 8) | ((value >> 8) & 0xff00) | (value >> 24); }
    static inline int32 EndianSwap(int32 value) { return int32(EndianSwap(uint32(value))); }
};
