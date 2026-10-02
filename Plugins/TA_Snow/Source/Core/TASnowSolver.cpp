#include "TASnowMathTypes.h"

namespace
{
bool Apply(TConstArrayView<FSnowPatch*> Patches,const FSnowContact& C,float Dt,TOptional<float> OverrideDepth)
{
    if(C.Radius<=0 || Dt<=0) return false;
    const FVector2D A(C.PreviousPosition),B(C.Position),Segment=B-A;
    const FVector2D Start=Segment.SizeSquared()<FMath::Square(4.f*C.Radius)?A:B;
    const FVector2D D=B-Start,Direction=D.GetSafeNormal();
    const double Length2=D.SizeSquared();
    auto Distance=[&](FVector2D V)
    {
        const double T=Length2>.001?FMath::Clamp(FVector2D::DotProduct(V-Start,D)/Length2,0.,1.):0.;
        return FVector2D::Distance(V,Start+T*D);
    };
    struct FContext
    {
        FSnowPatch* P;
        float Depth,Outer;
        double Area;
        int32 X0,Y0,X1,Y1;
    };
    TArray<FContext,TInlineAllocator<8>> Contexts;
    for(FSnowPatch* P:Patches)
    {
        if(!P || !P->bReady) continue;
        const float Spacing=float(FMath::Max(P->Size.X,P->Size.Y))/P->Resolution;
        const float Outer=C.Radius+FMath::Max(2.f*Spacing,C.Radius*.65f);
        const FVector2D Min(FMath::Min(Start.X,B.X)-Outer,FMath::Min(Start.Y,B.Y)-Outer);
        const FVector2D Max(FMath::Max(Start.X,B.X)+Outer,FMath::Max(Start.Y,B.Y)+Outer);
        if(Max.X<P->Min.X || Max.Y<P->Min.Y || Min.X>P->Min.X+P->Size.X || Min.Y>P->Min.Y+P->Size.Y) continue;
        FContext Q;
        Q.P=P; Q.Outer=Outer; Q.Depth=OverrideDepth.IsSet()?OverrideDepth.GetValue():P->SnowDepth;
        Q.Area=P->Size.X*P->Size.Y/FMath::Square(double(P->Resolution));
        Q.X0=FMath::Clamp(FMath::FloorToInt((Min.X-P->Min.X)/P->Size.X*P->Resolution),0,P->Resolution);
        Q.Y0=FMath::Clamp(FMath::FloorToInt((Min.Y-P->Min.Y)/P->Size.Y*P->Resolution),0,P->Resolution);
        Q.X1=FMath::Clamp(FMath::CeilToInt((Max.X-P->Min.X)/P->Size.X*P->Resolution),0,P->Resolution);
        Q.Y1=FMath::Clamp(FMath::CeilToInt((Max.Y-P->Min.Y)/P->Size.Y*P->Resolution),0,P->Resolution);
        Contexts.Add(Q);
    }
    // A top snapshot can contain a table, leaves and the ground in one XY tile.
    // Select the contacted height sheet; pushing is not a falling-snow simulation.
    double Closest=DBL_MAX;
    float MinBase=FLT_MAX,MaxBase=-FLT_MAX;
    FIntPoint ContactKey=FIntPoint::ZeroValue;
    auto CellKey=[](const FContext& Q,int32 I)
    { return Q.P->Key*Q.P->Resolution+FIntPoint(I%Q.P->Side(),I/Q.P->Side()); };
    for(const FContext& Q:Contexts)
        for(int32 Y=Q.Y0;Y<=Q.Y1;++Y) for(int32 X=Q.X0;X<=Q.X1;++X)
        {
            const int32 I=Y*Q.P->Side()+X;
            const auto& Cell=Q.P->Cells[I];
            if(Cell.Coverage>.1f) { MinBase=FMath::Min(MinBase,Cell.BaseZ); MaxBase=FMath::Max(MaxBase,Cell.BaseZ); }
            // Use the SAME penetration tolerance as the pressure loop. A capsule's
            // bottom or an artist-placed contact may be below the receiver surface.
            if(Cell.Coverage<.1f || C.BottomZ<Cell.BaseZ-FMath::Max(50.f,Q.Depth)
                || Cell.BaseZ+Cell.Height(Q.Depth)<=C.BottomZ+.02f) continue;
            const double SweptDistance=Distance(Q.P->CellPosition(I));
            if(SweptDistance>=C.Radius) continue; // the receiving ring must not select the height sheet
            const double D2=FMath::Square(SweptDistance)+1.e-6*FVector2D::DistSquared(Q.P->CellPosition(I),B);
            if(D2<Closest) { Closest=D2; ContactKey=CellKey(Q,I); }
        }
    if(Closest==DBL_MAX) return false;
    // Follow LOCAL continuity, not absolute height relative to the contact center.
    // The latter excluded most of a continuous slope at fine sample spacing.
    const double MaxStep=2*FMath::Max(Contexts[0].P->Size.X,Contexts[0].P->Size.Y)/Contexts[0].P->Resolution;
    const bool FlatRange=double(MaxBase)-MinBase<=MaxStep;
    TSet<FIntPoint> Connected;
    if(!FlatRange)
    {
    TMap<FIntPoint,float> Surface;
    for(const FContext& Q:Contexts)
        for(int32 Y=Q.Y0;Y<=Q.Y1;++Y) for(int32 X=Q.X0;X<=Q.X1;++X)
        {
            const int32 I=Y*Q.P->Side()+X;
            if(Q.P->Cells[I].Coverage>.1f) Surface.Add(CellKey(Q,I),Q.P->Cells[I].BaseZ);
        }
    TArray<FIntPoint> Frontier;
    Frontier.Add(ContactKey); Connected.Add(ContactKey);
    const FIntPoint Directions[]={FIntPoint(1,0),FIntPoint(-1,0),FIntPoint(0,1),FIntPoint(0,-1)};
    for(int32 Index=0;Index<Frontier.Num();++Index)
    {
        const FIntPoint Key=Frontier[Index];
        const float Height=Surface.FindChecked(Key);
        for(const FIntPoint& NeighborOffset:Directions)
        {
            const FIntPoint Next=Key+NeighborOffset;
            if(Connected.Contains(Next)) continue;
            if(const float* Z=Surface.Find(Next))
                if(FMath::Abs(double(*Z)-Height)<=MaxStep) { Connected.Add(Next); Frontier.Add(Next); }
        }
    }
    }
    auto SameSheet=[&](const FContext& Q,int32 I) { return FlatRange || Connected.Contains(CellKey(Q,I)); };
    auto RimWeight=[&](const FContext& Q,int32 I)
    {
        const auto& Cell=Q.P->Cells[I];
        if(Cell.Coverage<.1f || !SameSheet(Q,I) || C.BottomZ<Cell.BaseZ-FMath::Max(50.f,Q.Depth)) return 0.f;
        const FVector2D V=Q.P->CellPosition(I);
        const float R=float(Distance(V));
        if(R<C.Radius || R>Q.Outer) return 0.f;
        // The trailing capsule cap lies in the track we just cleared. Do not
        // deposit a fresh transverse ridge there on every fixed simulation step.
        float SideWeight=1.f;
        if(Length2>.001)
        {
            const FVector2D Relative=V-Start;
            const float Across=float(FMath::Abs(Relative.X*Direction.Y-Relative.Y*Direction.X));
            SideWeight=FMath::SmoothStep(C.Radius,C.Radius+.25f*(Q.Outer-C.Radius),Across);
        }
        const float Radial=FMath::Sin(PI*(R-C.Radius)/(Q.Outer-C.Radius));
        const float Front=Direction.IsNearlyZero()?1.f:.25f+.75f*FMath::Max(0.f,float(FVector2D::DotProduct((V-B).GetSafeNormal(),Direction)));
        return Radial*Front*SideWeight*Cell.Coverage;
    };
    double WeightSum=0,Displaced=0;
    for(const FContext& Q:Contexts)
        for(int32 Y=Q.Y0;Y<=Q.Y1;++Y) for(int32 X=Q.X0;X<=Q.X1;++X)
        {
            const int32 I=Y*Q.P->Side()+X;
            WeightSum+=RimWeight(Q,I)*Q.P->AreaWeight(I)*Q.Area;
        }
    const float Alpha=1.f-FMath::Exp(-C.Strength*Dt);
    const float PushAlpha=1.f-FMath::Exp(-20.f*C.Push*Dt);
    bool Changed=false;
    for(const FContext& Q:Contexts)
        for(int32 Y=Q.Y0;Y<=Q.Y1;++Y) for(int32 X=Q.X0;X<=Q.X1;++X)
        {
            const int32 I=Y*Q.P->Side()+X;
            auto& Cell=Q.P->Cells[I];
            if(Cell.Coverage<.1f || !SameSheet(Q,I)) continue;
            const float R=float(Distance(Q.P->CellPosition(I)));
            if(R>=C.Radius) continue;
            const float Available=FMath::Max(0.f,Q.Depth+Cell.MassOffset),Height=Cell.Height(Q.Depth);
            const float Target=FMath::Max(0.f,C.BottomZ-Cell.BaseZ);
            if(Height<=Target+.02f || C.BottomZ<Cell.BaseZ-FMath::Max(50.f,Q.Depth)) continue;
            // A broad pressure core avoids repeated bowl-shaped stamps along a swept track.
            const float Falloff=1.f-FMath::SmoothStep(.55f,1.f,R/C.Radius);
            const float NewCompact=FMath::Min(1.f,Cell.Compaction+Alpha*Falloff*(1.f-Cell.Compaction));
            const float Required=FMath::Max(0.f,Available-Target*(1.f+2.f*NewCompact));
            const float Out=WeightSum>SMALL_NUMBER?Required*PushAlpha*Falloff:0;
            const bool CellChanged=NewCompact>Cell.Compaction+.0001f || Out>.001f;
            Cell.Compaction=NewCompact; Cell.MassOffset-=Out;
            Displaced+=Out*Q.P->AreaWeight(I)*Q.Area;
            Q.P->bDirty|=CellChanged; Changed|=CellChanged;
        }
    // One receiving ring across the ground contact; tiling must not segment the berm.
    if(Displaced>0 && WeightSum>SMALL_NUMBER)
        for(const FContext& Q:Contexts)
            for(int32 Y=Q.Y0;Y<=Q.Y1;++Y) for(int32 X=Q.X0;X<=Q.X1;++X)
            {
                const int32 I=Y*Q.P->Side()+X;
                const float Added=float(Displaced*RimWeight(Q,I)/WeightSum);
                Q.P->Cells[I].MassOffset+=Added;
                Q.P->bDirty|=Added>1.e-6f; Changed|=Added>1.e-6f;
            }
    return Changed;
}
}
bool TASnow::ApplyContact(FSnowPatch& P,const FSnowContact& C,float Baseline,float Dt)
{
    FSnowPatch* Pointer=&P;
    return Apply(MakeArrayView(&Pointer,1),C,Dt,TOptional<float>(Baseline));
}
bool TASnow::ApplyContactBatch(TConstArrayView<FSnowPatch*> Patches,const FSnowContact& C,float Dt)
{
    return Apply(Patches,C,Dt,TOptional<float>());
}
