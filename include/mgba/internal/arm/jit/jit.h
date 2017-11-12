/* Copyright (c) 2017 Alexander Eckhart
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef ARM_JIT_H
#define ARM_JIT_H

#include <mgba-util/common.h>


CXX_GUARD_START
struct ARMCore;
struct ARMJit {
    bool wantJit;
    bool useJit;
};

#include <mgba/internal/arm/arm.h>

void ARMJitEnter(struct ARMCore* cpu);
void ARMJitReset(struct ARMCore* cpu);
void ARMJitInvalidateMemory(struct ARMCore* cpu, uint32_t address, uint32_t size);
void ARMJitWaitStateChanged(struct ARMCore* cpu);

CXX_GUARD_END

#endif
