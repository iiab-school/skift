#include <hjert-core/arch.h>
#include <hjert-core/cpu.h>
#include <hjert-core/mem.h>
#include <hjert-core/sched.h>
#include <karm-logger/logger.h>

#include <hal-x86_64/com.h>
#include <hal-x86_64/cpuid.h>
#include <hal-x86_64/gdt.h>
#include <hal-x86_64/idt.h>
#include <hal-x86_64/pic.h>
#include <hal-x86_64/pit.h>
#include <hal-x86_64/vmm.h>

#include "ints.h"

namespace Hjert::Arch {

static x86_64::Com _com1 = x86_64::Com::com1();

static x86_64::DualPic _pic = x86_64::DualPic::dualPic();
static x86_64::Pit _pit = x86_64::Pit::pit();

static x86_64::Tss _tss{};

static x86_64::Gdt _gdt{_tss};
static x86_64::GdtDesc _gdtDesc{_gdt};

static x86_64::Idt _idt{};
static x86_64::IdtDesc _idtDesc{_idt};

Res<> init(Handover::Payload &) {
    _com1.init();

    _gdtDesc.load();

    for (size_t i = 0; i < x86_64::Idt::LEN; i++) {
        _idt.entries[i] = x86_64::IdtEntry{_intVec[i], 0, x86_64::IdtEntry::GATE};
    }

    _idtDesc.load();

    _pic.init();
    _pit.init(1000);

    return Ok();
}

Io::TextWriter<> &loggerOut() {
    return _com1;
}

void stopAll() {
    while (true) {
        x86_64::cli();
        x86_64::hlt();
    }
}

/* --- Cpu ------------------------------------------------------------------ */

struct Cpu : public Core::Cpu {
    void enableInterrupts() override {
        x86_64::sti();
    }

    void disableInterrupts() override {
        x86_64::cli();
    }

    void relaxe() override {
        x86_64::hlt();
    }
};

static Cpu _cpu{};

Core::Cpu &cpu() {
    return _cpu;
}

/* --- Interrupts ----------------------------------------------------------- */

static char const *_faultMsg[32] = {
    "division-by-zero",
    "debug",
    "non-maskable-interrupt",
    "breakpoint",
    "detected-overflow",
    "out-of-bounds",
    "invalid-opcode",
    "no-coprocessor",
    "double-fault",
    "coprocessor-segment-overrun",
    "bad-tss",
    "segment-not-present",
    "stack-fault",
    "general-protection-fault",
    "page-fault",
    "unknown-interrupt",
    "coprocessor-fault",
    "alignment-check",
    "machine-check",
    "simd-floating-point-exception",
    "virtualization-exception",
    "control-protection-exception",
    "reserved",
    "hypervisor-injection-exception",
    "vmm-communication-exception",
    "security-exception",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
};

extern "C" uintptr_t _intDispatch(uintptr_t rsp) {
    auto *frame = reinterpret_cast<Frame *>(rsp);

    cpu().beginInterrupt();

    if (frame->intNo < 32) {
        logFatal("x86_64: cpu exception: {} (err={}, ip={x}, sp={x}, cr2={x}, cr3={x})", _faultMsg[frame->intNo], frame->errNo, frame->rip, frame->rsp, x86_64::rdcr2(), x86_64::rdcr3());
    } else {
        int irq = frame->intNo - 32;

        if (irq == 0) {
            Core::Task::self().stack().saveSp(rsp);
            Core::Sched::self().schedule();
            rsp = Core::Task::self().stack().loadSp();
        } else {
            logInfo("x86_64: irq: {}", irq);
        }
    }

    _pic.ack(frame->intNo);

    cpu().endInterrupt();

    return rsp;
}

static x86_64::Pml<4> *_pml4 = nullptr;
static Opt<x86_64::Vmm<Hal::UpperHalfMapper>> _vmm = NONE;

Hal::Vmm &vmm() {
    if (_vmm == NONE) {
        uintptr_t paddr = Hjert::Mem::heap()
                              .allocRange(Hal::PAGE_SIZE)
                              .unwrap("failed to allocate pml4")
                              .start;

        memset(reinterpret_cast<void *>(paddr), 0, Hal::PAGE_SIZE);
        _pml4 = reinterpret_cast<x86_64::Pml<4> *>(paddr);
        _vmm = x86_64::Vmm<Hal::UpperHalfMapper>{
            Hjert::Mem::pmm(), _pml4};
    }

    return *_vmm;
}

void start(Core::Task &task, uintptr_t ip, uintptr_t sp, Hj::Args args) {
    Frame frame{
        .r8 = args[4],
        .rdi = args[0],
        .rsi = args[1],
        .rdx = args[2],
        .rcx = args[3],

        .rip = ip,
        .rsp = sp,
    };

    /* if (task.isUser()) {
        frame.cs = x86_64::Gdt::UCODE * 8 | 3; // 3 = user mode
        frame.ss = x86_64::Gdt::UDATA * 8 | 3;
        frame.rflags = 0x202;
    } else */
    {
        frame.cs = x86_64::Gdt::KCODE * 8;
        frame.ss = x86_64::Gdt::KDATA * 8;
        frame.rflags = 0x202;
    }

    task
        .stack()
        .push(frame);
}

template <typename L, typename M>
Res<> destroyPml(Hal::Pmm &pmm, L *pml, M mapper = {}) {
    auto range = Hal::PmmRange{mapper.unmap((uintptr_t)pml), Hal::PAGE_SIZE};

    // NOTE: we only need to free the first half of the pml4 since hupper is for the kernel
    for (size_t i = 0; i < L::LEN / (L::LEVEL == 4 ? 2 : 1); i++) {
        if (pml->pages[i].present()) {
            if constexpr (L::LEVEL == 1) {
                auto page = Hal::PmmRange{mapper.map(pml->pages[i].paddr()), Hal::PAGE_SIZE};
                try$(pmm.free(page));
            } else {
                try$(destroyPml(pmm, (typename L::Lower *)mapper.map(pml->pages[i].paddr()), mapper));
            }
        }
    }

    try$(pmm.free(range));

    return Ok();
}

struct Space :
    public Core::Space {

    x86_64::Vmm<Hal::UpperHalfMapper> _vmm;
    x86_64::Pml<4> *_pml4;

    Space(x86_64::Pml<4> *pml4)
        : _vmm{Hjert::Mem::pmm(), pml4}, _pml4() {}

    Space(Space &&other)
        : _vmm(other._vmm),
          _pml4(std::exchange(other._pml4, nullptr)) {
    }

    ~Space() {
        destroyPml(Hjert::Mem::pmm(), _pml4, Hal::UpperHalfMapper{})
            .unwrap("failed to destroy pml4");
    }

    Hal::Vmm &vmm() override {
        return _vmm;
    }
};

Res<Strong<Core::Space>> createSpace() {
    uintptr_t paddr = Hjert::Mem::heap()
                          .allocRange(Hal::PAGE_SIZE)
                          .unwrap("failed to allocate pml4")
                          .start;

    auto *pml4 = reinterpret_cast<x86_64::Pml<4> *>(paddr);
    memset(pml4, 0, Hal::PAGE_SIZE);

    // Copy the kernel part of the pml4
    for (size_t i = _pml4->LEN / 2; i < _pml4->LEN; i++) {
        pml4->pages[i] = _pml4->pages[i];
    }

    return Ok(makeStrong<Space>(pml4));
}

} // namespace Hjert::Arch
