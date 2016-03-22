/*
Copyright (c) 2015-2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/**
 * @file Cuda2Hip.cpp
 *
 * This file is compiled and linked into clang based hipify tool.
 */
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/Debug.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/MacroArgs.h"

#include <fstream>
#include <cstdio>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

#define DEBUG_TYPE "cuda2hip"

namespace {
  struct hipName {
  hipName() {
      // defines
      cuda2hipRename["__CUDACC__"] = "__HIPCC__";

      // includes
      cuda2hipRename["cuda_runtime.h"] = "hip_runtime.h";
      cuda2hipRename["cuda_runtime_api.h"] = "hip_runtime_api.h";

      // Error codes and return types:
      cuda2hipRename["cudaError_t"] = "hipError_t";
      cuda2hipRename["cudaError"] = "hipError";
      cuda2hipRename["cudaSuccess"] = "hipSuccess";

      cuda2hipRename["cudaErrorUnknown"] = "hipErrorUnknown";
      cuda2hipRename["cudaErrorMemoryAllocation"] = "hipErrorMemoryAllocation";
      cuda2hipRename["cudaErrorMemoryFree"] = "hipErrorMemoryFree";
      cuda2hipRename["cudaErrorUnknownSymbol"] = "hipErrorUnknownSymbol";
      cuda2hipRename["cudaErrorOutOfResources"] = "hipErrorOutOfResources";
      cuda2hipRename["cudaErrorInvalidValue"] = "hipErrorInvalidValue";
      cuda2hipRename["cudaErrorInvalidResourceHandle"] = "hipErrorInvalidResourceHandle";
      cuda2hipRename["cudaErrorInvalidDevice"] = "hipErrorInvalidDevice";
      cuda2hipRename["cudaErrorNoDevice"] = "hipErrorNoDevice";
      cuda2hipRename["cudaErrorNotReady"] = "hipErrorNotReady";
      cuda2hipRename["cudaErrorUnknown"] = "hipErrorUnknown";

      // error APIs:
      cuda2hipRename["cudaGetLastError"] = "hipGetLastError";
      cuda2hipRename["cudaPeekAtLastError"] = "hipPeekAtLastError";
      cuda2hipRename["cudaGetErrorName"] = "hipGetErrorName";
      cuda2hipRename["cudaGetErrorString"] = "hipGetErrorString";

      // Memcpy
      cuda2hipRename["cudaMemcpy"] = "hipMemcpy";
      cuda2hipRename["cudaMemcpyHostToHost"] = "hipMemcpyHostToHost";
      cuda2hipRename["cudaMemcpyHostToDevice"] = "hipMemcpyHostToDevice";
      cuda2hipRename["cudaMemcpyDeviceToHost"] = "hipMemcpyDeviceToHost";
      cuda2hipRename["cudaMemcpyDeviceToDevice"] = "hipMemcpyDeviceToDevice";
      cuda2hipRename["cudaMemcpyDefault"] = "hipMemcpyDefault";
      cuda2hipRename["cudaMemcpyToSymbol"] = "hipMemcpyToSymbol";
      cuda2hipRename["cudaMemset"] = "hipMemset";
      cuda2hipRename["cudaMemsetAsync"] = "hipMemsetAsync";
      cuda2hipRename["cudaMemcpyAsync"] = "hipMemcpyAsync";
      cuda2hipRename["cudaMemGetInfo"] = "hipMemGetInfo";
      cuda2hipRename["cudaMemcpyKind"] = "hipMemcpyKind";

      // Memory management :
      cuda2hipRename["cudaMalloc"] = "hipMalloc";
      cuda2hipRename["cudaMallocHost"] = "hipHostAlloc";
      cuda2hipRename["cudaFree"] = "hipFree";
      cuda2hipRename["cudaFreeHost"] = "hipHostFree";

      // Coordinate Indexing and Dimensions:
      cuda2hipRename["threadIdx.x"] = "hipThreadIdx_x";
      cuda2hipRename["threadIdx.y"] = "hipThreadIdx_y";
      cuda2hipRename["threadIdx.z"] = "hipThreadIdx_z";

      cuda2hipRename["blockIdx.x"] = "hipBlockIdx_x";
      cuda2hipRename["blockIdx.y"] = "hipBlockIdx_y";
      cuda2hipRename["blockIdx.z"] = "hipBlockIdx_z";

      cuda2hipRename["blockDim.x"] = "hipBlockDim_x";
      cuda2hipRename["blockDim.y"] = "hipBlockDim_y";
      cuda2hipRename["blockDim.z"] = "hipBlockDim_z";

      cuda2hipRename["gridDim.x"] = "hipGridDim_x";
      cuda2hipRename["gridDim.y"] = "hipGridDim_y";
      cuda2hipRename["gridDim.z"] = "hipGridDim_z";

      cuda2hipRename["blockIdx.x"] = "hipBlockIdx_x";
      cuda2hipRename["blockIdx.y"] = "hipBlockIdx_y";
      cuda2hipRename["blockIdx.z"] = "hipBlockIdx_z";

      cuda2hipRename["blockDim.x"] = "hipBlockDim_x";
      cuda2hipRename["blockDim.y"] = "hipBlockDim_y";
      cuda2hipRename["blockDim.z"] = "hipBlockDim_z";

      cuda2hipRename["gridDim.x"] = "hipGridDim_x";
      cuda2hipRename["gridDim.y"] = "hipGridDim_y";
      cuda2hipRename["gridDim.z"] = "hipGridDim_z";


      cuda2hipRename["warpSize"] = "hipWarpSize";

      // Events
      cuda2hipRename["cudaEvent_t"] = "hipEvent_t";
      cuda2hipRename["cudaEventCreate"] = "hipEventCreate";
      cuda2hipRename["cudaEventCreateWithFlags"] = "hipEventCreateWithFlags";
      cuda2hipRename["cudaEventDestroy"] = "hipEventDestroy";
      cuda2hipRename["cudaEventRecord"] = "hipEventRecord";
      cuda2hipRename["cudaEventElapsedTime"] = "hipEventElapsedTime";
      cuda2hipRename["cudaEventSynchronize"] = "hipEventSynchronize";

      // Streams
      cuda2hipRename["cudaStream_t"] = "hipStream_t";
      cuda2hipRename["cudaStreamCreate"] = "hipStreamCreate";
      cuda2hipRename["cudaStreamCreateWithFlags"] = "hipStreamCreateWithFlags";
      cuda2hipRename["cudaStreamDestroy"] = "hipStreamDestroy";
      cuda2hipRename["cudaStreamWaitEvent"] = "hipStreamWaitEven";
      cuda2hipRename["cudaStreamSynchronize"] = "hipStreamSynchronize";
      cuda2hipRename["cudaStreamDefault"] = "hipStreamDefault";
      cuda2hipRename["cudaStreamNonBlocking"] = "hipStreamNonBlocking";

      // Other synchronization
      cuda2hipRename["cudaDeviceSynchronize"] = "hipDeviceSynchronize";
      cuda2hipRename["cudaThreadSynchronize"] = "hipDeviceSynchronize";  // translate deprecated cudaThreadSynchronize
      cuda2hipRename["cudaDeviceReset"] = "hipDeviceReset";
      cuda2hipRename["cudaThreadExit"] = "hipDeviceReset";               // translate deprecated cudaThreadExit
      cuda2hipRename["cudaSetDevice"] = "hipSetDevice";
      cuda2hipRename["cudaGetDevice"] = "hipGetDevice";

      // Device
      cuda2hipRename["cudaDeviceProp"] = "hipDeviceProp_t";
      cuda2hipRename["cudaGetDeviceProperties"] = "hipDeviceGetProperties";

      // Cache config
      cuda2hipRename["cudaDeviceSetCacheConfig"] = "hipDeviceSetCacheConfig";
      cuda2hipRename["cudaThreadSetCacheConfig"] = "hipDeviceSetCacheConfig"; // translate deprecated
      cuda2hipRename["cudaDeviceGetCacheConfig"] = "hipDeviceGetCacheConfig";
      cuda2hipRename["cudaThreadGetCacheConfig"] = "hipDeviceGetCacheConfig"; // translate deprecated
      cuda2hipRename["cudaFuncCache"] = "hipFuncCache";
      cuda2hipRename["cudaFuncCachePreferNone"] = "hipFuncCachePreferNone";
      cuda2hipRename["cudaFuncCachePreferShared"] = "hipFuncCachePreferShared";
      cuda2hipRename["cudaFuncCachePreferL1"] = "hipFuncCachePreferL1";
      cuda2hipRename["cudaFuncCachePreferEqual"] = "hipFuncCachePreferEqual";
      // function
      cuda2hipRename["cudaFuncSetCacheConfig"] = "hipFuncSetCacheConfig";

      cuda2hipRename["cudaDriverGetVersion"] = "hipDriverGetVersion";
      cuda2hipRename["cudaRuntimeGetVersion"] = "hipRuntimeGetVersion";

      // Peer2Peer
      cuda2hipRename["cudaDeviceCanAccessPeer"] = "hipDeviceCanAccessPeer";
      cuda2hipRename["cudaDeviceDisablePeerAccess"] = "hipDeviceDisablePeerAccess";
      cuda2hipRename["cudaDeviceEnablePeerAccess"] = "hipDeviceEnablePeerAccess";
      cuda2hipRename["cudaMemcpyPeerAsync"] = "hipMemcpyPeerAsync";
      cuda2hipRename["cudaMemcpyPeer"] = "hipMemcpyPeer";

      // Shared mem:
      cuda2hipRename["cudaDeviceSetSharedMemConfig"] = "hipDeviceSetSharedMemConfig";
      cuda2hipRename["cudaThreadSetSharedMemConfig"] = "hipDeviceSetSharedMemConfig"; // translate deprecated
      cuda2hipRename["cudaDeviceGetSharedMemConfig"] = "hipDeviceGetSharedMemConfig";
      cuda2hipRename["cudaThreadGetSharedMemConfig"] = "hipDeviceGetSharedMemConfig";  // translate deprecated
      cuda2hipRename["cudaSharedMemConfig"] = "hipSharedMemConfig";
      cuda2hipRename["cudaSharedMemBankSizeDefault"] = "hipSharedMemBankSizeDefault";
      cuda2hipRename["cudaSharedMemBankSizeFourByte"] = "hipSharedMemBankSizeFourByte";
      cuda2hipRename["cudaSharedMemBankSizeEightByte"] = "hipSharedMemBankSizeEightByte";

      cuda2hipRename["cudaGetDeviceCount"] = "hipGetDeviceCount";

      // Profiler
      //cuda2hipRename["cudaProfilerInitialize"] = "hipProfilerInitialize";  // see if these are called anywhere.
      cuda2hipRename["cudaProfilerStart"] = "hipProfilerStart";
      cuda2hipRename["cudaProfilerStop"] = "hipProfilerStop";

      cuda2hipRename["cudaChannelFormatDesc"] = "hipChannelFormatDesc";
      cuda2hipRename["cudaFilterModePoint"] = "hipFilterModePoint";
      cuda2hipRename["cudaReadModeElementType"] = "hipReadModeElementType";

      cuda2hipRename["cudaCreateChannelDesc"] = "hipCreateChannelDesc";
      cuda2hipRename["cudaBindTexture"] = "hipBindTexture";
      cuda2hipRename["cudaUnbindTexture"] = "hipUnbindTexture";
    }
    DenseMap<StringRef, StringRef> cuda2hipRename;
  };

