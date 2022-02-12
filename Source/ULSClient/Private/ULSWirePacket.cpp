// Fill out your copyright notice in the Description page of Project Settings.


#include "ULSWirePacket.h"

UULSWirePacket::UULSWirePacket()
{
	PacketType = 0;
	Payload = TArray<uint8>();
}

bool UULSWirePacket::ParseFromBytes(const TArray<uint8>& bytes)
{
	if (bytes.Num() < 4)
	{
		return false;
	}

	PacketType = *(int32*)bytes.GetData();

	int payloadLength = bytes.Num() - 4;
	Payload = TArray<uint8>(bytes.GetData() + 4, payloadLength);

	return true;
}

TArray<uint8> UULSWirePacket::SerializeToBytes() const
{
	int32 packetType = PacketType;

	TArray<uint8> result;
	result.Append((uint8*)&packetType, sizeof(int32));
	result.Append(Payload);
	return result;
}

int8 UULSWirePacket::ReadInt8(int index, int& advancedPosition) const
{
	if (Payload.Num() < (index + sizeof(int8)))
	{
		return 0;
	}

	advancedPosition += sizeof(int8);
	const uint8* dataPtr = Payload.GetData() + index;
	return *(int8*)dataPtr;
}

int16 UULSWirePacket::ReadInt16(int index, int& advancedPosition) const
{
	if (Payload.Num() < (index + sizeof(int16)))
	{
		return 0;
	}

	advancedPosition += sizeof(int16);
	const uint8* dataPtr = Payload.GetData() + index;
	return *(int16*)dataPtr;
}


int32 UULSWirePacket::ReadInt32(int index, int& advancedPosition) const
{
	if (Payload.Num() < (index + sizeof(int32)))
	{
		return 0;
	}

	advancedPosition += sizeof(int32);
	const uint8* dataPtr = Payload.GetData() + index;
	return *(int32*)dataPtr;
}

float UULSWirePacket::ReadFloat32(int index, int& advancedPosition) const
{
	if (Payload.Num() < (index + sizeof(float)))
	{
		return 0;
	}

	advancedPosition += sizeof(float);
	const uint8* dataPtr = Payload.GetData() + index;
	return *(float*)dataPtr;
}

int64 UULSWirePacket::ReadInt64(int index, int& advancedPosition) const
{
	if (Payload.Num() < (index + sizeof(int64)))
	{
		return 0;
	}

	advancedPosition += sizeof(int64);
	const uint8* dataPtr = Payload.GetData() + index;
	return *(int32*)dataPtr;
}

FString UULSWirePacket::ReadString(int index, int& advancedPosition) const
{
	if (Payload.Num() < (index + sizeof(int)))
	{
		return FString();
	}

	const uint8* dataPtr = Payload.GetData() + index;
	int len = *(int32*)dataPtr;
	advancedPosition += sizeof(int32);
	if (Payload.Num() < (index + sizeof(int) + len))
	{
		return FString();
	}
	advancedPosition += len;
	dataPtr += sizeof(int32);
	return FString(len, (UTF8CHAR*)dataPtr);
}

void UULSWirePacket::PutInt8(int8 value, int index, int& advancedPosition)
{
	if (Payload.Num() < (index + 1))
	{
		return;
	}

	Payload[index] = value;
	advancedPosition = index + sizeof(value);
}

void UULSWirePacket::PutInt16(int16 value, int index, int& advancedPosition)
{
	if (Payload.Num() < (index + sizeof(value)))
	{
		return;
	}

	int16* ptr = (int16*)&Payload[index];
	*ptr = value;
	advancedPosition = index + sizeof(value);
}

void UULSWirePacket::PutUInt16(uint16 value, int index, int& advancedPosition)
{
	if (Payload.Num() < (index + sizeof(value)))
	{
		return;
	}

	uint16* ptr = (uint16*)&Payload[index];
	*ptr = value;
	advancedPosition = index + sizeof(value);
}

void UULSWirePacket::PutInt32(int32 value, int index, int& advancedPosition)
{
	if (Payload.Num() < (index + sizeof(value)))
	{
		return;
	}

	int32* ptr = (int32*)&Payload[index];
	*ptr = value;
	advancedPosition = index + sizeof(value);
}

void UULSWirePacket::PutFloat32(float value, int index, int& advancedPosition)
{
	if (Payload.Num() < (index + sizeof(value)))
	{
		return;
	}

	float* ptr = (float*)&Payload[index];
	*ptr = value;
	advancedPosition = index + sizeof(value);
}

void UULSWirePacket::PutUInt32(uint32 value, int index, int& advancedPosition)
{
	if (Payload.Num() < (index + sizeof(value)))
	{
		return;
	}

	uint32* ptr = (uint32*)&Payload[index];
	*ptr = value;
	advancedPosition = index + sizeof(value);
}

void UULSWirePacket::PutInt64(int64 value, int index, int& advancedPosition)
{
	if (Payload.Num() < (index + sizeof(value)))
	{
		return;
	}

	int64* ptr = (int64*)&Payload[index];
	*ptr = value;
	advancedPosition = index + sizeof(value);
}

void UULSWirePacket::PutUInt64(uint64 value, int index, int& advancedPosition)
{
	if (Payload.Num() < (index + sizeof(value)))
	{
		return;
	}

	uint64* ptr = (uint64*)&Payload[index];
	*ptr = value;
	advancedPosition = index + sizeof(value);
}

void UULSWirePacket::PutString(FString value, int index, int& advancedPosition)
{
	TArray<uint8> bytes;
	FTCHARToUTF8 Src = FTCHARToUTF8(value.GetCharArray().GetData());
	bytes.Append((uint8*)Src.Get(), Src.Length());

	if (Payload.Num() < (index + sizeof(value.Len()) + bytes.Num()))
	{
		return;
	}
	
	// This is likely broken logic
	// TODO: Fixme

	// Length

	int32* ptr = (int32*)&Payload[index];
	*ptr = bytes.Num();
	advancedPosition += sizeof(int32);

	// String chars
	uint8* bptr = (uint8*)&Payload[index + 4];
	memcpy(bptr, bytes.GetData(), bytes.Num());
	advancedPosition += bytes.Num();
}

void UULSWirePacket::PutArray(TArray<uint8> bytes, int index, int& advancedPosition)
{
	if (Payload.Num() < (index + bytes.Num()))
	{
		return;
	}

	uint8* dataPtr = bytes.GetData();
	if (dataPtr == nullptr)
	{
		return;
	}

	uint8* ptr = (uint8*)&Payload[index];
	memcpy(ptr, dataPtr, bytes.Num());
}