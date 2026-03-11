# Nova GE — Goals

## Vision
A Vulkan renderer that is as capable and easy to use as raylib, but built on modern
Vulkan with proper multi-threading, VR support, and enough horsepower to drive real games.

---

## Core Goals

### 1. Solid Renderer Foundation
- Multiple objects rendered at once with correct transforms
- Material system — textures, samplers, basic PBR later
- Descriptor management that doesn't crumble under real usage
- Proper per-frame / per-material / per-object descriptor separation (set 0/1/2)

### 2. Multithreading
- CPUF redesign — the current single-threaded batch submit is a bottleneck
- Secondary command buffers recorded in parallel across worker threads
- Job system or task graph for render work
- Thread-safe resource management (textures, meshes, descriptors)

### 3. Usability (raylib parity)
- Simple API — create window, load mesh, load texture, draw, done
- No boilerplate at call sites
- Hot reload for shaders during development
- Basic primitives (quad, cube, sphere) built in
- 2D support for UI / sprites (the 2.5D indie game target)

### 4. VR Support
- OpenXR integration (was explored before, deferred to desktop-first)
- Stereo rendering via multiview or separate eye passes
- Head pose tracking
- Target: usable in games, not a tech demo

---

## Non-Goals (for now)
- Production-ready descriptor management rewrite (TODO, not blocking)
- Full PBR pipeline (basic diffuse is enough to start)
- Networking (Nova Network is separate)
- Editor tooling

---

## Current State (March 2026)
- [x] Vulkan instance, device, swapchain
- [x] Dynamic rendering (no renderpasses)
- [x] VK_EXT_descriptor_buffer (no pools)
- [x] Mesh loading (Assimp)
- [x] Texture loading and sampling
- [x] Camera UBO
- [x] ImGui integration
- [x] CPUF frame recording system
- [ ] Multi-set descriptor design (set 0 = frame, set 1 = material)
- [ ] Multiple objects at once
- [ ] Multithreaded command recording
- [ ] VR / OpenXR
- [ ] 2D rendering

---

## Immediate Next Steps
1. Move files into folders
2. Fix descriptor set design — split camera (set 0) from texture (set 1)
3. Multiple objects with per-object push constants for model matrix
4. CPUF redesign for parallel command recording
5. Then VR