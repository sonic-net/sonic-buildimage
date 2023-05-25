#!/usr/bin/python3

import argparse
import glob
import os
import sys
import re

ALL_DIST = 'all'
ALL_ARCH = 'all'
DEFAULT_MODULE = 'default'
DEFAULT_VERSION_PATH = 'files/build/versions'
VERSION_PREFIX="versions-"
VERSION_DEB_PREFERENCE = '01-versions-deb'
DEFAULT_OVERWRITE_COMPONENTS=['deb', 'py2', 'py3']
SLAVE_INDIVIDULE_VERSION = False


class Component:
    '''
    The component consists of mutiple packages

    ctype -- Component Type, such as deb, py2, etc
    dist  -- Distribution, such as stretch, buster, etc
    arch  -- Architectrue, such as amd64, arm64, etc

    '''
    def __init__(self, verbose=None, versions={}, ctype="deb", dist=ALL_DIST, arch=ALL_ARCH):
        self.versions = versions
        self.ctype = ctype
        if not dist:
            dist = ALL_DIST
        if not arch:
            arch = ALL_ARCH
        self.dist = dist
        self.arch = arch
        self.verbose = verbose

    @classmethod
    def get_versions(cls, version_file):
        result = {}
        if not os.path.exists(version_file):
            return result
        with open(version_file) as fp:
            for line in fp.readlines():
                offset = line.rfind('==')
                if offset > 0:
                    package = line[:offset].strip()
                    if 'py2' in version_file.lower() or 'py3' in version_file.lower():
                        package = package.lower()
                    version = line[offset+2:].strip()
                    result[package] = version
        return result

    def clone(self):
        return Component(self.verbose, self.versions.copy(), self.ctype, self.dist, self.arch)

    def merge(self, versions, overwritten=True):
        for package in versions:
            if overwritten or package not in self.versions:
                self.versions[package] = versions[package]

    def subtract(self, versions):
        for package in versions:
            if package in self.versions and self.versions[package] == versions[package]:
                del self.versions[package]

    def dump(self, config=False, priority=999):
        result = []
        for package in sorted(self.versions.keys(), key=str.casefold):
            if config and self.ctype == 'deb':
                lines = 'Package: {0}\nPin: version {1}\nPin-Priority: {2}\n\n'.format(package, self.versions[package], priority)
                result.append(lines)
            else:
                result.append('{0}=={1}'.format(package, self.versions[package]))
        return "\n".join(result)+'\n'

    def dump_to_file(self, version_file, config=False, priority=999):
        if len(self.versions) <= 0:
            return
        with open(version_file, 'w') as f:
            f.write(self.dump(config, priority))

    def dump_to_path(self, file_path, config=False, priority=999):
        if len(self.versions) <= 0:
            return
        if not os.path.exists(file_path):
            os.makedirs(file_path)
        filename = self.get_filename()
        if config and self.ctype == 'deb':
            none_config_file_path = os.path.join(file_path, filename)
            self.dump_to_file(none_config_file_path, False, priority)
            filename = VERSION_DEB_PREFERENCE
        file_path = os.path.join(file_path, filename)
        self.dump_to_file(file_path, config, priority)

    def print(self, file_path):
        if len(self.versions) <= 0:
            return

        if self.verbose is None:
            return

        filename = self.get_filename()
        file_path = os.path.join(file_path, filename)
        if self.verbose and re.search("cfile=", self.verbose) \
           and not re.search(self.verbose, "cfile=all".format(filename)) \
           and not re.search(self.verbose, "cfile={}".format(filename)):
                    return
        print("VERSION : {}".format(file_path))
        for package in sorted(self.versions.keys(), key=str.casefold):
            if self.verbose and re.search("ctype=", self.verbose) \
               and not re.search("ctype=all".format(self.ctype), self.verbose) \
               and not re.search("ctype={}".format(self.ctype), self.verbose):
                    continue
            if self.verbose and re.search("cname=", self.verbose) \
               and not re.search(self.verbose, "cname=all".format(package)) \
               and not re.search(self.verbose, "cname={}".format(package)):
                    continue
            if self.verbose and re.search("cver=", self.verbose) \
               and not re.search(self.verbose, "cver=all".format(self.versions[package])) \
               and not re.search(self.verbose, "cver={}".format(self.versions[package])):
                    continue
            print('{0}=={1}'.format(package, self.versions[package]))

    # Check if the self component can be overwritten by the input component
    def check_overwritable(self, component, for_all_dist=False, for_all_arch=False):
        if self.ctype != component.ctype:
            return False
        if self.dist != component.dist and not (for_all_dist and self.dist == ALL_DIST):
            return False
        if self.arch != component.arch and not (for_all_arch and self.arch == ALL_ARCH):
            return False
        return True

    # Check if the self component can inherit the package versions from the input component
    def check_inheritable(self, component):
        if self.ctype != component.ctype:
            return False
        if self.dist != component.dist and component.dist != ALL_DIST:
            return False
        if self.arch != component.arch and component.arch != ALL_ARCH:
            return False
        return True

    '''
    Get the file name

    The file name format: versions-{ctype}-{dist}-{arch}
    If {arch} is all, then the file name format: versions-{ctype}-{dist}
    if {arch} is all and {dist} is all, then the file name format: versions-{ctype}
    '''
    def get_filename(self):
        filename = VERSION_PREFIX + self.ctype
        dist = self.dist
        if self.arch and self.arch != ALL_ARCH:
            if not dist:
                dist = ALL_DIST
            return filename + '-' + dist + '-' + self.arch
        if dist and self.dist != ALL_DIST:
            filename = filename + '-' + dist
        return filename

    def get_order_keys(self):
        dist = self.dist
        if not dist or dist == ALL_DIST:
            dist = ''
        arch = self.arch
        if not arch or arch == ALL_ARCH:
            arch = ''
        return (self.ctype, dist, arch)

    def clean_info(self, clean_dist=True, clean_arch=True, force=False):
        if clean_dist:
            if force or self.ctype != 'deb':
                self.dist = ALL_DIST
        if clean_arch:
            self.arch = ALL_ARCH


