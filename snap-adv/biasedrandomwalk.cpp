#include "stdafx.h"
#include "Snap.h"
#include "biasedrandomwalk.h"

//Preprocess alias sampling method
void GetNodeAlias(TFltV& PTblV, TIntVFltVPr& NTTable) {
  int64 N = PTblV.Len();
  TIntV& KTbl = NTTable.Val1;
  TFltV& UTbl = NTTable.Val2;
  for (int64 i = 0; i < N; i++) {
    KTbl[i]=0;
    UTbl[i]=0;
  }
  TIntV UnderV;
  TIntV OverV;
  for (int64 i = 0; i < N; i++) {
    UTbl[i] = PTblV[i]*N;
    if (UTbl[i] < 1) {
      UnderV.Add(i);
    } else {
      OverV.Add(i);
    }
  }
  while (UnderV.Len() > 0 && OverV.Len() > 0) {
    int64 Small = UnderV.Last();
    int64 Large = OverV.Last();
    UnderV.DelLast();
    OverV.DelLast();
    KTbl[Small] = Large;
    UTbl[Large] = UTbl[Large] + UTbl[Small] - 1;
    if (UTbl[Large] < 1) {
      UnderV.Add(Large);
    } else {
      OverV.Add(Large);
    }
  }
  while(UnderV.Len() > 0){
    int64 curr = UnderV.Last();
    UnderV.DelLast();
    UTbl[curr]=1;
  }
  while(OverV.Len() > 0){
    int64 curr = OverV.Last();
    OverV.DelLast();
    UTbl[curr]=1;
  }

}

TIntVFltVPr GetNTTable(PWNet& InNet, int64 Dst, int64 Src, const double& ParamP, const double& ParamQ, TStrIntVFltVPrH& NTTabelCache) {
  TStr key = TStr::Fmt("%lld,%lld", Dst, Src);
  if(!NTTabelCache.IsKey(key)) {
    if (NTTabelCache.Len() >= 500000) {
      NTTabelCache.Clr();
    }
    TWNet::TNodeI DstNode = InNet->GetNI(Dst);
    TWNet::TNodeI SrcNode = InNet->GetNI(Src);
    THash <TInt, TBool> SrcNbrH;
    for (int64 i = 0; i < SrcNode.GetOutDeg(); i++) {
      SrcNbrH.AddKey(SrcNode.GetNbrNId(i));
    }
    double Psum = 0;
    TFltV PTable;
    for (int64 i = 0; i < DstNode.GetOutDeg(); i++) {
      int64 NbrId = DstNode.GetNbrNId(i);
      TFlt Weight;
      if (!(InNet->GetEDat(Dst, NbrId, Weight))){ continue; }
      if (NbrId==Src) {
        PTable.Add(Weight / ParamP);
        Psum += Weight / ParamP;
      } else if (SrcNbrH.IsKey(NbrId)) {
        PTable.Add(Weight);
        Psum += Weight;
      } else {
        PTable.Add(Weight / ParamQ);
        Psum += Weight / ParamQ;
      }
    }
    //Normalizing table
    for (int64 i = 0; i < DstNode.GetOutDeg(); i++) {
      PTable[i] /= Psum;
    }
    // must set default value "TIntVFltVPr(TIntV(1), TFltV(1))"
    NTTabelCache.AddDat(key, TPair<TIntV,TFltV>(TIntV(DstNode.GetOutDeg()), TFltV(DstNode.GetOutDeg())));
    GetNodeAlias(PTable, NTTabelCache.GetDat(key));
  }
  return NTTabelCache.GetDat(key);
}

void GetNTTable(PWNet& InNet, int64 Dst, int64 Src, const double& ParamP, const double& ParamQ, TIntVFltVPr& NTTable) {
    TWNet::TNodeI DstNode = InNet->GetNI(Dst);
    TWNet::TNodeI SrcNode = InNet->GetNI(Src);
    THash <TInt, TBool> SrcNbrH;
    for (int64 i = 0; i < SrcNode.GetOutDeg(); i++) {
      SrcNbrH.AddKey(SrcNode.GetNbrNId(i));
    }
    double Psum = 0;
    TFltV PTable;
    for (int64 i = 0; i < DstNode.GetOutDeg(); i++) {
      int64 NbrId = DstNode.GetNbrNId(i);
      TFlt Weight;
      if (!(InNet->GetEDat(Dst, NbrId, Weight))){ continue; }
      if (NbrId==Src) {
        PTable.Add(Weight / ParamP);
        Psum += Weight / ParamP;
      } else if (SrcNbrH.IsKey(NbrId)) {
        PTable.Add(Weight);
        Psum += Weight;
      } else {
        PTable.Add(Weight / ParamQ);
        Psum += Weight / ParamQ;
      }
    }
    //Normalizing table
    for (int64 i = 0; i < DstNode.GetOutDeg(); i++) {
      PTable[i] /= Psum;
    }
    GetNodeAlias(PTable, NTTable);
}

//Get random element using alias sampling method
int64 AliasDrawInt(TIntVFltVPr& NTTable, TRnd& Rnd) {
  int64 N = NTTable.GetVal1().Len();
  TInt X = static_cast<int64>(Rnd.GetUniDev()*N);
  double Y = Rnd.GetUniDev();
  return Y < NTTable.GetVal2()[X] ? X : NTTable.GetVal1()[X];
}

