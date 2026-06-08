/**********************************************************************************************
 *
 * @file kt_shell.h
 *   (c) 2025-2026 Jacques Morel
 * @brief Cross-platform shell process backend for KTerm
 * @version 1.0
 *
 * @section Overview:
 *   kt_shell.h provides a platform-abstracted shell subprocess with redirected I/O.
 *   The host reads stdout from the shell and pipes it into kterm; keyboard input goes
 *   to the shell's stdin.
 *
 * @section Platforms:
 *   - Windows: CreateProcess + anonymous pipes + reader thread
 *   - POSIX (Linux/macOS): forkpty + exec + reader thread
 *   - Emscripten: Stub (returns false — virtual shell is a separate library)
 *
 * @section Usage:
 *   #define KT_SHELL_IMPLEMENTATION in one .c file before including this header.
 *
 *   KTShell shell = {0};
 *   if (KTShell_Start(&shell, NULL)) {  // NULL = default shell
 *       // In your frame loop:
 *       char buf[4096];
 *       size_t n = KTShell_Read(&shell, buf, sizeof(buf));
 *       if (n > 0) KTerm_PushInput(term, buf, n);
 *
 *       // Forward keyboard input:
 *       KTShell_Write(&shell, key_data, key_len);
 *   }
 *   KTShell_Stop(&shell);
 *
 * @section Threading:
 *   KTShell_Read is non-blocking. Internally a reader thread buffers shell output.
 *   The caller (main thread) polls KTShell_Read each frame.
 *
 **********************************************************************************************/
#ifndef KT_SHELL_H
#define KT_SHELL_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Shell output ring buffer size (how much the reader thread can buffer before the main thread polls)
#ifndef KT_SHELL_BUFFER_SIZE
#define KT_SHELL_BUFFER_SIZE (64 * 1024)  // 64 KB
#endif

typedef struct {
    bool running;

    // Ring buffer for shell output (written by reader thread, read by main thread)
    char output_buffer[KT_SHELL_BUFFER_SIZE];
    volatile size_t output_head;  // Written by reader thread
    volatile size_t output_tail;  // Read by main thread

    // Platform-specific handles
#if defined(_WIN32)
    void* process_handle;     // HANDLE
    void* thread_handle;      // HANDLE (reader thread)
    void* stdin_write;        // HANDLE (write end of pipe to ConPTY input)
    void* stdout_read;        // HANDLE (read end of pipe from ConPTY output)
    void* pty_handle;         // HPCON (ConPTY pseudo-console handle)
#elif defined(__EMSCRIPTEN__)
    // No platform data — stub
    int _stub;
#else
    int master_fd;            // PTY master file descriptor
    int child_pid;            // Child process PID
    void* thread_handle;      // pthread_t (reader thread)
#endif
} KTShell;

// Start a shell subprocess. command=NULL uses platform default (cmd.exe / /bin/sh).
// cols/rows set the initial pseudo-terminal size.
bool KTShell_Start(KTShell* shell, const char* command, int cols, int rows);

// Write data to the shell's stdin. Returns bytes written.
size_t KTShell_Write(KTShell* shell, const void* data, size_t len);

// Non-blocking read from the shell's stdout. Returns bytes read (0 if nothing available).
size_t KTShell_Read(KTShell* shell, void* buffer, size_t max_len);

// Stop the shell subprocess and clean up resources.
void KTShell_Stop(KTShell* shell);

// Check if the shell process is still running.
bool KTShell_IsRunning(KTShell* shell);

// Resize the pseudo-terminal. Call when the terminal grid dimensions change.
void KTShell_Resize(KTShell* shell, int cols, int rows);

#ifdef __cplusplus
}
#endif

// =============================================================================
// IMPLEMENTATION
// =============================================================================
#ifdef KT_SHELL_IMPLEMENTATION

#if defined(_WIN32)
// =============================================================================
// WINDOWS IMPLEMENTATION (ConPTY — full interactive pseudo-terminal)
// =============================================================================
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00  // Windows 10 — required for ConPTY
#endif
#include <windows.h>
#include <process.h>

