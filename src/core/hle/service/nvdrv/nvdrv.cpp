// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"
#include "core/hle/service/nvdrv/devices/nvdisp_disp0.h"
#include "core/hle/service/nvdrv/devices/nvhost_as_gpu.h"
#include "core/hle/service/nvdrv/devices/nvhost_ctrl.h"
#include "core/hle/service/nvdrv/devices/nvhost_ctrl_gpu.h"
#include "core/hle/service/nvdrv/devices/nvhost_gpu.h"
#include "core/hle/service/nvdrv/devices/nvmap.h"
#include "core/hle/service/nvdrv/interface.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvdrv/nvmemp.h"

namespace Service::Nvidia {

std::weak_ptr<Module> nvdrv;

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto module_ = std::make_shared<Module>();
    std::make_shared<NVDRV>(module_, "nvdrv")->InstallAsService(service_manager);
    std::make_shared<NVDRV>(module_, "nvdrv:a")->InstallAsService(service_manager);
    std::make_shared<NVDRV>(module_, "nvdrv:s")->InstallAsService(service_manager);
    std::make_shared<NVDRV>(module_, "nvdrv:t")->InstallAsService(service_manager);
    std::make_shared<NVMEMP>()->InstallAsService(service_manager);
    nvdrv = module_;
}

Module::Module() {
    auto nvmap_dev = std::make_shared<Devices::nvmap>();
    devices["/dev/nvhost-as-gpu"] = std::make_shared<Devices::nvhost_as_gpu>(nvmap_dev);
    devices["/dev/nvhost-gpu"] = std::make_shared<Devices::nvhost_gpu>(nvmap_dev);
    devices["/dev/nvhost-ctrl-gpu"] = std::make_shared<Devices::nvhost_ctrl_gpu>();
    devices["/dev/nvmap"] = nvmap_dev;
    devices["/dev/nvdisp_disp0"] = std::make_shared<Devices::nvdisp_disp0>(nvmap_dev);
    devices["/dev/nvhost-ctrl"] = std::make_shared<Devices::nvhost_ctrl>();
}

u32 Module::Open(std::string device_name) {
    ASSERT_MSG(devices.find(device_name) != devices.end(), "Trying to open unknown device %s",
               device_name.c_str());

    auto device = devices[device_name];
    u32 fd = next_fd++;

    open_files[fd] = device;

    return fd;
}

u32 Module::Ioctl(u32 fd, u32_le command, const std::vector<u8>& input, std::vector<u8>& output) {
    auto itr = open_files.find(fd);
    ASSERT_MSG(itr != open_files.end(), "Tried to talk to an invalid device");

    auto device = itr->second;
    return device->ioctl({command}, input, output);
}

ResultCode Module::Close(u32 fd) {
    auto itr = open_files.find(fd);
    ASSERT_MSG(itr != open_files.end(), "Tried to talk to an invalid device");

    open_files.erase(itr);

    // TODO(flerovium): return correct result code if operation failed.
    return RESULT_SUCCESS;
}

} // namespace Service::Nvidia
