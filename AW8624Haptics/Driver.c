/*++
	Copyright (c) DuoWoA authors. All Rights Reserved.

	SPDX-License-Identifier: BSD-3-Clause

Module Name:

	driver.c

Abstract:

	This file contains the driver entry points and callbacks.

Environment:

	Kernel-mode Driver Framework

--*/

#include "driver.h"

#ifdef DEBUG
#include "driver.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, AW8624HapticsEvtDriverUnload)
#pragma alloc_text (PAGE, AW8624HapticsEvtDriverContextCleanup)
#endif

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
/*++

Routine Description:
	DriverEntry initializes the driver and is the first routine called by the
	system after the driver is loaded. DriverEntry specifies the other entry
	points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

	DriverObject - represents the instance of the function driver that is loaded
	into memory. DriverEntry must initialize members of DriverObject before it
	returns to the caller. DriverObject is allocated by the system before the
	driver is loaded, and it is released by the system after the system unloads
	the function driver from memory.

	RegistryPath - represents the driver specific path in the Registry.
	The function driver can use the path to store driver related data between
	reboots. The path does not store hardware instance specific data.

Return Value:

	STATUS_SUCCESS if successful,
	STATUS_UNSUCCESSFUL otherwise.

--*/
{
	WDF_DRIVER_CONFIG config;
	WDFDRIVER Driver;
	NTSTATUS status;
	WDF_OBJECT_ATTRIBUTES attributes;
	HWN_CLIENT_REGISTRATION_PACKET regPacket;

#ifdef DEBUG
	//
	// Initialize WPP Tracing
	//
	WPP_INIT_TRACING(DriverObject, RegistryPath);

	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
#endif

	WDF_DRIVER_CONFIG_INIT(&config, AW8624HapticsEvtDeviceAdd);
	config.EvtDriverUnload = AW8624HapticsEvtDriverUnload;
	config.DriverPoolTag = HAPTICS_POOL_TAG;

	//
	// Register a cleanup callback so that we can call WPP_CLEANUP when
	// the framework driver object is deleted during driver unload.
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.EvtCleanupCallback = AW8624HapticsEvtDriverContextCleanup;

	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		&attributes,
		&config,
		&Driver
	);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG
		Trace(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
		WPP_CLEANUP(DriverObject);
#endif
		return status;
	}

	regPacket.Version = HWN_CLIENT_VERSION;
	regPacket.Size = sizeof(HWN_CLIENT_REGISTRATION_PACKET);
	regPacket.DeviceContextSize = sizeof(DEVICE_CONTEXT);

	regPacket.ClientInitializeDevice = AW8624HapticsInitializeDevice;
	regPacket.ClientUnInitializeDevice = AW8624HapticsUnInitializeDevice;
	regPacket.ClientQueryDeviceInformation = AW8624HapticsQueryDeviceInformation;
	regPacket.ClientStartDevice = AW8624HapticsStartDevice;
	regPacket.ClientStopDevice = AW8624HapticsStopDevice;
	regPacket.ClientSetHwNState = AW8624HapticsSetState;
	regPacket.ClientGetHwNState = AW8624HapticsGetState;

	status = HwNRegisterClient(
		Driver,
		&regPacket,
		RegistryPath
	);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG
		Trace(TRACE_LEVEL_ERROR, TRACE_DRIVER, "HwNRegisterClient failed %!STATUS!", status);
		WPP_CLEANUP(DriverObject);
#endif
		return status;
	}

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");
#endif

	return status;
}

NTSTATUS
AW8624HapticsEvtDeviceAdd(
	_In_    WDFDRIVER       Driver,
	_Inout_ PWDFDEVICE_INIT DeviceInit
)
/*++
Routine Description:

	EvtDeviceAdd is called by the framework in response to AddDevice
	call from the PnP manager. We create and initialize a device object to
	represent a new instance of the device.

Arguments:

	Driver - Handle to a framework driver object created in DriverEntry

	DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

	NTSTATUS

--*/
{
	NTSTATUS status;

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
#endif

	status = AW8624HapticsCreateDevice(Driver, DeviceInit);

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");
#endif

	return status;
}

VOID
AW8624HapticsEvtDriverContextCleanup(
	_In_ WDFOBJECT DriverObject
)
/*++
Routine Description:

	Free all the resources allocated in DriverEntry.

Arguments:

	DriverObject - handle to a WDF Driver object.

Return Value:

	VOID.

--*/
{
	UNREFERENCED_PARAMETER(DriverObject);

	PAGED_CODE();

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

	//
	// Stop WPP Tracing
	//
	WPP_CLEANUP(NULL);
#endif
}

VOID
AW8624HapticsEvtDriverUnload(
	IN WDFDRIVER Driver
)
/*++
Routine Description:

	Free all the resources allocated in DriverEntry.

Arguments:

	Driver - handle to a WDF Driver object.

Return Value:

	VOID.

--*/
{
	PAGED_CODE();

	//
	// Unregister HwnHaptics client driver here
	//
	HwNUnregisterClient(Driver);

#ifdef DEBUG
	//
	// Stop WPP Tracing
	//
	WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));
#endif

	return;
}