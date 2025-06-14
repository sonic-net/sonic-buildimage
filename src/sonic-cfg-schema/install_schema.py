#!/usr/bin/env python
# install_schema.py - Install cfg_schema.h to the specified location

from __future__ import print_function
import os
import sys
import shutil
import argparse

def install_schema(dest_path):
    """Install the cfg_schema.h file to the specified location."""
    try:
        # Try to import the module to get the schema path
        from cfg_schema import get_schema_path
        schema_path = get_schema_path()
        
        # Create the destination directory if it doesn't exist
        if not os.path.exists(os.path.dirname(dest_path)):
            os.makedirs(os.path.dirname(dest_path))
        
        # Copy the schema file
        shutil.copy2(schema_path, dest_path)
        print("Successfully copied schema to {}".format(dest_path))
        return True
    except ImportError:
        print("Error: cfg_schema module not found. Is it installed?")
        return False
    except Exception as e:
        print("Error copying schema: {}".format(e))
        return False

def main():
    """Main function."""
    parser = argparse.ArgumentParser(description='Install cfg_schema.h')
    parser.add_argument('--dest', required=True, 
                        help='Destination path for cfg_schema.h')
    
    args = parser.parse_args()
    
    if not install_schema(args.dest):
        print("Failed to install schema")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