  StringRef unquoteStr(StringRef s) {
    if (s.size() > 1 && s.front() == '"' && s.back() == '"')
      return s.substr(1, s.size()-2);
    return s;
  }

  void processString(StringRef s, struct hipName & map,
   Replacements * Replace, SourceManager & SM, SourceLocation start)
  {
    size_t begin = 0;
    while ((begin = s.find("cuda", begin)) != StringRef::npos) {
      const size_t end = s.find_first_of(" ", begin + 4);
      StringRef name = s.slice(begin, end);
      StringRef repName = map.cuda2hipRename[name];
      if (!repName.empty()) {
        SourceLocation sl = start.getLocWithOffset(begin + 1);
        Replacement Rep(SM, sl, name.size(), repName);
        Replace->insert(Rep);
      }
      if (end == StringRef::npos) break;
      begin = end + 1;
    }
  }


  struct HipifyPPCallbacks : public PPCallbacks, public SourceFileCallbacks {
    HipifyPPCallbacks(Replacements * R)
      : SeenEnd(false), _sm(nullptr), _pp(nullptr), Replace(R)
    {
    }

    virtual bool handleBeginSource(CompilerInstance &CI, StringRef Filename) override
    {
      Preprocessor &PP = CI.getPreprocessor();
      SourceManager & SM = CI.getSourceManager();
      setSourceManager(&SM);
      PP.addPPCallbacks(std::unique_ptr<HipifyPPCallbacks>(this));
      PP.Retain();
      setPreprocessor(&PP);
      return true;
    }

