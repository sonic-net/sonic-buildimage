#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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

"""Input Data for dpuctl tests"""
testData = {
             'PW_OFF': {'AssertionError':
                        {'arg_list': [['dpu5'],
                                      ['dpu1', '--all'],
                                      ['dpu1,dpu2,dpu3,dpu5'],
                                      ['dpu5', '--all'],
                                      ['dpu5', '--all', '--force'],
                                      ]},
                        'Returncheck':
                        {'arg_list': [['dpu1'],
                                      ['dpu1, dpu2,dpu3', '--force'],
                                      ['--all', '--force'],
                                      ['dpu4', '--path'],
                                      ['--all', '--test'],
                                      ],
                         'rc': [0, 0, 0, 2, 2],
                         'return_message': ["",
                                            "",
                                            "",
                                            "Usage: dpu-power-off [OPTIONS]"
                                            " <dpu_names>\n"
                                            "Try 'dpu-power-off --help' for"
                                            " help.\n\nError: "
                                            "No such option: --path\n",
                                            "Usage: dpu-power-off [OPTIONS] "
                                            "<dpu_names>\n"
                                            "Try 'dpu-power-off --help' for"
                                            " help.\n\n"
                                            "Error: No such option: --test\n"],
                         }
                        },
             'PW_ON': {'AssertionError':
                       {'arg_list': [['dpu5'],
                                     ['dpu1', '--all'],
                                     ['dpu1,dpu2,dpu3,dpu5'],
                                     ['dpu5', '--all'],
                                     ['dpu5', '--all', '--force'],
                                     ]},
                       'Returncheck':
                       {'arg_list': [['dpu1'],
                                     ['dpu1,dpu2,dpu3', '--force'],
                                     ['--all'],
                                     ['--all', '--force'],
                                     ['dpu4', '--path'],
                                     ['--all', '--test'],
                                     ],
                        'rc': [0, 0, 0, 0, 2, 2],
                        'return_message': ["",
                                           "",
                                           "",
                                           "",
                                           "Usage: dpu-power-on [OPTIONS]"
                                           " <dpu_names>\n"
                                           "Try 'dpu-power-on --help'"
                                           " for help.\n\nError: "
                                           "No such option: --path\n",
                                           "Usage: dpu-power-on [OPTIONS]"
                                           " <dpu_names>\n"
                                           "Try 'dpu-power-on --help'"
                                           " for help.\n\nError: "
                                           "No such option: --test\n"],
                        }
                       },
             'RST': {'AssertionError':
                     {'arg_list': [['dpu5'],
                                   ['dpu1', '--all'],
                                   ['dpu1,dpu2,dpu3,dpu5'],
                                   ['dpu5', '--all'],
                                   ['dpu1,dpu5', '--all'],
                                   ]},
                     'Returncheck':
                     {'arg_list': [['dpu1'],
                                   ['dpu1,dpu2,dpu3', '--force'],
                                   ['--all'],
                                   ['--all', '--test'],
                                   ['dpu1,dpu2,dpu3'],
                                   ['--all', '--test'],
                                   ],
                      'rc': [0, 0, 0, 2, 0, 2],
                      'return_message': ["",
                                         "",
                                         "",
                                         "Usage: dpu-reset [OPTIONS]"
                                         " <dpu_names>\n"
                                         "Try 'dpu-reset --help' for help."
                                         "\n\nError: "
                                         "No such option: --test\n",
                                         "",
                                         "Usage: dpu-reset [OPTIONS]"
                                         " <dpu_names>\n"
                                         "Try 'dpu-reset --help' for help."
                                         "\n\nError: "
                                         "No such option: --test\n"],
                      }
                     },
}

status_output = ["""DPU    dpu ready    dpu shutdown ready    boot progress      Force Power Required
-----  -----------  --------------------  -----------------  ----------------------
dpu0   True         False                 5 - OS is running  False
dpu1   True         False                 5 - OS is running  False
dpu2   True         False                 5 - OS is running  False
dpu3   True         False                 5 - OS is running  False
""",
                 """DPU    dpu ready    dpu shutdown ready    boot progress      Force Power Required
-----  -----------  --------------------  -----------------  ----------------------
dpu1   True         False                 5 - OS is running  False
""",
                 """DPU    dpu ready    dpu shutdown ready    boot progress      Force Power Required
-----  -----------  --------------------  -----------------  ----------------------
dpu0   True         False                 5 - OS is running  False
""",
                 """An error occurred: AssertionError - Invalid Arguments provided!dpu5 does not exist!
""",
                 """An error occurred: AssertionError - Invalid Arguments provided!dpu10 does not exist!
""",
                 """DPU    dpu ready    dpu shutdown ready    boot progress       Force Power Required
-----  -----------  --------------------  ------------------  ----------------------
dpu0   False        True                  0 - Reset/Boot-ROM  False
dpu1   False        True                  0 - Reset/Boot-ROM  False
dpu2   False        True                  0 - Reset/Boot-ROM  False
dpu3   False        True                  0 - Reset/Boot-ROM  False
""",
                 ["dpu1", "True", "False", "False"],
                 ]