// ConPTY may not be declared in older MinGW headers — provide fallbacks
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
typedef void* HPCON;
HRESULT WINAPI CreatePseudoConsole(COORD size, HANDLE hInput, HANDLE hOutput, DWORD dwFlags, HPCON* phPC);
void WINAPI ClosePseudoConsole(HPCON hPC);
HRESULT WINAPI ResizePseudoConsole(HPCON hPC, COORD size);
#endif

static unsigned __stdcall KTShell_ReaderThread(void* arg) {
    KTShell* shell = (KTShell*)arg;
    char tmp[4096];
    DWORD bytes_read;

    while (shell->running) {
        BOOL ok = ReadFile(shell->stdout_read, tmp, sizeof(tmp), &bytes_read, NULL);
        if (!ok || bytes_read == 0) {
            shell->running = false;
            break;
        }

        for (DWORD i = 0; i < bytes_read; i++) {
            size_t next_head = (shell->output_head + 1) % KT_SHELL_BUFFER_SIZE;
            if (next_head == shell->output_tail) {
                shell->output_tail = (shell->output_tail + 1) % KT_SHELL_BUFFER_SIZE;
            }
            shell->output_buffer[shell->output_head] = tmp[i];
            shell->output_head = next_head;
        }
    }
    return 0;
}

bool KTShell_Start(KTShell* shell, const char* command, int cols, int rows) {
    if (!shell) return false;
    memset(shell, 0, sizeof(KTShell));

    const char* cmd = command ? command : "cmd.exe";
    if (cols <= 0) cols = 80;
    if (rows <= 0) rows = 25;

    // Create pipes for ConPTY communication
    HANDLE pty_input_read = NULL, pty_input_write = NULL;
    HANDLE pty_output_read = NULL, pty_output_write = NULL;

    if (!CreatePipe(&pty_input_read, &pty_input_write, NULL, 0)) return false;
    if (!CreatePipe(&pty_output_read, &pty_output_write, NULL, 0)) {
        CloseHandle(pty_input_read);
        CloseHandle(pty_input_write);
        return false;
    }

    // Create the pseudo-console (ConPTY) with actual terminal dimensions
    COORD pty_size = { (SHORT)cols, (SHORT)rows };
    HPCON hpc = NULL;
    HRESULT hr = CreatePseudoConsole(pty_size, pty_input_read, pty_output_write, 0, &hpc);
    if (FAILED(hr)) {
        CloseHandle(pty_input_read);
        CloseHandle(pty_input_write);
        CloseHandle(pty_output_read);
        CloseHandle(pty_output_write);
        return false;
    }

    // Close the pipe ends that the ConPTY now owns
    CloseHandle(pty_input_read);
    CloseHandle(pty_output_write);

    // Prepare startup info with the pseudo-console
    STARTUPINFOEXW si = {0};
    si.StartupInfo.cb = sizeof(STARTUPINFOEXW);

    // Allocate attribute list for PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
    SIZE_T attr_size = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attr_size);
    si.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attr_size);
    if (!si.lpAttributeList) {
        ClosePseudoConsole(hpc);
        CloseHandle(pty_input_write);
        CloseHandle(pty_output_read);
        return false;
    }
    InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attr_size);
    UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hpc, sizeof(HPCON), NULL, NULL);

    // Create the child process
    PROCESS_INFORMATION pi = {0};
    wchar_t cmd_w[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, cmd, -1, cmd_w, MAX_PATH);

    BOOL created = CreateProcessW(NULL, cmd_w, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL, &si.StartupInfo, &pi);

    DeleteProcThreadAttributeList(si.lpAttributeList);
    HeapFree(GetProcessHeap(), 0, si.lpAttributeList);

    if (!created) {
        ClosePseudoConsole(hpc);
        CloseHandle(pty_input_write);
        CloseHandle(pty_output_read);
        return false;
    }

    CloseHandle(pi.hThread);

    shell->process_handle = pi.hProcess;
    shell->stdin_write = pty_input_write;
    shell->stdout_read = pty_output_read;
    shell->pty_handle = hpc;
    shell->running = true;

    // Start reader thread
    shell->thread_handle = (void*)_beginthreadex(NULL, 0, KTShell_ReaderThread, shell, 0, NULL);
    if (!shell->thread_handle) {
        KTShell_Stop(shell);
        return false;
    }

    return true;
}

