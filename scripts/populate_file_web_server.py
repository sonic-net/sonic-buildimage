#!/usr/bin/env python3

import sys
import os
import time
import argparse
from http import HTTPStatus
try:
    import requests
except ImportError:
    print("requests module is not installed. script will fail to execute")

#debug print level
PRINT_LEVEL_ERROR         = "err"
PRINT_LEVEL_WARN          = "warn"
PRINT_LEVEL_INFO          = "info"
PRINT_LEVEL_VERBOSE       = "verbose"

PRINT_LEVEL_LUT = {PRINT_LEVEL_ERROR    : 1,
                   PRINT_LEVEL_WARN     : 2,
                   PRINT_LEVEL_INFO     : 3,
                   PRINT_LEVEL_VERBOSE  : 4 }

#return code
RET_CODE_SUCCESS                = 0
RET_CODE_CANNOT_CREATE_FILE     = -1
RET_CODE_CANNOT_OPEN_FILE       = -2
RET_CODE_HTTP_SERVER_ERROR      = -3
RET_CODE_CANNOT_WRITE_FILE      = -4

#constants
RESOURCES_FILE_NAME       = 'versions-web'
EXCLUDE_DIRECTORES        = ['fsroot', 'target']
HASH_SEPARATOR            = '-'
DEFAULT_INVALID_INPUT     = 'none'

# global variables
g_current_print_level     = PRINT_LEVEL_INFO

#Script debug features (disabled by default)
g_delete_resources_in_cache = True


# global Classes
class Resource:
    def __init__(self, line, file):
        self.file = file
        temp=line.split("==")
        assert(2==len(temp))
        self.url=temp[0].strip()
        self.hash=temp[1].strip()
        temp=self.url.split("/")
        assert(len(temp)>0)
        self.name=temp[len(temp)-1]
        #handle special scenarios
        if 0 != self.name.count('?') == True:
            temp = self.name.split("?")
            self.name = temp[0]

    def get_unique_name(self):
        return self.name + HASH_SEPARATOR + self.hash

    def get_url(self):
        return self.url

    def __str__(self):
        ret_val = "Resource name: " + self.name + "\n"
        ret_val += "File: " + self.file + "\n"
        ret_val += "Hash: " + self.hash + "\n"
        ret_val += "Full URL: " + self.url
        return ret_val

# Helper functions

def print_msg(print_level, msg, print_in_place=False):
    if PRINT_LEVEL_LUT[g_current_print_level] >= PRINT_LEVEL_LUT[print_level]:
        if True == print_in_place:
            print(msg, end='\r')
        else:
            print(msg)

def create_dir_if_not_exist(dir):
    if not os.path.exists(dir):
        try:
            os.makedirs(dir)
        except:
            print_msg(PRINT_LEVEL_WARN, "Cannot create directory " + dir)

def delete_file_if_exist(file):
    if os.path.exists(file):
        try:
            os.remove(file)
        except:
            print_msg(PRINT_LEVEL_WARN, "Cannot delete " + file)

# Logic functions

def generate_output_file(resources, dest_url_valid, dest_url, output_file_name):
    try:
        with open(output_file_name, 'w') as f:
            for unique_name in resources.keys():
                resource = resources[unique_name]
                if True == dest_url_valid:
                    line = dest_url
                else:
                    line = resource.get_url()
                if line[-1] != '/':
                    line += '/'
                line += resource.name + "==" + resource.hash
                f.write(line + '\n')
    except:
        print_msg(PRINT_LEVEL_WARN, output_file_name + " cannot be created")
        return RET_CODE_CANNOT_CREATE_FILE

    return RET_CODE_SUCCESS

def upload_resource_to_server(resource_path, resource_name, user, key, server_url):
    url_full_path = server_url + "/" + resource_name

    try:
        f = open(resource_path, 'rb')
    except:
        err_print("Cannot open " + resource_path)
        return RET_CODE_CANNOT_OPEN_FILE

    headers = {'Content-type': 'application', 'Slug': resource_name}
    response = requests.put(url_full_path, data=f,
                            headers=headers, auth=(user, key))

    f.close()

    if response.status_code != HTTPStatus.CREATED.value:
        err_print(f"HTTP request returned status code {response.status_code}, expected {HTTPStatus.CREATED.value}")
        return RET_CODE_HTTP_SERVER_ERROR

    # JSON response empty only when status code is 204
    reported_md5 = response.json().get('checksums', {}).get('md5')
    file_md5 = resource_name.split(HASH_SEPARATOR)[-1]

    # Check if server reports checksum, if so compare reported sum and the one
    # specified in filename
    if reported_md5 != None and reported_md5 != file_md5:
        print_msg(PRINT_LEVEL_WARN, f"Server reported file's chsum {reported_md5}, expected {file_md5}")


    return RET_CODE_SUCCESS

