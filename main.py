"""
FR2 DLL Injector - Pure Python 32-bit
Simple injector untuk Fieldrunners 2
"""

import ctypes
import ctypes.wintypes
import os
import subprocess
import sys
import struct
import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from pathlib import Path

# ============================================================
# CEK ARSITEKTUR
# ============================================================
def is_32bit_python():
    """Check if running on 32-bit Python"""
    return struct.calcsize("P") == 4

def check_architecture():
    """Verify we're running on correct architecture"""
    if not is_32bit_python():
        messagebox.showerror(
            "Wrong Architecture!",
            "⚠️  ERROR: 64-bit Python Terdeteksi!\n\n"
            "Anda HARUS menggunakan Python 32-bit untuk inject ke game 32-bit.\n\n"
            "SOLUSI:\n"
            "1. Download Python 32-bit dari python.org\n"
            "2. Install di folder terpisah (misal: C:\\Python312-32)\n"
            "3. Jalankan script ini dengan Python 32-bit\n\n"
            "Aplikasi akan ditutup."
        )
        sys.exit(1)

# ============================================================
# WINDOWS API SETUP
# ============================================================

# Constants
PROCESS_ALL_ACCESS = 0x1F0FFF
PROCESS_CREATE_THREAD = 0x0002
PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_OPERATION = 0x0008
PROCESS_VM_WRITE = 0x0020
PROCESS_VM_READ = 0x0010

MEM_COMMIT = 0x00001000
MEM_RESERVE = 0x00002000
PAGE_READWRITE = 0x04
MEM_RELEASE = 0x8000

TH32CS_SNAPPROCESS = 0x00000002
INFINITE = 0xFFFFFFFF
WAIT_OBJECT_0 = 0x00000000

# Structures
class PROCESSENTRY32(ctypes.Structure):
    _fields_ = [
        ('dwSize',              ctypes.wintypes.DWORD),
        ('cntUsage',            ctypes.wintypes.DWORD),
        ('th32ProcessID',       ctypes.wintypes.DWORD),
        ('th32DefaultHeapID',   ctypes.POINTER(ctypes.wintypes.ULONG)),
        ('th32ModuleID',        ctypes.wintypes.DWORD),
        ('cntThreads',          ctypes.wintypes.DWORD),
        ('th32ParentProcessID', ctypes.wintypes.DWORD),
        ('pcPriClassBase',      ctypes.wintypes.LONG),
        ('dwFlags',             ctypes.wintypes.DWORD),
        ('szExeFile',           ctypes.c_char * 260)
    ]

class THREADENTRY32(ctypes.Structure):
    _fields_ = [
        ('dwSize', ctypes.wintypes.DWORD),
        ('cntUsage', ctypes.wintypes.DWORD),
        ('th32ThreadID', ctypes.wintypes.DWORD),
        ('th32OwnerProcessID', ctypes.wintypes.DWORD),
        ('tpBasePri', ctypes.wintypes.LONG),
        ('tpDeltaPri', ctypes.wintypes.LONG),
        ('dwFlags', ctypes.wintypes.DWORD),
    ]

# Load kernel32
kernel32 = ctypes.windll.kernel32

# API Function Signatures
OpenProcess = kernel32.OpenProcess
OpenProcess.argtypes = [ctypes.wintypes.DWORD, ctypes.wintypes.BOOL, ctypes.wintypes.DWORD]
OpenProcess.restype = ctypes.wintypes.HANDLE

VirtualAllocEx = kernel32.VirtualAllocEx
VirtualAllocEx.argtypes = [ctypes.wintypes.HANDLE, ctypes.wintypes.LPVOID, 
                           ctypes.c_size_t, ctypes.wintypes.DWORD, ctypes.wintypes.DWORD]
VirtualAllocEx.restype = ctypes.wintypes.LPVOID

WriteProcessMemory = kernel32.WriteProcessMemory
WriteProcessMemory.argtypes = [ctypes.wintypes.HANDLE, ctypes.wintypes.LPVOID,
                               ctypes.wintypes.LPCVOID, ctypes.c_size_t,
                               ctypes.POINTER(ctypes.c_size_t)]
