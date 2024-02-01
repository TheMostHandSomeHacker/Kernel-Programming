#include <ntifs.h>
#include <ntddk.h>
#include "BoosterCommon.h"

// Empty for now
void SampleUnload(_In_ PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Booster");

	// delete symbolic link
	IoDeleteSymbolicLink(&symLink);

	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS SampleCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS SampleWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	auto status = STATUS_SUCCESS;
	ULONG_PTR information = 0; // tracked used bytes

	// irpSp is of type PIO_STACK_LOCATION
	auto irpSp = IoGetCurrentIrpStackLocation(Irp);

	do {
		if (irpSp->Parameters.Write.Length < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		auto data = static_cast<ThreadData*>(Irp->UserBuffer);
		if (data == nullptr || data->Priority < 1 || data->Priority > 31) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		PETHREAD thread;
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &thread);
		if (!NT_SUCCESS(status)) {
			break;
		}

	} while (false);
}

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = SampleUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = SampleCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = SampleCreateClose;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = SampleWrite;

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\Booster");
	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(
		DriverObject,
		0,
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&DeviceObject
	);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device object (0x%08X)\n",status));
		return status;
	}

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Booster");
	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	return STATUS_SUCCESS;
}

