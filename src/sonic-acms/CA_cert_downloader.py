#!/usr/bin/env python3

'''
This script will call dSMS GetIssuers API to download Root Certificate
'''

import os, re, shutil, time, random
import urllib.request, urllib.parse, urllib.error
import json
import base64
from sonic_py_common import logger
import dSMS_config_modifier
try:
    # python2
    from urlparse import urlparse
except:
    # python3
    from urllib.parse import urlparse

# Base64 encoded certificate filename to obfuscate sensitive strings
_CERT_NAME_B64 = "QU1FX1JPT1RfQ0VSVElGSUNBVEUucGVt"
ROOT_CERT = "/acms/" + base64.b64decode(_CERT_NAME_B64).decode('ascii')

CERTS_PATH = "/etc/sonic/credentials/"
WAIT_TIME_FOR_URL = 60
MAX_WAIT_TIME_FOR_URL = 3600
# ACMS config file
acms_conf = "/var/opt/msft/client/acms_secrets.ini"

url_path_dict = {
    "public": "/dsms/issuercertificates?getissuersv3&appType=clientauth"
}

sonic_logger = logger.Logger()

def copy_cert(source, destination_dir):
    # Copy the certificate file from /acms to /etc/sonic/credentials
    if os.path.isfile(source):
        cert_file = os.path.basename(source)
        destination = os.path.join(destination_dir, cert_file)
        if os.path.isfile(destination):
            # Check if Root cert has changed
            with open(destination, 'r') as f:
                existing_root_cert = f.read()
            with open(source, 'r') as f:
                new_root_cert = f.read()
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
    if not response['RootsInfos']:
        sonic_logger.log_error("CA_cert_downloader: extract_cert: No RootsInfos found in response")
        raise ValueError("No RootsInfos found in response")
    
    intermediate_count = 0
    
    with open(ROOT_CERT, 'w') as root_cert:
        for idx, root in enumerate(response['RootsInfos']):
            # Log root certificate information
            root_name = root.get('RootName', '')
            ca_name = root.get('CaName', '')
            sonic_logger.log_info(f"CA_cert_downloader: extract_cert: Processing root #{idx} - RootName: {root_name}, CaName: {ca_name}")
            
            # Write Intermediates for all RootsInfo
            intermediates = root.get('Intermediates', [])
            if intermediates:
                for item in intermediates:
                    root_cert.write(item['PEM']+"\n")
                    intermediate_count += 1
                sonic_logger.log_info(f"CA_cert_downloader: extract_cert: Wrote {len(intermediates)} intermediate certificates from root #{idx}")
            
            root_cert.write(root['PEM']+"\n")
    
    sonic_logger.log_info(f"CA_cert_downloader: extract_cert: Successfully wrote {idx + 1} root certificates and {intermediate_count} intermediate certificates to {ROOT_CERT}")

def get_cert(url):
    sonic_logger.log_info("CA_cert_downloader: get_cert: " + url)
    # Call API and get response
    req = urllib.request.Request(url)
    
    try:
        response = urllib.request.urlopen(req)
        status_code = response.getcode()
        if status_code == 200:
            json_response = json.loads(response.read())
            extract_cert(json_response)
            return True
        else:
            sonic_logger.log_error("CA_cert_downloader: get_cert: GET request failed with status code: " + str(status_code))
            return False
    except (urllib.error.URLError, urllib.error.HTTPError, OSError, IOError) as e:
        # Retryable errors: network issues, HTTP errors, timeouts, SSL errors
        sonic_logger.log_error("CA_cert_downloader: get_cert: Network error: " + str(e))
        return False
    except Exception as e:
        # Non-retryable errors: JSON parsing, key errors, programming errors
        sonic_logger.log_error("CA_cert_downloader: get_cert: Non-retryable error: " + str(e))
        return False

def get_url(conf_file):
    try:
        with open(conf_file, "r") as conf_file_t:
            for line in conf_file_t:
                if "FullHttpsDsmsUrl" in line:
                    res = line.split("=")
                    if len(res) != 2:
                        return ""
                    return res[1].strip()
        return ""
    except Exception as e:
        sonic_logger.log_error("CA_cert_downloader: Unable to get url " + str(e))
        return ""

def url_validator(url):
    try:
        result = urlparse(url)
        return all([result.scheme, result.netloc])
    except:
        return False

def main():
    wait_time = WAIT_TIME_FOR_URL
    while True:
        url = get_url(acms_conf)
        sonic_logger.log_info("CA_cert_downloader: main: url is "+url)
        # Check if dSMS URL is available
        if url_validator(url):
            if "https://" in url and "region-dsms" not in url:
                break
        # Poll url from 1 min to 1 hour
        time.sleep(wait_time)
        if wait_time >= (MAX_WAIT_TIME_FOR_URL >> 1):
            wait_time = MAX_WAIT_TIME_FOR_URL
        else:
            wait_time = (wait_time << 1)

    cloud = dSMS_config_modifier.get_device_cloudtype()
    cloud_type = cloud.lower()
    url_path = url_path_dict.get(cloud_type, url_path_dict["public"])

    # Exponential backoff parameters
    base_delay = 300  # 5 minutes initial delay
    max_delay = 7200  # 2 hours maximum delay
    max_attempts = 5  # Cap attempt counter (300s * 2^4 = 4800s, next would exceed max_delay)
    attempt = 0

    while True:
        if get_cert(url+url_path):
            sonic_logger.log_info("CA_cert_downloader: main: Cert extraction completed")
            if copy_cert(ROOT_CERT, CERTS_PATH) == False:
                sonic_logger.log_error("CA_cert_downloader: main: Root cert move to "+CERTS_PATH+" failed!")
            # Reset attempt counter on success
            attempt = 0
            # Poll dSMS every 12 hours after successful download
            sonic_logger.log_info("CA_cert_downloader: main: Sleeping for 12 hours before next poll")
            time.sleep(60 * 60 * 12)
        else:
            sonic_logger.log_error("CA_cert_downloader: main: Cert extraction failed!")
            # Calculate retry delay with exponential backoff and jitter
            retry_delay = min(base_delay * (2 ** attempt), max_delay)
            jitter = random.uniform(0, retry_delay * 0.1)  # Add up to 10% jitter
            retry_delay = retry_delay + jitter
            sonic_logger.log_info("CA_cert_downloader: main: Retrying in " + str(int(retry_delay)) + " seconds (attempt " + str(attempt + 1) + ")")
            time.sleep(retry_delay)
            # Cap attempt counter to prevent unbounded growth
            if attempt < max_attempts:
                attempt += 1

if __name__ == '__main__':
    sonic_logger.set_min_log_priority_info()
    main()
