#include "IOControl.h"
#include "Demo.h"
#include "Memory.h"
#include "CS2Reader.h"

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
    else if (code == IOCTL_DEMO_READ_PROCESS_MEMORY) {
        if (inputLen < sizeof(DEMO_READ_MEMORY_REQUEST) || outputLen == 0) {
            status = STATUS_INVALID_PARAMETER;
        }
        else {
            // BUG FIX: copy request locally before RtlZeroMemory overwrites the shared SystemBuffer
            DEMO_READ_MEMORY_REQUEST localReq = *(PDEMO_READ_MEMORY_REQUEST)buffer;
            ULONGLONG targetPid  = localReq.ProcessId;
            ULONGLONG targetAddr = localReq.Address;
            ULONG     targetSize = localReq.Size;

            PEPROCESS process = NULL;
            status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)targetPid, &process);
            if (NT_SUCCESS(status)) {
                SIZE_T bytesRead = 0;
                ULONG readSize = targetSize;
                if (readSize > outputLen) {
                    readSize = outputLen;
                }

                RtlZeroMemory(buffer, outputLen);

                status = SafeReadProcessMemory(process, (PVOID)(ULONG_PTR)targetAddr, buffer, readSize, &bytesRead);
                ObDereferenceObject(process);

                if (NT_SUCCESS(status)) {
                    info = (ULONG)bytesRead;
                }
            }
        }
    }
    else if (code == IOCTL_DEMO_GET_CS2_DATA) {
        if (inputLen < sizeof(CS2_DATA_REQUEST) || outputLen < sizeof(CS2_ESP_DATA)) {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            // Copy request locally before the output buffer is used
            CS2_DATA_REQUEST localReq = *(PCS2_DATA_REQUEST)buffer;
            HANDLE targetPid = (HANDLE)(ULONG_PTR)localReq.ProcessId;

            DbgPrint("[CS2ESP] IOCTL received: pid=%llu clientBase=0x%llX\n",
                     localReq.ProcessId, localReq.ClientBase);

            PCS2_ESP_DATA espData = (PCS2_ESP_DATA)buffer;
            RtlZeroMemory(espData, sizeof(CS2_ESP_DATA));

            // Pass clientBase directly as a parameter (no viewMatrix trick)
            status = ReadCS2Data(targetPid, localReq.ClientBase, espData);
            if (NT_SUCCESS(status)) {
                info = sizeof(CS2_ESP_DATA);
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