void PreprocessNode (PWNet& InNet, const double& ParamP, const double& ParamQ,
 TWNet::TNodeI NI, int64& NCnt, const bool& Verbose) {
  if (Verbose && NCnt%100 == 0) {
    printf("\rPreprocessing progress: %.2lf%% ",(double)NCnt*100/(double)(InNet->GetNodes()));fflush(stdout);
  }
  //for node t
  THash <TInt, TBool> NbrH;                                    //Neighbors of t
  for (int64 i = 0; i < NI.GetOutDeg(); i++) {
    NbrH.AddKey(NI.GetNbrNId(i));
  } 
  for (int64 i = 0; i < NI.GetOutDeg(); i++) {
    TWNet::TNodeI CurrI = InNet->GetNI(NI.GetNbrNId(i));      //for each node v
    double Psum = 0;
    TFltV PTable;                              //Probability distribution table
    for (int64 j = 0; j < CurrI.GetOutDeg(); j++) {           //for each node x
      int64 FId = CurrI.GetNbrNId(j);
      TFlt Weight;
      if (!(InNet->GetEDat(CurrI.GetId(), FId, Weight))){ continue; }
      if (FId==NI.GetId()) {
        PTable.Add(Weight / ParamP);
        Psum += Weight / ParamP;
      } else if (NbrH.IsKey(FId)) {
        PTable.Add(Weight);
        Psum += Weight;
      } else {
        PTable.Add(Weight / ParamQ);
        Psum += Weight / ParamQ;
      }
    }
    //Normalizing table
    for (int64 j = 0; j < CurrI.GetOutDeg(); j++) {
      PTable[j] /= Psum;
    }
    GetNodeAlias(PTable,CurrI.GetDat().GetDat(NI.GetId()));
  }
  NCnt++;
}

//Preprocess transition probabilities for each path t->v->x
void PreprocessTransitionProbs(PWNet& InNet, const double& ParamP, const double& ParamQ, const bool& Verbose) {
  for (TWNet::TNodeI NI = InNet->BegNI(); NI < InNet->EndNI(); NI++) {
    InNet->SetNDat(NI.GetId(),TIntIntVFltVPrH());
  }
  for (TWNet::TNodeI NI = InNet->BegNI(); NI < InNet->EndNI(); NI++) {
    for (int64 i = 0; i < NI.GetOutDeg(); i++) {                    //allocating space in advance to avoid issues with multithreading
      TWNet::TNodeI CurrI = InNet->GetNI(NI.GetNbrNId(i));
      CurrI.GetDat().AddDat(NI.GetId(),TPair<TIntV,TFltV>(TIntV(CurrI.GetOutDeg()),TFltV(CurrI.GetOutDeg())));
    }
  }
  int64 NCnt = 0;
  TIntV NIds;
  for (TWNet::TNodeI NI = InNet->BegNI(); NI < InNet->EndNI(); NI++) {
    NIds.Add(NI.GetId());
  }
#pragma omp parallel for schedule(dynamic)
  for (int64 i = 0; i < NIds.Len(); i++) {
    PreprocessNode(InNet, ParamP, ParamQ, InNet->GetNI(NIds[i]), NCnt, Verbose);
  }
  if(Verbose){ printf("\n"); }
}

int64 PredictMemoryRequirements(PWNet& InNet) {
  int64 MemNeeded = 0;
  for (TWNet::TNodeI NI = InNet->BegNI(); NI < InNet->EndNI(); NI++) {
    for (int64 i = 0; i < NI.GetOutDeg(); i++) {
      TWNet::TNodeI CurrI = InNet->GetNI(NI.GetNbrNId(i));
      MemNeeded += CurrI.GetOutDeg()*(sizeof(TInt) + sizeof(TFlt));
    }
  }
  return MemNeeded;
}

//Simulates a random walk
void SimulateWalk(PWNet& InNet, int64 StartNId, const int& WalkLen, TRnd& Rnd, TIntV& WalkV, const double& ParamP, const double& ParamQ, TStrIntVFltVPrH& NTTabelCache) {
  WalkV.Add(StartNId);
  if (WalkLen == 1) { return; }
  if (InNet->GetNI(StartNId).GetOutDeg() == 0) { return; }
  WalkV.Add(InNet->GetNI(StartNId).GetNbrNId(Rnd.GetUniDevInt(InNet->GetNI(StartNId).GetOutDeg())));
  while (WalkV.Len() < WalkLen) {
    int64 Dst = WalkV.Last();
    int64 Src = WalkV.LastLast();
    if (InNet->GetNI(Dst).GetOutDeg() == 0) { return; }
    TIntVFltVPr NTTable = TPair<TIntV,TFltV>(TIntV(InNet->GetNI(Dst).GetOutDeg()), TFltV(InNet->GetNI(Src).GetOutDeg()));
    GetNTTable(InNet, Dst, Src, ParamP, ParamQ, NTTable);
    int64 Next = AliasDrawInt(NTTable, Rnd);
    WalkV.Add(InNet->GetNI(Dst).GetNbrNId(Next));
  }
}
