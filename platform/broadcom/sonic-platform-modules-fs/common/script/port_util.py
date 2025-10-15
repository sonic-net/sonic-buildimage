#!/usr/bin/python3

import ast
import os
try:
    from platform_util import getplatform_name

except ImportError as error:
    raise ImportError(str(error) + "- required module not found")from error

def load_file_config(path: str, tag: str):
    """
    从指定文件中提取名为 tag 的字面量字典。
    如果出错或未找到，返回 None。
    """
    if not os.path.isfile(path):
        return None, f"file not exists: {path}"

    try:
        with open(path, 'r', encoding='utf-8') as f:
            src = f.read()
    except Exception as e:
        return None, f"open file failed with : {e}"

    try:
        tree = ast.parse(src, filename=path)
    except SyntaxError as e:
        return None, f"file syntax error: {e}"

    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == tag:
                    try:
                        value = ast.literal_eval(node.value)
                        if isinstance(value, dict):
                            return value, ""
                        else:
                            return None,  f"{tag} not dict"
                    except Exception as e:
                        return None, f"analysis {tag} failed with: {e}"

    return None, f"not found var: {tag}"

def get_port_config():
    """
    从独立的 {platform}_port_config.py 文件中加载配置
    """
    CONFIG_FILE_LIST = [
        "/usr/local/bin/",
        "/usr/local/lib/python3/dist-packages/config/",
        "/usr/local/lib/python3.7/dist-packages/config/",
        "/usr/local/lib/python3.9/dist-packages/config/"]
    real_path = None
    platform_name = (getplatform_name()).replace("-", "_")
    for configfile_path in CONFIG_FILE_LIST:
        configfile = configfile_path + platform_name + "_port_config.py"
        if os.path.exists(configfile):
            real_path = configfile
            break
    if real_path:
        return load_file_config(real_path, "port_config")
    else:
        return None, f"{configfile} not exists"