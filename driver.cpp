#include <ntifs.h>
#include "BoosterCommon.h"
#define DRIVER_PREFIX "BoosterDrv: "

void DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS DriverCreateClose(PDEVICE_OBJECT, PIRP Irp);
NTSTATUS DriverWrite(PDEVICE_OBJECT, PIRP Irp);

extern "C" 
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING)
{
	KdPrint((DRIVER_PREFIX "DriverEntry 0x%p\n", DriverObject));
	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = DriverWrite;

	PDEVICE_OBJECT deviceObj;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\Booster");
	NTSTATUS status = IoCreateDevice(
		DriverObject,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&deviceObj
	);

	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVER_PREFIX "Error creating device (0x%X)\n", status));
		return status;
	}

	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\Booster");
	status = IoCreateSymbolicLink(&symName, &deviceName);

	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVER_PREFIX "Error creating symbolic link (0x%X)\n", status));
		IoDeleteDevice(deviceObj);
		return status;
	}

	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT DriverObject)
{
	KdPrint((DRIVER_PREFIX "DriverUnload 0x%p\n", DriverObject));
	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\Booster");
	IoDeleteSymbolicLink(&symName);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS DriverCreateClose(PDEVICE_OBJECT, PIRP Irp)
{
	KdPrint((DRIVER_PREFIX "DriverCreateClose\n"));
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, 0);
	return STATUS_SUCCESS;
}

NTSTATUS DriverWrite(PDEVICE_OBJECT, PIRP Irp)
{
	KdPrint((DRIVER_PREFIX "DriverWrite\n"));

	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG info = 0;
	do
	{
		if (irpSp->Parameters.Write.Length < sizeof(ThreadData))
		{
			status = STATUS_BUFFER_TOO_SMALL; 
			KdPrint((DRIVER_PREFIX "Error: STATUS_BUFFER_TOO_SMALL\n"));
			break;
		}

		ThreadData* data = (ThreadData*)Irp->UserBuffer;

		if (data == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint((DRIVER_PREFIX "Error: STATUS_INVALID_PARAMETER\n"));
			break;
		}

		if (data->Priority < 1 || data->Priority > 31)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint((DRIVER_PREFIX "Error: STATUS_INVALID_PARAMETER\n"));
			break;
		}

		PETHREAD pEthread;
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &pEthread);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "Error: PsLookupThreadByThreadId (0x%X)\n", status));
			break;
		}

		KPRIORITY oldPriority = KeSetPriorityThread(pEthread,data->Priority);
		KdPrint((DRIVER_PREFIX "KeSetPriorityThread: oldPriority -> %d\n", oldPriority));

		ObDereferenceObject(pEthread);
		KdPrint((DRIVER_PREFIX "Done: KeSetPriorityThread!)\n"));
		info = sizeof(ThreadData);

	} while (false);

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, 0);
	return status;
}
