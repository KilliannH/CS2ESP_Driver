#include "IOControl.h"
#include "Game.h"

NTSTATUS DriverDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG outputLen = stack->Parameters.DeviceIoControl.OutputBufferLength;

    NTSTATUS status = STATUS_SUCCESS;
    ULONG info = 0;

    if (code == IOCTL_GET_ESP_DATA) {
        if (outputLen < sizeof(ESP_DATA)) {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            CollectEnemyData((PESP_DATA)buffer);
            info = sizeof(ESP_DATA);
        }
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
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}