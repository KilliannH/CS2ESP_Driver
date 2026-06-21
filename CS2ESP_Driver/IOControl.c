#include "IOControl.h"
#include "Demo.h"

NTSTATUS DriverDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG inputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outputLen = stack->Parameters.DeviceIoControl.OutputBufferLength;

    NTSTATUS status = STATUS_SUCCESS;
    ULONG info = 0;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!buffer && code != IOCTL_DEMO_CLEAR_TARGET) {
        status = STATUS_INVALID_PARAMETER;
    }
    else if (code == IOCTL_DEMO_REGISTER_TARGET) {
        if (inputLen < sizeof(DEMO_REGISTER_REQUEST)) {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            status = RegisterDemoTarget((PDEMO_REGISTER_REQUEST)buffer);
        }
    }
    else if (code == IOCTL_DEMO_READ_BLOCK) {
        if (outputLen < sizeof(DEMO_READ_RESULT)) {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            status = ReadRegisteredDemoBlock((PDEMO_READ_RESULT)buffer);
            if (NT_SUCCESS(status)) {
                info = sizeof(DEMO_READ_RESULT);
            }
        }
    }
    else if (code == IOCTL_DEMO_CLEAR_TARGET) {
        ClearDemoTarget();
    }
    else {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS DriverCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}