    virtual void InclusionDirective(
      SourceLocation hash_loc,
      const Token &include_token,
      StringRef file_name,
      bool is_angled,
      CharSourceRange filename_range,
      const FileEntry *file,
      StringRef search_path,
      StringRef relative_path,
      const clang::Module *imported) override
    {
      if (_sm->isWrittenInMainFile(hash_loc)) {
        if (is_angled) {
          if (N.cuda2hipRename.count(file_name)) {
            std::string repName = N.cuda2hipRename[file_name];
            DEBUG(dbgs() << "Include file found: " << file_name << "\n");
            DEBUG(dbgs() << "SourceLocation:"
              << filename_range.getBegin().printToString(*_sm) << "\n");
            DEBUG(dbgs() << "Will be replaced with " << repName << "\n");
            SourceLocation sl = filename_range.getBegin();
            SourceLocation sle = filename_range.getEnd();
            const char* B = _sm->getCharacterData(sl);
            const char* E = _sm->getCharacterData(sle);
            repName = "<" + repName + ">";
            Replacement Rep(*_sm, sl, E - B, repName);
            Replace->insert(Rep);
          }
        }
      }
    }

    virtual void MacroDefined(const Token &MacroNameTok,
      const MacroDirective *MD) override
    {
      if (_sm->isWrittenInMainFile(MD->getLocation()) &&
          MD->getKind() == MacroDirective::MD_Define)
      {
        for (auto T : MD->getMacroInfo()->tokens())
        {
          if (T.isAnyIdentifier()) {
            StringRef name = T.getIdentifierInfo()->getName();
            if (N.cuda2hipRename.count(name)) {
              StringRef repName = N.cuda2hipRename[name];
              DEBUG(dbgs() << "Identifier " << name
                << " found in definition of macro "
                << MacroNameTok.getIdentifierInfo()->getName() << "\n");
              DEBUG(dbgs() << "will be replaced with: " << repName << "\n");
              SourceLocation sl = T.getLocation();
              DEBUG(dbgs() << "SourceLocation: " << sl.printToString(*_sm) << "\n");
              Replacement Rep(*_sm, sl, name.size(), repName);
              Replace->insert(Rep);
            }
          }
        }
      }
    }

