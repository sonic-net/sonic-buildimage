#!/usr/bin/env bash

echo "Install IPython ClingKernel ..."
cd /opt/ && tar xvf cling-0.9-buster.tar -C /usr/ && \
cd /usr/share/cling/tools/Jupyter/kernel/ && \
pip3 install -e . && \
jupyter-kernelspec install cling-cpp11 && \
jupyter-kernelspec install cling-cpp14 && \
jupyter-kernelspec install cling-cpp1z && \
jupyter-kernelspec install cling-cpp17 && \
jupyter-kernelspec list && \
rm -rf /opt/cling-0.9-buster.tar
echo "Done"
