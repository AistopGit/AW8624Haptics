/*++
	Copyright (c) DuoWoA authors. All Rights Reserved.

	SPDX-License-Identifier: BSD-3-Clause

Module Name:

	HwnDefs.c - Translation functions for HwnClx requests

Abstract:

Environment:

	Kernel-mode Driver Framework

--*/

#pragma once

#include "device.h"

NTSTATUS
AW8624HapticsSetDevice(
	PDEVICE_CONTEXT devContext,
	PHWN_SETTINGS hwnSettings
);

NTSTATUS
AW8624HapticsInitializeDeviceState(
	PDEVICE_CONTEXT devContext
);

NTSTATUS
AW8624HapticsGetCurrentDeviceState(
	PDEVICE_CONTEXT devContext,
	PHWN_SETTINGS hwnSettings,
	ULONG hwnSettingsLength
);

NTSTATUS
AW8624HapticsSetCurrentDeviceState(
	PDEVICE_CONTEXT devContext,
	PHWN_SETTINGS hwnSettings,
	ULONG hwnSettingsLength
);