def download_external_resouce(resource, cache_path):
    resource_path_in_cache = cache_path + os.sep + resource.get_unique_name()

    r = requests.get(resource.get_url(), allow_redirects=True)

    try:
        f = open(resource_path_in_cache, 'wb')
        f.write(r.content)
        f.close()
    except:
        print_msg(PRINT_LEVEL_ERROR, "Cannot write " + resource_path_in_cache + " to cache")
        resource_path_in_cache = "" #report error

    return resource_path_in_cache

def get_resources_list(resource_files_list):
    resource_list = list()

    for file_name in resource_files_list:
        try:
            with open(file_name, 'r') as f:
                for line in f:
                    resource_list.append(Resource(line, file_name))
        except:
            print_msg(PRINT_LEVEL_WARN, file_name + " cannot be opened")

    return resource_list

def filter_out_dir(subdir):
    ret_val = False

    for exclude in EXCLUDE_DIRECTORES:
        if exclude in subdir.split(os.sep):
            ret_val = True
            break

    return ret_val

def get_resource_files_list(serach_path):
    resource_files_list = list()

    for subdir, dirs, files in os.walk(serach_path):
        for file in files:
            if False == filter_out_dir(subdir) and RESOURCES_FILE_NAME == file:
                file_full_path = os.path.join(subdir, file)
                print_msg(PRINT_LEVEL_VERBOSE, "Found resource file :" + file_full_path)
                resource_files_list.append(file_full_path)

    return resource_files_list

def parse_args():
    parser = argparse.ArgumentParser(description='Various pre-steps for build compilation')

    parser.add_argument('-s', '--source', default=".",
                        help='Search path for ' + RESOURCES_FILE_NAME + ' files')

    parser.add_argument('-c', '--cache', default="." + os.sep + "tmp",
                        help='Path to cache for storing content before uploading to server')

    parser.add_argument('-p', '--print', default=PRINT_LEVEL_INFO,
                        choices=[PRINT_LEVEL_ERROR, PRINT_LEVEL_WARN, PRINT_LEVEL_INFO, PRINT_LEVEL_VERBOSE],
                        help='Print level verbosity')

    parser.add_argument('-o', '--output', default=DEFAULT_INVALID_INPUT,
                        help='Output file name to hold the list of packages')

    parser.add_argument('-u', '--user', default=DEFAULT_INVALID_INPUT,
                        help='User for server authentication')

    parser.add_argument('-k', '--key', default=DEFAULT_INVALID_INPUT,
                        help='API key server authentication')

    parser.add_argument('-d', '--dest', default=DEFAULT_INVALID_INPUT,
                        help='URL for destination web file server')

    return parser.parse_args()

def main():
    global g_current_print_level
    ret_val = RET_CODE_SUCCESS
    resource_counter = 0.0
    resource_dict = dict()

    args = parse_args()

    g_current_print_level = args.print

    resource_files_list = get_resource_files_list(args.source)

    resource_list = get_resources_list(resource_files_list)

    #remove duplications
    for resource in resource_list:
        unique_name = resource.get_unique_name()
        if not unique_name in resource_dict.keys():
            resource_dict[unique_name] = resource

    print_msg(PRINT_LEVEL_INFO, "Found " + str(len(resource_files_list)) + " version files and " + str(len(resource_dict.keys())) + " unique resources")

    if args.dest != DEFAULT_INVALID_INPUT:
        upload_files_to_server = True
        print_msg(PRINT_LEVEL_INFO, "Upload files to URL - " + args.dest)
    else:
        upload_files_to_server = False
        print_msg(PRINT_LEVEL_INFO, "Skipping files upload to server")

    #create cache directory if not exist
    create_dir_if_not_exist(args.cache)

    #download content to cache and then upload to web server
    for unique_name in resource_dict.keys():

        resource = resource_dict[unique_name]

        print_msg(PRINT_LEVEL_VERBOSE, resource)

        resource_counter += 1.0

        #download content to cache
        file_in_cache = download_external_resouce(resource, args.cache)

        if "" == file_in_cache:
            return RET_CODE_CANNOT_WRITE_FILE

        if True == upload_files_to_server:
            #upload content to web server
            ret_val = upload_resource_to_server(file_in_cache, unique_name, args.user, args.key, args.dest)
            if ret_val != RET_CODE_SUCCESS:
                return ret_val

        if True == g_delete_resources_in_cache:
            delete_file_if_exist(file_in_cache)

        print_msg(PRINT_LEVEL_INFO, "Downloading Data. Progress " + str(int(100.0*resource_counter/len(resource_dict.keys()))) + "%", True) #print progress bar

    # generate version output file as needed
    if args.output != DEFAULT_INVALID_INPUT:
        ret_val = generate_output_file(resource_dict, upload_files_to_server, args.dest, args.output)
        print_msg(PRINT_LEVEL_INFO, "Generate output file " + args.output)

    return ret_val

#    Entry function
if __name__ == '__main__':

    ret_val = main()

    sys.exit(ret_val)
