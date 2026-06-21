#include <ntddk.h>
#include "Globals.h"
#include "IOControl.h"
#include "Demo.h"

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, DEMO_SYMBOLIC_LINK);
    IoDeleteSymbolicLink(&symLink);
    if (DriverObject->DeviceObject)
        IoDeleteDevice(DriverObject->DeviceObject);
    ClearDemoTarget();
    DbgPrint("[MemDemo] Driver unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNICODE_STRING devName, symLink;
    UNREFERENCED_PARAMETER(RegistryPath);

    ExInitializeFastMutex(&g_TargetLock);

    RtlInitUnicodeString(&devName, DEMO_DEVICE_NAME);
    RtlInitUnicodeString(&symLink, DEMO_SYMBOLIC_LINK);

    PDEVICE_OBJECT devObj = NULL;
    NTSTATUS status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &devObj);
    if (!NT_SUCCESS(status))
        return status;

    status = IoCreateSymbolicLink(&symLink, &devName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(devObj);
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    DbgPrint("[MemDemo] Driver loaded successfully\n");
    return STATUS_SUCCESS;
}