# Gravity Engine

## Introduction

![Icon](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/GravityEngine.png)

This is my personal game engine project that
uses DirectX 12 exclusively for rendering.


## Features

PBR

![PBR_1](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/PBR_1.png)

![PBR_2](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/PBR_2.png)

![PBR_3](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/PBR_3.png)

Manipulation Gizmo

![Manipulation](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/Manipulation.gif)

Clustered Deferred (1000 lights)

![Clustered](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/Clustered.jpg)

Temporal Anti-Aliasing

![TAA](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/TAA.png)

Cascaded Shadow Map

![CSM](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/CascadedShadowMap.png)

Percentage Closer Soft Shadow

![PCSS](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/PCSS.png)

Capsule Soft Shadow

![CapsuleShadow](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/CapsuleShadow.png)

Signed Distance Field Baking

![SDF_Baking](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/SDF_Debug_2.png)

SDF Ray-Traced Shadow

![SDF_Shadow_1](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/SDF_Shadow_1.png)

![SDF_Shadow_2](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/SDF_Shadow_2.jpg)

Ground Truth Ambient Occlusion & Reflection Occlusion

![AO_RO](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/AO_RO.png)

Stochastic Screen Space Reflection

with
- Importance GGX
- Prefilter
- HiZ Tracing
- BRDF-weighted resolve

![SSR](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/SSR.png)

Depth of Field

![SSR](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/DoF.png)

Motion Blur

![SSR](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/MotionBlur.png)

Bloom

![SSR](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/Bloom.png)

ACES Tone Mapping & Uncharted 2 Tone Mapping

## Dependencies

<br>

**1. DirectXTK12**

**2. fbxsdk**


copy

*FBX SDK\2019.2\lib\vs2017\x64\debug\ {libfbxsdk.dll，libfbxsdk.lib，libfbxsdk.pdb}*

to

*Debug\Build*

**3. boost**

**4. DirectXTex**

## todo
- [x] Editor
  - [x] Scene Serialization
  - [x] Manipulation Gizmo
- [ ] Renderer
  - [x] TBDR/CBDR
  - [x] PBR
  - [x] Temporal AA
  - [x] Occlusion Culling
  - [x] CSM/PCSS
  - [x] Signed Distance Field Ray-Traced Shadow
  - [x] GTAO/GTSO
  - [x] Stochastic SSR
  - [X] Depth of Field
  - [x] McGuire Motion Blur
  - [x] Bloom
  - [ ] TLOU-Styled Indirect Shadow
  - [ ] DDGI
  - [ ] Glossy GI
  - [ ] Refactoring
- [ ] Mono
- [ ] Particle System
- [ ] Animation System
