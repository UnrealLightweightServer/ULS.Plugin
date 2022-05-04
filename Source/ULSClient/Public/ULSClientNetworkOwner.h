// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ULSDefines.h"
#include "UObject/NoExportTypes.h"
#include "ULSClientNetworkOwner.generated.h"

enum EReplicatedFieldType : int8
{
	Reference = 0,
	PrimitiveInt = 1,
	String = 2,
	Vector3 = 3,
    PrimitiveFloat = 4,
};

/**
 * 
 */
UCLASS(Blueprintable)
class ULSCLIENT_API UULSClientNetworkOwner : public UObject
{
    GENERATED_BODY()

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConnectionEvent, bool, bSuccess);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDisconnectionEvent, int32, statusCode, bool, bWasClean);
	
public:
	UPROPERTY(BlueprintReadWrite)
		class UULSTransport* Transport;

    UPROPERTY(BlueprintAssignable)
        FConnectionEvent OnConnectionEvent;

    UPROPERTY(BlueprintAssignable)
        FDisconnectionEvent OnDisconnectionEvent;

    void OnConnected(bool success, const FString& errorMessage);

    void OnDisconnected(int32 StatusCode, const FString& Reason, bool bWasClean);

	void HandleWirePacket(const UULSWirePacket* packet);

	UFUNCTION(BlueprintImplementableEvent, Category = WebsocketMasterServer)
		void OnReceivePacket(const UULSWirePacket* packet);

	UFUNCTION(BlueprintCallable)
		AActor* FindActorByUniqueId(int64 uniqueId) const;

    UFUNCTION(BlueprintCallable)
        UObject* FindObjectRefByUniqueId(int64 uniqueId) const;

protected:
    /*
    * Fills the connection request packet with user-specific data.
    * 
    * The default implementation writes the UniqueNetId as an FString to the packet
    */
    virtual void BuildConnectionRequestPacket(UULSWirePacket* packet);

    /*
    * Process the response packet containing user-specific data.
    * 
    * Return true if the connection should be accepted
    * 
    * The default implementation reads a single byte from the packet and interprets
    * that as a bool.
    */
    virtual bool ProcessConnectionResponsePacket(const UULSWirePacket* packet);

    virtual void ProcessHandleRpcPacket(const UULSWirePacket* packet, int packetReadPosition, UObject* existingActor, const FString& methodName,
        const FString& returnType, const int32 numberOfParameters);

    virtual void HandleConnectionResponseMessage(const UULSWirePacket* packet);

    virtual void HandleConnectionEndMessage(const UULSWirePacket* packet);

private:
    void HandleRpcPacket(const UULSWirePacket* packet);

    void HandleRpcResponsePacket(const UULSWirePacket* packet);

    void HandleSpawnActorMessage(const UULSWirePacket* packet);

    void HandleDespawnActorMessage(const UULSWirePacket* packet);

    void HandleCreateObjectMessage(const UULSWirePacket* packet);

    void HandleDestroyObjectMessage(const UULSWirePacket* packet);

    void HandleReplicationMessage(const UULSWirePacket* packet);

    void ReadProperties(const UULSWirePacket* packet, const UClass* theClass, int numProperties, UObject* targetObject, int position, bool callOnRep = false);

    void ReadProperties(const UULSWirePacket* packet, const UClass* theClass, int numProperties, void* targetObject, int position, bool callOnRep = false);

private:
	UPROPERTY()
		TMap<int64, UObject*> objectMap;
	UPROPERTY()
		TMap<UObject*, int64> uniqueIdLookup;

protected:
    UObject* FindObjectRef(int64 uniqueId) const;

	int64 FindUniqueId(const UObject* actor) const;

	AActor* SpawnNetworkActor(int64 uniqueId, UClass* cls);

    UObject* CreateNetworkObject(int64 uniqueId, UClass* cls);
	
    UFUNCTION()
        UObject* DeserializeRef(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        int8 DeserializeInt8(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        int16 DeserializeInt16(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        int32 DeserializeInt32(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        int64 DeserializeInt64(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        float DeserializeFloat32(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        double DeserializeFloat64(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        bool DeserializeBool(const UULSWirePacket* packet, int index, int& advancedPosition, int boolSize) const;
    UFUNCTION()
        FString DeserializeString(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        FVector DeserializeVector(const UULSWirePacket* packet, int index, int& advancedPosition) const;

    UFUNCTION()
        UObject* DeserializeRefParameter(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        int16 DeserializeInt16Parameter(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        int32 DeserializeInt32Parameter(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        int64 DeserializeInt64Parameter(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        float DeserializeFloat32Parameter(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        double DeserializeFloat64Parameter(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        bool DeserializeBoolParameter(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        FString DeserializeStringParameter(const UULSWirePacket* packet, int index, int& advancedPosition) const;
    UFUNCTION()
        FVector DeserializeVectorParameter(const UULSWirePacket* packet, int index, int& advancedPosition) const;

    UFUNCTION()
        void SerializeRefParameter(UULSWirePacket* packet, FString fieldname, const UObject* value, int index, int& advancedPosition) const;
    UFUNCTION()
        void SerializeInt16Parameter(UULSWirePacket* packet, FString fieldname, int16 value, int index, int& advancedPosition) const;
    UFUNCTION()
        void SerializeInt32Parameter(UULSWirePacket* packet, FString fieldname, int32 value, int index, int& advancedPosition) const;
    UFUNCTION()
        void SerializeInt64Parameter(UULSWirePacket* packet, FString fieldname, int64 value, int index, int& advancedPosition) const;
    UFUNCTION()
        void SerializeFloat32Parameter(UULSWirePacket* packet, FString fieldname, float value, int index, int& advancedPosition) const;
    UFUNCTION()
        void SerializeFloat64Parameter(UULSWirePacket* packet, FString fieldname, double value, int index, int& advancedPosition) const;
    UFUNCTION()
        void SerializeBoolParameter(UULSWirePacket* packet, FString fieldname, bool value, int index, int& advancedPosition) const;
    UFUNCTION()
        void SerializeStringParameter(UULSWirePacket* packet, FString fieldname, FString value, int index, int& advancedPosition) const;
    UFUNCTION()
        void SerializeVectorParameter(UULSWirePacket* packet, FString fieldname, FVector value, int index, int& advancedPosition) const;

    UFUNCTION()
        int32 GetSerializeRefParameterSize(FString fieldname) const;
    UFUNCTION()
        int32 GetSerializeInt16ParameterSize(FString fieldname) const;
    UFUNCTION()
        int32 GetSerializeInt32ParameterSize(FString fieldname) const;
    UFUNCTION()
        int32 GetSerializeInt64ParameterSize(FString fieldname) const;
    UFUNCTION()
        int32 GetSerializeFloat32ParameterSize(FString fieldname) const;
    UFUNCTION()
        int32 GetSerializeFloat64ParameterSize(FString fieldname) const;
    UFUNCTION()
        int32 GetSerializeBoolParameterSize(FString fieldname) const;
    UFUNCTION()
        int32 GetSerializeStringParameterSize(FString fieldname, int stringLen) const;
    UFUNCTION()
        int32 GetSerializeVectorParameterSize(FString fieldname) const;
};