size_t KTShell_Write(KTShell* shell, const void* data, size_t len) {
    if (!shell || !shell->running || !data || len == 0) return 0;
    DWORD written;
    if (WriteFile(shell->stdin_write, data, (DWORD)len, &written, NULL)) {
        return (size_t)written;
    }
    return 0;
}

size_t KTShell_Read(KTShell* shell, void* buffer, size_t max_len) {
    if (!shell || !buffer || max_len == 0) return 0;
    char* out = (char*)buffer;
    size_t count = 0;

    while (count < max_len && shell->output_tail != shell->output_head) {
        out[count++] = shell->output_buffer[shell->output_tail];
        shell->output_tail = (shell->output_tail + 1) % KT_SHELL_BUFFER_SIZE;
    }
    return count;
}

void KTShell_Stop(KTShell* shell) {
    if (!shell) return;
    shell->running = false;

    // Close stdin first (signals EOF to child)
    if (shell->stdin_write) {
        CloseHandle(shell->stdin_write);
        shell->stdin_write = NULL;
    }
    // Close the PTY — this tears down the console and breaks pipes
    if (shell->pty_handle) {
        ClosePseudoConsole((HPCON)shell->pty_handle);
        shell->pty_handle = NULL;
    }
    // Close stdout read pipe — unblocks reader thread's ReadFile
    if (shell->stdout_read) {
        CloseHandle(shell->stdout_read);
        shell->stdout_read = NULL;
    }
    // Give reader thread a short moment to notice, then move on
    if (shell->thread_handle) {
        DWORD result = WaitForSingleObject(shell->thread_handle, 200);
        if (result == WAIT_TIMEOUT) {
            // Thread still stuck — just close the handle and abandon it
            // (it will exit on its own when ReadFile fails)
        }
        CloseHandle(shell->thread_handle);
        shell->thread_handle = NULL;
    }
    if (shell->process_handle) {
        // Don't TerminateProcess if it already exited
        DWORD exit_code;
        if (GetExitCodeProcess(shell->process_handle, &exit_code) && exit_code == STILL_ACTIVE) {
            TerminateProcess(shell->process_handle, 0);
        }
        CloseHandle(shell->process_handle);
        shell->process_handle = NULL;
    }
}

bool KTShell_IsRunning(KTShell* shell) {
    if (!shell) return false;
    if (!shell->running) return false;
    if (shell->process_handle) {
        DWORD exit_code;
        if (GetExitCodeProcess(shell->process_handle, &exit_code)) {
            if (exit_code != STILL_ACTIVE) {
                shell->running = false;
                return false;
            }
        }
    }
    return true;
}

void KTShell_Resize(KTShell* shell, int cols, int rows) {
    if (!shell || !shell->running || !shell->pty_handle) return;
    if (cols <= 0 || rows <= 0) return;
    COORD size = { (SHORT)cols, (SHORT)rows };
    ResizePseudoConsole((HPCON)shell->pty_handle, size);
}

#elif defined(__EMSCRIPTEN__)
// =============================================================================
// EMSCRIPTEN STUB (virtual shell is a separate library)
// =============================================================================
bool KTShell_Start(KTShell* shell, const char* command, int cols, int rows) {
    (void)shell; (void)command; (void)cols; (void)rows;
    return false;
}
size_t KTShell_Write(KTShell* shell, const void* data, size_t len) {
    (void)shell; (void)data; (void)len;
    return 0;
}
size_t KTShell_Read(KTShell* shell, void* buffer, size_t max_len) {
    (void)shell; (void)buffer; (void)max_len;
    return 0;
}
void KTShell_Stop(KTShell* shell) { (void)shell; }
bool KTShell_IsRunning(KTShell* shell) { (void)shell; return false; }
void KTShell_Resize(KTShell* shell, int cols, int rows) { (void)shell; (void)cols; (void)rows; }

#else
// =============================================================================
// POSIX IMPLEMENTATION (Linux / macOS)
// =============================================================================
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

// Try pty.h (Linux) or util.h (macOS)
#if defined(__APPLE__)
#include <util.h>
#else
#include <pty.h>
#endif

