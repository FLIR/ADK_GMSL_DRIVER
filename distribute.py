import os
import shutil
import subprocess


def mkdir(path):
    try:
        os.mkdir(path)
    except FileExistsError:
        pass

dist_folder = 'dist'
shutil.rmtree(dist_folder)
mkdir(dist_folder)

omit_files = 'distribute.py .gitignore'.split()
add_files = 'FSLP_64.so C_SDK.so libopencvConnector.so'.split()

list_of_files = list(map(lambda a: a.decode('utf-8'), 
    subprocess.check_output("git ls-files", shell=True).splitlines()))
list_of_files = list(filter(lambda f: f not in omit_files, list_of_files))
list_of_files += add_files

app_folder = os.path.join(dist_folder, 'app')
mkdir(app_folder)

for f in list_of_files:
    shutil.copy(f, app_folder)

folders = 'cv_install include make toolchains utils BosonSDK'.split()
for folder in folders:
    shutil.copytree(os.path.join('..', folder),
        os.path.join(dist_folder, folder))


