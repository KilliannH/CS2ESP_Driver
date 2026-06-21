#include <ntddk.h>
#include "Globals.h"
#include "IOControl.h"
#include "Process.h"

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, L"\\DosDevices\\CS2ESP");
    IoDeleteSymbolicLink(&symLink);
    if (DriverObject->DeviceObject)
        IoDeleteDevice(DriverObject->DeviceObject);
    if (g_TargetProcess)
        ObDereferenceObject(g_TargetProcess);
    DbgPrint("[CS2] Driver Unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNICODE_STRING devName, symLink;
    RtlInitUnicodeString(&devName, L"\\Device\\CS2ESP");
    RtlInitUnicodeString(&symLink, L"\\DosDevices\\CS2ESP");

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

    DbgPrint("[CS2] Driver Loaded successfully\n");
    return STATUS_SUCCESS;
}