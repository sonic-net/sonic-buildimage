#!/bin/bash

# read virtual environment name
read -p "Enter the name of the virtual environment: " venv

# Install virtual env
sudo apt-get install python3-pip

sudo apt-get install virtualenv

# Create a virtual environment
virtualenv $venv
echo
echo
echo "To activate the virtualenv, run:"

echo " source $venv/bin/activate"
