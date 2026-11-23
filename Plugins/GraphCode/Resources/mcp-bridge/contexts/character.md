# Unreal Engine 5.7 Character System Context

This context is automatically loaded when working with Character MCP tools.

## Core Classes

### ACharacter

Base class for characters with movement, collision, and skeletal mesh support.

```cpp
// ACharacter inherits from APawn
// Key components:
// - UCapsuleComponent (collision)
// - USkeletalMeshComponent (mesh + animation)
// - UCharacterMovementComponent (movement logic)

ACharacter* Character = GetWorld()->SpawnActor<ACharacter>(CharacterClass, SpawnTransform);

// Access components
UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
USkeletalMeshComponent* Mesh = Character->GetMesh();
UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
```

### UCharacterMovementComponent

Handles all character movement physics and state.

```cpp
// Walking/Running properties
Movement->MaxWalkSpeed = 600.0f;        // cm/s
Movement->MaxWalkSpeedCrouched = 300.0f;
Movement->MaxAcceleration = 2048.0f;    // cm/s^2
Movement->GroundFriction = 8.0f;

// Jumping properties
Movement->JumpZVelocity = 420.0f;       // Initial jump velocity
Movement->AirControl = 0.35f;           // 0-1, how much control in air
Movement->GravityScale = 1.0f;          // Multiplier for gravity

// Step/Slope handling
Movement->MaxStepHeight = 45.0f;        // Max height character can step up
Movement->SetWalkableFloorAngle(45.0f); // Max slope angle for walking

// Braking
Movement->BrakingDecelerationWalking = 2048.0f;
Movement->BrakingDecelerationFalling = 0.0f;
Movement->BrakingFriction = 0.0f;
Movement->bUseSeparateBrakingFriction = false;

// Swimming/Flying
Movement->MaxSwimSpeed = 300.0f;
Movement->MaxFlySpeed = 600.0f;
```

### Movement Modes

```cpp
// EMovementMode
MOVE_None,      // No movement
MOVE_Walking,   // On ground
MOVE_Falling,   // In air (jumping/falling)
MOVE_Swimming,  // In water volume
MOVE_Flying,    // Flying mode
MOVE_Custom     // Custom movement mode

// Check current mode
if (Movement->IsMovingOnGround()) { /* walking */ }
if (Movement->IsFalling()) { /* in air */ }
if (Movement->IsSwimming()) { /* swimming */ }

// Set movement mode
Movement->SetMovementMode(MOVE_Flying);
```

## Authoring characters — the tool surface moved, 2026-08-08 (C22 T8)

Fifteen of the seventeen `character` operations are retired. Calling any of them by name still
answers, with the exact replacement — this section is the same information without the round trip.

### A placed character is the level document

| You want to | Call |
|---|---|
| find the characters in the level | `unreal_gc_read kind:"level"` — read `actors { }`, a Character is an entry whose class derives from `/Script/Engine.Character` |
| read a character's mesh, capsule, anim class, components | the same call — the braces after an actor ARE its component list, each with the properties that deviate from the class default |
| change movement, capsule or mesh values | `unreal_gc_apply kind:"level" block:"actors"`, writing the field on the component inside that actor's braces |

Three things that differ from the retired operations, all in the caller's favour except the last:

- **Every editable property is writable**, not the ten `set_movement_params` named.
- **The silent clamps are gone.** That tool clamped `air_control` to 0–1 and `gravity_scale` to
  −10–10 without saying so; the document writes what you send.
- **An absent property means equal to the archetype**, which the document's `baseline:` line states.
  It does not mean the property is missing.

`class_filter` and `component_class` have no successor and that is deliberate: both matched a
case-insensitive **substring** of the class name, so `class_filter:"a"` selected most of a level.
Read the document and filter the text.

### The runtime booleans were never runtime

`get_character_info` returned `can_jump`, `is_crouched`, `velocity` and `is_falling`. Every MCP tool
here reads `GEditor->GetEditorWorldContext().World()` — the **editor** world, which does not
simulate — so those described a character standing still in a world with no game running.

To observe a character that is actually moving, capture during Play with `unreal_gcp_capture`. Of
the four, only `Velocity` is an editable property and therefore the only one a projection carries:
`CanJump()` and `IsFalling()` are functions, and `bIsCrouched` and `MovementMode` are
`BlueprintReadOnly`, which every projection's `omits: non_visible` already declares.

### Config assets and stats tables are ordinary assets

| You want to | Call |
|---|---|
| create a config | `unreal_gc_apply create:true`, a `data_asset` document with `parent: /Script/UnrealClaude.CharacterConfigDataAsset` |
| read or find one | `unreal_gc_read` / `unreal_gc_search kind:"asset"` with the class **path** |
| edit one | `unreal_gc_apply block:"values"` |
| create a stats table | `unreal_gc_apply create:true`, a `data_table` whose `settings { }` names `RowStruct = obj'/Script/UnrealClaude.CharacterStatsRow'` |
| add, edit or delete a row | `unreal_gc_apply block:"rows"` |
| apply a config to a placed character | two calls: `unreal_gc_read` the config, then `unreal_gc_apply kind:"level" block:"actors"` |

**`values { }` is the complete deviation set and an omitted field RESETS to the class default**,
where `update_character_data` left unnamed fields alone. Read the asset and send the whole block
back. `rows { }` deletes by an explicit `delete <RowName>` line under the default
`omission:"patch"`, or by omission under `omission:"snapshot"` — each mode refuses the other's
spelling.

