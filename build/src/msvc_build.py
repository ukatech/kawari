#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""MSVC / VC6 build script for KAWARI (Windows only; no makefile or IDE project is needed).

The sources under build/src are EUC-JP, which cl.exe cannot read, so this script
converts a copy (UTF-8 for current MSVC, CP932 for VC6) and compiles that copy.
The working tree is never modified.

usage:
    python msvc_build.py <target> [options]

targets:
    x64    current Visual Studio, 64bit  (build/mach/msvc_x64)
    x86    current Visual Studio, 32bit  (build/mach/msvc_x86)
    vc6    Visual C++ 6.0, 32bit         (build/mach/vc6)

options:
    --vs-dir DIR     Visual Studio install folder (default: found with vswhere, or %VSINSTALLDIR%)
    --vc6-dir DIR    VC98 folder of Visual C++ 6.0
                     (default: %VC6DIR% or C:\\Program Files (x86)\\Microsoft Visual Studio\\VC98)
    -j N             parallel compiles (default: number of CPUs)
    --warnings       show compiler warnings
    --no-link        compile only

Output: shiori.dll / kosui.exe / kawari_encode.exe / kawari_encode2.exe / kawari_decode2.exe
in build/mach/<target>/ (intermediate files are in the work/ folder below it).
Exit code: 0 = success, 1 = failure.

Notes:
  * Static CRT (/MT). misc/fastalloc.cpp is MinGW only and is not built.
  * VC6 has no _time64, so `date` / `mktime` cannot handle years after 2037 there.