    virtual void MacroExpands(const Token &MacroNameTok,
      const MacroDefinition &MD, SourceRange Range,
      const MacroArgs *Args) override
    {
      if (_sm->isWrittenInMainFile(MacroNameTok.getLocation()))
      {
        for (unsigned int i = 0; Args && i < MD.getMacroInfo()->getNumArgs(); i++)
        {
          StringRef macroName = MacroNameTok.getIdentifierInfo()->getName();
          std::vector<Token> toks;
          // Code below is a kind of stolen from 'MacroArgs::getPreExpArgument'
          // to workaround the 'const' MacroArgs passed into this hook.
          const Token * start = Args->getUnexpArgument(i);
          size_t len = Args->getArgLength(start) + 1;
#if (LLVM_VERSION_MAJOR >= 3) && (LLVM_VERSION_MINOR >= 9)
          _pp->EnterTokenStream(ArrayRef<Token>(start,len), false);
#else
          _pp->EnterTokenStream(start, len, false, false);
#endif
          do {
            toks.push_back(Token());
            Token & tk = toks.back();
            _pp->Lex(tk);
          } while (toks.back().isNot(tok::eof));
          _pp->RemoveTopOfLexerStack();
          // end of stolen code
          for (auto tok : toks) {
            if (tok.isAnyIdentifier())
            {
              StringRef name = tok.getIdentifierInfo()->getName();
              if (N.cuda2hipRename.count(name)) {
                StringRef repName = N.cuda2hipRename[name];
                DEBUG(dbgs() << "Identifier " << name
                  << " found as an actual argument in expansion of macro "
                  << macroName << "\n");
                DEBUG(dbgs() << "will be replaced with: " << repName << "\n");
                SourceLocation sl = tok.getLocation();
                Replacement Rep(*_sm, sl, name.size(), repName);
                Replace->insert(Rep);
              }
            }
            if (tok.is(tok::string_literal))
            {
              StringRef s(tok.getLiteralData(), tok.getLength());
              processString(unquoteStr(s), N, Replace, *_sm, tok.getLocation());
            }
          }
        }
      }
    }

    void EndOfMainFile() override
    {

    }

    bool SeenEnd;
    void setSourceManager(SourceManager * sm) { _sm = sm; }
    void setPreprocessor (Preprocessor  * pp) { _pp = pp; }

    private:

    SourceManager * _sm;
    Preprocessor * _pp;

    Replacements * Replace;
    struct hipName N;
  };

class Cuda2HipCallback : public MatchFinder::MatchCallback {
 public:
   Cuda2HipCallback(Replacements *Replace, ast_matchers::MatchFinder *parent) 
   : Replace(Replace), owner(parent) {}

