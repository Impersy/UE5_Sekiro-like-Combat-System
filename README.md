# Sekiro-like Combat System

UE5 C++ 기반의 3인칭 액션 보스전 포트폴리오 프로젝트입니다.  
Sekiro 스타일의 공방 흐름을 참고해 플레이어 액션, 패링/가드, 보스 AI, 공격 판정, 처형, 튜토리얼을 직접 구현했습니다.

> This repository is focused on the C++ gameplay architecture of a Sekiro-like action combat prototype.

## Demo

- Gameplay Video: https://youtu.be/nCBhZN9Cqqk?si=QAx6-cIGDm8YFD9u
- Build Type: Packaged Shipping Build
- Scope: Tutorial + One Boss Fight

## Project Info

- Engine: Unreal Engine 5.6
- Language: C++
- Genre: Third-person action combat
- Development: Personal project
- Development Period: 60 days

## Core Features

### Player Action Pipeline

Player input is not executed immediately.  
It is converted into an action request, checked against the current action state, and either executed, cancelled into, or stored in the input buffer.

Key points:

- Centralized player action state machine
- Cancel rule table for attack, guard, dodge, jump, heavy attack, skill actions
- Input buffering for smooth combo and action chaining
- Blend-based montage transition for responsive but natural action flow

### Defense / Parry System

The defense system separates tap, hold, parry, guard, chain parry, and hit reaction cancel behavior.

Key points:

- Perfect parry and normal parry windows
- Chain parry handling
- Guard hold conversion after parry success
- Front-angle validation for defense
- Guard block, guard break, and posture interaction

### Boss Combat FSM / Attack Link

Boss combat is built around explicit combat states and attack planning.

Key points:

- Boss combat state transition entry point
- Attack selection by distance, phase, cooldown, and probability
- Montage notify based attack link system
- Approach / reposition fallback when attack range is not satisfied
- Parry, counter, evade, rebound, and special reaction handling

### Common Attack Trace / Hit Reaction

Attack traces produce a shared hit request, and each target interprets that request through its own reaction logic.

Key points:

- Common attack hit request structure
- Player / monster specific hit interpretation
- Parry, guard, damage, posture damage, and hit reaction branching
- Super armor support with physical hit reaction
- Direction and hit type based montage selection

### Reusable Execution System

Execution is implemented through a target interface instead of direct boss-only references.

Key points:

- Execution target interface
- Player only handles execution input and execution montage
- Boss, monster, and tutorial NPC define their own executed result
- Boss can consume life / change phase
- Tutorial NPC can recover and continue tutorial flow

## Additional Systems

- Tutorial task system
- Tutorial NPC dialogue and interaction flow
- Danger attack marker UI
- Mikiri counter
- Jump counter / stomp follow-up
- Lock-on camera
- Potion system
- Monster overhead HUD
- BGM manager
- Niagara sword trail control
- Combat VFX subsystem
- Hit stop / slow motion subsystem
- Foot placement IK support

## Architecture Highlights

The project uses several gameplay architecture patterns to reduce direct dependencies between gameplay classes.

- Interfaces: execution targets, attack targets, lock-on targets, special counter interaction
- Actor Components: player equipment, potion, defense, combat reaction
- Subsystems: combat VFX, time effect
- State Machines: player action state, locomotion state, boss combat state
- Data-driven notify settings: attack trace, sword trail, code move, facing, danger attack

## Repository Note

This repository is intended as a portfolio code repository.  
Some assets, marketplace content, or large Unreal-generated folders may be excluded from source control.

Recommended folders to exclude:

- `Binaries/`
- `Intermediate/`
- `Saved/`
- `DerivedDataCache/`
- `.vs/`

## Portfolio Focus

The main focus of this project is not simply reproducing a boss fight, but designing a reusable action combat architecture:

1. Smooth player action control through FSM, cancel rules, and input buffering
2. Defense and parry logic that supports practical combat feel
3. Boss AI structure with attack planning and montage-based links
4. Common attack trace and hit reaction pipeline
5. Interface-based execution and special counter systems
