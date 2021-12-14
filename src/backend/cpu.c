#include "cpu.h"
#include <string.h>
#include <stdio.h>
#include <backend/events.h>

#define SET_Z(state) (cpu->f.z = (state) != 0)
#define SET_N(state) (cpu->f.n = (state) != 0)
#define SET_H(state) (cpu->f.h = (state) != 0)
#define SET_C(state) (cpu->f.c = (state) != 0)

void tickM(CPU *cpu, size_t cycles) {
    size_t t_cycles = cycles * 4;
    cpu->t_cycles += t_cycles;
    while (t_cycles--)
        ppuTick(&cpu->ppu, &cpu->memory);
}

typedef enum {
    _V00h = 0x00,
    _V08h = 0x08,
    _V10h = 0x10,
    _V18h = 0x18,
    _V20h = 0x20,
    _V28h = 0x28,
    _V30h = 0x30,
    _V38h = 0x38
} RESET_VEC;

static uint8_t fetch(CPU *cpu) {
    tickM(cpu, 1);
    return memRead(&cpu->memory, cpu->pc++);
}

static uint16_t fetch16(CPU *cpu) {
    uint16_t val = fetch(cpu);
    return val | (fetch(cpu) << 8);
}

static uint8_t cpuRead(CPU *cpu, uint16_t adr) {
    tickM(cpu, 1);
    return memRead(&cpu->memory, adr);
}

static void cpuWrite(CPU *cpu, uint16_t adr, uint8_t val) {
    tickM(cpu, 1);
    memWrite(&cpu->memory, adr, val);
}

uint8_t determineRegister(CPU *cpu, uint8_t opc) {
    switch (opc % 8) {
    case 0:
        return cpu->b;
    case 1:
        return cpu->c;
    case 2:
        return cpu->d;
    case 3:
        return cpu->e;
    case 4:
        return cpu->h;
    case 5:
        return cpu->l;
    case 6:
        return cpuRead(cpu, cpu->hl);
    case 7:
        return cpu->a;
    }
    PANIC;
    return 0;
}

uint8_t *determineRegisterCB(CPU *cpu, uint8_t opc) {
    switch (opc % 8) {
    case 0:
        return &cpu->b;
    case 1:
        return &cpu->c;
    case 2:
        return &cpu->d;
    case 3:
        return &cpu->e;
    case 4:
        return &cpu->h;
    case 5:
        return &cpu->l;
    case 7:
        return &cpu->a;
    }
    PANIC;
    return NULL;
}

//  LOAD INSTRUCTIONS

static void LD_r8_r8(CPU *cpu, uint8_t *dest, uint8_t src) { *dest = src; }

static void LD_r8_u8(CPU *cpu, uint8_t *dest) { *dest = fetch(cpu); }

static void LD_r16_u16(CPU *cpu, uint16_t *dest) { *dest = fetch16(cpu); }

static void LD_pHL_r8(CPU *cpu, uint8_t src) { cpuWrite(cpu, cpu->hl, src); }

static void LD_pHL_u8(CPU *cpu) { cpuWrite(cpu, cpu->hl, fetch(cpu)); }

static void LD_pu16_A(CPU *cpu) { cpuWrite(cpu, fetch16(cpu), cpu->a); }

static void LDH_pu8_A(CPU *cpu) { cpuWrite(cpu, 0xFF00 + fetch(cpu), cpu->a); }

static void LDH_pC_A(CPU *cpu) { cpuWrite(cpu, 0xFF00 + cpu->c, cpu->a); }

static void LD_A_pr16(CPU *cpu, uint16_t adr) { cpu->a = cpuRead(cpu, adr); }

static void LD_pr16_A(CPU *cpu, uint16_t adr) { cpuWrite(cpu, adr, cpu->a); }

static void LD_A_pu16(CPU *cpu) { cpu->a = cpuRead(cpu, fetch16(cpu)); }

static void LDH_A_pu8(CPU *cpu) { cpu->a = cpuRead(cpu, 0xFF00 + fetch(cpu)); }

static void LDH_A_pC(CPU *cpu) { cpu->a = cpuRead(cpu, 0xFF00 + cpu->c); }

static void LD_pHLi_A(CPU *cpu) { cpuWrite(cpu, cpu->hl++, cpu->a); }

static void LD_pHLd_A(CPU *cpu) { cpuWrite(cpu, cpu->hl--, cpu->a); }