class VersionModule:
    '''
    The version module represents a build target, such as docker image, host image, consists of multiple components.

    name   -- The name of the image, such as sonic-slave-buster, docker-lldp, etc
    '''
    def __init__(self, verbose=None, name=None, components=None):
        self.name = name
        self.components = components
        self.module_path=""
        self.verbose=verbose

    # Overwrite the docker/host image/base image versions
    def overwrite(self, module, for_all_dist=False, for_all_arch=False):
        # Overwrite from generic one to detail one
        # For examples: versions-deb overwrtten by versions-deb-buster, and versions-deb-buster overwritten by versions-deb-buster-amd64
        components = sorted(module.components, key = lambda x : x.get_order_keys())
        for merge_component in components:
            merged = False
            for component in self.components:
                if component.check_overwritable(merge_component, for_all_dist=for_all_dist, for_all_arch=for_all_arch):
                    component.merge(merge_component.versions, True)
                    merged = True
            if not merged:
                tmp_component = merge_component.clone()
                tmp_component.clean_info(clean_dist=for_all_dist, clean_arch=for_all_arch)
                self.components.append(tmp_component)
        self.adjust()

    def get_config_module(self, source_path, dist, arch):
        if self.is_individule_version():
            return self
        default_module_path = VersionModule.get_module_path_by_name(source_path, DEFAULT_MODULE)
        default_module = VersionModule()
        default_module.load(default_module_path, filter_dist=dist, filter_arch=arch)
        module = default_module
        if self.name == 'host-image':
            base_module_path = VersionModule.get_module_path_by_name(source_path, 'host-base-image')
            base_module = VersionModule()
            base_module.load(base_module_path, filter_dist=dist, filter_arch=arch)
            module = default_module.clone(exclude_ctypes=DEFAULT_OVERWRITE_COMPONENTS)
            module.overwrite(base_module, True, True)
        elif not self.is_aggregatable_module(self.name):
            module = default_module.clone(exclude_ctypes=DEFAULT_OVERWRITE_COMPONENTS)
        return self._get_config_module(module, dist, arch)

    #Merge the default with specific version
    def _get_config_module(self, default_module, dist, arch):
        module = default_module.clone()
        default_ctype_components = module._get_components_per_ctypes()
        module.overwrite(self)
        config_components = []
        ctype_components = module._get_components_per_ctypes()
        for ctype in default_ctype_components:
            if ctype not in ctype_components:
                ctype_components[ctype] = []
        for components in ctype_components.values():
            if len(components) == 0:
                continue
            config_component = self._get_config_for_ctype(components, dist, arch)
            config_components.append(config_component)
        config_module = VersionModule(self.verbose, self.name, config_components)
        return config_module

    def _get_config_for_ctype(self, components, dist, arch):
        result = Component(self.verbose, {}, components[0].ctype, dist, arch)
        for component in sorted(components, key = lambda x : x.get_order_keys()):
            if result.check_inheritable(component):
                result.merge(component.versions, True)
        return result

    def subtract(self, default_module):
        module = self.clone()
        result = []
        ctype_components = module._get_components_per_ctypes()
        for ctype in ctype_components:
            components = ctype_components[ctype]
            components = sorted(components, key = lambda x : x.get_order_keys())
            for i in range(0, len(components)):
                component = components[i]
                base_module = VersionModule(self.verbose, self.name, components[0:i])
                config_module = base_module._get_config_module(default_module, component.dist, component.arch)
                config_components = config_module._get_components_by_ctype(ctype)
                if len(config_components) > 0:
                    config_component = config_components[0]
                    component.subtract(config_component.versions)
                if len(component.versions):
                    result.append(component)
        self.components = result

    def adjust(self):
        result_components = []
        ctype_components = self._get_components_per_ctypes()
        for components in ctype_components.values():
            result_components += self._adjust_components_for_ctype(components)
        self.components = result_components

    def _get_components_by_ctype(self, ctype):
        components = []
        for component in self.components:
            if component.ctype == ctype:
                components.append(component)
        return components

    def _adjust_components_for_ctype(self, components):
        components = sorted(components, key = lambda x : x.get_order_keys())
        result = []
        for i in range(0, len(components)):
            component = components[i]
            inheritable_component = Component(self.verbose, {}, component.ctype)
            for j in range(0, i):
                base_component = components[j]
                if component.check_inheritable(base_component):
                    inheritable_component.merge(base_component.versions, True)
            component.subtract(inheritable_component.versions)
            if len(component.versions) > 0:
                result.append(component)
        return result

    def _get_components_per_ctypes(self):
        result = {}
        for component in self.components:
            components = result.get(component.ctype, [])
            components.append(component)
            result[component.ctype] = components
        return result

    def load(self, image_path, filter_ctype=None, filter_dist=None, filter_arch=None):
        version_file_pattern = os.path.join(image_path, VERSION_PREFIX) + '*'
        file_paths = glob.glob(version_file_pattern)
        components = []
        self.name = os.path.basename(image_path)
        self.module_path = image_path
        self.components = components
        for file_path in file_paths:
            filename = os.path.basename(file_path)
            items = filename.split('-')
            if len(items) < 2:
                continue
            ctype = items[1]
            if filter_ctype and filter_ctype != ctype:
                continue
            dist = ''
            arch = ''
            if len(items) > 2:
                dist = items[2]
            if filter_dist and dist and filter_dist != dist and dist != ALL_DIST:
                continue
            if len(items) > 3:
                arch = items[3]
            if filter_arch and arch and filter_arch != arch and arch != ALL_ARCH:
                continue
            versions = Component.get_versions(file_path)
            component = Component(self.verbose, versions, ctype, dist, arch)
            components.append(component)
            if self.verbose and re.search("stage=load", self.verbose):
                component.print(file_path)

    def load_from_target(self, image_path):
        self.module_path=image_path
        post_versions = os.path.join(image_path, 'post-versions')
        if os.path.exists(post_versions):
            self.load(post_versions)
            self.name = os.path.basename(image_path)
            if self.verbose and re.search("stage=post", self.verbose):
                self.print(post_versions)
            pre_versions = os.path.join(image_path, 'pre-versions')
            if os.path.exists(pre_versions):
                pre_module = VersionModule(self.verbose)
                pre_module.load(pre_versions)
                if self.verbose and re.search("stage=pre", self.verbose):
                    pre_module.print(pre_versions)
                self.subtract(pre_module)
        else:
            self.load(image_path)

    def dump(self, module_path, config=False, priority=999):
        version_file_pattern = os.path.join(module_path, VERSION_PREFIX + '*')
        for filename in glob.glob(version_file_pattern):
            os.remove(filename)
        for component in self.components:
            component.dump_to_path(module_path, config, priority)

    def print(self, module_path):
        if self.verbose is None:
            return
        if re.search("cmod=", self.verbose) \
           and not re.search(self.verbose, "cmod=all".format(self.name)) \
           and not re.search(self.verbose, "cmod={}".format(self.name)):
                    return
        for component in self.components:
            component.print(module_path)
    def filter(self, ctypes=[]):
        if 'all' in ctypes:
            return self
        components = []
        for component in self.components:
            if component.ctype in ctypes:
                components.append(component)
        self.components = components

    def clean_info(self, clean_dist=True, clean_arch=True, force=False):
        for component in self.components:
            component.clean_info(clean_dist=clean_dist, clean_arch=clean_arch, force=force)

    def clone(self, ctypes=None, exclude_ctypes=None):
        components = []
        for component in self.components:
            if exclude_ctypes and component.ctype in exclude_ctypes:
                continue
            if ctypes and component.ctype not in ctypes:
                continue
            components.append(component.clone())
        return VersionModule(self.verbose, self.name, components)

    def is_slave_module(self):
        return self.name.startswith('sonic-slave-')

    # Do not inherit the version from the default module
    def is_individule_version(self):
        return self.is_slave_module() and SLAVE_INDIVIDULE_VERSION

    @classmethod
    def is_aggregatable_module(cls, module_name):
        if module_name.startswith('sonic-slave-'):
            return False
        if module_name.startswith('build-sonic-slave-'):
            return False
        if module_name == DEFAULT_MODULE:
            return False
        if module_name == 'host-image' or module_name == 'host-base-image':
            return False
        return True

    @classmethod
    def get_module_path_by_name(cls, source_path, module_name):
        common_modules = ['default', 'host-image', 'host-base-image']
        if module_name in common_modules:
            return os.path.join(source_path, 'files/build/versions', module_name)
        if module_name.startswith('build-sonic-slave-'):
            return os.path.join(source_path, 'files/build/versions/build', module_name)
        return os.path.join(source_path, 'files/build/versions/dockers', module_name)

    def __repr__(self):
        return repr(self.name)