"""
import argparse
import concurrent.futures as cf
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
MACH = os.path.normpath(os.path.join(HERE, "..", "mach"))
NAMES = {"x64": "msvc_x64", "x86": "msvc_x86", "vc6": "vc6"}
LIBS = "kernel32.lib user32.lib shell32.lib advapi32.lib"
# folders that are not part of the build (old sources, tests, the dropped VC project)
SKIP_DIRS = {"old", "test", "vc_kawari"}


def parse_files_mak():
    """Read the source lists from files.mak (UTF-8, make syntax)."""
    with open(os.path.join(HERE, "files.mak"), encoding="utf-8") as f:
        text = f.read().replace("\\\n", " ")
    raw = {m.group(1): m.group(2) for m in re.finditer(r"^(\w+)\s*=(.*)$", text, re.M)}

    def expand(s):
        return re.sub(r"\$\((\w+)\)", lambda m: expand(raw.get(m.group(1), "")), s)

    return {k: expand(v).split() for k, v in raw.items()}


def read_source(path):
    """EUC-JP (or UTF-8 / ASCII) bytes -> str."""
    data = open(path, "rb").read()
    try:
        return data.decode("utf-8-sig")
    except UnicodeDecodeError:
        return data.decode("euc_jis_2004")


def convert_tree(dst, target):
    """Copy the sources into dst, converting the encoding. Returns the number of files with unconvertible characters."""
    if os.path.isdir(dst):
        # ウイルス対策ソフト等が掴んでいて消せないことがあるので、少し粘って、駄目なら上書きで進める
        for _ in range(5):
            shutil.rmtree(dst, ignore_errors=True)
            if not os.path.isdir(dst):
                break
            time.sleep(0.5)
    vc6 = target == "vc6"
    lost = 0
    for root, dirs, files in os.walk(HERE):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS and not d.startswith(".")]
        rel = os.path.relpath(root, HERE)
        for name in files:
            if not name.lower().endswith((".cpp", ".h", ".rc", ".def")):
                continue
            text = read_source(os.path.join(root, name)).replace("\r\n", "\n")
            if vc6:
                text = text.replace("\n", "\r\n")
                try:
                    raw = text.encode("cp932")
                except UnicodeEncodeError:
                    raw = text.encode("cp932", errors="replace")
                    lost += 1
            else:
                raw = b"\xef\xbb\xbf" + text.encode("utf-8")
            out = os.path.normpath(os.path.join(dst, rel, name))
            os.makedirs(os.path.dirname(out), exist_ok=True)
            with open(out, "wb") as f:
                f.write(raw)
    return lost


def find_vcvars(target, vs_dir):
    if not vs_dir:
        vs_dir = os.environ.get("VSINSTALLDIR")
    if not vs_dir:
        pf = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
        vswhere = os.path.join(pf, "Microsoft Visual Studio", "Installer", "vswhere.exe")
        if os.path.exists(vswhere):
            r = subprocess.run([vswhere, "-latest", "-products", "*", "-property", "installationPath"],
                               capture_output=True, text=True)
            vs_dir = r.stdout.strip().splitlines()[0] if r.stdout.strip() else None
    if not vs_dir:
        sys.exit("error: Visual Studio was not found. Pass --vs-dir.")
    bat = os.path.join(vs_dir, "VC", "Auxiliary", "Build", "vcvars64.bat" if target == "x64" else "vcvars32.bat")
    if not os.path.exists(bat):
        sys.exit("error: %s was not found." % bat)
    return bat


def environment_lines(target, vs_dir, vc6_dir):
    """Batch lines that set up the compiler environment."""
    if target != "vc6":
        return ['call "%s" >nul 2>&1' % find_vcvars(target, vs_dir)]
    vc6_dir = vc6_dir or os.environ.get("VC6DIR") or \
        r"C:\Program Files (x86)\Microsoft Visual Studio\VC98"
    if not os.path.exists(os.path.join(vc6_dir, "Bin", "CL.EXE")):
        sys.exit("error: VC6 (%s\\Bin\\CL.EXE) was not found. Pass --vc6-dir." % vc6_dir)
    msdev = os.path.join(os.path.dirname(vc6_dir), "Common", "MSDev98", "Bin")  # MSPDB60.DLL, RC.EXE
    return [
        'set "PATH=%s\\Bin;%s;%%PATH%%"' % (vc6_dir, msdev),
        'set "INCLUDE=%s\\Include"' % vc6_dir,
        'set "LIB=%s\\Lib"' % vc6_dir,
    ]


def cl_flags(target, wlevel=3):
    f = ("/nologo /EHsc /W%d /O2 /MT /I. /DNDEBUG /DUSEKDB /DENABLE_SAORI_NATIVE "
         "/DUNICODE /D_UNICODE /DWINVER=0x0500 /D_WIN32_WINNT=0x0500" % wlevel)
    if target == "vc6":
        return f + " /GR"
    return f + " /utf-8 /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS"


class Runner:
    def __init__(self, env_lines, work):
        self.env_lines = env_lines
        self.work = work

    def run(self, cmd, cwd):
        fd, bat = tempfile.mkstemp(suffix=".bat", dir=self.work)
        text = "\r\n".join(["@echo off"] + self.env_lines + [cmd]) + "\r\n"
        os.write(fd, text.encode("cp932", errors="replace"))
        os.close(fd)
        try:
            r = subprocess.run(["cmd", "/c", bat], cwd=cwd, stdin=subprocess.DEVNULL,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        finally:
            os.remove(bat)
        return r.returncode, r.stdout.decode("cp932", errors="replace")


def main():
    ap = argparse.ArgumentParser(description="KAWARI MSVC / VC6 build")
    ap.add_argument("target", choices=sorted(NAMES))
    ap.add_argument("--vs-dir")
    ap.add_argument("--vc6-dir")
    ap.add_argument("-j", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--warnings", action="store_true")
    ap.add_argument("--wlevel", type=int, default=3, choices=[1, 2, 3, 4], help="cl /W level (default 3)")
    ap.add_argument("--no-link", action="store_true")
    a = ap.parse_args()
    if os.name != "nt":
        sys.exit("error: this script is for Windows.")

    target = a.target
    out_dir = os.path.join(MACH, NAMES[target])
    work = os.path.join(out_dir, "work")
    src = os.path.join(work, "src")
    obj = os.path.join(work, "obj")
    os.makedirs(obj, exist_ok=True)
    os.makedirs(out_dir, exist_ok=True)

    runner = Runner(environment_lines(target, a.vs_dir, a.vc6_dir), work)

    lost = convert_tree(src, target)
    if lost > 0:
        print("note: %d file(s) have comment characters outside CP932 (replaced with ?)" % lost)

    mak = parse_files_mak()
    core = mak["CORESRC"] + mak["CORESRC_NATIVE"]
    shiori = mak["SHIOSRC"]
    kosui = mak["KOSUISRC"] + mak["KOSUISRC_mingw"]
    tools = mak["TOOLSRC"]
    crypt = mak["CRYPTSRC"]
    sources = sorted(set(core + shiori + kosui + tools))

    def objname(s):
        return os.path.join(obj, s.replace("/", "_")[:-4] + ".obj")

    flags = cl_flags(target, a.wlevel)

    def compile_one(s):
        rc, text = runner.run('cl %s /c /Fo"%s" %s' % (flags, objname(s), s), src)
        return s, rc, text

    failed = []
    with cf.ThreadPoolExecutor(a.j) as ex:
        for s, rc, text in ex.map(compile_one, sources):
            lines = [l for l in text.splitlines() if l.strip()]
            if rc != 0:
                failed.append(s)
                print("FAIL %s\n%s" % (s, "\n".join(lines)))
            else:
                warn = [l for l in lines if "warning" in l.lower()]
                print("ok   %s%s" % (s, "  (%d warnings)" % len(warn) if warn else ""))
                if warn and a.warnings:
                    print("\n".join("     " + l for l in warn))
    if failed:
        print("\ncompile failed: %d file(s)" % len(failed))
        return 1
    if a.no_link:
        return 0

    objs = lambda lst: " ".join('"%s"' % objname(s) for s in lst)

    # version information resources (kawari_version.rc, windres on MinGW)
    res = {}
    for kind in ("shiori", "kosui", "tool"):
        r = os.path.join(work, "ver_%s.res" % kind)
        rc, text = runner.run('rc %s/I. /DKAWARI_RES_%s /fo"%s" kawari_version.rc' % ("" if target == "vc6" else "/nologo ", kind, r), src)
        if rc != 0:
            print("FAIL rc (%s)\n%s" % (kind, text))
            return 1
        res[kind] = '"%s"' % r

    steps = [
        ("shiori.dll", 'link /nologo /dll /out:"%s\\shiori.dll" %s %s %s %s'
         % (out_dir, objs(shiori), objs(core), res["shiori"], LIBS)),
        ("kosui.exe", 'link /nologo /out:"%s\\kosui.exe" %s %s %s %s'
         % (out_dir, objs(kosui), objs(core), res["kosui"], LIBS)),
    ]
    for t in tools:
        n = os.path.basename(t)[:-4]
        steps.append((n + ".exe", 'link /nologo /out:"%s\\%s.exe" %s %s %s %s'
                      % (out_dir, n, objs([t]), objs(crypt), res["tool"], LIBS)))
    bad = False
    for name, cmd in steps:
        rc, text = runner.run(cmd, work)
        print("link %s: %s" % (name, "ok" if rc == 0 else "FAILED\n" + text))
        bad = bad or rc != 0
    if not bad:
        print("\ndone: %s" % out_dir)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