WriteProcessMemory.restype = ctypes.wintypes.BOOL

GetModuleHandleA = kernel32.GetModuleHandleA
GetModuleHandleA.argtypes = [ctypes.wintypes.LPCSTR]
GetModuleHandleA.restype = ctypes.wintypes.HMODULE

GetProcAddress = kernel32.GetProcAddress
GetProcAddress.argtypes = [ctypes.wintypes.HMODULE, ctypes.wintypes.LPCSTR]
GetProcAddress.restype = ctypes.wintypes.LPVOID

CreateRemoteThread = kernel32.CreateRemoteThread
CreateRemoteThread.argtypes = [ctypes.wintypes.HANDLE, ctypes.wintypes.LPVOID,
                               ctypes.c_size_t, ctypes.wintypes.LPVOID,
                               ctypes.wintypes.LPVOID, ctypes.wintypes.DWORD,
                               ctypes.wintypes.LPVOID]
CreateRemoteThread.restype = ctypes.wintypes.HANDLE

WaitForSingleObject = kernel32.WaitForSingleObject
WaitForSingleObject.argtypes = [ctypes.wintypes.HANDLE, ctypes.wintypes.DWORD]
WaitForSingleObject.restype = ctypes.wintypes.DWORD

CloseHandle = kernel32.CloseHandle
CloseHandle.argtypes = [ctypes.wintypes.HANDLE]
CloseHandle.restype = ctypes.wintypes.BOOL

GetLastError = kernel32.GetLastError
GetLastError.restype = ctypes.wintypes.DWORD

CreateToolhelp32Snapshot = kernel32.CreateToolhelp32Snapshot
CreateToolhelp32Snapshot.argtypes = [ctypes.wintypes.DWORD, ctypes.wintypes.DWORD]
CreateToolhelp32Snapshot.restype = ctypes.wintypes.HANDLE

Process32First = kernel32.Process32First
Process32First.argtypes = [ctypes.wintypes.HANDLE, ctypes.POINTER(PROCESSENTRY32)]
Process32First.restype = ctypes.wintypes.BOOL

Process32Next = kernel32.Process32Next
Process32Next.argtypes = [ctypes.wintypes.HANDLE, ctypes.POINTER(PROCESSENTRY32)]
Process32Next.restype = ctypes.wintypes.BOOL

VirtualFreeEx = kernel32.VirtualFreeEx
VirtualFreeEx.argtypes = [ctypes.wintypes.HANDLE, ctypes.wintypes.LPVOID,
                          ctypes.c_size_t, ctypes.wintypes.DWORD]
VirtualFreeEx.restype = ctypes.wintypes.BOOL

GetExitCodeThread = kernel32.GetExitCodeThread
GetExitCodeThread.argtypes = [ctypes.wintypes.HANDLE, ctypes.POINTER(ctypes.wintypes.DWORD)]
GetExitCodeThread.restype = ctypes.wintypes.BOOL

# ============================================================
# INJECTOR CLASS
# ============================================================

