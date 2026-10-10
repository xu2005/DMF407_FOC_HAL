from pathlib import Path
import ctypes, json, subprocess

root = Path.cwd()
out = root / 'build/audit_20261010/preprocessed'
out.mkdir(exist_ok=True)
shell32 = ctypes.WinDLL('shell32')
shell32.CommandLineToArgvW.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_int)]
shell32.CommandLineToArgvW.restype = ctypes.POINTER(ctypes.c_wchar_p)
kernel32 = ctypes.WinDLL('kernel32')
kernel32.LocalFree.argtypes = [ctypes.c_void_p]
for record in json.loads((root / 'build/Debug/compile_commands.json').read_text(encoding='utf-8')):
    source = Path(record['file'])
    if source.suffix != '.c' or source.relative_to(root).parts[0] not in ('Core','BSP'):
        continue
    argc = ctypes.c_int()
    ptr = shell32.CommandLineToArgvW(record['command'], ctypes.byref(argc))
    args = [ptr[i] for i in range(argc.value)]
    kernel32.LocalFree(ptr)
    pos = args.index('-o')
    del args[pos:pos+2]
    args.remove('-c')
    args.extend(['-E', '-g0', '-o', str(out / (source.stem+'.i'))])
    subprocess.run(args, cwd=record['directory'], check=True)
    macros = args[:]
    pos = macros.index('-o')
    del macros[pos:pos+2]
    macros.extend(['-dM'])
    content = subprocess.run(macros, cwd=record['directory'], check=True, capture_output=True).stdout
    (out / (source.stem+'.macros')).write_bytes(content)
    print(source.relative_to(root).as_posix())
