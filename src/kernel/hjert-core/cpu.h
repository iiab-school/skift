#pragma once

#include <hal/io.h>
#include <hal/pmm.h>
#include <hal/vmm.h>
#include <hjert-api/raw.h>
#include <karm-base/lock.h>
#include <karm-base/rc.h>
#include <karm-base/ring.h>
#include <karm-base/var.h>
#include <karm-base/vec.h>

#include "arch.h"

namespace Hjert::Core {

struct Cpu {
    bool _retainEnabled = false;
    isize _depth = 0;

    void beginInterrupt() {
        _retainEnabled = false;
    }

    void endInterrupt() {
        _retainEnabled = true;
    }

    void retainDisable() {
        _retainEnabled = false;
    }

    void retainEnable() {
        _retainEnabled = true;
    }

    void retainInterrupts() {
        if (_retainEnabled) {
            disableInterrupts();
            _depth++;
        }
    }

    void releaseInterrupts() {
        if (_retainEnabled) {
            _depth--;
            if (_depth == 0) {
                enableInterrupts();
            }
        }
    }

    virtual void enableInterrupts() = 0;

    virtual void disableInterrupts() = 0;

    virtual void relaxe() = 0;
};

struct InterruptRetainer : public Meta::Static {
    InterruptRetainer() {
        Arch::cpu().retainInterrupts();
    }

    ~InterruptRetainer() {
        Arch::cpu().releaseInterrupts();
    }
};

} // namespace Hjert::Core
