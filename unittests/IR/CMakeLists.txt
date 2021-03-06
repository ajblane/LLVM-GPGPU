set(LLVM_LINK_COMPONENTS
  Analysis
  AsmParser
  Core
  IPA
  Support
  )

set(IRSources
  AttributesTest.cpp
  ConstantRangeTest.cpp
  ConstantsTest.cpp
  DominatorTreeTest.cpp
  IRBuilderTest.cpp
  InstructionsTest.cpp
  LeakDetectorTest.cpp
  LegacyPassManagerTest.cpp
  MDBuilderTest.cpp
  MetadataTest.cpp
  PassManagerTest.cpp
  PatternMatch.cpp
  TypeBuilderTest.cpp
  TypesTest.cpp
  UserTest.cpp
  ValueHandleTest.cpp
  ValueMapTest.cpp
  ValueTest.cpp
  VerifierTest.cpp
  WaymarkTest.cpp
  )

# HACK: Declare a couple of source files as optionally compiled to satisfy the
# missing-file-checker in LLVM's weird CMake build.
set(LLVM_OPTIONAL_SOURCES
  ValueMapTest.cpp
  )

add_llvm_unittest(IRTests
  ${IRSources}
  )
