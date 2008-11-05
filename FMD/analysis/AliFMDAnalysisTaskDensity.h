#ifndef ALIFMDANALYSISTASKDENSITY_H
#define ALIFMDANALYSISTASKDENSITY_H
 
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */
 
#include "AliAnalysisTask.h"

#include "TObjArray.h"
#include "AliESDFMD.h"
#include "AliESDEvent.h"
#include "TObjString.h"
#include "TTree.h"
class AliESDEvent;
class TChain;
class AliAODEvent;



class AliFMDAnalysisTaskDensity : public AliAnalysisTask
{
 public:
    AliFMDAnalysisTaskDensity();
    AliFMDAnalysisTaskDensity(const char* name);
    virtual ~AliFMDAnalysisTaskDensity() {;}
 AliFMDAnalysisTaskDensity(const AliFMDAnalysisTaskDensity& o) : AliAnalysisTask(),
      fDebug(o.fDebug),
      fOutputList(),
      fArray(o.fArray),
      fESD(o.fESD),
      fVertexString(o.fVertexString) {}
    AliFMDAnalysisTaskDensity& operator=(const AliFMDAnalysisTaskDensity&) { return *this; }
    // Implementation of interface methods
    virtual void ConnectInputData(Option_t *option);
    virtual void CreateOutputObjects();
    virtual void Init() {}
    virtual void LocalInit() {Init();}
    virtual void Exec(Option_t *option);
    virtual void Terminate(Option_t */*option*/) {}
    virtual void SetDebugLevel(Int_t level) {fDebug = level;}
    
 private:
    Int_t         fDebug;        //  Debug flag
    TList         fOutputList;
    TObjArray     fArray;
    AliESDEvent*  fESD;
    TObjString    fVertexString;
    ClassDef(AliFMDAnalysisTaskDensity, 0); // Analysis task for FMD analysis
};
 
#endif
