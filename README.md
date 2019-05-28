# Gravity Engine

## Introduction


This is my personal game engine project that
uses DirectX 12 exclusively for rendering.


## Showcase

PBR

![PBR_1](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/PBR_1.png)

![PBR_2](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/PBR_2.png)

![PBR_3](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/PBR_3.png)

Clustered Deferred (1000 lights)

![Clustered](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/Clustered.jpg)

Temporal AA

Before

![Before](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/TAA_Before.jpg)

After

![After](https://github.com/MrySwk/GravityEngine/blob/master/screenshot/TAA_After.jpg)

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
- [ ] Culling
- [ ] Shadowing
- [ ] AO
