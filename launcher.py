import requests
import readchar
import os
import sys
import shutil
import subprocess

def stage1():
    version_path = getattr(sys, '_MEIPASS', os.getcwd())

    with open(f'{version_path}/version.txt') as version:
        current_version = version.read().strip()

    def exit_err(err):
        print(f"Error: {err}")
        print("Press any key to quit")
        readchar.readchar()
        exit(-1)

    print(f'Current version: {current_version}\nChecking for updates...')

    response = requests.get("https://api.github.com/repos/sarahkittyy/blockquest-remake/releases/latest")
    if not response:
        exit_err('Could not reach github api to update.')

    body = response.json()
    tag = body["tag_name"]

    if tag == current_version:
        print('Up to date!')
        exit(0)
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

    shutil.unpack_archive(zip_name, 'new/')
    os.remove(zip_name)
    print('Extracted latest version to new/')


    shutil.rmtree('old/', True)
    os.mkdir('old/')

    try:
        os.rename('new/launcher.exe', './launcher_new.exe')
        os.execv('./launcher_new.exe', ['./launcher_new.exe', 'stage-2'])
    except Exception as e:
        print(f'Failed to launch stage 2 of updater: {e}')

def stage2():
    print('in stage 2')
    pass
def stage3():
    print('in stage 3')
    pass

if len(sys.argv) < 2:
    stage1()
elif sys.argv[1] == "stage-2":
    stage2()
elif sys.argv[1] == "stage-3":
    stage3()