`apply_character_data` also resized the collision capsule on **every** call, with no flag to stop
it, while `apply_movement`/`apply_mesh`/`apply_anim` gated the other three. A document that
reproduces it faithfully is writing something the old call did behind the caller's back.

## MCP Tool: character — what is left

Two operations, both taking `blueprint_path`:

| Operation | Description | Required Params |
|-----------|-------------|-----------------|
| `get_character_config` | Read a Character Blueprint CDO's mesh, capsule, anim class and movement defaults | **blueprint_path** |
| `assign_anim_bp` | Assign an AnimBlueprint to the Character Blueprint's mesh | **blueprint_path** + **anim_blueprint_path** |

They stay because they are the only route to a Character Blueprint's **native** components. A
Character's `Mesh`, `CapsuleComponent` and `CharacterMovement` are built in `ACharacter`'s C++
constructor, and no `.gc` block reaches one: `class_defaults { }` drops every component reference by
type, and `components { }` resolves a name only against this Blueprint's SimpleConstructionScript or
a parent **Blueprint**'s inherited SCS node. A `components { }` entry naming a native component does
not override it — it falls through to create-by-presence and **adds a second component** of that
name.

```json
{"operation": "get_character_config", "blueprint_path": "/Game/Blueprints/BP_TestCharacter"}

{
  "operation": "assign_anim_bp",
  "blueprint_path": "/Game/Blueprints/BP_TestCharacter",
  "anim_blueprint_path": "/Game/Characters/ABP_Character"
}
```

## Character config and stats data structures

These are the field names a `values { }` or `rows { }` block writes.

#### UCharacterConfigDataAsset

Blueprint-friendly configuration asset for character settings.

```cpp
UCLASS(BlueprintType)
class UCharacterConfigDataAsset : public UDataAsset
{
    // Identity
    FName ConfigId;
    FString DisplayName;
    FString Description;

    // Visuals (soft references for async loading)
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;
    TSoftClassPtr<UAnimInstance> AnimBlueprintClass;

    // Movement
    float BaseWalkSpeed = 600.0f;
    float BaseRunSpeed = 1000.0f;
    float BaseJumpVelocity = 420.0f;
    float BaseAcceleration = 2048.0f;
    float BaseGroundFriction = 8.0f;
    float BaseAirControl = 0.35f;
    float BaseGravityScale = 1.0f;

    // Combat
    float BaseHealth = 100.0f;
    float BaseStamina = 100.0f;
    float BaseDamage = 10.0f;
    float BaseDefense = 0.0f;

    // Collision
    float CapsuleRadius = 42.0f;
    float CapsuleHalfHeight = 96.0f;

    // Progression
    TSoftObjectPtr<UDataTable> StatsTable;
    FName DefaultStatsRowName;

    // Tags
    TArray<FName> GameplayTags;
    bool bIsPlayerCharacter = false;
};
```

#### FCharacterStatsRow

DataTable row structure for level-based or variant stats.

```cpp
USTRUCT(BlueprintType)
struct FCharacterStatsRow : public FTableRowBase
{
    // Identity
    FName StatsId;
    FString DisplayName;

    // Vitals
    float BaseHealth = 100.0f;
    float MaxHealth = 100.0f;
    float BaseStamina = 100.0f;
    float MaxStamina = 100.0f;

    // Movement
    float WalkSpeed = 600.0f;
    float RunSpeed = 1000.0f;
    float JumpVelocity = 420.0f;

    // Combat/Progression
    float DamageMultiplier = 1.0f;
    float DefenseMultiplier = 1.0f;
    float XPMultiplier = 1.0f;
    int32 Level = 1;

    // Tags
    TArray<FName> Tags;
};
```

## Common Patterns

### Character Setup in C++

```cpp
void AMyCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Load config from DataAsset
    if (UCharacterConfigDataAsset* Config = ConfigAsset.LoadSynchronous())
    {
        // Apply movement settings
        if (UCharacterMovementComponent* CMC = GetCharacterMovement())
        {
            CMC->MaxWalkSpeed = Config->BaseWalkSpeed;
            CMC->JumpZVelocity = Config->BaseJumpVelocity;
            CMC->AirControl = Config->BaseAirControl;
        }

        // Apply capsule
        if (UCapsuleComponent* Capsule = GetCapsuleComponent())
        {
            Capsule->SetCapsuleRadius(Config->CapsuleRadius);
            Capsule->SetCapsuleHalfHeight(Config->CapsuleHalfHeight);
        }
    }
}
```

### Blueprint Integration

```cpp
// In Blueprint, use the GetStatsRow function:
UFUNCTION(BlueprintCallable)
FCharacterStatsRow UCharacterConfigDataAsset::GetStatsRow(FName RowName) const
{
    if (UDataTable* Table = StatsTable.LoadSynchronous())
    {
        if (FCharacterStatsRow* Row = Table->FindRow<FCharacterStatsRow>(RowName, TEXT("")))
        {
            return *Row;
        }
    }
    return FCharacterStatsRow();
}
```

### Iterating Characters

```cpp
// Find all characters in world
for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
{
    ACharacter* Character = *It;
    if (IsValid(Character))
    {
        // Process character
        FString Name = Character->GetName();
        FVector Location = Character->GetActorLocation();
    }
}
```

## Default Asset Paths

- Character configs: `/Game/Characters/`
- Stats tables: `/Game/Characters/`

These are a convention, not a default any tool applies now: `package_path` went with
`create_character_data`, and `unreal_gc_apply create:true` builds the asset at the `asset_path` you
name.