   void convertKernelDecl(const FunctionDecl * kernelDecl, const MatchFinder::MatchResult &Result)
   {
     SourceManager * SM = Result.SourceManager;
     LangOptions DefaultLangOptions;

     SmallString<40> XStr;
     raw_svector_ostream OS(XStr);
     StringRef initialParamList;
     OS << "hipLaunchParm lp";
     size_t replacementLength = OS.str().size();
     SourceLocation sl = kernelDecl->getNameInfo().getEndLoc();
     SourceLocation kernelArgListStart = clang::Lexer::findLocationAfterToken(sl, clang::tok::l_paren, *SM, DefaultLangOptions, true);
     DEBUG(dbgs() << kernelArgListStart.printToString(*SM));
     if (kernelDecl->getNumParams() > 0) {
       const ParmVarDecl * pvdFirst = kernelDecl->getParamDecl(0);
       const ParmVarDecl * pvdLast = kernelDecl->getParamDecl(kernelDecl->getNumParams() - 1);
       SourceLocation kernelArgListStart(pvdFirst->getLocStart());
       SourceLocation kernelArgListEnd(pvdLast->getLocEnd());
       SourceLocation stop = clang::Lexer::getLocForEndOfToken(kernelArgListEnd, 0, *SM, DefaultLangOptions);
       size_t replacementLength = SM->getCharacterData(stop) - SM->getCharacterData(kernelArgListStart);
       initialParamList = StringRef(SM->getCharacterData(kernelArgListStart), replacementLength);
       OS << ", " << initialParamList;
     }
     DEBUG(dbgs() << "initial paramlist: " << initialParamList << "\n");
     DEBUG(dbgs() << "new paramlist: " << OS.str() << "\n");
     Replacement Rep0(*(Result.SourceManager), kernelArgListStart, replacementLength, OS.str());
     Replace->insert(Rep0);
  }

