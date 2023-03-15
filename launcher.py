import requests
import os
import sys
import shutil
import distutils.dir_util
import time
import subprocess

def stage1():
    version_path = getattr(sys, '_MEIPASS', os.getcwd())

    with open(f'{version_path}/version.txt') as version:
        current_version = version.read().strip()

    def exit_err(err):
        print(f"Could not update: {err}")
        print("Launching...")
        time.sleep(1)
        return launch()

    print(f'Current version: {current_version}\nChecking for updates...')

    response = requests.get("https://api.github.com/repos/sarahkittyy/blockquest-remake/releases/latest")
    if not response:
        exit_err('Could not reach github api to update.')

    body = response.json()
    tag = body["tag_name"]

    if tag == current_version:
        print('Up to date!')
        return launch()
    else:
        print(f'New version available: {current_version} -> {tag}')

    zip_asset = [asset for asset in body["assets"] if 'BlockQuestRemake' in asset["name"]]
    if len(zip_asset) == 0:
        exit_err('No zip in latest release')

    zip_asset = zip_asset[0]
    zip_name = zip_asset["name"]
    download_url = zip_asset["browser_download_url"]

    print('Downloading...')

    downloaded_zip = requests.get(download_url, allow_redirects=True)
    if not downloaded_zip:
        exit_err('Could not download zip file')

    open(zip_name, 'wb').write(downloaded_zip.content)

    print('Unzipping...')
    shutil.unpack_archive(zip_name, 'new/')
    os.remove(zip_name)
    print('Extracted latest version to new/')

    try:
        shutil.copyfile('new/launcher.exe', './launcher_new.exe')
        print('Launching stage 2...')
        if os.name == 'nt':
            os.system('start ./launcher_new.exe stage-2')
        else:
            os.execv('./launcher_new.exe', ['./launcher_new.exe', 'stage-2'])
    except Exception as e:
        print(f'Failed to launch stage 2 of updater: {e}')

def stage2():
    # remove the old launcher file
    try:
        print('Removing old launcher...')
        os.remove('./launcher.exe')
        print('Updating...')
        distutils.dir_util.copy_tree('new', '.')
        print('Launching cleanup...')
        if os.name == 'nt':
            os.system('start ./launcher.exe stage-3')
        else:
            os.execv('./launcher.exe', ['./launcher.exe', 'stage-3'])
    except Exception as e:
        print(f'Failed to launch stage 3 of updater: {e}')

def stage3():
    # remove temp launcher
    print('Cleaning up...')
    try:
        os.remove('./launcher_new.exe')
        shutil.rmtree('new/')
    except Exception as e:
        print(f'Could not remove old files. {e}')
    print('Done!')
    return launch()

def launch():
    if os.name == 'nt':
        with open('latest.log', 'w') as logfile:
            subprocess.run('blockquest-remake.exe', stdout=logfile, stderr=logfile)
    else:
        fdout = os.open('latest.log', os.O_WRONLY | os.O_CREAT | os.O_TRUNC)
        os.dup2(fdout, 1)
        os.dup2(fdout, 2)
        os.execv('./blockquest-remake.exe', ['./blockquest-remake.exe'])
        os._exit(127)

if len(sys.argv) < 2:
    stage1()
elif sys.argv[1] == "stage-2":
    stage2()
elif sys.argv[1] == "stage-3":
    stage3()
