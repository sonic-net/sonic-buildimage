import os
import hashlib

def find_and_sort_rules_files(base_dir):
    rules_files = []
    for file in os.listdir(base_dir):
        full_path = os.path.join(base_dir, file)
        if os.path.isfile(full_path) and file.endswith('.rules'):
            rel_path = os.path.relpath(full_path, base_dir)
            rules_files.append(rel_path)
    rules_files.sort(key=lambda x: x.encode('utf-8'))
    return [os.path.join(base_dir, f) for f in rules_files]

def find_and_sort_rules_files_with_32bit(base_dir):
    base_files = {}
    for file in os.listdir(base_dir):
        if file.endswith('.rules') and os.path.isfile(os.path.join(base_dir, file)):
            base_files[file] = os.path.join(base_dir, file)
    folder_32bit = os.path.join(base_dir, "32bit")
    if os.path.isdir(folder_32bit):
        for file in os.listdir(folder_32bit):
            if file.endswith('.rules') and os.path.isfile(os.path.join(folder_32bit, file)):
                base_files[file] = os.path.join(folder_32bit, file)
    sorted_files = sorted(base_files.keys(), key=lambda x: x.encode('utf-8'))
    return [base_files[f] for f in sorted_files]

def compute_sha1_cat(files):
    sha1 = hashlib.sha1()
    for file_path in files:
        with open(file_path, 'rb') as f:
            content = f.read()
            content = content.replace(b'\r\n', b'\n')
            sha1.update(content)
    return sha1.hexdigest()

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    base_dir = os.path.join(script_dir, "auditd_config_files")
    base_dir = os.path.abspath(base_dir)
    if not os.path.isdir(base_dir):
        print("Directory does not exist.")
        return

    rules_files = find_and_sort_rules_files(base_dir)
    checksum = compute_sha1_cat(rules_files)
    print(f"\nSHA1 (cat all sorted .rules): {checksum}")

    rules_files_32 = find_and_sort_rules_files_with_32bit(base_dir)
    checksum_32 = compute_sha1_cat(rules_files_32)
    print(f"\nSHA1 (cat all sorted .rules, 32bit replaced): {checksum_32}")

if __name__ == "__main__":
    main()