class VersionBuild:
    '''
    The VersionBuild consists of multiple version modules.

    '''
    def __init__(self, verbose=None, target_path="./target", source_path='.'):
        self.target_path = target_path
        self.source_path = source_path
        self.verbose = verbose
        self.modules = {}

    def load_from_target(self):
        dockers_path = os.path.join(self.target_path, 'versions/dockers')
        build_path = os.path.join(self.target_path, 'versions/build')
        default_path = os.path.join(self.target_path, 'versions/default')
        modules = {}
        self.modules = modules
        file_paths = glob.glob(dockers_path + '/*')
        file_paths += glob.glob(build_path + '/build-*')
        file_paths += glob.glob(default_path)
        file_paths.append(os.path.join(self.target_path, 'versions/host-image'))
        file_paths.append(os.path.join(self.target_path, 'versions/host-base-image'))
        for file_path in file_paths:
            if not os.path.isdir(file_path):
                continue
            module = VersionModule(self.verbose)
            module.load_from_target(file_path)
            if self.verbose and re.search("stage=tmodname", self.verbose):
                print("Target modname={}, path={}".format(module.name, file_path))
                module.print(file_path)
            modules[module.name] = module
        self._merge_dgb_modules()

    def load_from_source(self):
        # Load default versions and host image versions
        versions_path = os.path.join(self.source_path, 'files/build/versions')
        dockers_path = os.path.join(versions_path, "dockers")
        build_path = os.path.join(versions_path, "build")
        paths = [os.path.join(versions_path, 'default')]
        paths += glob.glob(versions_path + '/host-*')
        paths += glob.glob(dockers_path + '/*')
        paths += glob.glob(build_path + '/*')
        modules = {}
        self.modules = modules
        for image_path in paths:
            module = VersionModule(self.verbose)
            module.load(image_path)
            if self.verbose and re.search("stage=smodname", self.verbose):
                print("Source modname={}, path={}".format(module.name, image_path))
                module.print(image_path)
            modules[module.name] = module

    def overwrite(self, build, for_all_dist=False, for_all_arch=False):
        for target_module in build.modules.values():
            module = self.modules.get(target_module.name, None)
            tmp_module = target_module.clone()
            tmp_module.clean_info(for_all_dist, for_all_arch)
            if module:
                module.overwrite(tmp_module, for_all_dist=for_all_dist, for_all_arch=for_all_arch)
            else:
                self.modules[target_module.name] = tmp_module

    def dump(self):
        for module in self.modules.values():
            module_path = self.get_module_path(module)
            module.dump(module_path)

    def print(self, message=None):
        if self.verbose is None:
            return
        if message is not None:
            print("[============={}===========]".format(message))
        for module in [ self.modules[x] for x in (sorted(self.modules, key = lambda x : x)) ]:
            module.print(module.module_path)
    def subtract(self, default_module):
        none_aggregatable_module = default_module.clone(exclude_ctypes=DEFAULT_OVERWRITE_COMPONENTS)
        for module in self.modules.values():
            if module.name == DEFAULT_MODULE:
                continue
            if module.name == 'host-base-image':
                continue
            if module.is_individule_version():
                continue
            tmp_module = default_module
            if not module.is_aggregatable_module(module.name):
                tmp_module = none_aggregatable_module
            module.subtract(tmp_module)

    def freeze(self, rebuild=False, for_all_dist=False, for_all_arch=False, ctypes=['all']):
        if rebuild:
            self.load_from_target()
            self.filter(ctypes=ctypes)
            default_module = self.get_default_module()
            self._clean_component_info()
            self.subtract(default_module)
            self.modules[DEFAULT_MODULE] = default_module
            self.dump()
            return
        self.load_from_source()
        if self.verbose and re.search("stage=init", self.verbose):
            self.print("Initial Source")

        default_module = self.modules.get(DEFAULT_MODULE, None)
        if self.verbose and re.search("stage=init", self.verbose):
            default_module.print("Default Module")

        target_build = VersionBuild(self.verbose, self.target_path, self.source_path)
        target_build.load_from_target()
        target_build.filter(ctypes=ctypes)
        if self.verbose and re.search("stage=init", self.verbose):
            target_build.print("Initial Target")

        if not default_module:
            raise Exception("The default versions does not exist")
        for module in [ target_build.modules[x] for x in (sorted(target_build.modules, key = lambda x : x)) ] :
            if module.is_individule_version():
                continue
            tmp_module = module.clone(exclude_ctypes=DEFAULT_OVERWRITE_COMPONENTS)
            default_module.overwrite(tmp_module, for_all_dist=True, for_all_arch=True)
            if self.verbose and re.search("stage=tmp", self.verbose):
                default_module.print("TMP DEFAULT MODULE")

        target_build.subtract(default_module)
        if self.verbose and re.search("stage=tmp", self.verbose):
            target_build.print("After Subtract Target")
            self.print("After Subtract Source")
        self.overwrite(target_build, for_all_dist=for_all_dist, for_all_arch=for_all_arch)

        if self.verbose and re.search("stage=add", self.verbose):
            self.print("After Merge")
        if not self.verbose or not re.search("dryrun", self.verbose):
            self.dump()

    def filter(self, ctypes=[]):
        for module in self.modules.values():
            module.filter(ctypes=ctypes)

    def get_default_module(self):
        default_module = self.modules.get(DEFAULT_MODULE, VersionModule(DEFAULT_MODULE, []))
        ctypes = self.get_component_types()
        dists = self.get_dists()
        components = []
        for ctype in ctypes:
            if ctype in DEFAULT_OVERWRITE_COMPONENTS:
                continue
            if ctype == 'deb':
                for dist in dists:
                    versions = self._get_versions(ctype, dist)
                    common_versions = self._get_common_versions(versions)
                    component = Component(self.verbose, common_versions, ctype, dist)
                    components.append(component)
            else:
                versions = self._get_versions(ctype)
                common_versions = self._get_common_versions(versions)
                component = Component(self.verbose, common_versions, ctype)
                components.append(component)
        module = VersionModule(self.verbose, DEFAULT_MODULE, components)
        module.overwrite(default_module, True, True)
        return module

    def get_aggregatable_modules(self):
        modules = {}
        for module_name in self.modules:
            if not VersionModule.is_aggregatable_module(module_name):
                continue
            module = self.modules[module_name]
            modules[module_name] = module
        return modules

    def get_components(self):
        components = []
        for module_name in self.modules:
            module = self.modules[module_name]
            for component in module.components:
                components.append(component)
        return components

    def get_component_types(self):
        ctypes = []
        for module_name in self.modules:
            module = self.modules[module_name]
            for component in module.components:
               if component.ctype not in ctypes:
                   ctypes.append(component.ctype)
        return ctypes

    def get_dists(self):
        dists = []
        components = self.get_components()
        for component in components:
            if component.dist not in dists:
                dists.append(component.dist)
        return dists

    def get_archs(self):
        archs = []
        components = self.get_components()
        for component in components:
            if component.arch not in archs:
                archs.append(component.arch)
        return archs

    def get_module_path(self, module):
        return self.get_module_path_by_name(module.name)

    def get_module_path_by_name(self, module_name):
        return VersionModule.get_module_path_by_name(self.source_path, module_name)

    def _merge_dgb_modules(self):
        dbg_modules = []
        for module_name in self.modules:
            if not module_name.endswith('-dbg'):
                continue
            dbg_modules.append(module_name)
            base_module_name = module_name[:-4]
            if base_module_name not in self.modules:
                raise Exception('The Module {0} not found'.format(base_module_name))
            base_module = self.modules[base_module_name]
            dbg_module = self.modules[module_name]
            base_module.overwrite(dbg_module)
        for module_name in dbg_modules:
            del self.modules[module_name]

    def _clean_component_info(self, clean_dist=True, clean_arch=True):
        for module in self.modules.values():
            module.clean_info(clean_dist, clean_arch)

    def _get_versions(self, ctype, dist=None, arch=None):
        versions = {}
        modules = self.get_aggregatable_modules()
        for module_name in self.modules:
            if module_name not in modules:
                temp_module = self.modules[module_name].clone(exclude_ctypes=DEFAULT_OVERWRITE_COMPONENTS)
                modules[module_name] = temp_module
        for module in modules.values():
            for component in module.components:
                if ctype != component.ctype:
                    continue
                if dist and dist != component.dist:
                    continue
                if arch and arch != component.arch:
                    continue
                for package in component.versions:
                    version = component.versions[package]
                    package_versions = versions.get(package, [])
                    if version not in package_versions:
                        package_versions.append(version)
                    versions[package] = package_versions
        return versions

    def _get_common_versions(self, versions):
        common_versions = {}
        for package in versions:
            package_versions = versions[package]
            if len(package_versions) == 1:
                common_versions[package] = package_versions[0]
        return common_versions


