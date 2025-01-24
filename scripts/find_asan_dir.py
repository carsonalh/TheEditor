import subprocess
import os.path
import sys

text = subprocess.check_output('"C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath', shell=True, text=True)
vs_install_path = text.strip()

toolchain_installation_dir = os.path.join(vs_install_path, 'VC\\Tools\\MSVC\\')
toolchain_installations = os.listdir(toolchain_installation_dir)

toolchain_installations = list(map(lambda d : os.path.join(toolchain_installation_dir, d), toolchain_installations))

if len(toolchain_installations) < 1:
    sys.stderr.write(f'Could not find any MSVC toolchains installed (checked in "{toolchain_installation_dir}").')
    sys.exit(1)

# TODO find out if there are ever more than one of these directories
installation = toolchain_installations[0]
asan_path = os.path.join(installation, "bin\\Hostx64\\x64\\")
print(asan_path)
sys.exit(0)
