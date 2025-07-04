/*++
	Copyright (c) DuoWoA authors. All Rights Reserved.

	SPDX-License-Identifier: BSD-3-Clause

Module Name:

	device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.

Environment:

	Kernel-mode Driver Framework

--*/

#include "driver.h"

#ifdef DEBUG
#include "device.tmh"
#endif

NTSTATUS
AW8624HapticsCreateDevice(
	_Inout_ WDFDRIVER Driver,
	_Inout_ PWDFDEVICE_INIT DeviceInit
)
/*++

Routine Description:

	Worker routine called to create a device and its software resources.

Arguments:

	DeviceInit - Pointer to an opaque init structure. Memory for this
					structure will be freed by the framework when the WdfDeviceCreate
					succeeds. So don't access the structure after that point.

Return Value:

	NTSTATUS

--*/
{
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	WDFDEVICE device;
	NTSTATUS status;
	PDEVICE_CONTEXT devContext;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

	status = HwNProcessAddDevicePreDeviceCreate(
		Driver,
		DeviceInit,
		&deviceAttributes
	);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG
		Trace(TRACE_LEVEL_ERROR, TRACE_DRIVER, "HwNProcessAddDevicePreDeviceCreate failed %!STATUS!", status);
#endif
		return status;
	}

	status = WdfDeviceCreate(
		&DeviceInit,
		&deviceAttributes,
		&device
	);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG
		Trace(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDeviceCreate failed %!STATUS!", status);
#endif
		return status;
	}

	status = HwNProcessAddDevicePostDeviceCreate(
		Driver,
		device,
		(LPGUID)&HWN_DEVINTERFACE_VIBRATOR
	);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG
		Trace(TRACE_LEVEL_ERROR, TRACE_DRIVER, "HwNProcessAddDevicePostDeviceCreate failed %!STATUS!", status);
#endif
		return status;
	}

	devContext = DeviceGetContext(device);
	if (devContext == NULL)
	{
#ifdef DEBUG
		Trace(TRACE_LEVEL_ERROR, TRACE_DRIVER, "DeviceGetContext failed");
#endif
		//status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else
	{
		devContext->Device = device;
	}

	return status;
}