  void run(const MatchFinder::MatchResult &Result) override {

    SourceManager * SM = Result.SourceManager;
    LangOptions DefaultLangOptions;

    if (const CallExpr * call = Result.Nodes.getNodeAs<clang::CallExpr>("cudaCall"))
    {
      const FunctionDecl * funcDcl = call->getDirectCallee();
      std::string name = funcDcl->getDeclName().getAsString();
      if (N.cuda2hipRename.count(name)) {
        std::string repName = N.cuda2hipRename[name];
        SourceLocation sl = call->getLocStart();
        Replacement Rep(*SM, SM->isMacroArgExpansion(sl) ?
        SM->getImmediateSpellingLoc(sl) : sl, name.length(), repName);
        Replace->insert(Rep);
      }
    }

	  if (const CUDAKernelCallExpr * launchKernel = Result.Nodes.getNodeAs<clang::CUDAKernelCallExpr>("cudaLaunchKernel"))
	  {
      SmallString<40> XStr;
      raw_svector_ostream OS(XStr);
      StringRef calleeName;
		  const FunctionDecl * kernelDecl = launchKernel->getDirectCallee();
      if (kernelDecl) {
        calleeName = kernelDecl->getName();
        convertKernelDecl(kernelDecl, Result);
      }
      else {
        const Expr * e = launchKernel->getCallee();
        if (const UnresolvedLookupExpr * ule = dyn_cast<UnresolvedLookupExpr>(e)) {
          calleeName = ule->getName().getAsIdentifierInfo()->getName();
          owner->addMatcher(functionTemplateDecl(hasName(calleeName)).bind("unresolvedTemplateName"), this);
        }
      }


      XStr.clear();
      OS << "hipLaunchKernel(HIP_KERNEL_NAME(" << calleeName << "), ";

      const CallExpr * config = launchKernel->getConfig();
      DEBUG(dbgs() << "Kernel config arguments:" << "\n");
      for (unsigned argno = 0; argno < config->getNumArgs(); argno++)
      {
        const Expr * arg = config->getArg(argno);
        if (!isa<CXXDefaultArgExpr>(arg)) {
          const ParmVarDecl * pvd = config->getDirectCallee()->getParamDecl(argno);

          SourceLocation sl(arg->getLocStart());
          SourceLocation el(arg->getLocEnd());
          SourceLocation stop = clang::Lexer::getLocForEndOfToken(el, 0, *SM, DefaultLangOptions);
          StringRef outs(SM->getCharacterData(sl), SM->getCharacterData(stop) - SM->getCharacterData(sl));
          DEBUG(dbgs() << "args[ " << argno << "]" << outs << " <"
            << pvd->getType().getAsString() << ">" << "\n");
          if (pvd->getType().getAsString().compare("dim3") == 0)
            OS << " dim3(" << outs << "),";
          else
            OS << " " << outs << ",";
        } else
          OS << " 0,";
      }

      for (unsigned argno = 0; argno < launchKernel->getNumArgs(); argno++)
      {
        const Expr * arg = launchKernel->getArg(argno);
        SourceLocation sl(arg->getLocStart());
        SourceLocation el(arg->getLocEnd());
        SourceLocation stop = clang::Lexer::getLocForEndOfToken(el, 0, *SM, DefaultLangOptions);
        std::string outs(SM->getCharacterData(sl), SM->getCharacterData(stop) - SM->getCharacterData(sl));
        DEBUG(dbgs() << outs << "\n");
        OS << " " << outs << ",";
      }
      XStr.pop_back();
      OS << ")";
      size_t length = SM->getCharacterData(clang::Lexer::getLocForEndOfToken(launchKernel->getLocEnd(), 0, *SM, DefaultLangOptions)) -
        SM->getCharacterData(launchKernel->getLocStart());
      Replacement Rep(*SM, launchKernel->getLocStart(), length, OS.str());
      Replace->insert(Rep);
	  }

    if (const FunctionTemplateDecl * templateDecl = Result.Nodes.getNodeAs<clang::FunctionTemplateDecl>("unresolvedTemplateName"))
    {
      FunctionDecl * kernelDecl = templateDecl->getTemplatedDecl();
      convertKernelDecl(kernelDecl, Result);
    }

	  if (const MemberExpr * threadIdx = Result.Nodes.getNodeAs<clang::MemberExpr>("cudaBuiltin"))
	  {
		  if (const OpaqueValueExpr * refBase = dyn_cast<OpaqueValueExpr>(threadIdx->getBase())) {
        if (const DeclRefExpr * declRef = dyn_cast<DeclRefExpr>(refBase->getSourceExpr())) {
          std::string name = declRef->getDecl()->getNameAsString();
          std::string memberName = threadIdx->getMemberDecl()->getNameAsString();
          size_t pos = memberName.find_first_not_of("__fetch_builtin_");
          memberName = memberName.substr(pos, memberName.length() - pos);
          name += "." + memberName;
     		  std::string repName = N.cuda2hipRename[name];
          SourceLocation sl = threadIdx->getLocStart();
          Replacement Rep(*SM, sl, name.length(), repName);
			    Replace->insert(Rep);
        }
		  }
	  }

    if (const DeclRefExpr * cudaEnumConstantRef = Result.Nodes.getNodeAs<clang::DeclRefExpr>("cudaEnumConstantRef"))
    {
      std::string name = cudaEnumConstantRef->getDecl()->getNameAsString();
      std::string repName = N.cuda2hipRename[name];
      SourceLocation sl = cudaEnumConstantRef->getLocStart();
      Replacement Rep(*SM, sl, name.length(), repName);
      Replace->insert(Rep);
    }

    if (const VarDecl * cudaEnumConstantDecl = Result.Nodes.getNodeAs<clang::VarDecl>("cudaEnumConstantDecl"))
    {
      std::string name = cudaEnumConstantDecl->getType()->getAsTagDecl()->getNameAsString();
      std::string repName = N.cuda2hipRename[name];
      SourceLocation sl = cudaEnumConstantDecl->getLocStart();
      Replacement Rep(*SM, sl, name.length(), repName);
      Replace->insert(Rep);
    }

    if (const VarDecl * cudaStructVar = Result.Nodes.getNodeAs<clang::VarDecl>("cudaStructVar"))
    {
      std::string name = 
        cudaStructVar->getType()->getAsStructureType()->getDecl()->getNameAsString();
      std::string repName = N.cuda2hipRename[name];
      TypeLoc TL = cudaStructVar->getTypeSourceInfo()->getTypeLoc();
      SourceLocation sl = TL.getUnqualifiedLoc().getLocStart();
      Replacement Rep(*SM, sl, name.length(), repName);
      Replace->insert(Rep);
    }

    if (const VarDecl * cudaStructVarPtr = Result.Nodes.getNodeAs<clang::VarDecl>("cudaStructVarPtr"))
    {
      const Type * t = cudaStructVarPtr->getType().getTypePtrOrNull();
      if (t) {
        StringRef name = t->getPointeeCXXRecordDecl()->getName();
        StringRef repName = N.cuda2hipRename[name];
        TypeLoc TL = cudaStructVarPtr->getTypeSourceInfo()->getTypeLoc();
        SourceLocation sl = TL.getUnqualifiedLoc().getLocStart();
        Replacement Rep(*SM, sl, name.size(), repName);
        Replace->insert(Rep);
      }
    }


    if (const ParmVarDecl * cudaParamDecl = Result.Nodes.getNodeAs<clang::ParmVarDecl>("cudaParamDecl"))
    {
      QualType QT = cudaParamDecl->getOriginalType().getUnqualifiedType();
      StringRef name = QT.getAsString();
      const Type * t = QT.getTypePtr();
      if (t->isStructureOrClassType()) {
        name = t->getAsCXXRecordDecl()->getName();
      }
      StringRef repName = N.cuda2hipRename[name];
      TypeLoc TL = cudaParamDecl->getTypeSourceInfo()->getTypeLoc();
      SourceLocation sl = TL.getUnqualifiedLoc().getLocStart();
      Replacement Rep(*SM, sl, name.size(), repName);
      Replace->insert(Rep);
    }

    if (const ParmVarDecl * cudaParamDeclPtr = Result.Nodes.getNodeAs<clang::ParmVarDecl>("cudaParamDeclPtr"))
    {
      const Type * pt = cudaParamDeclPtr->getType().getTypePtrOrNull();
      if (pt) {
        QualType QT = pt->getPointeeType();
        const Type * t = QT.getTypePtr();
        StringRef name = t->isStructureOrClassType()?
          t->getAsCXXRecordDecl()->getName() : StringRef(QT.getAsString());
        StringRef repName = N.cuda2hipRename[name];
        TypeLoc TL = cudaParamDeclPtr->getTypeSourceInfo()->getTypeLoc();
        SourceLocation sl = TL.getUnqualifiedLoc().getLocStart();
        Replacement Rep(*SM, sl, name.size(), repName);
        Replace->insert(Rep);
      }
    }


    if (const StringLiteral * stringLiteral = Result.Nodes.getNodeAs<clang::StringLiteral>("stringLiteral"))
    {
      if (stringLiteral->getCharByteWidth() == 1) {
        StringRef s = stringLiteral->getString();
        processString(s, N, Replace, *SM, stringLiteral->getLocStart());
      }
    }

    if (const UnaryExprOrTypeTraitExpr * expr = Result.Nodes.getNodeAs<clang::UnaryExprOrTypeTraitExpr>("cudaStructSizeOf"))
    {
      TypeSourceInfo * typeInfo = expr->getArgumentTypeInfo();
      QualType QT = typeInfo->getType().getUnqualifiedType();
      const Type * type = QT.getTypePtr();
      StringRef name = type->getAsCXXRecordDecl()->getName();
      StringRef repName = N.cuda2hipRename[name];
      TypeLoc TL = typeInfo->getTypeLoc();
      SourceLocation sl = TL.getUnqualifiedLoc().getLocStart();
      Replacement Rep(*SM, sl, name.size(), repName);
      Replace->insert(Rep);
    }
  }

