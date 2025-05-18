/*++
	Copyright (c) DuoWoA authors. All Rights Reserved.

	SPDX-License-Identifier: BSD-3-Clause

Module Name:

	HwnDefs.c - Translation functions for HwnClx requests

Abstract:

Environment:

	Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "spb.h"
#include "controller.h"

#ifdef DEBUG
#include "HwnDefs.tmh"
#endif

NTSTATUS
AW8624HapticsToggleVibrationMotor(
	PDEVICE_CONTEXT devContext,
	HWN_STATE hwnState,
	ULONG* hwnIntensity
)
{
#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
#endif

	UNREFERENCED_PARAMETER(hwnIntensity);

	switch (hwnState) {
	case HWN_OFF:
	{
		return AW8624Stop(devContext);
		break;
	}
	case HWN_ON:
	{
		return AW8624VibrateUntilStopped(devContext);
		break;
	}
	default:
	{
		return STATUS_NOT_IMPLEMENTED;
	}
	}
}

NTSTATUS
AW8624HapticsSetDevice(
	PDEVICE_CONTEXT devContext,
	PHWN_SETTINGS hwnSettings
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UINT8 i = 0;

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
#endif

	if (devContext == NULL || hwnSettings == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (hwnSettings->HwNId >= (ULONG)devContext->NumberOfHapticsDevices)
	{
		return STATUS_INVALID_PARAMETER;
	}

	for (i = 0; i < devContext->NumberOfHapticsDevices; i++)
	{
		Status = AW8624HapticsToggleVibrationMotor(
			devContext,
			hwnSettings->OffOnBlink,
			&(hwnSettings->HwNSettings[HWN_INTENSITY])
		);
	}

	return Status;
}

NTSTATUS
AW8624HapticsInitializeDeviceState(
	PDEVICE_CONTEXT devContext
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UINT8 i = 0;

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
#endif

	if (devContext == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	devContext->CurrentStates = (PAW8624_HAPTICS_CURRENT_STATE)ExAllocatePool2(
		PagedPool,
		sizeof(AW8624_HAPTICS_CURRENT_STATE),
		HAPTICS_POOL_TAG
	);

	if (devContext->CurrentStates)
	{
		PHWN_SETTINGS HwNSettingsInfo = &devContext->CurrentStates->CurrentState;

		HwNSettingsInfo->HwNId = 0;
		HwNSettingsInfo->HwNType = HWN_VIBRATOR;
		HwNSettingsInfo->OffOnBlink = HWN_OFF;

		for (i = 0; i < HWN_TOTAL_SETTINGS; i++)
		{
			HwNSettingsInfo->HwNSettings[i] = 0;
		}

		HwNSettingsInfo->HwNSettings[HWN_CURRENT_MTE_RESERVED] = HWN_CURRENT_MTE_NOT_SUPPORTED;

		devContext->CurrentStates->NextState = NULL;
	}
	else
	{
		Status = STATUS_UNSUCCESSFUL;
	}

	return Status;
}

NTSTATUS
AW8624HapticsGetCurrentDeviceState(
	PDEVICE_CONTEXT devContext,
	PHWN_SETTINGS hwnSettings,
	ULONG hwnSettingsLength
)
{
	NTSTATUS Status = STATUS_SUCCESS;

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
#endif

	if (devContext == NULL || hwnSettings == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	PAW8624_HAPTICS_CURRENT_STATE currentState = devContext->CurrentStates;

	if (!currentState)
	{
		Status = AW8624HapticsInitializeDeviceState(devContext);
		if (!NT_SUCCESS(Status))
		{
			return Status;
		}
		currentState = devContext->CurrentStates;
	}

	while (currentState && (hwnSettings->HwNId != currentState->CurrentState.HwNId))
	{
		currentState = currentState->NextState;
	}

	if (!currentState)
	{
		Status = STATUS_UNSUCCESSFUL;
	}
	else
	{
		Status = memcpy_s(
			(PVOID)hwnSettings,
			HWN_SETTINGS_SIZE,
			&(currentState->CurrentState),
			hwnSettingsLength
		);

		if (!NT_SUCCESS(Status))
		{
			Status = STATUS_UNSUCCESSFUL;
		}
	}

	return Status;
}

NTSTATUS
AW8624HapticsSetCurrentDeviceState(
	PDEVICE_CONTEXT devContext,
	PHWN_SETTINGS hwnSettings,
	ULONG hwnSettingsLength
)
{
	NTSTATUS Status = STATUS_SUCCESS;

#ifdef DEBUG
	Trace(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
#endif

	if (devContext == NULL || hwnSettings == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	PAW8624_HAPTICS_CURRENT_STATE previousState = NULL;
	PAW8624_HAPTICS_CURRENT_STATE currentState = devContext->CurrentStates;

	hwnSettings->HwNSettings[HWN_CYCLE_GRANULARITY] = 0;
	hwnSettings->HwNSettings[HWN_CURRENT_MTE_RESERVED] = HWN_CURRENT_MTE_NOT_SUPPORTED;

	if (NULL == currentState)
	{
		devContext->CurrentStates = (PAW8624_HAPTICS_CURRENT_STATE)ExAllocatePool2(
			PagedPool,
			sizeof(AW8624_HAPTICS_CURRENT_STATE),
			HAPTICS_POOL_TAG);

		if (NULL != devContext->CurrentStates)
		{
			Status = memcpy_s(
				&(devContext->CurrentStates->CurrentState),
				HWN_SETTINGS_SIZE,
				(PVOID)hwnSettings, hwnSettingsLength
			);

			if (!NT_SUCCESS(Status))
			{
				Status = STATUS_UNSUCCESSFUL;
			}
			else
			{
				devContext->CurrentStates->NextState = NULL;
			}
		}
		else
		{
			Status = STATUS_UNSUCCESSFUL;
		}
	}
	else
	{
		while (currentState && (hwnSettings->HwNId != currentState->CurrentState.HwNId))
		{
			previousState = currentState;
			currentState = currentState->NextState;
		}

		if (currentState == NULL)
		{
			currentState = (PAW8624_HAPTICS_CURRENT_STATE)ExAllocatePool2(
				PagedPool,
				sizeof(AW8624_HAPTICS_CURRENT_STATE),
				HAPTICS_POOL_TAG
			);

			if (currentState)
			{
				Status = memcpy_s(
					&(currentState->CurrentState),
					HWN_SETTINGS_SIZE,
					(PVOID)hwnSettings,
					hwnSettingsLength
				);

				if (!NT_SUCCESS(Status))
				{
					Status = STATUS_UNSUCCESSFUL;
				}
				else
				{
					previousState->NextState = currentState;
					currentState->NextState = NULL;
				}
			}
			else
			{
				Status = STATUS_UNSUCCESSFUL;
			}
		}
		else
		{
			Status = memcpy_s(
				&(currentState->CurrentState),
				HWN_SETTINGS_SIZE,
				(PVOID)hwnSettings,
				hwnSettingsLength
			);

			if (!NT_SUCCESS(Status))
			{
				Status = STATUS_UNSUCCESSFUL;
			}
		}
	}

	return Status;
}