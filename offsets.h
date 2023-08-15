#define __OFFSET__Decrypt 0x5026128
#define __OFFSET__UWorld 0x7336738
#define __OFFSET__GNames 0x75B5E78

#define __OFFSET__GameInst 0x188
#define __OFFSET__SPECTATED_COUNT 0xEB8 // ATslCharacter -> SpectatedCount
#define __OFFSET__PlayerController 0x38 // Player -> CurrentNetSpeed
#define __OFFSET__Pawn 0x488 // APlayerController -> AcknowledgedPawn
#define __OFFSET__LocalPlayer 0xE0
#define __OFFSET__Location 0x260 // ComponentToWorld + 0x10

#define __OFFSET__WeaponProcessor 0x10D0 // ATslCharacter -> WeaponProcessor
#define __OFFSET_WEAPONPROCESSER_EQUIPPEDWEAPONS 0x2B8 // WeaponProcessorComponent -> EquippedWeapons
#define __OFFSET_WEAPONPROCESSER_WEAPONINDEX 0x21
#define __OFFSET_WEAPON_TRAJECTORYDATA 0xF40 // ATslWeapon_Trajectory -> WeaponTrajectoryData
#define __OFFSET_TRAJECTORYDATA_CONFIG 0xB8 // UWeaponTrajectoryData -> TrajectoryConfig

#define __OFFSET_TRAJECTORYCONFIG_VELOCITY 0x0
#define __OFFSET_TRAJECTORYCONFIG_VDRAG_COEFICIENT 0x2C // WeaponRecoilConfig -> VDragCoefficient

#define __OFFSET__AnimInst 0xC68 // USkeletalMeshComponent -> AnimScriptInstance
#define __OFFSET__ControlRotation_CP 0x5F8 // TslAnimInstance -> ControlRotation_CP
#define __OFFSET__RecoilAdsRotation_CP 0xAA0 // TslAnimInstance -> RecoilAdsRotation_CP
#define __OFFSET__LastRenderTime 0x768 // UProjectileMovementComponent -> LastRenderTimeOnScreen

#define __OFFSET__CameraManager 0x4B0 // PlayerController -> PlayerCameraManager
#define __OFFSET__CameraCache 0xFF0 // PlayerController -> PlayerCameraManager
#define __OFFSET__CameraCache_POV __OFFSET__CameraCache + 0x10
#define __OFFSET__CameraCache_Loc __OFFSET__CameraCache_POV + 0x594 // CameraCache + POV + MinimalViewInfo + LOC
#define __OFFSET__CameraCache_Rot __OFFSET__CameraCache_POV + 0x4 // CameraCache + POV + MinimalViewInfo + ROT
#define __OFFSET__CameraCache_FOV __OFFSET__CameraCache_POV + 0x58C // CameraCache + POV + MinimalViewInfo + FOV

#define __OFFSET__Level 0x1E8 // UWorld -> CurrentLevel
#define __OFFSET__Actors 0xC0
#define __OFFSET__ActorID 0x18
#define __OFFSET__ActorTeam 0xD20 // TslCharacter -> LastTeamNum

#define __OFFSET__Health 0xC20 // TslCharacter -> Health
#define __OFFSET__GroggyHealth 0x16D0 // TslCharacter -> GroggyHealth
#define __OFFSET__RootComponent 0x1A8 // AActor -> RootComponent
#define __OFFSET__ActorVelocity 0x328 // USceneComponent -> ComponentVelocity

#define __OFFSET__ActorMesh 0x4C0 // ACharacter -> Mesh
#define __OFFSET__ComponentToWorld 0x250 // *(C-Style)* K2_GetComponentToWorld -> XREF -> sub_module -> Call sub_module -> 1. CTW 2.Loc 3.Rot
#define __OFFSET__BoneArray 0xAC0 // UCustomMeshComponent
#define __OFFSET__ChunkSize 0x41EC

#define offs_DroppedItemGroup 0x1B8
#define offs_DroppedItemGroup_Count offs_DroppedItemGroup + 0x8
#define offs_DroppedItemGroup_UItem   0x650
#define offs_UITEM_ID				  0x270
#define offs_ITEM_PACKAGE             0x528

///////////////// Bullet Table ///////////////////

#define __OFFSET__BulletTable 0xEA0
#define __OFFSET__BulletLocation 0xC8
#define __OFFSET__BulletDirection 0xE0
#define __OFFSET__BulletAccZ 0x5C
#define __OFFSET__BulletTravelDistance 0xA8
#define __OFFSET__BulletStructSize 0xF0 - 0x1