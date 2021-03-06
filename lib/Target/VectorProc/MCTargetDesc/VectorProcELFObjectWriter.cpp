//===-- VectorProcELFObjectWriter.cpp - VectorProc ELF Writer
//-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/VectorProcFixupKinds.h"
#include "MCTargetDesc/VectorProcMCTargetDesc.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
class VectorProcELFObjectWriter : public MCELFObjectTargetWriter {
public:
  VectorProcELFObjectWriter(uint8_t OSABI);

  virtual ~VectorProcELFObjectWriter();

protected:
  virtual unsigned GetRelocType(const MCValue &Target, const MCFixup &Fixup,
                                bool IsPCRel) const override;
};
}

VectorProcELFObjectWriter::VectorProcELFObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(/*Is64Bit*/ false, OSABI, ELF::EM_VECTORPROC,
                              /*HasRelocationAddend*/ true) {}

VectorProcELFObjectWriter::~VectorProcELFObjectWriter() {}

unsigned VectorProcELFObjectWriter::GetRelocType(const MCValue &Target,
                                                 const MCFixup &Fixup,
                                                 bool IsPCRel) const {
  unsigned Type;
  unsigned Kind = (unsigned)Fixup.getKind();
  switch (Kind) {
  default:
    llvm_unreachable("Invalid fixup kind!");
  case FK_Data_4:
  case VectorProc::fixup_VectorProc_Abs32:
    Type = ELF::R_VECTORPROC_ABS32;
    break;

  case VectorProc::fixup_VectorProc_PCRel_Branch:
    Type = ELF::R_VECTORPROC_BRANCH;
    break;

  // In normal cases, these types should not be emitted because they can be fixed up immediately.
  // This generally happens if there is an undefined symbol.  This will cause an error later
  // during linking.
  case VectorProc::fixup_VectorProc_PCRel_MemAccExt:
    Type = ELF::R_VECTORPROC_PCREL_MEM_EXT;
    break;
    
  case VectorProc::fixup_VectorProc_PCRel_MemAcc:
    Type = ELF::R_VECTORPROC_PCREL_MEM;
    break;
  }
  return Type;
}

MCObjectWriter *llvm::createVectorProcELFObjectWriter(raw_ostream &OS,
                                                      uint8_t OSABI) {
  MCELFObjectTargetWriter *MOTW = new VectorProcELFObjectWriter(OSABI);
  return createELFObjectWriter(MOTW, OS, true);
}
