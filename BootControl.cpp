/*
 * Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
 * Copyright (C) 2020 The Android Open Source Project
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "BootControl.h"

#include <fstream>

namespace android {
namespace hardware {
namespace boot {
namespace V1_0 {
namespace implementation {

// Private methods follow.
bool BootControl::readMetadata(smd_partition_t *smd_partition) {
    int fd;
    ssize_t sz, size = sizeof(smd_partition_t);
    char *buf = (char*)smd_partition;

    if((fd = open(BOOTCTRL_SLOTMETADATA_FILE, O_RDWR)) < 0) {
        ALOGE("Fail to open metadata file");
        return false;
    }
    if (lseek(fd, OFFSET_SLOT_METADATA, SEEK_SET) < 0) {
        ALOGE("Error seeking to metadata offset");
        goto error;
    }

    /* Read slot_metadata */
    sz = read(fd, buf, size);

    if(sz < 0) {
        ALOGE("Fail to read slot metadata");
        goto error;
    }

    /* Check if the data is correct */
    if (smd_partition->magic != BOOTCTRL_MAGIC) {
        ALOGE("Slot metadata is corrupted.");
        goto error;
    }

    return true;

error:
    close(fd);
    return false;
}

bool BootControl::writeMetadata(smd_partition_t *smd_partition) {
    int fd;
    ssize_t sz, size = sizeof(smd_partition_t);
    char *buf = (char*)smd_partition;

    if((fd = open(BOOTCTRL_SLOTMETADATA_FILE, O_RDWR)) < 0) {
        ALOGE("Fail to open metadata file");
        return false;
    }
    if (lseek(fd, OFFSET_SLOT_METADATA, SEEK_SET) < 0) {
        ALOGE("Error seeking to metadata offset");
        goto error;
    }

    /* Check if the data is correct before writing*/
    if (smd_partition->magic != BOOTCTRL_MAGIC) {
        ALOGE("Slot metadata is corrupted. Not writing.");
        goto error;
    }

    /* Write slot_metadata */
    sz = write(fd, buf, size);

    if(sz < 0) {
        ALOGE("Fail to write slot metadata");
        goto error;
    }

    return true;

error:
    close(fd);
    return false;
}

// Methods from ::android::hardware::boot::V1_0::IBootControl follow.
Return<uint32_t> BootControl::getNumberSlots() {
    smd_partition_t smd_partition;

    if (readMetadata(&smd_partition))
        return smd_partition.num_slots;

    // Return the default max slots if we reach here
    return MAX_SLOTS;
}

Return<uint32_t> BootControl::getCurrentSlot() {
    smd_partition_t smd_partition;
    std::string slot_suffix = GetProperty("ro.boot.slot_suffix", "");

    if (readMetadata(&smd_partition)) {
        // Fallback to slot _a if we can't read metadata
        return 0;
    }

    for (int i = 0 ; i < MAX_SLOTS; i++) {
        if (slot_suffix.compare(smd_partition.slot_info[i].suffix))
            return i;
    }

    // Fallback to slot _a if for some reason we reach here
    return 0;
}

Return<void> BootControl::markBootSuccessful(markBootSuccessful_cb _hidl_cb) {
    smd_partition_t smd_partition;
    int slot = getCurrentSlot();

    if (readMetadata(&smd_partition)) {
        smd_partition.slot_info[slot].boot_successful = 1;
        smd_partition.slot_info[slot].retry_count = MAX_COUNT;

        if (writeMetadata(&smd_partition)) {
            _hidl_cb(CommandResult{true, ""});
        } else {
            _hidl_cb(CommandResult{false, "Failed to write metadata"});
        }
    } else {
        _hidl_cb(CommandResult{false, "Failed to read metadata"});
    }

    return Void();
}

Return<void> BootControl::setActiveBootSlot(uint32_t slot, setActiveBootSlot_cb _hidl_cb) {
    smd_partition_t smd_partition;
    uint32_t slot_s;

    if (slot < MAX_SLOTS) {
        if (readMetadata(&smd_partition)) {
            /*
             * Set the target slot priority to max value 15.
             * and reset the retry count to 7.
             */
            smd_partition.slot_info[slot].priority = 15;
            smd_partition.slot_info[slot].boot_successful = 0;
            smd_partition.slot_info[slot].retry_count = MAX_COUNT;

            slot?(slot_s = 0):(slot_s = 1);

            /*
             * Since we use target slot to boot,
             * lower source slot priority.
             */
            smd_partition.slot_info[slot_s].priority = 14;

            if (writeMetadata(&smd_partition)) {
                _hidl_cb(CommandResult{true, ""});
            } else {
                _hidl_cb(CommandResult{false, "Failed to write metadata"});
            }
        } else {
            _hidl_cb(CommandResult{false, "Failed to read metadata"});
        }
    } else {
        _hidl_cb(CommandResult{false, "Invalid slot"});
    }

    return Void();
}

Return<void> BootControl::setSlotAsUnbootable(uint32_t slot, setSlotAsUnbootable_cb _hidl_cb) {
    smd_partition_t smd_partition;

    if (slot < MAX_SLOTS) {
        if (readMetadata(&smd_partition)) {
            /*
             * As this slot is unbootable, set all of value to zero
             * so boot-loader does not rollback to this slot.
             */
            smd_partition.slot_info[slot].priority = 0;
            smd_partition.slot_info[slot].boot_successful = 0;
            smd_partition.slot_info[slot].retry_count = 0;

            if (writeMetadata(&smd_partition)) {
                _hidl_cb(CommandResult{true, ""});
            } else {
                _hidl_cb(CommandResult{false, "Failed to write metadata"});
            }
        } else {
            _hidl_cb(CommandResult{false, "Failed to read metadata"});
        }
    } else {
        _hidl_cb(CommandResult{false, "Invalid slot"});
    }

    return Void();
}

Return<BoolResult> BootControl::isSlotBootable(uint32_t slot) {
    smd_partition_t smd_partition;
    BoolResult ret = BoolResult::FALSE;

    if (slot < MAX_SLOTS) {
        if (readMetadata(&smd_partition)) {
            if (smd_partition.slot_info[slot].priority != 0) {
                ret = BoolResult::TRUE;
            } else {
                ret = BoolResult::FALSE;
            }
        } else {
            ret = BoolResult::FALSE;
        }
    } else {
        ret = BoolResult::INVALID_SLOT;
    }

    return ret;
}

Return<BoolResult> BootControl::isSlotMarkedSuccessful(uint32_t slot) {
    smd_partition_t smd_partition;
    BoolResult ret = BoolResult::FALSE;

    if (slot < MAX_SLOTS) {
        if (readMetadata(&smd_partition)) {
            ret = static_cast<BoolResult>(smd_partition.slot_info[slot].boot_successful);
        } else {
            ret = BoolResult::FALSE;
        }
    } else {
        ret = BoolResult::INVALID_SLOT;
    }

    return ret;
}

Return<void> BootControl::getSuffix(uint32_t slot, getSuffix_cb _hidl_cb) {
    if (slot < MAX_SLOTS) {
        if (slot == 0) {
            _hidl_cb(BOOTCTRL_SUFFIX_A);
        } else {
            _hidl_cb(BOOTCTRL_SUFFIX_B);
        }
    } else {
        // default to slot A
        _hidl_cb(BOOTCTRL_SUFFIX_A);
    }

    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace boot
}  // namespace hardware
}  // namespace android