static void LD_A_pHLi(CPU *cpu) { cpu->a = cpuRead(cpu, cpu->hl++); }

static void LD_A_pHLd(CPU *cpu) { cpu->a = cpuRead(cpu, cpu->hl--); }

static void LD_SP_u16(CPU *cpu) { cpu->sp = fetch16(cpu); }

static void LD_pu16_SP(CPU *cpu) {
    uint16_t adr = fetch16(cpu);
    cpuWrite(cpu, adr, cpu->sp);
    cpuWrite(cpu, adr + 1, cpu->sp >> 8);
}

static void LD_HL_SPi8(CPU *cpu) {
    uint8_t imm = fetch(cpu);
    SET_Z(false);
    SET_N(false);
    SET_H((cpu->sp & 0x0F) + (imm & 0x0F) > 0x0F);
    SET_C((cpu->sp & 0xFF) + imm > 0xFF);
    cpu->hl = cpu->sp + (int8_t)imm;
    tickM(cpu, 1);
}

static void LD_SP_HL(CPU *cpu) {
    cpu->sp = cpu->hl;
    tickM(cpu, 1);
}

//  ALU INSTRUCTIONS

/* ADC */
__always_inline void __ADC(CPU *cpu, uint8_t src, bool carry) {
    SET_N(false);
    SET_H((cpu->a & 0x0F) + (src & 0x0F) + carry > 0x0F);
    SET_C(cpu->a + src + carry > 0xFF);
    cpu->a += src + carry;
    SET_Z(cpu->a == 0);
}

static void ADC_r8(CPU *cpu, uint8_t src) { __ADC(cpu, src, cpu->f.c); }

static void ADC_u8(CPU *cpu) { __ADC(cpu, fetch(cpu), cpu->f.c); }

/* ADD */

static void ADD_r8(CPU *cpu, uint8_t src) { __ADC(cpu, src, 0); }

static void ADD_u8(CPU *cpu) { __ADC(cpu, fetch(cpu), 0); }

/* CP */

__always_inline void __CP(CPU *cpu, uint8_t src) {
    SET_Z(cpu->a == src);
    SET_N(true);
    SET_H((cpu->a & 0x0F) < (src & 0x0F));
    SET_C(cpu->a < src);
}

static void CP_r8(CPU *cpu, uint8_t src) { __CP(cpu, src); }

static void CP_u8(CPU *cpu) { __CP(cpu, fetch(cpu)); }

/* CPL */

static void CPL(CPU *cpu) {
    cpu->a = ~cpu->a;
    SET_N(true);
    SET_H(true);
}

/* DEC */

__always_inline void __DEC(CPU *cpu, uint8_t *dest) {
    SET_N(true);
    SET_H((*dest & 0x0F) == 0);
    --(*dest);
    SET_Z(*dest == 0);
}

static void DEC_r8(CPU *cpu, uint8_t *dest) { __DEC(cpu, dest); }

