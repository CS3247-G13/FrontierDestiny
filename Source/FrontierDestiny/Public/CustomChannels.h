#pragma once

#include "CoreMinimal.h"

// Trace channels
inline constexpr ECollisionChannel CC_Grid       = ECC_GameTraceChannel1;
inline constexpr ECollisionChannel CC_Projectile = ECC_GameTraceChannel2;
inline constexpr ECollisionChannel CC_Laser      = ECC_GameTraceChannel3;
inline constexpr ECollisionChannel CC_Core		 = ECC_GameTraceChannel6;
// Object channels
inline constexpr ECollisionChannel CC_Enemy  = ECC_GameTraceChannel4;
inline constexpr ECollisionChannel CC_Player = ECC_GameTraceChannel5;
