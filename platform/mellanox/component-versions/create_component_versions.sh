#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

echo "SDK $1" > temp_versions_file
echo $2 | sed -r 's/([0-9]*)\.([0-9]*)\.([0-9]*)/FW \2\.\3/g' >> temp_versions_file
echo "SAI $3" >> temp_versions_file
echo "HW-MGMT $4" >> temp_versions_file
echo "MFT $5-$6" >> temp_versions_file
echo "Kernel $7" >> temp_versions_file