class VersionManagerCommands:
    def __init__(self):
        usage = 'version_manager.py <command> [<args>]\n\n'
        usage = usage + 'The most commonly used commands are:\n'
        usage = usage + '   freeze     Freeze the version files\n'
        usage = usage + '   generate   Generate the version files\n'
        usage = usage + '   merge      Merge the version files'
        parser = argparse.ArgumentParser(description='Version manager', usage=usage)
        parser.add_argument('command', help='Subcommand to run')
        args = parser.parse_args(sys.argv[1:2])
        if not hasattr(self, args.command):
            print('Unrecognized command: {0}'.format(args.command))
            parser.print_help()
            exit(1)
        getattr(self, args.command)()

    def freeze(self):
        parser = argparse.ArgumentParser(description = 'Freeze the version files')
        parser.add_argument('-t', '--target_path', default='./target', help='target path')
        parser.add_argument('-s', '--source_path', default='.', help='source path')

        # store_true which implies default=False
        parser.add_argument('-r', '--rebuild', action='store_true', help='rebuild all versions')
        parser.add_argument('-d', '--for_all_dist', action='store_true', help='apply the versions for all distributions')
        parser.add_argument('-a', '--for_all_arch', action='store_true', help='apply the versions for all architectures')
        parser.add_argument('-c', '--ctypes', default='all', help='component types to freeze')
        parser.add_argument('-v', '--verbose', default=None, help="verbose mode")
        args = parser.parse_args(sys.argv[2:])
        ctypes = args.ctypes.split(',')
        if len(ctypes) == 0:
            ctypes = ['all']

        build = VersionBuild(verbose=args.verbose, target_path=args.target_path, source_path=args.source_path)
        build.freeze(rebuild=args.rebuild, for_all_dist=args.for_all_dist, for_all_arch=args.for_all_arch, ctypes=ctypes)

    def merge(self):
        parser = argparse.ArgumentParser(description = 'Merge the version files')
        parser.add_argument('-t', '--target_path', required=True, help='target path to save the merged version files')
        parser.add_argument('-m', '--module_path', default=None, help='merge path, use the target path if not specified')
        parser.add_argument('-b', '--base_path', required=True, help='base path, merge to the module path')
        parser.add_argument('-e', '--exclude_module_path', default=None, help='exclude module path')
        parser.add_argument('-i', '--include_module_path', default=None, help='include module path')
        parser.add_argument('-v', '--verbose', default=None, help="verbose mode")
        args = parser.parse_args(sys.argv[2:])
        module_path = args.module_path
        if not module_path:
            module_path = args.target_path
        if not os.path.exists(module_path):
            print('The module path {0} does not exist'.format(module_path))
        if not os.path.exists(args.target_path):
            os.makedirs(args.target_path)
        module = VersionModule(args.verbose)
        module.load(module_path)
        base_module = VersionModule(args.verbose)
        base_module.load(args.base_path)
        module.overwrite(base_module)
        if args.exclude_module_path:
            exclude_module = VersionModule(args.verbose)
            exclude_module.load(args.exclude_module_path)
            module.subtract(exclude_module)
        if args.include_module_path:
            include_module = VersionModule(args.verbose)
            include_module.load(args.include_module_path)
            if args.verbose:
                include_module.print(args.include_module_path)
            include_module.overwrite(module)
            module.overwrite(include_module)
        module.dump(args.target_path)

    def generate(self):
        parser = argparse.ArgumentParser(description = 'Generate the version files')
        parser.add_argument('-t', '--target_path', required=True, help='target path to generate the version lock files')
        group = parser.add_mutually_exclusive_group(required=True)
        group.add_argument('-n', '--module_name', help="module name, such as docker-lldp, sonic-slave-buster, etc")
        group.add_argument('-m', '--module_path', help="module apth, such as files/docker/versions/dockers/docker-lldp, files/docker/versions/dockers/sonic-slave-buster, etc")
        parser.add_argument('-s', '--source_path', default='.', help='source path')
        parser.add_argument('-d', '--distribution', required=True, help="distribution")
        parser.add_argument('-a', '--architecture', required=True, help="architecture")
        parser.add_argument('-p', '--priority', default=999, help="priority of the debian apt preference")
        parser.add_argument('-v', '--verbose', default=None, help="verbose mode")

        args = parser.parse_args(sys.argv[2:])
        module_path = args.module_path
        if not module_path:
            module_path = VersionModule.get_module_path_by_name(args.source_path, args.module_name)
        if not os.path.exists(args.target_path):
            os.makedirs(args.target_path)
        module = VersionModule(args.verbose)
        module.load(module_path, filter_dist=args.distribution, filter_arch=args.architecture)
        config = module.get_config_module(args.source_path, args.distribution, args.architecture)
        if args.verbose:
            config.print(args.source_path)
        config.clean_info(force=True)
        config.dump(args.target_path, config=True, priority=args.priority)

if __name__ == "__main__":
    VersionManagerCommands()


"""
Dry run examples:
   scripts/versions_manager.py freeze -v 'dryrun|cmod=docker-config-engine-stretch|cfile=versions-py2|cname=all|stage=sub|stage=add|stage=init|stage=tmodname|stage=tmp'
   scripts/versions_manager.py freeze -v 'dryrun|cmod=default|cfile=versions-docker|cname=all|stage=sub|stage=add|stage=init|stage=tmodname|stage=tmp'
"""
