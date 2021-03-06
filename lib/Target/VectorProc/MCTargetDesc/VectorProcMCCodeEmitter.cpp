//===-- VectorProcMCCodeEmitter.cpp - Convert VectorProc code to machine code
//---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the VectorProcMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "mccodeemitter"
#include "MCTargetDesc/VectorProcFixupKinds.h"
#include "MCTargetDesc/VectorProcMCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
using namespace llvm;

STATISTIC(MCNumEmitted, "Number of MC instructions emitted");

namespace {
class VectorProcMCCodeEmitter : public MCCodeEmitter {
public:
  VectorProcMCCodeEmitter(const MCInstrInfo &mcii, const MCSubtargetInfo &sti,
                          MCContext &ctx)
      : MCII(mcii), STI(sti), Ctx(ctx) {}


  ~VectorProcMCCodeEmitter() {}

  // getBinaryCodeForInstr - TableGen'erated function for getting the
  // binary encoding for an instruction.
  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  // getMachineOpValue - Return binary encoding of operand. If the machine
  // operand requires relocation, record the relocation and return zero.
  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  unsigned encodeMemoryOpValue(const MCInst &MI, unsigned Op,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCSubtargetInfo &STI) const;

  unsigned encodeLEAValue(const MCInst &MI, unsigned Op,
                          SmallVectorImpl<MCFixup> &Fixups,
                          const MCSubtargetInfo &STI) const;

  unsigned encodeBranchTargetOpValue(const MCInst &MI, unsigned OpNo,
                                     SmallVectorImpl<MCFixup> &Fixups,
                                     const MCSubtargetInfo &STI) const;

  unsigned encodeJumpTableAddr(const MCInst &MI, unsigned OpNo,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCSubtargetInfo &STI) const;

  // Emit one byte through output stream (from MCBlazeMCCodeEmitter)
  void EmitByte(unsigned char C, unsigned &CurByte, raw_ostream &OS) const {
    OS << (char)C;
    ++CurByte;
  }

  void EmitLEConstant(uint64_t Val, unsigned Size, unsigned &CurByte,
                      raw_ostream &OS) const {
    assert(Size <= 8 && "size too big in emit constant");

    for (unsigned i = 0; i != Size; ++i) {
      EmitByte(Val & 255, CurByte, OS);
      Val >>= 8;
    }
  }

  virtual void EncodeInstruction(const MCInst &MI, raw_ostream &OS,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const override;

private:
  VectorProcMCCodeEmitter(const VectorProcMCCodeEmitter &) LLVM_DELETED_FUNCTION; 
  void operator=(const VectorProcMCCodeEmitter &) LLVM_DELETED_FUNCTION; 
  const MCInstrInfo &MCII;
  const MCSubtargetInfo &STI;
  MCContext &Ctx;
};
} // end anonymous namepsace

MCCodeEmitter *llvm::createVectorProcMCCodeEmitter(const MCInstrInfo &MCII,
                                                   const MCRegisterInfo &MRI,
                                                   const MCSubtargetInfo &STI,
                                                   MCContext &Ctx) {

  return new VectorProcMCCodeEmitter(MCII, STI, Ctx);
}

/// getMachineOpValue - Return binary encoding of operand.
unsigned VectorProcMCCodeEmitter::getMachineOpValue(
    const MCInst &MI, const MCOperand &MO, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {

  if (MO.isReg())
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());

  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());

  return 0;
}

