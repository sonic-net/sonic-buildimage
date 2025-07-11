#!/usr/bin/env python
# generate_schema.py - Script to generate cfg_schema.h

from __future__ import print_function
import os
import sys
import subprocess
import argparse

def generate_schema(output_path, yang_dir='/usr/local/yang-models'):
    """Generate cfg_schema.h file."""
    try:
        # Import the schema module
        from cfg_schema import generate_schema as gen_schema
        
        # Generate the schema
        success = gen_schema(output_path, yang_dir)
        
        if success:
            print("Successfully generated schema at {}".format(output_path))
            return True
        else:
            print("Failed to generate schema")
            return False
    except Exception as e:
        print("Error generating schema: {}".format(e))
        return False

def main():
    """Generate cfg_schema.h file."""
    parser = argparse.ArgumentParser(description='Generate cfg_schema.h')
    parser.add_argument('--yang-dir', dest='yang_dir', 
                        default='/usr/local/yang-models',
                        help='Path to YANG models')
    parser.add_argument('--output', dest='output',
                        default='cfg_schema.h',
                        help='Output file path')
    args = parser.parse_args()
    
    output_path = os.path.abspath(args.output)
    
    print("Generating cfg_schema.h at {}".format(output_path))
    generate_schema(output_path, args.yang_dir)

if __name__ == "__main__":
    main()
