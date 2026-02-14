#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import sys
import re
import json
import requests

def get(url, file_name=None, dir_name=None):
    '''
    Get the files in the given URL
    :param url: The URL is a github API URL that returns a JSON object
    :param file_name: if specified, url will be github download_url
    :return: None
    :raises: Exception if the download fails
    '''
    try:
        response = requests.get(url)
        response.raise_for_status()
    except requests.exceptions.RequestException as e:
        raise Exception(f"Request failed: {e}")

    if file_name:
        with open(file_name, 'wb') as file:
            file.write(response.content)
        return

    def get_file(file):
        if file['download_url']:
            get(file['download_url'], file_name=file['name'])
        else:
            if file['type'] == 'dir':
                get(file['url'], dir_name=file['name'])
            else:
                get(file['git_url'].replace('/git/trees/', '/contents?ref='))

    try:
        data = response.json()
    except json.JSONDecodeError as e:
        raise Exception(f"JSON decode error: {e}")

    if isinstance(data, list):
        if not dir_name:
            pattern = r"https://api\.github\.com/repos/[^/]+/([^/]+)/contents(.*)"
            repo_name, path = re.search(pattern, url).groups()
            path = path.split('/')[-1].split('?')[0]
            dir_name = path if path else repo_name

        cur = os.getcwd()
        os.mkdir(dir_name)
        os.chdir(dir_name)
        for item in data:
            get_file(item)
        os.chdir(cur)
    else:
        get_file(data)

def help():
    print(f"Usage: {sys.argv[0]} <github REST API url>")
    print("        url format: https://api.github.com/repos/{owner}/{repo}/contents/{directory}?ref={branch}")
    print(f"       url example: https://api.github.com/repos/sonic-net/DASH/contents/dash-pipeline/utils?ref=main")
    sys.exit(1)

if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] == '-h':
        help()

    try:
        get(sys.argv[1])
    except Exception as e:
        print(e)
        sys.exit(1)
