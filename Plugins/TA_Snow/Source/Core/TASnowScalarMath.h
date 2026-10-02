#pragma once
#include "CoreMinimal.h"
// Scalar/vector formula excerpts from the runtime. Caller establishes support,
// connected-surface selection, positive radii, time steps and area integrals.
namespace TASnow
{
    // TASnowTopSurface.cpp: display height, not solver mass.
    inline float ShapeHeight(float Height,float Fade,const FVector2f& Small)
    {
        Height=FMath::Max(0.f,Height);
        return Small.Y>0?Small.X*Small.Y*(1.f-FMath::Exp(-Height/Small.Y)):Height*Fade;
    }
    // TASnowWeatherActor.cpp: smooth compact-support landing kernel.
    inline float LandingWeight(double Distance,double Radius)
    {
        if(Radius<=0 || Distance>=Radius) return 0;
        const float U2=float(Distance*Distance/(Radius*Radius));
        return FMath::Exp(-4.5f*U2)*FMath::Square(1-U2);
    }
    inline float DepositedDepth(double Volume,float Weight,double WeightedArea)
    { return WeightedArea>SMALL_NUMBER?float(Volume*Weight/WeightedArea):0; }
    // Runtime weather reference after noise retention and signed accumulation.
    inline float WeatherReference(float Initial,float Captured,float Retention,double Offset,float Maximum)
    { return float(FMath::Clamp(double(Initial)+(Captured-Initial)*Retention+Offset,0.,double(FMath::Max(0.f,Maximum)))); }
    inline float WeatherNoiseRetention(float Retention,float Amount,float Leveling,float Amplitude)
    { return Retention*FMath::Exp(-FMath::Abs(Amount)*FMath::Clamp(Leveling,0.f,1.f)/FMath::Max(1.f,Amplitude)); }
    // TASnowFX.cpp: exact integration of first-order fixed-wind response.
    inline FVector WindDisplacement(const FVector& Previous,const FVector& Target,double Dt,double Response)
    { return Target*Dt+(Previous-Target)*Response*(1-FMath::Exp(-Dt/Response)); }
    inline FVector WindVelocity(const FVector& Previous,const FVector& Target,double Dt,double Response)
    { return Target+(Previous-Target)*FMath::Exp(-Dt/Response); }
}
