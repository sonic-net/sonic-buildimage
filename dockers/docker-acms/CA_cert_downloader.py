#!/usr/bin/env python3

'''
This script will call dSMS GetIssuers API to download AME Root Certificate
'''

import os, re, shutil, time
import urllib.request, urllib.parse, urllib.error
import json
from sonic_py_common import logger

CERTIFICATE_AUTHORITY = "ame"
API_URL = "https://ussouth-dsms.dsms.core.windows.net/dsms/issuercertificates?getissuersv3"
ROOT_CERT = "/acms/AME_ROOT_CERTIFICATE.pem"
CERTS_PATH = "/etc/sonic/credentials/"

sonic_logger = logger.Logger()

def copy_cert(source, destination_dir):
    # Copy the certificate file from /acms to /etc/sonic/credentials
    if os.path.isfile(source):
        cert_file = os.path.basename(source)
        destination = os.path.join(destination_dir, cert_file)
        if os.path.isfile(destination):
            # Check if Root cert has changed
            existing_root_cert = open(destination, 'r').read()
            new_root_cert = open(source, 'r').read()
            if existing_root_cert == new_root_cert:
                # No changes in root cert, nothing to copy
                sonic_logger.log_info("CA_cert_downloader: copy_cert: Root cert has not changed")
                return True
            # Delete existing file before copying latest one
            os.remove(destination)
            sonic_logger.log_info("CA_cert_downloader: copy_cert: Existing Root Cert file removed")
        try:
            shutil.copyfile(source, destination)
        except OSError as e:
            sonic_logger.log_error("CA_cert_downloader: copy_cert: Failed to copy to the destination: " + str(e))
            return False
        return True
    else:
        sonic_logger.log_error("CA_cert_downloader: copy_cert: File does not exist")
        return False

def extract_cert(response):
    # Extract certificate from the API response
    if os.path.exists(ROOT_CERT):
        os.remove(ROOT_CERT)
    with open(ROOT_CERT, 'w') as root_cert:
        root = response['RootsInfos'][0]
        for item in root['Intermediates']:
            root_cert.write(item['PEM']+"\n")
        root_cert.write(root['PEM']+"\n")

def get_cert(ca, url):
    # Call API and get response
    payload = {'caname': ca}
    url = url+"&"+urllib.parse.urlencode(payload)
    req = urllib.request.Request(url)
    while True:
        try:
            response = urllib.request.urlopen(req)
            sonic_logger.log_info("CA_cert_downloader: get_cert: URL: "+url)
            if response.getcode() == 200:
                json_response = json.loads(response.read())
                extract_cert(json_response)
                return True
            else:
                sonic_logger.log_error("CA_cert_downloader: get_cert: GET request failed!")
        except Exception as e:
            sonic_logger.log_error("CA_cert_downloader: get_cert: Unable to reach "+url)
            sonic_logger.log_error("CA_cert_downloader: get_cert: "+str(e))
            # Retry every 5 min
            sonic_logger.log_error("CA_cert_downloader: get_cert: Retrying in 5min!")
            time.sleep(60 * 5)


def main():
    while True:
        if get_cert(CERTIFICATE_AUTHORITY, API_URL):
            sonic_logger.log_info("CA_cert_downloader: main: Cert extraction completed")
            if copy_cert(ROOT_CERT, CERTS_PATH) == False:
                sonic_logger.log_error("CA_cert_downloader: main: Root cert move to "+CERTS_PATH+" failed!")
        else:
            sonic_logger.log_error("CA_cert_downloader: main: Cert extraction failed!")
        # Poll dSMS every 12 hours
        time.sleep(60 * 60 * 12)

if __name__ == '__main__':
    sonic_logger.set_min_log_priority_info()
    main()