static void* KTShell_ReaderThread_POSIX(void* arg) {
    KTShell* shell = (KTShell*)arg;
    char tmp[4096];

    while (shell->running) {
        ssize_t n = read(shell->master_fd, tmp, sizeof(tmp));
        if (n <= 0) {
            shell->running = false;
            break;
        }

        for (ssize_t i = 0; i < n; i++) {
            size_t next_head = (shell->output_head + 1) % KT_SHELL_BUFFER_SIZE;
            if (next_head == shell->output_tail) {
                shell->output_tail = (shell->output_tail + 1) % KT_SHELL_BUFFER_SIZE;
            }
            shell->output_buffer[shell->output_head] = tmp[i];
            shell->output_head = next_head;
        }
    }
    return NULL;
}

bool KTShell_Start(KTShell* shell, const char* command, int cols, int rows) {
    if (!shell) return false;
    memset(shell, 0, sizeof(KTShell));

    const char* cmd = command ? command : "/bin/sh";
    if (cols <= 0) cols = 80;
    if (rows <= 0) rows = 25;

    struct winsize ws = {0};
    ws.ws_col = (unsigned short)cols;
    ws.ws_row = (unsigned short)rows;

    int master_fd;
    pid_t pid = forkpty(&master_fd, NULL, NULL, &ws);

    if (pid < 0) return false;

    if (pid == 0) {
        // Child process
        execlp(cmd, cmd, (char*)NULL);
        _exit(127);  // exec failed
    }

    // Parent
    shell->master_fd = master_fd;
    shell->child_pid = pid;
    shell->running = true;

    // Set master_fd to non-blocking for the reader thread (optional, read blocks anyway)
    // fcntl(master_fd, F_SETFL, fcntl(master_fd, F_GETFL) | O_NONBLOCK);

    // Start reader thread
    pthread_t* pt = (pthread_t*)malloc(sizeof(pthread_t));
    if (!pt || pthread_create(pt, NULL, KTShell_ReaderThread_POSIX, shell) != 0) {
        if (pt) free(pt);
        KTShell_Stop(shell);
        return false;
    }
    shell->thread_handle = pt;

    return true;
}

size_t KTShell_Write(KTShell* shell, const void* data, size_t len) {
    if (!shell || !shell->running || !data || len == 0) return 0;
    ssize_t n = write(shell->master_fd, data, len);
    return (n > 0) ? (size_t)n : 0;
}

size_t KTShell_Read(KTShell* shell, void* buffer, size_t max_len) {
    if (!shell || !buffer || max_len == 0) return 0;
    char* out = (char*)buffer;
    size_t count = 0;

    while (count < max_len && shell->output_tail != shell->output_head) {
        out[count++] = shell->output_buffer[shell->output_tail];
        shell->output_tail = (shell->output_tail + 1) % KT_SHELL_BUFFER_SIZE;
    }
    return count;
}

void KTShell_Stop(KTShell* shell) {
    if (!shell) return;
    shell->running = false;

    if (shell->master_fd > 0) {
        close(shell->master_fd);
        shell->master_fd = -1;
    }
    if (shell->child_pid > 0) {
        kill(shell->child_pid, SIGTERM);
        waitpid(shell->child_pid, NULL, WNOHANG);
        shell->child_pid = 0;
    }
    if (shell->thread_handle) {
        pthread_join(*(pthread_t*)shell->thread_handle, NULL);
        free(shell->thread_handle);
        shell->thread_handle = NULL;
    }
}

bool KTShell_IsRunning(KTShell* shell) {
    if (!shell || !shell->running) return false;
    if (shell->child_pid > 0) {
        int status;
        pid_t result = waitpid(shell->child_pid, &status, WNOHANG);
        if (result == shell->child_pid) {
            shell->running = false;
            return false;
        }
    }
    return true;
}

void KTShell_Resize(KTShell* shell, int cols, int rows) {
    if (!shell || !shell->running || shell->master_fd <= 0) return;
    if (cols <= 0 || rows <= 0) return;
    struct winsize ws = {0};
    ws.ws_col = (unsigned short)cols;
    ws.ws_row = (unsigned short)rows;
    ioctl(shell->master_fd, TIOCSWINSZ, &ws);
}

#endif // Platform selection

#endif // KT_SHELL_IMPLEMENTATION
#endif // KT_SHELL_H
