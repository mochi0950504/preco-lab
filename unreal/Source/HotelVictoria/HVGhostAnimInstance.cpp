// 女鬼 AnimInstance 實作：自擁有者 AHVGhostCharacter 拉取姿態與速度。
#include "HVGhostAnimInstance.h"

#include "HVGhostCharacter.h"

void UHVGhostAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (const AHVGhostCharacter* Ghost = Cast<AHVGhostCharacter>(TryGetPawnOwner()))
	{
		GhostMode = Ghost->GetGhostMode();
		MoveSpeed = Ghost->GetCurrentMoveSpeed();
		bIsMoving = MoveSpeed > 1.f;
	}
}