static void DEC_pHL(CPU *cpu) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __DEC(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

static void DEC_r16(CPU *cpu, uint16_t *dest) { --(*dest); }

/* INC */

__always_inline void __INC(CPU *cpu, uint8_t *dest) {
    SET_N(false);
    SET_H((*dest & 0x0F) == 0x0F);
    ++(*dest);
    SET_Z(*dest == 0);
}

static void INC_r8(CPU *cpu, uint8_t *dest) { __INC(cpu, dest); }

static void INC_pHL(CPU *cpu) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __INC(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

static void INC_r16(CPU *cpu, uint16_t *dest) { ++(*dest); }

/* SBC */

__always_inline void __SBC(CPU *cpu, uint8_t src, bool carry) {
    SET_N(true);
    SET_H((cpu->a & 0x0F) < (src & 0x0F) + carry);
    SET_C(cpu->a < src + carry);
    cpu->a -= src + carry;
    SET_Z(cpu->a == 0);
}

static void SBC_r8(CPU *cpu, uint8_t src) { __SBC(cpu, src, cpu->f.c); }

static void SBC_u8(CPU *cpu) { __SBC(cpu, fetch(cpu), cpu->f.c); }

/* SUB */

static void SUB_r8(CPU *cpu, uint8_t src) { __SBC(cpu, src, 0); }

static void SUB_u8(CPU *cpu) { __SBC(cpu, fetch(cpu), 0); }

/* ADD */

static void ADD_HL_r16(CPU *cpu, uint16_t src) {
    SET_N(false);
    SET_H((cpu->hl & 0x0FFF) + (src & 0x0FFF) > 0x0FFF);
    SET_C(cpu->hl + src > 0xFFFF);
    cpu->hl += src;
    tickM(cpu, 1);
}

static void ADD_SP_i8(CPU *cpu) {
    uint8_t imm = fetch(cpu);
    SET_Z(false);
    SET_N(false);
    SET_H((cpu->sp & 0x0F) + (imm & 0x0F) > 0x0F);
    SET_C((cpu->sp & 0xFF) + imm > 0xFF);
    cpu->sp += (int8_t)imm;
}

//  BIT OPERATIONS

/* BIT */

__always_inline void __BIT(CPU *cpu, uint8_t bit, uint8_t val) {
    SET_Z((val & BIT(bit)) == 0);
    SET_N(false);
    SET_H(true);
}

static void BIT_u3_r8(CPU *cpu, uint8_t bit, uint8_t src) {
    __BIT(cpu, bit, src);
}

static void BIT_u3_pHL(CPU *cpu, uint8_t bit) {
    __BIT(cpu, bit, cpuRead(cpu, cpu->hl));
}

/* AND */

__always_inline void __AND(CPU *cpu, uint8_t src) {
    SET_N(false);
    SET_H(true);
    SET_C(false);
    cpu->a &= src;
    SET_Z(cpu->a == 0);
}

static void AND_r8(CPU *cpu, uint8_t src) { __AND(cpu, src); }

static void AND_u8(CPU *cpu) { __AND(cpu, fetch(cpu)); }

/* OR */

__always_inline void __OR(CPU *cpu, uint8_t src) {
    SET_N(false);
    SET_H(false);
    SET_C(false);
    cpu->a |= src;
    SET_Z(cpu->a == 0);
}

static void OR_r8(CPU *cpu, uint8_t src) { __OR(cpu, src); }

static void OR_u8(CPU *cpu) { __OR(cpu, fetch(cpu)); }

/* RES */

__always_inline void __RES(CPU *cpu, uint8_t bit, uint8_t *dest) {
    *dest &= ~BIT(bit);
}

static void RES_u3_r8(CPU *cpu, uint8_t bit, uint8_t *dest) {
    __RES(cpu, bit, dest);
}

static void RES_u3_pHL(CPU *cpu, uint8_t bit) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __RES(cpu, bit, &val);
    cpuWrite(cpu, cpu->hl, val);
}

/* RL */

__always_inline void __RL(CPU *cpu, uint8_t *dest) {
    SET_N(false);
    SET_H(false);
    uint8_t c = cpu->f.c;
    SET_C(*dest & BIT(7));
    *dest <<= 1;
    *dest |= c;
    SET_Z(*dest == 0);
}

static void RL_r8(CPU *cpu, uint8_t *dest) { __RL(cpu, dest); }

static void RL_pHL(CPU *cpu) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __RL(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

static void RLA(CPU *cpu) {
    __RL(cpu, &cpu->a);
    SET_Z(false);
}

/* RLC */

__always_inline void __RLC(CPU *cpu, uint8_t *dest) {
    SET_N(false);
    SET_H(false);
    uint8_t hb = *dest >> 7;
    SET_C(hb);
    *dest <<= 1;
    *dest |= hb;
    SET_Z(*dest == 0);
}

static void RLC_r8(CPU *cpu, uint8_t *dest) { __RLC(cpu, dest); }

static void RLC_pHL(CPU *cpu) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __RLC(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

static void RLCA(CPU *cpu) {
    __RLC(cpu, &cpu->a);
    SET_Z(false);
}

/* RR */

__always_inline void __RR(CPU *cpu, uint8_t *dest) {
    SET_N(false);
    SET_H(false);
    uint8_t c = cpu->f.c;
    SET_C(*dest & BIT(0));
    *dest >>= 1;
    *dest |= (c << 7);
    SET_Z(*dest == 0);
}

static void RR_r8(CPU *cpu, uint8_t *dest) { __RR(cpu, dest); }

static void RR_pHL(CPU *cpu) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __RR(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

static void RRA(CPU *cpu) {
    __RR(cpu, &cpu->a);
    SET_Z(false);
}

/* RRC */

__always_inline void __RRC(CPU *cpu, uint8_t *dest) {
    SET_N(false);
    SET_H(false);
    uint8_t lb = *dest & BIT(0);
    SET_C(lb);
    *dest >>= 1;
    *dest |= lb << 7;
    SET_Z(*dest == 0);
}

static void RRC_r8(CPU *cpu, uint8_t *dest) { __RRC(cpu, dest); }

static void RRC_pHL(CPU *cpu) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __RRC(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

static void RRCA(CPU *cpu) {
    __RRC(cpu, &cpu->a);
    SET_Z(false);
}

/* SET */

__always_inline void __SET(CPU *cpu, uint8_t bit, uint8_t *dest) {
    *dest |= BIT(bit);
}

static void SET_u3_r8(CPU *cpu, uint8_t bit, uint8_t *dest) {
    __SET(cpu, bit, dest);
}

static void SET_u3_pHL(CPU *cpu, uint8_t bit) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __SET(cpu, bit, &val);
    cpuWrite(cpu, cpu->hl, val);
}

/* SLA */

__always_inline void __SLA(CPU *cpu, uint8_t *dest) {
    SET_N(false);
    SET_H(false);
    SET_C(*dest & BIT(7));
    *dest <<= 1;
    SET_Z(*dest == 0);
}

static void SLA_r8(CPU *cpu, uint8_t *dest) { __SLA(cpu, dest); }

static void SLA_pHL(CPU *cpu) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __SLA(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

/* SRA */

__always_inline void __SRA(CPU *cpu, uint8_t *dest) {
    SET_N(false);
    SET_H(false);
    SET_C(*dest & BIT(0));
    uint8_t hb = *dest & BIT(7);
    *dest >>= 1;
    *dest |= hb;
    SET_Z(*dest == 0);
}

static void SRA_r8(CPU *cpu, uint8_t *dest) { __SRA(cpu, dest); }

static void SRA_pHL(CPU *cpu) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __SRA(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

/* SRL */

__always_inline void __SRL(CPU *cpu, uint8_t *dest) {
    SET_N(false);
    SET_H(false);
    SET_C(*dest & BIT(0));
    *dest >>= 1;
    SET_Z(*dest == 0);
}

static void SRL_r8(CPU *cpu, uint8_t *dest) { __SRL(cpu, dest); }

static void SRL_pHL(CPU *cpu) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __SRL(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

/* SWAP */

__always_inline void __SWAP(CPU *cpu, uint8_t *dest) {
    SET_N(false);
    SET_H(false);
    SET_C(false);
    SET_Z(*dest == 0);
    *dest = (*dest << 4) | (*dest >> 4);
}

static void SWAP_r8(CPU *cpu, uint8_t *dest) { __SWAP(cpu, dest); }

static void SWAP_pHL(CPU *cpu) {
    uint8_t val = cpuRead(cpu, cpu->hl);
    __SWAP(cpu, &val);
    cpuWrite(cpu, cpu->hl, val);
}

/* XOR */

__always_inline void __XOR(CPU *cpu, uint8_t src) {
    SET_N(false);
    SET_H(false);
    SET_C(false);
    cpu->a ^= src;
    SET_Z(cpu->a == 0);
}

static void XOR_r8(CPU *cpu, uint8_t src) { __XOR(cpu, src); }

static void XOR_u8(CPU *cpu) { __XOR(cpu, fetch(cpu)); }

//  STACK OPERATIONS

static void POP_r16(CPU *cpu, uint16_t *dest) {
    uint16_t val = 0;
    val |= cpuRead(cpu, cpu->sp++);
    val |= cpuRead(cpu, cpu->sp++) << 8;
    *dest = val;
}

static void POP_AF(CPU *cpu) {
    cpu->af = (cpuRead(cpu, cpu->sp++) & 0xF0);
    cpu->a = cpuRead(cpu, cpu->sp++);
}

static void PUSH_r16(CPU *cpu, uint16_t src) {
    cpuWrite(cpu, --cpu->sp, src >> 8);
    cpuWrite(cpu, --cpu->sp, src);
    tickM(cpu, 1);
}

//  MISC INSTRUCTIONS

static void NOP(CPU *cpu) {}

static void DAA(CPU *cpu) {
    if (!cpu->f.n) {
        if (cpu->f.c || (cpu->a > 0x99)) {
            cpu->a += 0x60;
            SET_C(true);
        }
        if (cpu->f.h || (cpu->a & 0x0F) > 0x09) {
            cpu->a += 0x06;
        }
    } else {
        if (cpu->f.c) {
            cpu->a -= 0x60;
            SET_C(true);
        }
        if (cpu->f.h)
            cpu->a -= 0x06;
    }
    SET_H(false);
    SET_Z(cpu->a == 0);
}

static void HALT(CPU *cpu) { cpu->halted = true; }

static void STOP(CPU *cpu) { PANIC; }

static void CCF(CPU *cpu) {
    SET_N(false);
    SET_H(false);
    SET_C(!cpu->f.c);
}

static void DI(CPU *cpu) {
    eventDI(&cpu->sched);
    // scheduleEvent(&cpu->sched, 1, eventDI);
}

static void EI(CPU *cpu) { scheduleEvent(&cpu->sched, 1, eEI); }

static void SCF(CPU *cpu) {
    SET_N(false);
    SET_H(false);
    SET_C(true);
}

static void CB(CPU *cpu) {
    uint8_t opc = fetch(cpu);
    switch (opc) {
    case 0x00 ... 0x05:
    case 0x07:
        RLC_r8(cpu, determineRegisterCB(cpu, opc));
        break;
    case 0x06:
        RLC_pHL(cpu);
        break;

    case 0x08 ... 0x0D:
    case 0x0F:
        RRC_r8(cpu, determineRegisterCB(cpu, opc));
        break;
    case 0x0E:
        RRC_pHL(cpu);
        break;

    case 0x10 ... 0x15:
    case 0x17:
        RL_r8(cpu, determineRegisterCB(cpu, opc));
        break;
    case 0x16:
        RL_pHL(cpu);
        break;

    case 0x18 ... 0x1D:
    case 0x1F:
        RR_r8(cpu, determineRegisterCB(cpu, opc));
        break;
    case 0x1E:
        RR_pHL(cpu);
        break;

    case 0x20 ... 0x25:
    case 0x27:
        SLA_r8(cpu, determineRegisterCB(cpu, opc));
        break;
    case 0x26:
        SLA_pHL(cpu);
        break;

    case 0x28 ... 0x2D:
    case 0x2F:
        SRA_r8(cpu, determineRegisterCB(cpu, opc));
        break;
    case 0x2E:
        SRA_pHL(cpu);
        break;

    case 0x30 ... 0x35:
    case 0x37:
        SWAP_r8(cpu, determineRegisterCB(cpu, opc));
        break;
    case 0x36:
        SWAP_pHL(cpu);
        break;

    case 0x38 ... 0x3D:
    case 0x3F:
        SRL_r8(cpu, determineRegisterCB(cpu, opc));
        break;
    case 0x3E:
        SRL_pHL(cpu);
        break;

    case 0x40 ... 0x7F:
        if (opc % 8 != 6)
            BIT_u3_r8(cpu, (opc - 0x40) / 8, *determineRegisterCB(cpu, opc));
        else
            BIT_u3_pHL(cpu, (opc - 0x40) / 8);
        break;
    case 0x80 ... 0xBF:
        if (opc % 8 != 6)
            RES_u3_r8(cpu, (opc - 0x80) / 8, determineRegisterCB(cpu, opc));
        else
            RES_u3_pHL(cpu, (opc - 0x80) / 8);
        break;
    case 0xC0 ... 0xFF:
        if (opc % 8 != 6)
            SET_u3_r8(cpu, (opc - 0xC0) / 8, determineRegisterCB(cpu, opc));
        else
            SET_u3_pHL(cpu, (opc - 0xC0) / 8);
        break;
    default:
        PANIC;
    }
}

//  CONTROL OPERATIONS

static void CALL_u16(CPU *cpu) {
    uint16_t imm = fetch16(cpu);
    PUSH_r16(cpu, cpu->pc);
    cpu->pc = imm;
    tickM(cpu, 1);
}

static void CALL_cc_u16(CPU *cpu, bool cond) {
    uint16_t imm = fetch16(cpu);
    if (cond) {
        PUSH_r16(cpu, cpu->pc);
        cpu->pc = imm;
        tickM(cpu, 1);
    }
}

static void JP_u16(CPU *cpu) {
    cpu->pc = fetch16(cpu);
    tickM(cpu, 1);
}

static void JP_cc_u16(CPU *cpu, bool cond) {
    uint16_t imm = fetch16(cpu);
    if (cond) {
        cpu->pc = imm;
        tickM(cpu, 1);
    }
}

static void JP_HL(CPU *cpu) { cpu->pc = cpu->hl; }

static void JR_i8(CPU *cpu) {
    cpu->pc += (int8_t)fetch(cpu);
    tickM(cpu, 1);
}

static void JR_cc_i8(CPU *cpu, bool cond) {
    int8_t imm = (int8_t)fetch(cpu);
    if (cond) {
        cpu->pc += imm;
        tickM(cpu, 1);
    }
}

static void RET(CPU *cpu) {
    POP_r16(cpu, &cpu->pc);
    tickM(cpu, 1);
}

static void RET_cc(CPU *cpu, bool cond) {
    if (cond) {
        RET(cpu);
        tickM(cpu, 1);
    } else
        tickM(cpu, 1);
}

static void RETI(CPU *cpu) {
    RET(cpu);
    EI(cpu);
}

static void RST(CPU *cpu, RESET_VEC vec) {
    PUSH_r16(cpu, cpu->pc);
    cpu->pc = vec;
    tickM(cpu, 1);
}

static void fetchAndExecuteInstruction(CPU *cpu) {
    uint8_t opcode = fetch(cpu);
    switch (opcode) {
    case 0x00:
        NOP(cpu);
        break;
    case 0x01:
        LD_r16_u16(cpu, &cpu->bc);
        break;
    case 0x02:
        LD_pr16_A(cpu, cpu->bc);
        break;
    case 0x03:
        INC_r16(cpu, &cpu->bc);
        break;
    case 0x04:
        INC_r8(cpu, &cpu->b);
        break;
    case 0x05:
        DEC_r8(cpu, &cpu->b);
        break;
    case 0x06:
        LD_r8_u8(cpu, &cpu->b);
        break;
    case 0x07:
        RLCA(cpu);
        break;
    case 0x08:
        LD_pu16_SP(cpu);
        break;
    case 0x09:
        ADD_HL_r16(cpu, cpu->bc);
        break;
    case 0x0A:
        LD_A_pr16(cpu, cpu->bc);
        break;
    case 0x0B:
        DEC_r16(cpu, &cpu->bc);
        break;
    case 0x0C:
        INC_r8(cpu, &cpu->c);
        break;
    case 0x0D:
        DEC_r8(cpu, &cpu->c);
        break;
    case 0x0E:
        LD_r8_u8(cpu, &cpu->c);
        break;
    case 0x0F:
        RRCA(cpu);
        break;

    case 0x10:
        STOP(cpu);
        break;
    case 0x11:
        LD_r16_u16(cpu, &cpu->de);
        break;
    case 0x12:
        LD_pr16_A(cpu, cpu->de);
        break;
    case 0x13:
        INC_r16(cpu, &cpu->de);
        break;
    case 0x14:
        INC_r8(cpu, &cpu->d);
        break;
    case 0x15:
        DEC_r8(cpu, &cpu->d);
        break;
    case 0x16:
        LD_r8_u8(cpu, &cpu->d);
        break;
    case 0x17:
        RLA(cpu);
        break;
    case 0x18:
        JR_i8(cpu);
        break;
    case 0x19:
        ADD_HL_r16(cpu, cpu->de);
        break;
    case 0x1A:
        LD_A_pr16(cpu, cpu->de);
        break;
    case 0x1B:
        DEC_r16(cpu, &cpu->de);
        break;
    case 0x1C:
        INC_r8(cpu, &cpu->e);
        break;
    case 0x1D:
        DEC_r8(cpu, &cpu->e);
        break;
    case 0x1E:
        LD_r8_u8(cpu, &cpu->e);
        break;
    case 0x1F:
        RRA(cpu);
        break;

    case 0x20:
        JR_cc_i8(cpu, !cpu->f.z);
        break;
    case 0x21:
        LD_r16_u16(cpu, &cpu->hl);
        break;
    case 0x22:
        LD_pHLi_A(cpu);
        break;
    case 0x23:
        INC_r16(cpu, &cpu->hl);
        break;
    case 0x24:
        INC_r8(cpu, &cpu->h);
        break;
    case 0x25:
        DEC_r8(cpu, &cpu->h);
        break;
    case 0x26:
        LD_r8_u8(cpu, &cpu->h);
        break;
    case 0x27:
        DAA(cpu);
        break;
    case 0x28:
        JR_cc_i8(cpu, cpu->f.z);
        break;
    case 0x29:
        ADD_HL_r16(cpu, cpu->hl);
        break;
    case 0x2A:
        LD_A_pHLi(cpu);
        break;
    case 0x2B:
        DEC_r16(cpu, &cpu->hl);
        break;
    case 0x2C:
        INC_r8(cpu, &cpu->l);
        break;
    case 0x2D:
        DEC_r8(cpu, &cpu->l);
        break;
    case 0x2E:
        LD_r8_u8(cpu, &cpu->l);
        break;
    case 0x2F:
        CPL(cpu);
        break;

    case 0x30:
        JR_cc_i8(cpu, !cpu->f.c);
        break;
    case 0x31:
        LD_SP_u16(cpu);
        break;
    case 0x32:
        LD_pHLd_A(cpu);
        break;
    case 0x33:
        INC_r16(cpu, &cpu->sp);
        break;
    case 0x34:
        INC_pHL(cpu);
        break;
    case 0x35:
        DEC_pHL(cpu);
        break;
    case 0x36:
        LD_pHL_u8(cpu);
        break;
    case 0x37:
        SCF(cpu);
        break;
    case 0x38:
        JR_cc_i8(cpu, cpu->f.c);
        break;
    case 0x39:
        ADD_HL_r16(cpu, cpu->sp);
        break;
    case 0x3A:
        LD_A_pHLd(cpu);
        break;
    case 0x3B:
        DEC_r16(cpu, &cpu->sp);
        break;
    case 0x3C:
        INC_r8(cpu, &cpu->a);
        break;
    case 0x3D:
        DEC_r8(cpu, &cpu->a);
        break;
    case 0x3E:
        LD_r8_u8(cpu, &cpu->a);
        break;
    case 0x3F:
        CCF(cpu);
        break;

    case 0x40 ... 0x47:
        LD_r8_r8(cpu, &cpu->b, determineRegister(cpu, opcode));
        break;
    case 0x48 ... 0x4F:
        LD_r8_r8(cpu, &cpu->c, determineRegister(cpu, opcode));
        break;
    case 0x50 ... 0x57:
        LD_r8_r8(cpu, &cpu->d, determineRegister(cpu, opcode));
        break;
    case 0x58 ... 0x5F:
        LD_r8_r8(cpu, &cpu->e, determineRegister(cpu, opcode));
        break;
    case 0x60 ... 0x67:
        LD_r8_r8(cpu, &cpu->h, determineRegister(cpu, opcode));
        break;
    case 0x68 ... 0x6F:
        LD_r8_r8(cpu, &cpu->l, determineRegister(cpu, opcode));
        break;
    case 0x70 ... 0x77:
        if (opcode % 8 == 6)
            HALT(cpu);
        else
            LD_pHL_r8(cpu, determineRegister(cpu, opcode));
        break;
    case 0x78 ... 0x7F:
        LD_r8_r8(cpu, &cpu->a, determineRegister(cpu, opcode));
        break;

    case 0x80 ... 0x87:
        ADD_r8(cpu, determineRegister(cpu, opcode));
        break;
    case 0x88 ... 0x8F:
        ADC_r8(cpu, determineRegister(cpu, opcode));
        break;
    case 0x90 ... 0x97:
        SUB_r8(cpu, determineRegister(cpu, opcode));
        break;
    case 0x98 ... 0x9F:
        SBC_r8(cpu, determineRegister(cpu, opcode));
        break;
    case 0xA0 ... 0xA7:
        AND_r8(cpu, determineRegister(cpu, opcode));
        break;
    case 0xA8 ... 0xAF:
        XOR_r8(cpu, determineRegister(cpu, opcode));
        break;
    case 0xB0 ... 0xB7:
        OR_r8(cpu, determineRegister(cpu, opcode));
        break;
    case 0xB8 ... 0xBF:
        CP_r8(cpu, determineRegister(cpu, opcode));
        break;

    case 0xC0:
        RET_cc(cpu, !cpu->f.z);
        break;
    case 0xC1:
        POP_r16(cpu, &cpu->bc);
        break;
    case 0xC2:
        JP_cc_u16(cpu, !cpu->f.z);
        break;
    case 0xC3:
        JP_u16(cpu);
        break;
    case 0xC4:
        CALL_cc_u16(cpu, !cpu->f.z);
        break;
    case 0xC5:
        PUSH_r16(cpu, cpu->bc);
        break;
    case 0xC6:
        ADD_u8(cpu);
        break;
    case 0xC7:
        RST(cpu, _V00h);
        break;
    case 0xC8:
        RET_cc(cpu, cpu->f.z);
        break;
    case 0xC9:
        RET(cpu);
        break;
    case 0xCA:
        JP_cc_u16(cpu, cpu->f.z);
        break;
    case 0xCB:
        CB(cpu);
        break;
    case 0xCC:
        CALL_cc_u16(cpu, cpu->f.z);
        break;
    case 0xCD:
        CALL_u16(cpu);
        break;
    case 0xCE:
        ADC_u8(cpu);
        break;
    case 0xCF:
        RST(cpu, _V08h);
        break;

    case 0xD0:
        RET_cc(cpu, !cpu->f.c);
        break;
    case 0xD1:
        POP_r16(cpu, &cpu->de);
        break;
    case 0xD2:
        JP_cc_u16(cpu, !cpu->f.c);
        break;
    case 0xD3:
        PANIC;
        break;
    case 0xD4:
        CALL_cc_u16(cpu, !cpu->f.c);
        break;
    case 0xD5:
        PUSH_r16(cpu, cpu->de);
        break;
    case 0xD6:
        SUB_u8(cpu);
        break;
    case 0xD7:
        RST(cpu, _V10h);
        break;
    case 0xD8:
        RET_cc(cpu, cpu->f.c);
        break;
    case 0xD9:
        RETI(cpu);
        break;
    case 0xDA:
        JP_cc_u16(cpu, cpu->f.c);
        break;
    case 0xDB:
        PANIC;
        break;
    case 0xDC:
        CALL_cc_u16(cpu, cpu->f.c);
        break;
    case 0xDD:
        PANIC;
        break;
    case 0xDE:
        SBC_u8(cpu);
        break;
    case 0xDF:
        RST(cpu, _V18h);
        break;

    case 0xE0:
        LDH_pu8_A(cpu);
        break;
    case 0xE1:
        POP_r16(cpu, &cpu->hl);
        break;
    case 0xE2:
        LDH_pC_A(cpu);
        break;
    case 0xE3:
        PANIC;
        break;
    case 0xE4:
        PANIC;
        break;
    case 0xE5:
        PUSH_r16(cpu, cpu->hl);
        break;
    case 0xE6:
        AND_u8(cpu);
        break;
    case 0xE7:
        RST(cpu, _V20h);
        break;
    case 0xE8:
        ADD_SP_i8(cpu);
        break;
    case 0xE9:
        JP_HL(cpu);
        break;
    case 0xEA:
        LD_pu16_A(cpu);
        break;
    case 0xEB:
        PANIC;
        break;
    case 0xEC:
        PANIC;
        break;
    case 0xED:
        PANIC;
        break;
    case 0xEE:
        XOR_u8(cpu);
        break;
    case 0xEF:
        RST(cpu, _V28h);
        break;

    case 0xF0:
        LDH_A_pu8(cpu);
        break;
    case 0xF1:
        POP_AF(cpu);
        break;
    case 0xF2:
        LDH_A_pC(cpu);
        break;
    case 0xF3:
        DI(cpu);
        break;
    case 0xF4:
        PANIC;
        break;
    case 0xF5:
        PUSH_r16(cpu, cpu->af);
        break;
    case 0xF6:
        OR_u8(cpu);
        break;
    case 0xF7:
        RST(cpu, _V30h);
        break;
    case 0xF8:
        LD_HL_SPi8(cpu);
        break;
    case 0xF9:
        LD_SP_HL(cpu);
        break;
    case 0xFA:
        LD_A_pu16(cpu);
        break;
    case 0xFB:
        EI(cpu);
        break;
    case 0xFC:
        PANIC;
        break;
    case 0xFD:
        PANIC;
        break;
    case 0xFE:
        CP_u8(cpu);
        break;
    case 0xFF:
        RST(cpu, _V38h);
        break;
    default:
        PANIC;
    }
}

CPU *createCPU(void) {
    CPU *cpu = malloc(sizeof(*cpu));
    memset(cpu, 0, sizeof(*cpu));
    initScheduler(&cpu->sched, cpu);
    cpu->memory.sched = &cpu->sched;
    return cpu;
}

void destroyCPU(CPU *cpu) { free(cpu); }

void updateCPU(CPU *cpu) {
    if (!cpu->halted)
        fetchAndExecuteInstruction(cpu);
    else
        tickM(cpu, 1);
    tickScheduler(&cpu->sched);
}