 private:
  Replacements *Replace;
  ast_matchers::MatchFinder * owner;
  struct hipName N;
};

} // end anonymous namespace

// Set up the command line options
static cl::OptionCategory ToolTemplateCategory("CUDA to HIP source translator options");
static cl::extrahelp MoreHelp( "<source0> specify the path of source file\n\n" );

static cl::opt<std::string>
OutputFilename("o", cl::desc("Output filename"), cl::value_desc("filename"), cl::cat(ToolTemplateCategory));

//static cl::opt<bool, true>
//Debug("debug", cl::desc("Enable debug output"), cl::Hidden, cl::location(llvm::DebugFlag));

static cl::opt<bool>
Inplace("inplace", cl::desc("Modify input file inplace, replacing input with hipified output, save backup in .prehip file. "
  "If .prehip file exists, use that as input to hip."), cl::value_desc("inplace"), cl::cat(ToolTemplateCategory));

int main(int argc, const char **argv) {

  llvm::sys::PrintStackTraceOnErrorSignal();

  int Result;

  CommonOptionsParser OptionsParser(argc, argv, ToolTemplateCategory, llvm::cl::Required);
  std::string dst = OutputFilename;
  std::vector<std::string> fileSources = OptionsParser.getSourcePathList();
  if (dst.empty()) {
    dst = fileSources[0];
    if (!Inplace) {
      size_t pos = dst.rfind(".cu");
      if (pos != std::string::npos) {
        dst = dst.substr(0, pos) + ".hip.cu";
      } else {
        llvm::errs() << "Input .cu file was not specified.\n";
        return 1;
      }
    }
  } else {
    if (Inplace) {
      llvm::errs() << "Conflict: both -o and -inplace options are specified.";
    }
    dst += ".cu";
  }

  std::ifstream source(fileSources[0], std::ios::binary);
  std::ofstream dest(Inplace ? dst+".prehip" : dst, std::ios::binary);
  dest << source.rdbuf();
  source.close();
  dest.close();

  RefactoringTool Tool(OptionsParser.getCompilations(), dst);
  ast_matchers::MatchFinder Finder;
  Cuda2HipCallback Callback(&Tool.getReplacements(), &Finder);
  HipifyPPCallbacks PPCallbacks(&Tool.getReplacements());
  Finder.addMatcher(callExpr(isExpansionInMainFile(), callee(functionDecl(matchesName("cuda.*")))).bind("cudaCall"), &Callback);
  Finder.addMatcher(cudaKernelCallExpr().bind("cudaLaunchKernel"), &Callback);
  Finder.addMatcher(memberExpr(isExpansionInMainFile(), hasObjectExpression(hasType(cxxRecordDecl(matchesName("__cuda_builtin_"))))).bind("cudaBuiltin"), &Callback);
  Finder.addMatcher(declRefExpr(isExpansionInMainFile(), to(enumConstantDecl(matchesName("cuda.*")))).bind("cudaEnumConstantRef"), &Callback);
  Finder.addMatcher(varDecl(isExpansionInMainFile(), hasType(enumDecl(matchesName("cuda.*")))).bind("cudaEnumConstantDecl"), &Callback);
  Finder.addMatcher(varDecl(isExpansionInMainFile(), hasType(cxxRecordDecl(matchesName("cuda.*")))).bind("cudaStructVar"), &Callback);
  Finder.addMatcher(varDecl(isExpansionInMainFile(), hasType(pointsTo(cxxRecordDecl(matchesName("cuda.*"))))).bind("cudaStructVarPtr"), &Callback);
  Finder.addMatcher(parmVarDecl(isExpansionInMainFile(), hasType(namedDecl(matchesName("cuda.*")))).bind("cudaParamDecl"), &Callback);
  Finder.addMatcher(parmVarDecl(isExpansionInMainFile(), hasType(pointsTo(namedDecl(matchesName("cuda.*"))))).bind("cudaParamDeclPtr"), &Callback);
  Finder.addMatcher(expr(isExpansionInMainFile(), sizeOfExpr(hasArgumentOfType(recordType(hasDeclaration(cxxRecordDecl(matchesName("cuda.*"))))))).bind("cudaStructSizeOf"), &Callback);
  Finder.addMatcher(stringLiteral(isExpansionInMainFile()).bind("stringLiteral"), &Callback);

  auto action = newFrontendActionFactory(&Finder, &PPCallbacks);

  std::vector<const char *> compilationStages;
  compilationStages.push_back("--cuda-host-only");
  compilationStages.push_back("--cuda-device-only");

  for (auto Stage : compilationStages)
  {
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(Stage, ArgumentInsertPosition::BEGIN));
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-std=c++11"));
#if defined(HIPIFY_CLANG_RES)
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-resource-dir=" HIPIFY_CLANG_RES));
#endif // defined(HIPIFY_CLANG_HEADERS)
    Tool.appendArgumentsAdjuster(getClangSyntaxOnlyAdjuster());
    Result = Tool.run(action.get());

    Tool.clearArgumentsAdjusters();
  }

  LangOptions DefaultLangOptions;
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
  TextDiagnosticPrinter DiagnosticPrinter(llvm::errs(), &*DiagOpts);
  DiagnosticsEngine Diagnostics(
    IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs()),
    &*DiagOpts, &DiagnosticPrinter, false);
  SourceManager Sources(Diagnostics, Tool.getFiles());

  DEBUG(dbgs() << "Replacements collected by the tool:\n");
  for (const auto &r : Tool.getReplacements()) {
    DEBUG(dbgs() << r.toString() << "\n");
  }

  Rewriter Rewrite(Sources, DefaultLangOptions);

  if (!Tool.applyAllReplacements(Rewrite)) {
    DEBUG(dbgs() << "Skipped some replacements.\n");
  }

  Result = Rewrite.overwriteChangedFiles();

  if (!Inplace) {
    size_t pos = dst.rfind(".cu");
    if (pos != std::string::npos)
    {
      rename(dst.c_str(), dst.substr(0, pos).c_str());
    }
  }
  return Result;
}