class DLLInjector:
    def __init__(self, process_name="fieldrunners2.exe"):
        self.process_name = process_name
        self.logs = []
    
    def log(self, message, level="INFO"):
        """Add log message"""
        timestamp = time.strftime("%H:%M:%S")
        log_msg = f"[{timestamp}] [{level}] {message}"
        self.logs.append(log_msg)
        print(log_msg)
    
    def find_process_id(self):
        """Find process ID - cari yang punya window game class Fieldrunners 2"""
        self.log(f"Mencari process game...")
        
        target_pid = None
        
        def enum_windows_callback(hwnd, lParam):
            nonlocal target_pid
            
            class_name = ctypes.create_unicode_buffer(256)
            ctypes.windll.user32.GetClassNameW(hwnd, class_name, 256)
            
            title = ctypes.create_unicode_buffer(256)
            ctypes.windll.user32.GetWindowTextW(hwnd, title, 256)
            
            # Cari window dengan class DAN title = "Fieldrunners 2"
            if class_name.value == "Fieldrunners 2" and title.value == "Fieldrunners 2":
                pid = ctypes.wintypes.DWORD()
                ctypes.windll.user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                
                self.log(f"  Game window: PID={pid.value}")
                target_pid = pid.value
                return False  # Stop enum
            
            return True
        
        WNDENUMPROC = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)
        ctypes.windll.user32.EnumWindows(WNDENUMPROC(enum_windows_callback), 0)
        
        if target_pid:
            self.log(f"Target: PID {target_pid}", "SUCCESS")
            return target_pid
        
        # Fallback: cari dari process name
        snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
        if snapshot != -1:
            process_entry = PROCESSENTRY32()
            process_entry.dwSize = ctypes.sizeof(PROCESSENTRY32)
            
            if Process32First(snapshot, ctypes.byref(process_entry)):
                while True:
                    exe_name = process_entry.szExeFile.decode('utf-8', errors='ignore').lower()
                    if exe_name == "fieldrunners2.exe":
                        pid = process_entry.th32ProcessID
                        self.log(f"  Fallback: PID={pid}")
                        target_pid = pid
                    
                    if not Process32Next(snapshot, ctypes.byref(process_entry)):
                        break
            
            CloseHandle(snapshot)
        
        if target_pid:
            self.log(f"Target: PID {target_pid}", "SUCCESS")
            return target_pid
        
        self.log("Process tidak ditemukan!", "ERROR")
        return None
    
    def inject(self, dll_path):
        """Inject DLL ke target process"""
        self.log("=" * 50)
        self.log("MEMULAI INJEKSI")
        self.log("=" * 50)
        
        if not os.path.isabs(dll_path):
            dll_path = str(Path(__file__).parent / dll_path)
        
        self.log(f"DLL Path: {dll_path}")
        
        if not os.path.exists(dll_path):
            self.log(f"DLL tidak ditemukan: {dll_path}", "ERROR")
            return False, f"DLL tidak ditemukan:\n{dll_path}"
        
        pid = self.find_process_id()
        if not pid:
            return False, f"Process '{self.process_name}' tidak berjalan!\n\nJalankan game dulu."
        
        self.log("Membuka process...")
        process_handle = OpenProcess(PROCESS_ALL_ACCESS, False, pid)
        
        if not process_handle:
            error = GetLastError()
            self.log(f"Gagal membuka process! Error: {error}", "ERROR")
            if error == 5:
                return False, "AKSES DITOLAK!\n\nJalankan sebagai Administrator!"
            return False, f"Gagal membuka process (Error: {error})"
        
        self.log(f"Process handle: {hex(process_handle)}")
        
        try:
            dll_path_bytes = dll_path.encode('utf-8')
            dll_path_size = len(dll_path_bytes) + 1
            
            self.log(f"Mengalokasikan {dll_path_size} bytes memory...")
            
            remote_memory = VirtualAllocEx(
                process_handle, None, dll_path_size,
                MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
            
            if not remote_memory:
                error = GetLastError()
                self.log(f"VirtualAllocEx gagal! Error: {error}", "ERROR")
                return False, f"Alokasi memory gagal (Error: {error})"
            
            self.log(f"Memory dialokasikan di: {hex(remote_memory)}")
            
            self.log("Menulis DLL path ke memory...")
            bytes_written = ctypes.c_size_t(0)
            
            if not WriteProcessMemory(process_handle, remote_memory, 
                                       dll_path_bytes, dll_path_size, 
                                       ctypes.byref(bytes_written)):
                error = GetLastError()
                self.log(f"WriteProcessMemory gagal! Error: {error}", "ERROR")
                VirtualFreeEx(process_handle, remote_memory, 0, MEM_RELEASE)
                return False, f"Gagal menulis memory (Error: {error})"
            
            self.log(f"Berhasil menulis {bytes_written.value} bytes")
            
            self.log("Mendapatkan alamat LoadLibraryA...")
            kernel32_handle = GetModuleHandleA(b"kernel32.dll")
            if not kernel32_handle:
                VirtualFreeEx(process_handle, remote_memory, 0, MEM_RELEASE)
                return False, "Gagal mendapatkan kernel32.dll"
            
            load_library_addr = GetProcAddress(kernel32_handle, b"LoadLibraryA")
            if not load_library_addr:
                VirtualFreeEx(process_handle, remote_memory, 0, MEM_RELEASE)
                return False, "Gagal mendapatkan LoadLibraryA"
            
            self.log(f"LoadLibraryA address: {hex(load_library_addr)}")
            
            self.log("Membuat remote thread...")
            remote_thread = CreateRemoteThread(
                process_handle, None, 0,
                load_library_addr, remote_memory, 0, None)
            
            if not remote_thread:
                error = GetLastError()
                self.log(f"CreateRemoteThread gagal! Error: {error}", "ERROR")
                VirtualFreeEx(process_handle, remote_memory, 0, MEM_RELEASE)
                if error == 299:
                    return False, "ERROR_PARTIAL_COPY (299)\nGunakan Python 32-bit!"
                return False, f"Gagal membuat remote thread (Error: {error})"
            
            self.log(f"Remote thread dibuat: {hex(remote_thread)}")
            
            self.log("Menunggu DLL loading...")
            wait_result = WaitForSingleObject(remote_thread, 5000)
            
            if wait_result == WAIT_OBJECT_0:
                exit_code = ctypes.wintypes.DWORD()
                GetExitCodeThread(remote_thread, ctypes.byref(exit_code))
                
                if exit_code.value != 0:
                    self.log(f"DLL berhasil diload! Base: {hex(exit_code.value)}", "SUCCESS")
                    result = True, "✅ INJEKSI BERHASIL!"
                else:
                    last_error = GetLastError()
                    error_msg = f"Error code: {last_error}"
                    if last_error == 0:
                        error_msg = "DLL return FALSE di DllMain"
                    self.log(f"LoadLibraryA return NULL - {error_msg}", "WARNING")
                    result = False, f"⚠️ DLL gagal diload!\n{error_msg}"
            else:
                result = False, "⚠️ Timeout!"
            
            CloseHandle(remote_thread)
            return result
            
        finally:
            CloseHandle(process_handle)
            self.log("Process handle ditutup")
            self.log("=" * 50)

# ============================================================
# GUI APPLICATION
# ============================================================

class InjectorApp:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("FR2 DLL Injector - Fieldrunners 2")
        
        # Set minimum size tapi bisa di-resize
        self.root.minsize(500, 600)
        self.root.geometry("550x650")
        self.root.configure(bg='#1a1a2e')
        
        # State
        self.is_injecting = False
        self.dll_path = tk.StringVar(value="FR2_Mod.dll")
        self.process_name = tk.StringVar(value="fieldrunners2.exe")
        
        # Check architecture
        if not is_32bit_python():
            check_architecture()
            return
        
        self.setup_ui()
    
    def setup_ui(self):
        """Setup GUI components"""
        # Main container dengan scroll
        main_frame = tk.Frame(self.root, bg='#1a1a2e')
        main_frame.pack(fill='both', expand=True, padx=20, pady=20)
        
        # Title
        title = tk.Label(
            main_frame,
            text="🎮 Fieldrunners 2\nDLL Injector",
            font=('Segoe UI', 18, 'bold'),
            fg='#e94560',
            bg='#1a1a2e'
        )
        title.pack(pady=(0, 15))
        
        # Architecture badge
        arch_frame = tk.Frame(main_frame, bg='#16213e', relief='solid', bd=1)
        arch_frame.pack(fill='x', pady=(0, 15))
        
        arch_label = tk.Label(
            arch_frame,
            text="✅ Python 32-bit - Siap Inject!",
            font=('Segoe UI', 10, 'bold'),
            fg='#00ff88',
            bg='#16213e'
        )
        arch_label.pack(padx=15, pady=8)
        
        # Process name frame
        proc_frame = tk.Frame(main_frame, bg='#1a1a2e')
        proc_frame.pack(fill='x', pady=(0, 10))
        
        tk.Label(
            proc_frame,
            text="🎯 Target Process:",
            font=('Segoe UI', 10, 'bold'),
            fg='#ffffff',
            bg='#1a1a2e'
        ).pack(anchor='w')
        
        proc_entry = tk.Entry(
            proc_frame,
            textvariable=self.process_name,
            font=('Segoe UI', 11),
            bg='#16213e',
            fg='#ffffff',
            insertbackground='#ffffff',
            relief='flat'
        )
        proc_entry.pack(fill='x', ipady=5, pady=(5, 0))
        
        # DLL path frame
        dll_frame = tk.Frame(main_frame, bg='#1a1a2e')
        dll_frame.pack(fill='x', pady=(0, 15))
        
        tk.Label(
            dll_frame,
            text="📦 DLL File:",
            font=('Segoe UI', 10, 'bold'),
            fg='#ffffff',
            bg='#1a1a2e'
        ).pack(anchor='w')
        
        dll_select_frame = tk.Frame(dll_frame, bg='#1a1a2e')
        dll_select_frame.pack(fill='x', pady=(5, 0))
        
        dll_entry = tk.Entry(
            dll_select_frame,
            textvariable=self.dll_path,
            font=('Segoe UI', 11),
            bg='#16213e',
            fg='#ffffff',
            insertbackground='#ffffff',
            relief='flat'
        )
        dll_entry.pack(side='left', fill='x', expand=True, ipady=5)
        
        browse_btn = tk.Button(
            dll_select_frame,
            text="📂 Browse",
            command=self.browse_dll,
            bg='#0f3460',
            fg='white',
            font=('Segoe UI', 9),
            relief='flat',
            padx=10,
            cursor='hand2'
        )
        browse_btn.pack(side='left', padx=(5, 0))

        self.launch_btn = tk.Button(
            main_frame,
            text="🚀 LAUNCH GAME & INJECT",
            command=self.launch_and_inject,
            bg='#0f3460',
            fg='white',
            font=('Segoe UI', 12, 'bold'),
            relief='flat',
            pady=10,
            cursor='hand2'
        )
        self.launch_btn.pack(fill='x', pady=(0, 10))
        
        # Inject button
        self.inject_btn = tk.Button(
            main_frame,
            text="💉 INJECT DLL",
            command=self.start_injection,
            bg='#e94560',
            fg='white',
            font=('Segoe UI', 14, 'bold'),
            relief='flat',
            pady=12,
            cursor='hand2'
        )
        self.inject_btn.pack(fill='x', pady=(10, 15))
        
        # Status label
        self.status_label = tk.Label(
            main_frame,
            text="Status: Menunggu...\nJalankan game lalu klik INJECT",
            font=('Segoe UI', 10),
            fg='#a0a0a0',
            bg='#1a1a2e',
            wraplength=500,
            justify='center'
        )
        self.status_label.pack(pady=(0, 10))
        
        # Log label
        tk.Label(
            main_frame,
            text="📋 Log:",
            font=('Segoe UI', 10, 'bold'),
            fg='#ffffff',
            bg='#1a1a2e'
        ).pack(anchor='w')
        
        # Log frame
        log_frame = tk.Frame(main_frame, bg='#1a1a2e')
        log_frame.pack(fill='both', expand=True, pady=(5, 0))
        
        # Log text dengan scrollbar
        self.log_text = tk.Text(
            log_frame,
            height=10,
            font=('Consolas', 9),
            bg='#0f0f1a',
            fg='#00ff00',
            relief='flat',
            wrap='word',
            state='disabled',
            insertbackground='#00ff00'
        )
        
        scrollbar = tk.Scrollbar(log_frame, command=self.log_text.yview)
        self.log_text.configure(yscrollcommand=scrollbar.set)
        
        self.log_text.pack(side='left', fill='both', expand=True)
        scrollbar.pack(side='right', fill='y')
        
        # Bottom info
        info_frame = tk.Frame(main_frame, bg='#1a1a2e')
        info_frame.pack(fill='x', pady=(10, 0))
        
        info_label = tk.Label(
            info_frame,
            text="💡 Tips:\n"
                 "   🚀 Launch & Inject: Jalankan game + inject otomatis (RECOMMENDED!)\n"
                 "   💉 Inject: Untuk game yang sudah berjalan\n"
                 "   Run as Administrator untuk hasil terbaik",
            font=('Segoe UI', 8),
            fg='#666666',
            bg='#1a1a2e',
            justify='left'
        )
        info_label.pack()
    
    def browse_dll(self):
        """Open file browser"""
        filename = filedialog.askopenfilename(
            title="Pilih DLL File",
            filetypes=[("DLL Files", "*.dll"), ("All Files", "*.*")],
            initialdir=Path(__file__).parent
        )
        if filename:
            self.dll_path.set(filename)
            self.add_log(f"Dipilih: {filename}")
    
    def add_log(self, message):
        """Add message to log widget"""
        self.log_text.config(state='normal')
        timestamp = time.strftime("%H:%M:%S")
        self.log_text.insert('end', f"[{timestamp}] {message}\n")
        self.log_text.see('end')
        self.log_text.config(state='disabled')
        self.root.update_idletasks()
    
    def start_injection(self):
        """Start injection in thread"""
        if self.is_injecting:
            return
        
        # Validate
        dll_path = self.dll_path.get()
        if not dll_path:
            messagebox.showwarning("Error", "Pilih DLL file dulu!")
            return
        
        self.is_injecting = True
        self.inject_btn.config(state='disabled', text="⏳ INJECTING...")
        self.status_label.config(text="Status: Sedang meng-inject...", fg='#ffd43b')
        
        # Clear log
        self.log_text.config(state='normal')
        self.log_text.delete(1.0, 'end')
        self.log_text.config(state='disabled')
        
        # Run in thread
        thread = threading.Thread(
            target=self.perform_injection,
            args=(dll_path,),
            daemon=True
        )
        thread.start()
    
    def perform_injection(self, dll_path):
        """Perform injection process"""
        try:
            injector = DLLInjector(self.process_name.get())
            
            # Capture logs
            original_log = injector.log
            def custom_log(message, level="INFO"):
                original_log(message, level)
                self.root.after(0, self.add_log, f"[{level}] {message}")
            injector.log = custom_log
            
            success, message = injector.inject(dll_path)
            
            # Update UI
            self.root.after(0, self.injection_done, success, message)
        except Exception as e:
            self.root.after(0, self.injection_done, False, f"Error: {str(e)}")
    
    def injection_done(self, success, message):
        """Handle injection completion"""
        self.is_injecting = False
        self.inject_btn.config(state='normal', text="💉 INJECT DLL")
        
        if success:
            self.status_label.config(
                text=f"Status: ✅ BERHASIL!\n{message}",
                fg='#00ff00'
            )
            self.add_log("✅ INJEKSI BERHASIL!")
            self.add_log("Aplikasi akan menutup otomatis dalam 3 detik...")
            
            # Countdown 3 detik lalu tutup
            def countdown_and_close():
                for i in range(3, 0, -1):
                    self.root.after(0, self.status_label.config, 
                        {'text': f"Status: ✅ BERHASIL!\nMenutup dalam {i} detik..."})
                    time.sleep(1)
                self.root.after(0, self.root.destroy)
            
            threading.Thread(target=countdown_and_close, daemon=True).start()
        else:
            self.status_label.config(
                text=f"Status: ❌ GAGAL!\n{message[:100]}...",
                fg='#ff4444'
            )
            self.add_log("❌ INJEKSI GAGAL!")
            messagebox.showerror("❌ Gagal", message)

    def launch_and_inject(self):
        """Jalankan game lalu inject DLL"""
        if self.is_injecting:
            return
        
        # Pilih game executable
        game_path = filedialog.askopenfilename(
            title="Pilih fieldrunners2.exe",
            filetypes=[("EXE files", "*.exe")],
            initialdir=str(Path(__file__).parent)
        )
        
        if not game_path:
            return
        
        # Validasi DLL
        dll_path = self.dll_path.get()
        if not os.path.isabs(dll_path):
            dll_path = str(Path(__file__).parent / dll_path)
        
        if not os.path.exists(dll_path):
            messagebox.showerror("Error", f"DLL tidak ditemukan:\n{dll_path}")
            return
        
        self.is_injecting = True
        self.launch_btn.config(state='disabled', text="⏳ LAUNCHING...")
        self.status_label.config(text="Status: Menjalankan game...", fg='#ffd43b')
        
        # Clear log
        self.log_text.config(state='normal')
        self.log_text.delete(1.0, 'end')
        self.log_text.config(state='disabled')
        
        # Jalankan di thread terpisah
        thread = threading.Thread(
            target=self._launch_and_inject_thread,
            args=(game_path, dll_path),
            daemon=True
        )
        thread.start()
    
    def _launch_and_inject_thread(self, game_path, dll_path):
        """Thread untuk launch & inject"""
        try:
            self.root.after(0, self.add_log, "[INFO] Menjalankan game dalam mode SUSPENDED...")
            self.root.after(0, self.add_log, f"[INFO] Game: {game_path}")
            
            # Constants
            CREATE_SUSPENDED = 0x00000004
            NORMAL_PRIORITY_CLASS = 0x00000020
            
            # Baca DLL untuk inject
            dll_path_bytes = dll_path.encode('utf-8')
            
            # Jalankan game dalam keadaan SUSPENDED
            startup_info = subprocess.STARTUPINFO()
            startup_info.dwFlags = 0x00000001  # STARTF_USESHOWWINDOW
            startup_info.wShowWindow = 1  # SW_SHOWNORMAL
            
            # Dapatkan direktori game
            game_dir = os.path.dirname(game_path)
            
            # Jalankan game dalam keadaan SUSPENDED dengan working directory yang benar
            process = subprocess.Popen(
                game_path,
                creationflags=CREATE_SUSPENDED,
                cwd=game_dir  # ← PENTING! Working directory harus folder game
            )
            
            pid = process.pid
            self.root.after(0, self.add_log, f"[SUCCESS] Game berjalan! PID: {pid}")
            self.root.after(0, self.add_log, "[INFO] Meng-inject DLL sebelum game resume...")
            
            # Inject DLL ke process yang masih suspended
            process_handle = OpenProcess(PROCESS_ALL_ACCESS, False, pid)
            
            if not process_handle:
                error = GetLastError()
                self.root.after(0, self.add_log, f"[ERROR] Gagal buka process! Error: {error}")
                kernel32.TerminateProcess(process._handle, 0)
                self.root.after(0, self.injection_done, False, f"Gagal buka process (Error: {error})")
                return
            
            # Allocate memory
            dll_path_size = len(dll_path_bytes) + 1
            remote_memory = VirtualAllocEx(
                process_handle, None, dll_path_size,
                MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE
            )
            
            if not remote_memory:
                error = GetLastError()
                kernel32.TerminateProcess(process._handle, 0)
                CloseHandle(process_handle)
                self.root.after(0, self.add_log, f"[ERROR] VirtualAllocEx gagal! Error: {error}")
                self.root.after(0, self.injection_done, False, f"Alokasi memory gagal (Error: {error})")
                return
            
            self.root.after(0, self.add_log, f"[INFO] Memory dialokasikan: {hex(remote_memory)}")
            
            # Write DLL path
            bytes_written = ctypes.c_size_t(0)
            if not WriteProcessMemory(process_handle, remote_memory, dll_path_bytes, dll_path_size, ctypes.byref(bytes_written)):
                error = GetLastError()
                VirtualFreeEx(process_handle, remote_memory, 0, MEM_RELEASE)
                kernel32.TerminateProcess(process._handle, 0)
                CloseHandle(process_handle)
                self.root.after(0, self.add_log, f"[ERROR] WriteProcessMemory gagal! Error: {error}")
                self.root.after(0, self.injection_done, False, f"Gagal menulis memory (Error: {error})")
                return
            
            self.root.after(0, self.add_log, f"[INFO] DLL path ditulis ({bytes_written.value} bytes)")
            
            # Get LoadLibraryA
            kernel32_handle = GetModuleHandleA(b"kernel32.dll")
            load_library_addr = GetProcAddress(kernel32_handle, b"LoadLibraryA")
            
            if not load_library_addr:
                VirtualFreeEx(process_handle, remote_memory, 0, MEM_RELEASE)
                kernel32.TerminateProcess(process._handle, 0)
                CloseHandle(process_handle)
                self.root.after(0, self.injection_done, False, "Gagal mendapatkan LoadLibraryA")
                return
            
            # Create remote thread
            remote_thread = CreateRemoteThread(
                process_handle, None, 0,
                load_library_addr, remote_memory, 0, None
            )
            
            if not remote_thread:
                error = GetLastError()
                VirtualFreeEx(process_handle, remote_memory, 0, MEM_RELEASE)
                kernel32.TerminateProcess(process._handle, 0)
                CloseHandle(process_handle)
                self.root.after(0, self.add_log, f"[ERROR] CreateRemoteThread gagal! Error: {error}")
                self.root.after(0, self.injection_done, False, f"Gagal membuat remote thread (Error: {error})")
                return
            
            self.root.after(0, self.add_log, "[INFO] DLL Thread dibuat, menunggu...")
            
            # Wait for DLL to load
            WaitForSingleObject(remote_thread, 5000)
            
            exit_code = ctypes.wintypes.DWORD()
            GetExitCodeThread(remote_thread, ctypes.byref(exit_code))
            
            if exit_code.value != 0:
                self.root.after(0, self.add_log, f"[SUCCESS] DLL loaded! Base: {hex(exit_code.value)}")
            else:
                self.root.after(0, self.add_log, "[WARNING] DLL mungkin gagal inisialisasi")
            
            # Cleanup
            CloseHandle(remote_thread)
            CloseHandle(process_handle)
            
            # RESUME game!
            self.root.after(0, self.add_log, "[INFO] Me-resume game...")
            
            # Resume semua thread di process
            
            TH32CS_SNAPTHREAD = 0x00000004
            
            # Dapatkan snapshot thread
            thread_snapshot = kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
            
            if thread_snapshot != -1:
                
                thread_entry = THREADENTRY32()
                thread_entry.dwSize = ctypes.sizeof(THREADENTRY32)
                
                if kernel32.Thread32First(thread_snapshot, ctypes.byref(thread_entry)):
                    while True:
                        if thread_entry.th32OwnerProcessID == pid:
                            # Buka thread
                            h_thread = kernel32.OpenThread(0x0002, False, thread_entry.th32ThreadID)
                            if h_thread:
                                # Resume thread
                                kernel32.ResumeThread(h_thread)
                                kernel32.CloseHandle(h_thread)
                        
                        if not kernel32.Thread32Next(thread_snapshot, ctypes.byref(thread_entry)):
                            break
                
                kernel32.CloseHandle(thread_snapshot)
            
            self.root.after(0, self.add_log, "[INFO] Semua thread di-resume!")
            
            time.sleep(0.5)
            
            self.root.after(0, self.add_log, "[SUCCESS] Game berjalan dengan DLL!")
            self.root.after(0, self.injection_done, True, 
                          "✅ Game diluncurkan & DLL di-inject!\n\n"
                          "Menu akan muncul di game!")
            
        except Exception as e:
            self.root.after(0, self.add_log, f"[ERROR] Exception: {str(e)}")
            self.root.after(0, self.injection_done, False, f"Error: {str(e)}")
    
    def run(self):
        """Start GUI"""
        # Center window
        self.root.update_idletasks()
        width = self.root.winfo_width()
        height = self.root.winfo_height()
        x = (self.root.winfo_screenwidth() // 2) - (width // 2)
        y = (self.root.winfo_screenheight() // 2) - (height // 2)
        self.root.geometry(f'{width}x{height}+{x}+{y}')
        
        self.root.mainloop()

# ============================================================
# MAIN
# ============================================================

if __name__ == "__main__":
    # Check admin
    try:
        is_admin = ctypes.windll.shell32.IsUserAnAdmin()
        if not is_admin:
            print("[!] Tidak running sebagai Admin")
            print("[!] Beberapa fitur mungkin gagal")
            print("[!] Disarankan: Run as Administrator\n")
    except:
        pass
    
    # Start app
    app = InjectorApp()
    app.run()