/// encodeBranchTargetOpValue - Return binary encoding of the jump
/// target operand. If the machine operand requires relocation,
/// record the relocation and return zero.
unsigned VectorProcMCCodeEmitter::encodeBranchTargetOpValue(
    const MCInst &MI, unsigned OpNo, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {

  const MCOperand &MO = MI.getOperand(OpNo);
  if (MO.isImm())
    return MO.getImm();

  assert(MO.isExpr() &&
         "encodeBranchTargetOpValue expects only expressions or an immediate");

  const MCExpr *Expr = MO.getExpr();
  Fixups.push_back(MCFixup::Create(
      0, Expr, MCFixupKind(VectorProc::fixup_VectorProc_PCRel_Branch)));
  return 0;
}

void
VectorProcMCCodeEmitter::EncodeInstruction(const MCInst &MI, raw_ostream &OS,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {

  // Keep track of the current byte being emitted
  unsigned CurByte = 0;

  // Get instruction encoding and emit it
  ++MCNumEmitted; // Keep track of the number of emitted insns.
  unsigned Value = getBinaryCodeForInstr(MI, Fixups, STI);
  EmitLEConstant(Value, 4, CurByte, OS);
}

unsigned
VectorProcMCCodeEmitter::encodeJumpTableAddr(const MCInst &MI, unsigned Op,
                                             SmallVectorImpl<MCFixup> &Fixups,
                                             const MCSubtargetInfo &STI) const {
  MCOperand label = MI.getOperand(2);
  Fixups.push_back(MCFixup::Create(
      0, label.getExpr(),
      MCFixupKind(VectorProc::fixup_VectorProc_PCRel_ComputeLabelAddress)));
  return 0;
}

unsigned
VectorProcMCCodeEmitter::encodeLEAValue(const MCInst &MI, unsigned Op,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {

  MCOperand baseReg = MI.getOperand(1);
  MCOperand offsetOp = MI.getOperand(2);

  assert(baseReg.isReg() && "First operand of LEA op is not register.");
  unsigned encoding = Ctx.getRegisterInfo()->getEncodingValue(baseReg.getReg());

  if (offsetOp.isExpr()) {
    // Load with a label. This is a PC relative load.  Add a fixup.
    Fixups.push_back(MCFixup::Create(
        0, offsetOp.getExpr(),
        MCFixupKind(VectorProc::fixup_VectorProc_PCRel_ComputeLabelAddress)));
  } else if (offsetOp.isImm())
    encoding |= static_cast<short>(offsetOp.getImm()) << 5;
  else
    assert(offsetOp.isImm() && "Second operand of LEA op is unknown type.");

  return encoding;
}

// Encode VectorProc Memory Operand.  The result is a packed field with the
// register in the low 5 bits and the offset in the remainder.  The instruction
// patterns will put these into the proper part of the instruction
// (VectorProcInstrFormats.td).
unsigned
VectorProcMCCodeEmitter::encodeMemoryOpValue(const MCInst &MI, unsigned Op,
                                             SmallVectorImpl<MCFixup> &Fixups,
                                             const MCSubtargetInfo &STI) const {
  unsigned encoding;

  MCOperand baseReg;
  MCOperand offsetOp;

  if (MI.getOpcode() == VectorProc::STORE_SYNC) {
    // Store sync has an additional machine operand for the success value
    baseReg = MI.getOperand(2);
    offsetOp = MI.getOperand(3);
  } else {
    baseReg = MI.getOperand(1);
    offsetOp = MI.getOperand(2);
  }

  // Register
  // This is register/offset.  No need for relocation.
  assert(baseReg.isReg() && "First operand is not register.");
  encoding = Ctx.getRegisterInfo()->getEncodingValue(baseReg.getReg());

  // Offset
  if (offsetOp.isExpr()) {
    // Load with a label. This is a PC relative load.  Add a fixup.
    // XXX Note that this assumes unmasked instructions.  A masked
    // instruction will not work and should nto be used.
    Fixups.push_back(MCFixup::Create(
        0, offsetOp.getExpr(),
        MCFixupKind(VectorProc::fixup_VectorProc_PCRel_MemAccExt)));
  } else if (offsetOp.isImm())
    encoding |= static_cast<short>(offsetOp.getImm()) << 5;
  else
    assert(offsetOp.isImm() && "Second operand of memory op is unknown type.");

  return encoding;
}

#include "VectorProcGenMCCodeEmitter.inc"
