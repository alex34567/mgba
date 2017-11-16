/* Copyright (c) 2017 Alexander Eckhart
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba/internal/arm/jit/jit.h>

#include <mgba/core/log.h>
#include <mgba/internal/gba/memory.h>
#include <mgba-util/memory.h>

// Only allow jit for code in bios, iram, wram, and cart

int ARMJitIsJitableRegion(uint32_t address) {
	switch (address >> BASE_OFFSET) {
		case REGION_BIOS:
		case REGION_WORKING_RAM:
		case REGION_WORKING_IRAM:
		case REGION_CART0:
		case REGION_CART0_EX:
		case REGION_CART1:
		case REGION_CART1_EX:
		case REGION_CART2:
		case REGION_CART2_EX:
			return 1;
		default:
			return 0;
	}
}

void ARMJitEnter(struct ARMCore* cpu) {
	if (!cpu->jit.useJit) {
		return;
	}

	uint32_t address;
	if (cpu->executionMode == MODE_THUMB) {
		address = cpu->gprs[15] - 4;
	} else {
		address = cpu->gprs[15] - 8;
	}

	if (!ARMJitIsJitableRegion(address)) {
		return;
	}

	return;
}

void ARMJitFreeBlock(struct ARMCore* cpu, uint16_t block) {
	cpu->jit.symbolTable[block].nextFree = cpu->jit.freeBlock;
	cpu->jit.freeBlock = block;
	cpu->jit.symbolTable[block].guestLocation = 0xFFFFFFFF;
	cpu->jit.symbolTable[block].guestSize = 0;
	for (int x = 0; x < ARM_JIT_BLOCK_BYTE_SIZE; x += 2) {
		// Fill freed jit block with the opcode lock nop to allow catching of use after frees
		cpu->jit.jitBlocks[block].jitcode[x] = 0xF0;
		cpu->jit.jitBlocks[block].jitcode[x + 1] = 0x90;
	}
}

static inline void _ARMJitCleanOldBlocks(struct ARMCore* cpu) {
#define ARM_JIT_BLOCKS_TO_CLEAN (ARM_JIT_BLOCK_AMOUNT / 2)
	uint16_t leastCalledBlocks[ARM_JIT_BLOCKS_TO_CLEAN];
	// Guarantee there are always blocks to free
	for (int x = 0; x < ARM_JIT_BLOCKS_TO_CLEAN; x++) {
		leastCalledBlocks[x] = x;
	}
	uint16_t callAmounts[ARM_JIT_BLOCKS_TO_CLEAN];
	memset(callAmounts, 0xFF, sizeof (callAmounts));
	for (int x = 0; x < ARM_JIT_BLOCK_AMOUNT; x++) {
		if (cpu->jit.symbolTable[x].callNum < callAmounts[ARM_JIT_BLOCKS_TO_CLEAN - 1]) {
			int y = 0;
			for (; y < ARM_JIT_BLOCKS_TO_CLEAN; y++) {
				if (cpu->jit.symbolTable[x].callNum >= callAmounts[y]) {
					break;
				}
			}
			y--;
			if (y < ARM_JIT_BLOCKS_TO_CLEAN - 1) {
				// Shift list forward to maintain least to greatest order
				memmove(&callAmounts[y + 1], &callAmounts[y], sizeof (uint16_t) * (ARM_JIT_BLOCKS_TO_CLEAN - 1 - y));
				memmove(&leastCalledBlocks[y + 1], &leastCalledBlocks[y], sizeof (uint16_t) * (ARM_JIT_BLOCKS_TO_CLEAN - 1 - y));
			}
			callAmounts[y] = cpu->jit.symbolTable[x].callNum;
			leastCalledBlocks[y] = x;
		}
		// Reset callnum to track changing function access patterns
		cpu->jit.symbolTable[x].callNum = 0;
	}
	for (int x = 0; x < ARM_JIT_BLOCKS_TO_CLEAN; x++) {
		ARMJitFreeBlock(cpu, leastCalledBlocks[x]);
	}
#undef ARM_JIT_BLOCKS_TO_CLEAN
}

uint16_t ARMJitAllocateBlock(struct ARMCore* cpu) {
	uint16_t ret = cpu->jit.freeBlock;
	if (ret >= ARM_JIT_BLOCK_AMOUNT) {
		_ARMJitCleanOldBlocks(cpu);
		ret = cpu->jit.freeBlock;
	}
	cpu->jit.freeBlock = cpu->jit.symbolTable[ret].nextFree;
	return ret;
}

void ARMJitInit(struct ARMCore* cpu) {
	cpu->jit.symbolTable = anonymousMemoryMap(sizeof (struct ARMJitBlockInfo) * ARM_JIT_BLOCK_AMOUNT);
	cpu->jit.jitBlocks = executableMemoryMap(sizeof (struct ARMJitBlock) * ARM_JIT_BLOCK_AMOUNT);
	return;
}

void ARMJitDeinit(struct ARMCore* cpu) {
	mappedMemoryFree(cpu->jit.symbolTable, sizeof (struct ARMJitBlockInfo) * ARM_JIT_BLOCK_AMOUNT);
	mappedMemoryFree(cpu->jit.jitBlocks, sizeof (struct ARMJitBlock) * ARM_JIT_BLOCK_AMOUNT);
	return;
}

void ARMJitReset(struct ARMCore* cpu) {
	bool oldState = cpu->jit.useJit;
	cpu->jit.useJit = cpu->jit.wantJit;
	if (oldState != cpu->jit.useJit) {
		if (oldState == false) {
			ARMJitInit(cpu);
		} else if (oldState == true) {
			ARMJitDeinit(cpu);
		}
	}
	if (cpu->jit.useJit == false) {
		return;
	}
	cpu->jit.freeBlock = ARM_JIT_BLOCK_AMOUNT + 1;
	for (int i = 0; i < ARM_JIT_BLOCK_AMOUNT; i++) {
		ARMJitFreeBlock(cpu, i);
	}
	return;
}

void ARMJitInvalidateMemory(struct ARMCore* cpu, uint32_t address, uint32_t size) {
	if (!cpu->jit.useJit) {
		return;
	}
	if (!ARMJitIsJitableRegion(address)) {
		return;
	}
	uint32_t address_begin = address;
	uint32_t address_end = address + size;
	for (int i = 0; i < ARM_JIT_BLOCK_AMOUNT; i++) {
		uint32_t begin = cpu->jit.symbolTable[i].guestLocation;
		uint32_t end = begin + cpu->jit.symbolTable[i].guestSize;
		if (address_end >= begin && address_begin <= end) {
			ARMJitFreeBlock(cpu, i);
		}
	}
	return;
}

void ARMJitWaitStateChanged(struct ARMCore* cpu) {
	ARMJitReset(cpu);
	return;
}
