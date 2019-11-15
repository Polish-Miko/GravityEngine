# Gravity Engine

## Introduction


This is my personal game engine project that
uses DirectX 12 exclusively for rendering.


## Showcase

PBR

![PBR_1](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/PBR_1.png)

![PBR_2](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/PBR_2.png)

![PBR_3](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/PBR_3.png)

Manipulation Gizmo

![Manipulation](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/Manipulation.gif)

Clustered Deferred (1000 lights)

![Clustered](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/Clustered.jpg)

Temporal Anti-Aliasing (before and after)

![TAA_Before](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/TAA_Before.jpg)

![TAA_After](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/TAA_After.jpg)

Cascaded Shadow Map

![SDF](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/CascadedShadowMap.png)

Percentage Closer Soft Shadow

![SDF](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/PCSS.png)

Capsule Soft Shadow

![SDF](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/CapsuleShadow.png)

Signed Distance Field Baking

![SDF](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/SDF_Debug_2.png)

SDF Ray-Traced Shadow

![SDF_Shadow](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/SDF_Shadow_2.gif)

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
- [x] TBDR/CBDR
- [x] PBR
- [x] Scene Editing
  - [x] Scene Serialization
  - [x] Manipulation Gizmo
- [x] Temporal AA
- [x] Masked Occlusion Culling
- [x] CSM/PCSS
- [x] Signed Distance Field Ray-Traced Shadow
- [ ] AO
- [ ] SSR
