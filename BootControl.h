/*
 * Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef ANDROID_HARDWARE_BOOT_V1_0_BOOTCONTROL_H
#define ANDROID_HARDWARE_BOOT_V1_0_BOOTCONTROL_H

#include <android-base/properties.h>
#include <android/hardware/boot/1.0/IBootControl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "nv-bootcontrolhal"

#include <log/log.h>

#include "bootctrl_nvidia.h"

namespace android {
namespace hardware {
namespace boot {
namespace V1_0 {
namespace implementation {

using ::android::base::GetProperty;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::boot::V1_0::BoolResult;
using ::android::hardware::boot::V1_0::CommandResult;

class BootControl : public IBootControl {
  public:
    // Methods from ::android::hardware::boot::V1_0::IBootControl follow.
    Return<uint32_t> getNumberSlots() override;
    Return<uint32_t> getCurrentSlot() override;
    Return<void> markBootSuccessful(markBootSuccessful_cb _hidl_cb) override;
    Return<void> setActiveBootSlot(uint32_t slot, setActiveBootSlot_cb _hidl_cb) override;
    Return<void> setSlotAsUnbootable(uint32_t slot, setSlotAsUnbootable_cb _hidl_cb) override;
    Return<BoolResult> isSlotBootable(uint32_t slot) override;
    Return<BoolResult> isSlotMarkedSuccessful(uint32_t slot) override;
    Return<void> getSuffix(uint32_t slot, getSuffix_cb _hidl_cb) override;

  private:
    bool readMetadata(smd_partition_t *smd_partition);
    bool writeMetadata(smd_partition_t *smd_partition);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace boot
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BOOT_V1_0_BOOTCONTROL_H
