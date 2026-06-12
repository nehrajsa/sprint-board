Import("env")
import subprocess
import os
import sys

frontend_dir = os.path.join(env["PROJECT_DIR"], "frontend")
webgui_cpp   = os.path.join(env["PROJECT_DIR"], "src", "webgui.cpp")

def needs_rebuild():
    if not os.path.exists(webgui_cpp):
        return True
    mtime = os.path.getmtime(webgui_cpp)
    src_dir = os.path.join(frontend_dir, "src")
    for root, _, files in os.walk(src_dir):
        for f in files:
            if os.path.getmtime(os.path.join(root, f)) > mtime:
                return True
    return False

if needs_rebuild():
    print("Frontend sources changed — rebuilding...")
    result = subprocess.run("pnpm build", cwd=frontend_dir, shell=True)
    if result.returncode != 0:
        print("ERROR: frontend build failed")
        env.Exit(1)
else:
    print("Frontend up to date, skipping rebuild")
