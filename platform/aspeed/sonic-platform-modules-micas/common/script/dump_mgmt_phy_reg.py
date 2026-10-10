#!/usr/bin/env python_nos
from tech_support.phy_collector import PhyCollector

if __name__ == '__main__':
    phycollector = PhyCollector()
    ret = phycollector.collect()
    if len(ret) == 0:
        print("collect mgmt_phy info fail")
    for k, v in ret.items():
        print(f'============================> {k} <============================')
        if isinstance(v, (dict, list)):
            try:
                print(json.dumps(v, indent=2, ensure_ascii=False))
            except Exception:
                print(str(v))
        else:
            print(str(v))
