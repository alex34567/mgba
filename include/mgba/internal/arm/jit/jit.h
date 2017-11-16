/* Copyright (c) 2017 Alexander Eckhart
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef ARM_JIT_H
#define ARM_JIT_H

#include <mgba-util/common.h>

#define ARM_JIT_BLOCK_AMOUNT (1024 * 5)
#define ARM_JIT_BLOCK_BYTE_SIZE 1024

enum {
	JIT_SIZE_WORKING_RAM = 0x00040000,
	JIT_SIZE_WORKING_IRAM = 0x00008000,
};

CXX_GUARD_START
struct ARMCore;
struct ARMJitBlockInfo {
    uint32_t guestLocation;
    uint32_t guestSize;
    uint16_t nextFree;
    uint16_t callNum;
    bool isThumb;
};

struct ARMJitBlock {
    uint8_t jitcode[ARM_JIT_BLOCK_BYTE_SIZE];
};

struct ARMJit {
    bool wantJit;
    bool useJit;
    bool inJit;
    uint16_t freeBlock;
    //Bit vectors to keep track of cached jit memory
    uint8_t cachedIramAdresses[JIT_SIZE_WORKING_IRAM / ARM_JIT_BLOCK_BYTE_SIZE / 8];
    uint8_t cachedWramAdresses[JIT_SIZE_WORKING_RAM / ARM_JIT_BLOCK_BYTE_SIZE / 8];
    struct ARMJitBlockInfo* symbolTable;
    struct ARMJitBlock* jitBlocks;
};

#include <mgba/internal/arm/arm.h>

void ARMJitEnter(struct ARMCore* cpu);
void ARMJitSetGuestAddressUsed(struct ARMCore* cpu, uint32_t address);
void ARMJitInit(struct ARMCore* cpu);
void ARMJitDeinit(struct ARMCore* cpu);
void ARMJitReset(struct ARMCore* cpu);
uint16_t ARMJitAllocateBlock(struct ARMCore* cpu);
void ARMJitFreeBlock(struct ARMCore* cpu, uint16_t block);
void ARMJitFreeBlocks(struct ARMCore* cpu, const uint16_t* block_list);
bool ARMJitIsJitableRegion(uint32_t address);
void ARMJitInvalidateMemory(struct ARMCore* cpu, uint32_t address, uint32_t size);
void ARMJitWaitStateChanged(struct ARMCore* cpu);

CXX_GUARD_END

#endif
