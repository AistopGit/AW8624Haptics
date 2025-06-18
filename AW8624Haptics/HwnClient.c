/*++
	Copyright (c) DuoWoA authors. All Rights Reserved.

	SPDX-License-Identifier: BSD-3-Clause

Module Name:

	HwnClient.c - Handling HwnClx Requests

Abstract:

Environment:

	Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "spb.h"
#include "controller.h"
#include "hwndefs.h"

#ifdef DEBUG
#include "hwnclient.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, AW8624HapticsInitializeDevice)
#pragma alloc_text (PAGE, AW8624HapticsUnInitializeDevice)
#pragma alloc_text (PAGE, AW8624HapticsQueryDeviceInformation)
#pragma alloc_text (PAGE, AW8624HapticsStartDevice)
#pragma alloc_text (PAGE, AW8624HapticsStopDevice)
#pragma alloc_text (PAGE, AW8624HapticsSetState)
#pragma alloc_text (PAGE, AW8624HapticsGetState)
#endif

PDEVICE_CONTEXT globalContext = NULL;

BOOLEAN
AW8624HapticsEvtInterruptIsr(
	WDFINTERRUPT Interrupt,
	ULONG MessageID
)
{
	UNREFERENCED_PARAMETER(Interrupt);
	UNREFERENCED_PARAMETER(MessageID);

	return TRUE;
}

NTSTATUS
AW8624HapticsInitializeDevice(
	__in WDFDEVICE Device,
	__in PVOID Context,
	__in WDFCMRESLIST ResourcesRaw,
	__in WDFCMRESLIST ResourcesTranslated
)
{
	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR res, resRaw;
	ULONG resourceCount;
	ULONG i;
	BOOLEAN I2CDetected = FALSE;
	BOOLEAN InterruptDetected = FALSE;
	WDF_INTERRUPT_CONFIG interruptConfig;

	PAGED_CODE();

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
#endif

	PDEVICE_CONTEXT devContext = (PDEVICE_CONTEXT)Context;
	
	globalContext = devContext;
	
	devContext->Device = Device;

	//
	// Get the resouce hub connection ID for our I2C driver
	//
	resourceCount = WdfCmResourceListGetCount(ResourcesTranslated);

	for (i = 0; i < resourceCount; i++)
	{
		res = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);
		resRaw = WdfCmResourceListGetDescriptor(ResourcesRaw, i);

		if (res->Type == CmResourceTypeConnection &&
			res->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
			res->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C)
		{
			devContext->I2CContext.I2cResHubId.LowPart =
				res->u.Connection.IdLowPart;
			devContext->I2CContext.I2cResHubId.HighPart =
				res->u.Connection.IdHighPart;

			I2CDetected = TRUE;
		}
		else if (res->Type == CmResourceTypeInterrupt)
		{
			WDF_INTERRUPT_CONFIG_INIT(
				&interruptConfig,
				AW8624HapticsEvtInterruptIsr,
				NULL
			);

			interruptConfig.InterruptTranslated = res;
			interruptConfig.InterruptRaw = resRaw;
			interruptConfig.PassiveHandling = TRUE;

			status = WdfInterruptCreate(
				Device,
				&interruptConfig,
				WDF_NO_OBJECT_ATTRIBUTES,
				&devContext->InterruptObject
			);

			if (!NT_SUCCESS(status))
			{
#ifdef DEBUG
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_INIT,
					"WdfInterruptCreate failed %!STATUS!",
					status
				);
#endif

				goto exit;
			}

			InterruptDetected = TRUE;
		}
	}

	if (I2CDetected && InterruptDetected)
	{
		status = STATUS_SUCCESS;
	}

	if (!NT_SUCCESS(status))
	{
#ifdef DEBUG
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error finding CmResourceTypeConnection resource - %!STATUS!",
			status);
#endif
		goto exit;
	}

	//
	// Initialize Spb so the driver can issue reads/writes
	//
	status = SpbTargetInitialize(Device, &devContext->I2CContext);

	if (!NT_SUCCESS(status))
	{
#ifdef DEBUG
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error in Spb initialization - %!STATUS!",
			status);
#endif
		goto exit;
	}

	status = AW8624Initialize(devContext);

	if (!NT_SUCCESS(status))
	{
#ifdef DEBUG
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error in AW8624 initialization - %!STATUS!",
			status);
#endif
		goto exit;
	}

	devContext->NumberOfHapticsDevices = 1;

exit:
	return status;
}

NTSTATUS
AW8624HapticsUnInitializeDevice(
	__in WDFDEVICE Device,
	__in PVOID Context
)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
#endif

	PAW8624_HAPTICS_CURRENT_STATE currentState = NULL;
	PAW8624_HAPTICS_CURRENT_STATE stateToFree = NULL;

	PDEVICE_CONTEXT devContext = (PDEVICE_CONTEXT)Context;

	currentState = devContext->CurrentStates;

	while (currentState != NULL)
	{
		stateToFree = currentState;
		currentState = currentState->NextState;
		ExFreePoolWithTag(stateToFree, HAPTICS_POOL_TAG);
	}

	SpbTargetDeinitialize(Device, &devContext->I2CContext);

	return status;
}

NTSTATUS
AW8624HapticsQueryDeviceInformation(
	__in PVOID Context,
	__out PCLIENT_DEVICE_INFORMATION Information
)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();

	PDEVICE_CONTEXT devContext = (PDEVICE_CONTEXT)Context;

#ifdef DEBUG
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"%!FUNC! Entry");
#endif

	Information->Version = HWN_DEVICE_INFORMATION_VERSION;
	Information->Size = sizeof(CLIENT_DEVICE_INFORMATION);

	Information->TotalHwNs = devContext->NumberOfHapticsDevices;

#ifdef DEBUG
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"%!FUNC! Exit");
#endif

	return status;
}

NTSTATUS
AW8624HapticsStartDevice(
	__in PVOID Context
)
{
	UNREFERENCED_PARAMETER(Context);
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
#endif

	return status;
}

NTSTATUS
AW8624HapticsStopDevice(
	__in PVOID Context
)
{
	UNREFERENCED_PARAMETER(Context);
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
#endif

	return status;
}

#define NUMBER_OF_HWN_DEVICES(x) (x - HWN_HEADER_SIZE) / HWN_SETTINGS_SIZE
#define EXTRA_BYTES_AFTER_HWN_DEVICES(x) ((x - HWN_HEADER_SIZE) % HWN_SETTINGS_SIZE)

NTSTATUS
AW8624HapticsSetState(
	__in PVOID Context,
	__in_bcount(BufferLength) PVOID Buffer,
	__in ULONG BufferLength,
	__out PULONG BytesWritten
)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();

#ifdef DEBUG
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"%!FUNC! Entry");
#endif

	if (Context == NULL || Buffer == NULL || BytesWritten == NULL)
	{
#ifdef DEBUG
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INIT,
			"Invalid buffers");
#endif

		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	if (BufferLength <= HWN_HEADER_SIZE || EXTRA_BYTES_AFTER_HWN_DEVICES(BufferLength) != 0)
	{
#ifdef DEBUG
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INIT,
			"Invalid Output buffer sizes");
#endif

		status = STATUS_INVALID_BUFFER_SIZE;
		goto exit;
	}

	PDEVICE_CONTEXT devContext = (PDEVICE_CONTEXT)Context;
	PHWN_HEADER hwnHeader = (PHWN_HEADER)Buffer;
	ULONG NumberOfHwnDevicesInBuffer = 1;
	ULONG i;

	*BytesWritten = 0;

	NumberOfHwnDevicesInBuffer = NUMBER_OF_HWN_DEVICES(BufferLength);

	for (i = 0; i < NumberOfHwnDevicesInBuffer; i++)
	{
		// Set device settings here
		status = AW8624HapticsSetDevice(Context, &(hwnHeader->HwNSettingsInfo[i]));
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}

		// Backup device settings somewhere to retrieve them later
		status = AW8624HapticsSetCurrentDeviceState(devContext, &(hwnHeader->HwNSettingsInfo[i]), HWN_SETTINGS_SIZE);
		if (!NT_SUCCESS(status))
		{
			goto exit;
		}
	}

	*BytesWritten = BufferLength;

exit:

#ifdef DEBUG
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"%!FUNC! Exit");
#endif

	return status;
}

NTSTATUS
AW8624HapticsGetState(
	__in PVOID Context,
	__out_bcount(OutputBufferLength) PVOID OutputBuffer,
	__in ULONG OutputBufferLength,
	__in_bcount(InputBufferLength) PVOID InputBuffer,
	__in ULONG InputBufferLength,
	__out PULONG BytesRead
)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();

#ifdef DEBUG
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"%!FUNC! Entry");
#endif

	if (Context == NULL || OutputBuffer == NULL || BytesRead == NULL)
	{
#ifdef DEBUG
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INIT,
			"Invalid buffers");
#endif

		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	if (OutputBufferLength <= HWN_HEADER_SIZE || EXTRA_BYTES_AFTER_HWN_DEVICES(OutputBufferLength) != 0)
	{
#ifdef DEBUG
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INIT,
			"Invalid Output buffer sizes");
#endif

		status = STATUS_INVALID_BUFFER_SIZE;
		goto exit;
	}

	PDEVICE_CONTEXT devContext = (PDEVICE_CONTEXT)Context;
	PHWN_HEADER hwnHeader;
	ULONG NumberOfHwnDevicesInBuffer = 1;
	ULONG i;

	*BytesRead = 0;

	if (InputBuffer == NULL)
	{
		// Send information about all Haptics devices

		hwnHeader = (PHWN_HEADER)OutputBuffer;

		hwnHeader->HwNPayloadSize = HWN_HEADER_SIZE + devContext->NumberOfHapticsDevices * HWN_SETTINGS_SIZE;
		hwnHeader->HwNPayloadVersion = 1;
		hwnHeader->HwNRequests = devContext->NumberOfHapticsDevices;

		NumberOfHwnDevicesInBuffer = NUMBER_OF_HWN_DEVICES(OutputBufferLength);

		for (i = 0; i < NumberOfHwnDevicesInBuffer; i++)
		{
			hwnHeader->HwNSettingsInfo[i].HwNId = i;

			// Set other settings values into the buffer here.
			status = AW8624HapticsGetCurrentDeviceState(devContext, &hwnHeader->HwNSettingsInfo[i], HWN_SETTINGS_SIZE);
			if (!NT_SUCCESS(status))
			{
				goto exit;
			}
		}

		*BytesRead = OutputBufferLength;
	}
	else
	{
		// Send information about a specific Haptics device

		if (InputBufferLength <= HWN_HEADER_SIZE || EXTRA_BYTES_AFTER_HWN_DEVICES(InputBufferLength) != 0)
		{
#ifdef DEBUG
			Trace(
				TRACE_LEVEL_INFORMATION,
				TRACE_INIT,
				"Invalid Input buffer sizes");
#endif

			status = STATUS_INVALID_BUFFER_SIZE;
			goto exit;
		}

		hwnHeader = (PHWN_HEADER)InputBuffer;

		NumberOfHwnDevicesInBuffer = NUMBER_OF_HWN_DEVICES(InputBufferLength);

		for (i = 0; i < NumberOfHwnDevicesInBuffer; i++)
		{
			// Set other settings values into the buffer here.
			status = AW8624HapticsGetCurrentDeviceState(devContext, &hwnHeader->HwNSettingsInfo[i], HWN_SETTINGS_SIZE);
			if (!NT_SUCCESS(status))
			{
				goto exit;
			}
		}

		*BytesRead = InputBufferLength;
	}

exit:

#ifdef DEBUG
	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"%!FUNC! Exit");
#endif

	return status;
}