import os
import gzip
import shutil
import subprocess
import SCons.Script
from SCons.Script import COMMAND_LINE_TARGETS

import_env = SCons.Script.DefaultEnvironment()


def gzip_webfiles(source, target, env):
    print("Executing custom python script to gzip web files...")
    data_dir = os.path.join(env.get("PROJECT_DIR"), "data")
    if not os.path.isdir(data_dir):
        return

    for root, _, files in os.walk(data_dir):
        for f in files:
            if f.endswith(('.html', '.css', '.js')) and not f.endswith('.gz'):
                file_path = os.path.join(root, f)
                with open(file_path, 'rb') as f_in:
                    with gzip.open(file_path + '.gz', 'wb') as f_out:
                        shutil.copyfileobj(f_in, f_out)
                os.remove(file_path)


def upload_littlefs(source, target, env):
    if "uploadfs" in COMMAND_LINE_TARGETS:
        return

    cmd = [
        env.subst("$PYTHONEXE"),
        "-m",
        "platformio",
        "run",
        "--environment",
        env.subst("$PIOENV"),
        "--target",
        "uploadfs",
    ]
    upload_port = env.subst("$UPLOAD_PORT")
    if upload_port:
        cmd.extend(["--upload-port", upload_port])

    print("Uploading LittleFS image...")
    subprocess.run(cmd, cwd=env.subst("$PROJECT_DIR"), check=True)


# Tell PlatformIO to run this step before generating the littlefs.bin image.
import_env.AddPreAction("$BUILD_DIR/littlefs.bin", gzip_webfiles)
import_env.AddPostAction("upload", upload_littlefs)
