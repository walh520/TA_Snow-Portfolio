#pragma once
#include "CoreMinimal.h"
// Numerical-only extraction. No UObject, capture, rendering or task ownership.
struct FSnowCell
{
    float BaseZ = 0;
    float MassOffset = 0;
    // CapturedHeight stays fixed. InitialHeight is the weather-evolving, untouched reference.
    // Render filtering never feeds back into mass/history.
    float CapturedHeight = 0;
    float InitialHeight = 0;
    float Compaction = 0;
    float Coverage = 0;
    float Height(float Baseline) const { return FMath::Max(0.f, Baseline + MassOffset) / (1.f + 2.f * Compaction); }
};

struct FSnowContact
{
    FVector Position = FVector::ZeroVector;
    FVector PreviousPosition = FVector::ZeroVector;
    float Radius = 40;
    float BottomZ = 0;
    float Strength = 3;
    float Push = 0.75f;
};


struct FSnowPatch
{
    FIntPoint Key=FIntPoint::ZeroValue;
    FVector2D Min=FVector2D::ZeroVector,Size=FVector2D(800,800);
    int32 Resolution=64;
    bool bReady=false,bDirty=false;
    float SnowDepth=0;
    TArray<FSnowCell> Cells;
    int32 Side() const { return Resolution+1; }
    float AreaWeight(int32 I) const
    {
        const int32 X=I%Side(),Y=I/Side();
        return ((X==0 || X==Resolution)? .5f:1.f)*((Y==0 || Y==Resolution)? .5f:1.f);
    }
    FVector2D CellPosition(int32 I) const
    { return Min+FVector2D(I%Side(),I/Side())*Size/double(Resolution); }
};
namespace TASnow
{
    bool ApplyContact(FSnowPatch& Patch,const FSnowContact& Contact,float Baseline,float Dt);
    bool ApplyContactBatch(TConstArrayView<FSnowPatch*> Patches,const FSnowContact& Contact,float Dt);
}
struct FSnowInitialShape
{
    FVector2D Origin = FVector2D::ZeroVector;
    float Depth = 12, Maximum = 35, Amplitude = 0, Scale = 400;
    int32 Seed = 1337;
};
namespace TASnow
{
    float InitialDepth(const FSnowInitialShape& Shape,const FVector2D& XY);
    float InterpolatedInitialDepth(const FSnowInitialShape& Shape,const FSnowPatch& Background,const FVector2D& XY);
}
