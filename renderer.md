# CPU/GPU Command & Frame Pipeline Design

This document describes a **modern, fully decoupled CPU/GPU rendering pipeline**, designed for low-latency, asynchronous execution. The system separates **command generation** (CPUF) from **frame execution** (GPUF) while maintaining safe resource management and synchronization.

---

## Table of Contents

1. [Design Goals](#design-goals)  
2. [Components](#components)  
   - CPUF - CommandBufferFactory  
   - GPUF - FrameFactory  
   - GPUSlot  
   - FrameResource  
   - GPUJob  
3. [Workflow](#workflow)  
4. [Resource Management](#resource-management)  
5. [Job Scheduling & “Newest Frame Wins” Strategy](#job-scheduling--newest-frame-wins-strategy)  
6. [Synchronization](#synchronization)  
7. [Implementation Notes & Best Practices](#implementation-notes--best-practices)  
8. [Summary Diagram](#summary-diagram)  

---

## Design Goals

1. **Decoupled CPU/GPU Pipelines**  
   - CPU can generate command buffers without waiting for GPU execution.  
   - GPU executes commands independently, ensuring maximum parallelism.  

2. **Low Latency Rendering**  
   - GPU always executes the **most recent available commands**, discarding outdated work if necessary.  

3. **Safe Resource Updates**  
   - Each GPU slot has its own **copy of per-frame resources**.  
   - CPU can safely update resources without interfering with GPU execution.  

4. **Automatic GPU Job Scheduling**  
   - GPUF handles job creation, execution, and synchronization internally.  
   - Users interact only with CPUF; the GPU pipeline is automated.  

---

## Components

### 1. CPUF - CommandBufferFactory

**Responsibility:** Generate GPU work (CommandBuffers) asynchronously.

- Maintains a **pool of pre-recorded command buffers**.
- Pushes CommandBuffers to **GPUF queue** for execution.
- Fully decoupled from frame slots or resources.
- Can be multithreaded or single-threaded.
- Optional: limit number of pre-recorded CommandBuffers to avoid unbounded growth.

```cpp
std::vector<GE::CommandBuffer> commandPool;
