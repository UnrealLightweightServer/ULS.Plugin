// Fill out your copyright notice in the Description page of Project Settings.


#include "ULSTransport.h"
#include "ULSWirePacket.h"

bool UULSTransport::IsConnected() const
{
	return false;
}

bool UULSTransport::Connect()
{
	return false;
}

void UULSTransport::Disconnect()
{
	//
}

void UULSTransport::SendWirePacket(const UULSWirePacket* packet)
{
	//
}