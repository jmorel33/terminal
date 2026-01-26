# KTerm Thread Safety

**Status: NOT Thread-Safe**

The `kterm` library is currently designed as a single-threaded library. It relies on shared internal state within the `KTerm` instance and interacts with underlying graphics APIs (via the `Situation` library) that typically have strict thread-affinity requirements.

## Core Constraints

### 1. Shared Mutable State
The `KTerm` structure maintains significant state that is accessed and modified by various API functions without internal locking. Key examples include:
*   **Active Session**: `term->active_session` is a global index used to determine which session receives input and commands.
    *   *Update v2.2.17*: `KTerm_Update` no longer modifies this global index to iterate through sessions, removing the critical race condition where input could be routed to the wrong session during a background update. However, `term->active_session` is still shared state.
*   **Input Pipeline**: `term->sessions[i].input_pipeline` is a ring buffer. `KTerm_WriteChar` (often called from a host/PTY source) writes to the head, while `KTerm_Update` (called from the main loop) reads from the tail. Simultaneous access without synchronization causes race conditions and data corruption.
*   **Screen Buffers**: Modification of the screen buffer (parsing escape sequences) happens during `KTerm_Update`. Concurrent reads or writes to the buffer are unsafe.

### 2. Graphics API Affinity
`KTerm_Draw` issues commands to the GPU via the `Situation` library.
*   **Main Thread Requirement**: Most graphics backends (OpenGL, Vulkan command recording) require that rendering commands be issued from a specific thread (usually the main thread) or strictly synchronized.
*   `KTerm` does not manage these contexts internally. Calling `KTerm_Draw` from a background thread while the main thread handles window events will likely result in crashes or undefined behavior.

## Safe Usage Guidelines

### Option A: Single-Threaded (Recommended)
The simplest and most robust approach is to perform all `KTerm` operations on the main application thread.
1.  Read from your PTY/Network source in a non-blocking manner or separate thread.
2.  Buffer this data in your application.
3.  In your main loop (before `KTerm_Update`), push the buffered data into KTerm using `KTerm_WriteChar` or `KTerm_WriteString`.
4.  Call `KTerm_Update` and `KTerm_Draw`.

### Option B: External Synchronization
If you must call `KTerm` functions from multiple threads (e.g., a dedicated PTY reader thread pushing data directly), you **must** use a mutex to lock the `KTerm*` instance.

```c
// Pseudo-code example
pthread_mutex_lock(&term_mutex);
// Ideally use KTerm_WriteCharToSession to avoid active_session ambiguity
KTerm_WriteCharToSession(term, session_id, ch);
pthread_mutex_unlock(&term_mutex);

// In Main Loop
pthread_mutex_lock(&term_mutex);
KTerm_Update(term);
KTerm_Draw(term);
pthread_mutex_unlock(&term_mutex);
```

**Critical**: You must ensure that `KTerm_Draw` is still called on the thread that owns the graphics context (usually the main thread).

---

## Roadmap to Thread Safety

This section outlines the planned phases to evolve `kterm` into a thread-safe library.

### Phase 1: State Isolation (Architecture Hardening) - **COMPLETED (v2.2.17)**
**Objective**: Remove dangerous global state mutations that make concurrency impossible even with locking.

*   **Actionable 1.1**: Refactor `KTerm_Update` to remove its dependency on modifying `term->active_session`.
    *   *Issue*: Currently, the loop `for(i=0..MAX) { term->active_session = i; ... }` breaks any concurrent logic relying on the "User's Active Session".
    *   *Fix*: Pass `session_index` explicitly to internal update functions (`KTerm_ProcessEvents`, `KTerm_ProcessChar`). Use a local variable for iteration. **(Done)**
*   **Actionable 1.2**: Promote `KTerm_WriteCharToSession` as the primary API.
    *   *Fix*: Make `KTerm_WriteChar` a simple wrapper that reads `term->active_session` atomically (once) and calls `KTerm_WriteCharToSession`. **(Done)**

### Phase 2: Input Pipeline Safety (Lock-Free) - **COMPLETED (v2.2.21)**
**Objective**: Allow high-performance input injection from background threads without locking the entire terminal.

*   **Actionable 2.1**: Convert `input_pipeline` indices (`head`, `tail`) to `atomic_int`. **(Done)**
*   **Actionable 2.2**: Implement a Single-Producer/Single-Consumer (SPSC) or Multi-Producer/Single-Consumer (MPSC) lock-free ring buffer for `KTerm_WriteCharToSession`. **(Done)**
    *   *Benefit*: PTY threads can push data without waiting for `KTerm_Update` to finish rendering.

### Phase 3: Coarse-Grained Locking (The Big Lock)
**Objective**: Protect complex shared state (Screen Buffers, Resize logic) with minimal complexity.

*   **Actionable 3.1**: Add `kterm_mutex_t lock` to the `KTerm` struct.
*   **Actionable 3.2**: Add `kterm_mutex_t lock` to `KTermSession` struct.
*   **Actionable 3.3**: Lock the Session Mutex during `KTerm_Update` (logic update) and `KTerm_ResizeSession`.
    *   *Constraint*: This lock must be held while reading/writing the `screen_buffer`.

### Phase 4: Render Decoupling
**Objective**: Allow `KTerm_Update` (Logic) and `KTerm_Draw` (Rendering) to run in parallel.

*   **Actionable 4.1**: Implement "Double Buffering" for the Render State.
    *   *Concept*: `KTerm_Update` writes to a "Back Buffer" (logical grid).
    *   *Action*: `KTerm_Draw` snapshots the "Front Buffer".
    *   *Sync*: A brief lock is required only to swap buffers at the end of a frame.
*   **Actionable 4.2**: Decouple `KTerm_UpdateSSBO` from `Draw`.
    *   *Fix*: The CPU-to-GPU upload logic should happen on a stable snapshot of the data.

### Feasibility Notes
*   **C11 Atomics**: The library should adopt `<stdatomic.h>` for Phase 2.
*   **Platform Support**: Mutex primitives need to be abstracted (e.g., `pthread` for Linux, `CriticalSection` for Windows) or provided by the `Situation` platform layer.
