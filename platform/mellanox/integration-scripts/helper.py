import os
import glob
import re

MARK_ID = "###->"
MLNX_KFG_MARKER = "mellanox"
HW_MGMT_MARKER = "mellanox_hw_mgmt"
SLK_PATCH_LOC = "src/sonic-linux-kernel/patch/"
SLK_KCONFIG = SLK_PATCH_LOC + "kconfig-inclusions"
SLK_KCONFIG_EXCLUDE = SLK_PATCH_LOC + "kconfig-exclusions"
SLK_SERIES = SLK_PATCH_LOC + "series"
NON_UP_PATCH_DIR = "platform/mellanox/non-upstream-patches/"
NON_UP_PATCH_LOC = NON_UP_PATCH_DIR + "patches"
NON_UP_PATCH_DIFF = NON_UP_PATCH_DIR + "series.patch"
KCFG_HDR_RE = "\[(.*)\]"
 # kconfig_inclusion headers to consider
HDRS = ["common", "amd64"]

class FileHandler:
    
    @staticmethod
    def write_lines(path, lines, raw=False):
        # Create the dir if it doesn't exist already
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w') as f:
            for line in lines:
                if raw:
                    f.write(f"{line}")
                else:
                    f.write(f"{line}\n")

    @staticmethod
    def read_raw(path):
        # Read the data line by line into a list
        data = []
        with open(path) as im:
            data = im.readlines()
        return data

    @staticmethod
    def read_strip(path, as_string=False):
        # Read the data line by line into a list and strip whitelines
        data = FileHandler.read_raw(path)
        data = [d.strip() for d in data]
        if as_string:
            return "\n".join(data)
        return data

    @staticmethod
    def read_strip_minimal(path, as_string=False, ignore_start_with="#"):
        # Read the data line by line into a list, strip spaces and ignore comments
        data = FileHandler.read_raw(path)
        filtered_data = []
        for l in data:
            l = l.strip()
            if l and not l.startswith(ignore_start_with):
                filtered_data.append(l)
        if as_string:
            return "\n".join(filtered_data)
        return filtered_data
       
    @staticmethod
    def read_dir(path, ext="*") -> list:
        return [os.path.basename(f) for f in glob.glob(os.path.join(path, ext))]

    @staticmethod
    def find_marker_indices(lines: list, marker=None) -> tuple:
        i_start = -1
        i_end = len(lines)
        # print("TEST", marker, lines)
        if marker:
            for index, line in enumerate(lines):
                # assumes one unique marker per file
                # if multiple marker sections are present, reads the first one
                if line.strip().startswith(MARK_ID):
                    if marker+"-start" in line:
                        i_start = index
                    elif marker+"-end" in line:
                        i_end = index
        # print(i_start, i_end)             
        return (i_start, i_end)

    @staticmethod
    def read_kconfig_inclusion(path, marker=MLNX_KFG_MARKER):
        lines = FileHandler.read_strip(path)
        if not marker:
            return lines
        i_start, i_end = FileHandler.find_marker_indices(lines, marker)
        
        if i_start < 0 or i_end >= len(lines):
            print("-> WARNING No Marker Found")
            return []

        return lines[i_start+1:i_end]
    
    @staticmethod
    def write_lines_marker(path, writable_opts: list, marker=None):
        # if marker is none, just write the opts into the file,
        # otherwise write the data only b/w the marker
        curr_data = FileHandler.read_raw(path)
        i_start, i_end = FileHandler.find_marker_indices(curr_data, marker)
        newline_writ_opts = [opt + "\n" for opt in writable_opts]
        if i_start < 0 or i_end >= len(curr_data):
            print("-> WARNING No Marker Found, writing data at the end of file")
            curr_data.extend(["\n"])
            curr_data.extend(newline_writ_opts)
        else:
            curr_data = curr_data[0:i_start+1] + newline_writ_opts + curr_data[i_end:]

        print("-> INFO Written the following opts: \n{}".format("".join(FileHandler.read_raw(path))))
        FileHandler.write_lines(path, curr_data, True)

    @staticmethod
    def read_kconfig_parser(path) -> dict:
        # kconfig_inclusion output formatted to {"no_parent", "common":[,], "amd64": [,], "arm64": [,]}
        lines = FileHandler.read_strip_minimal(path)
        ret = dict({"no_parent":[]})
        curr_hdr = ""
        for line in lines:
            match = re.search(KCFG_HDR_RE, line)
            if match:
                curr_hdr = match.group(1)
                ret[curr_hdr] = []
            elif curr_hdr in ret:
                ret[curr_hdr].append(line)
            else:
                ret["no_parent"].append(line)
        return ret


class KCFG:

    @staticmethod
    def parse_opt_str(opt: str) -> tuple:
        if not opt.startswith("CONFIG"):
            print("-> DEBUG: Malformed kconfig opt, {}".format(opt))
            return ()

        tmp = opt.split("=")
        if len(tmp) != 2:
            print("-> DEBUG: Malformed kconfig opt, {}".format(opt))
            return ()
        
        return (tmp[0], tmp[1])

    @staticmethod
    def parse_opts_strs(kcfg_sec: list) -> list(tuple()):
        opts = [] # list of tuples (CONFIG_*, "m|y|n")
        for kcfg in kcfg_sec:
            tmp = KCFG.parse_opt_str(kcfg)
            if tmp: 
                opts.append(tmp)
        return opts

    @staticmethod
    def get_writable_opts(opts):
        lines = []
        for opt in opts:
            lines.append("{}={}".format(opt[0], opt[1]))
        return lines


class Action():
    def __init__(self, args):
        self.args = args
    
    def perform(self):
        pass

    def write_user_out(self):
        pass
