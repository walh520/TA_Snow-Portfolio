#include "TASnowMathTypes.h"

float TASnow::InitialDepth(const FSnowInitialShape& S,const FVector2D& XY)
{
    const float Maximum=FMath::Max(0.f,S.Maximum),Depth=FMath::Clamp(S.Depth,0.f,Maximum);
    if(Depth<=0 || S.Amplitude<=0) return Depth;
    // Seeded offset, not a mutable random stream: capture order cannot change the field.
    const uint32 Seed=uint32(S.Seed)*1664525u+1013904223u;
    const FVector2D Offset((Seed&65535u)*.03125,((Seed>>16)&65535u)*.03125);
    const float Noise=FMath::Clamp(FMath::PerlinNoise2D((XY-S.Origin)/FMath::Max(25.f,S.Scale)+Offset),-1.f,1.f);
    return FMath::Clamp(Depth+FMath::Max(0.f,S.Amplitude)*Noise,0.f,Maximum);
}

float TASnow::InterpolatedInitialDepth(const FSnowInitialShape& S,const FSnowPatch& P,const FVector2D& XY)
{
    if(S.Amplitude<=0 || S.Depth<=0) return InitialDepth(S,XY);
    const FVector2D Grid=(XY-P.Min)/P.Size*P.Resolution;
    const int32 X=FMath::Clamp(FMath::FloorToInt(Grid.X),0,P.Resolution-1);
    const int32 Y=FMath::Clamp(FMath::FloorToInt(Grid.Y),0,P.Resolution-1);
    const float U=float(FMath::Clamp(Grid.X-X,0.,1.)),V=float(FMath::Clamp(Grid.Y-Y,0.,1.));
    auto At=[&](int32 DX,int32 DY){return InitialDepth(S,P.CellPosition((Y+DY)*P.Side()+X+DX));};
    // Matches BuildMesh's two triangles, including the anti-diagonal split.
    if(U+V<=1) return At(0,0)*(1-U-V)+At(1,0)*U+At(0,1)*V;
    return At(1,1)*(U+V-1)+At(1,0)*(1-V)+At(0,1)*(1-U);
}
