// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cinttypes>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvdrv/devices/nvmap.h"

namespace Service::Nvidia::Devices {

VAddr nvmap::GetObjectAddress(u32 handle) const {
    auto object = GetObject(handle);
    ASSERT(object);
    ASSERT(object->status == Object::Status::Allocated);
    return object->addr;
}

u32 nvmap::ioctl(Ioctl command, const std::vector<u8>& input, std::vector<u8>& output) {
    switch (static_cast<IoctlCommand>(command.raw)) {
    case IoctlCommand::Create:
        return IocCreate(input, output);
    case IoctlCommand::Alloc:
        return IocAlloc(input, output);
    case IoctlCommand::GetId:
        return IocGetId(input, output);
    case IoctlCommand::FromId:
        return IocFromId(input, output);
    case IoctlCommand::Param:
        return IocParam(input, output);
    }

    UNIMPLEMENTED();
    return 0;
}

u32 nvmap::IocCreate(const std::vector<u8>& input, std::vector<u8>& output) {
    IocCreateParams params;
    std::memcpy(&params, input.data(), sizeof(params));

    // Create a new nvmap object and obtain a handle to it.
    auto object = std::make_shared<Object>();
    object->id = next_id++;
    object->size = params.size;
    object->status = Object::Status::Created;

    u32 handle = next_handle++;
    handles[handle] = std::move(object);

    LOG_DEBUG(Service_NVDRV, "size=0x%08X", params.size);

    params.handle = handle;

    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

u32 nvmap::IocAlloc(const std::vector<u8>& input, std::vector<u8>& output) {
    IocAllocParams params;
    std::memcpy(&params, input.data(), sizeof(params));

    auto object = GetObject(params.handle);
    ASSERT(object);

    object->flags = params.flags;
    object->align = params.align;
    object->kind = params.kind;
    object->addr = params.addr;
    object->status = Object::Status::Allocated;

    LOG_DEBUG(Service_NVDRV, "called, addr=0x%" PRIx64, params.addr);

    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

u32 nvmap::IocGetId(const std::vector<u8>& input, std::vector<u8>& output) {
    IocGetIdParams params;
    std::memcpy(&params, input.data(), sizeof(params));

    LOG_WARNING(Service_NVDRV, "called");

    auto object = GetObject(params.handle);
    ASSERT(object);

    params.id = object->id;

    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

u32 nvmap::IocFromId(const std::vector<u8>& input, std::vector<u8>& output) {
    IocFromIdParams params;
    std::memcpy(&params, input.data(), sizeof(params));

    LOG_WARNING(Service_NVDRV, "(STUBBED) called");

    auto itr = std::find_if(handles.begin(), handles.end(),
                            [&](const auto& entry) { return entry.second->id == params.id; });
    ASSERT(itr != handles.end());

    // Return the existing handle instead of creating a new one.
    params.handle = itr->first;

    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

u32 nvmap::IocParam(const std::vector<u8>& input, std::vector<u8>& output) {
    enum class ParamTypes { Size = 1, Alignment = 2, Base = 3, Heap = 4, Kind = 5, Compr = 6 };

    IocParamParams params;
    std::memcpy(&params, input.data(), sizeof(params));

    LOG_WARNING(Service_NVDRV, "(STUBBED) called type=%u", params.type);

    auto object = GetObject(params.handle);
    ASSERT(object);
    ASSERT(object->status == Object::Status::Allocated);

    switch (static_cast<ParamTypes>(params.type)) {
    case ParamTypes::Size:
        params.value = object->size;
        break;
    case ParamTypes::Alignment:
        params.value = object->align;
        break;
    case ParamTypes::Heap:
        // TODO(Subv): Seems to be a hardcoded value?
        params.value = 0x40000000;
        break;
    case ParamTypes::Kind:
        params.value = object->kind;
        break;
    default:
        UNIMPLEMENTED();
    }

    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

} // namespace Service::Nvidia::Devices
