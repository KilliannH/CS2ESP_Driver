#pragma once
#include "Driver.h"

NTSTATUS DriverDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DriverCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);