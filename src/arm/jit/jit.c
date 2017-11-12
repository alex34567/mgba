/* Copyright (c) 2017 Alexander Eckhart
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba/internal/arm/jit/jit.h>

#include <mgba/core/log.h>

mLOG_DEFINE_CATEGORY(ARM_JIT, "Arm dynamic recompiler", "arm.jit")

void ARMJitEnter(struct ARMCore* cpu) {
	if(cpu->jit.useJit) {
		mLOG(ARM_JIT, STUB, "Jit is not implemented");
	}
	return;
}

void ARMJitReset(struct ARMCore* cpu) {
	cpu->jit.useJit = cpu->jit.wantJit;
	return;
}

void ARMJitInvalidateMemory(struct ARMCore* cpu, uint32_t address, uint32_t size) {
	return;
}

void ARMJitWaitStateChanged(struct ARMCore* cpu) {
	return;
}
