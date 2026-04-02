#!/usr/bin/env python3

import json
import sys
import os
from jinja2 import Environment, FileSystemLoader


def json_type_to_c(prop, defs):
    """Map JSON Schema prtperty to C type."""
    if "$ref" in prop:
        ref = prop["$ref"]
        if ref == "#/$defs/ip_address":
            return "union C_g_addr"
        elif ref == "#/$defs/uint8":
            return "uint8_t"
        elif ref == "#/$defs/in_address":
            return "struct in_addr"
        elif ref == "#/$defs/in6_address":
            return "struct in6_addr"
        elif ref.startswith("#/$defs/"):
            typename = ref.split("/")[-1]
            target = defs.get(typename, {})
            if target.get("type") == "string" and "enum" in target:
                return f"enum C_{typename}"
            else:
                return f"struct C_{typename}"
        else:
            return "void*"

    typ = prop.get("type")
    if typ == "integer":
        maximum = prop.get("maximum", None)
        if maximum is None:
            return "uint32_t" if prop.get("minimum", 0) >= 0 else "int32_t"
        elif maximum <= 255:
            return "uint8_t" if prop.get("minimum", 0) >= 0 else "int8_t"
    elif typ == "string":
        return "char*"
    elif typ == "array":
        item_type = json_type_to_c(prop.get("items", {}), defs)
        return item_type
    elif typ == "boolean":
        return "bool"
    elif typ == "null":
        return "void *"
    return "void"


def json_type_to_cpp(prop, defs):
    """Map JSON Schema property to C++ type."""
    if "$ref" in prop:
        ref = prop["$ref"]
        if ref == "#/$defs/ip_address":
            return "union g_addr"
        elif ref == "#/$defs/uint8":
            return "std::uint8_t"
        elif ref == "#/$defs/in_address":
            return "struct in_addr"
        elif ref == "#/$defs/in6_address":
            return "struct in6_addr"
        elif ref.startswith("#/$defs/"):
            typename = ref.split("/")[-1]
            target = defs.get(typename, {})
            if target.get("type") == "string" and "enum" in target:
                return f"enum {typename}"
            else:
                return f"struct {typename}"
        else:
            return "void*"

    typ = prop.get("type")
    if typ == "integer":
        maximum = prop.get("maximum", None)
        if maximum is None:
            return "std::uint32_t" if prop.get("minimum", 0) >= 0 else "std::int32_t"
        elif maximum <= 255:
            return "std::uint8_t" if prop.get("minimum", 0) >= 0 else "std::int8_t"
    elif typ == "string":
        return "std::string"
    elif typ == "array":
        item_type = json_type_to_cpp(prop.get("items", {}), defs)
        if "minItems" in prop and prop.get("minItems", 0) == 0:
            return  item_type
        return f"std::vector<{item_type}>"
    elif typ == "boolean":
        return "bool"
    elif typ == "null":
        return "std::nullptr_t"
    return "void"


def extract_c_enums(defs):
    """Extract C enums from $defs."""
    c_enums = {}
    for name, schema in defs.items():
        if schema.get("type") == "string" and "enum" in schema:
            c_enums[f"C_{name}"] = schema["enum"]
    return c_enums


def extract_enums(defs):
    """Extract enums from $defs."""
    enums = {}
    for name, schema in defs.items():
        if schema.get("type") == "string" and "enum" in schema:
            enums[name] = schema["enum"]
    return enums


def build_c_root_struct(schema, defs):
    """Build C root struct from top-level properties."""
    fields = []
    for name, prop in schema.get("properties", {}).items():
        c_type = json_type_to_c(prop, defs)
        data = {"name": name, "c_type": c_type}
        position = prop.get("position", 0)
        data["position"] = position

        # array type
        if prop.get("type") == "array":
            min_items = prop.get("minItems")
            if min_items == 0:
                # min_items is 0 indicates it's a flexible array
                data["flexible_array"] = True
            elif "C_len" in prop:
                # static array with fixed length
                data["fixed_array"] = True
                data["array_size"] = prop["C_len"]

        if "default_value" in prop:
            dvalue = prop.get("default_value")
            data["default_value"] = dvalue
        if "data_prefix" in prop:
            dvalue = prop.get("data_prefix")
            data["data_prefix"] = dvalue
        fields.append(data)
    name = "C_NextHopGroupFull"
    return {"name": name, "fields": fields}


def build_root_struct(schema, defs):
    """Build root struct from top-level properties."""
    fields = []
    for name, prop in schema.get("properties", {}).items():
        cpp_type = json_type_to_cpp(prop, defs)
        data = {"name": name, "cpp_type": cpp_type}
        position = prop.get("position", 0)
        data["position"] = position
        if "default_value" in prop:
            dvalue = prop.get("default_value")
            data["default_value"] = dvalue
        if "data_prefix" in prop:
            dvalue = prop.get("data_prefix")
            data["data_prefix"] = dvalue
        fields.append(data)
    name = schema.get("title", "NextHopGroupFull")
    return {"name": name, "fields": fields}


def build_c_def_structs(defs):
    """Build structs from $defs."""
    structs = {}
    for name, schema in defs.items():
        if schema.get("type") == "object":
            fields = []
            for fname, fprop in schema.get("properties", {}).items():
                c_type = json_type_to_c(fprop, defs)

                # nexthop_srv6's seg6_segs
                if name == "nexthop_srv6" and fname == "seg6_segs":
                    c_type = "struct C_seg6_seg_stack*"

                field_data = {"name": fname, "c_type": c_type}

                # array type
                if fprop.get("type") == "array":
                    min_items = fprop.get("minItems")
                    if min_items == 0:
                        # flexible array
                        field_data["flexible_array"] = True
                    elif "C_len" in fprop:
                        # fixed array
                        field_data["fixed_array"] = True
                        field_data["array_size"] = fprop["C_len"]

                fields.append(field_data)
            structs[f"C_{name}"] = {"name": f"C_{name}", "fields": fields}

    # Build the deps
    deps = {}
    for name, struct_info in structs.items():
        deps[name] = set()
        for field in struct_info.get("fields", []):
            field_type = field.get("c_type", "")
            for other_name in structs.keys():
                if other_name != name and other_name in field_type:
                    deps[name].add(other_name)

    # Sort the dict by DFS topo
    ordered = {}
    visited = set()

    def visit(name):
        if name in visited:
            return
        visited.add(name)
        for dep in deps[name]:
            visit(dep)
        ordered[name] = structs[name]

    for name in structs.keys():
        visit(name)

    return ordered


def build_def_structs(defs):
    """Build structs from $defs."""
    structs = {}
    for name, schema in defs.items():
        if schema.get("type") == "object":
            fields = []
            for fname, fprop in schema.get("properties", {}).items():
                cpp_type = json_type_to_cpp(fprop, defs)
                if name == "nexthop_srv6" and fname == "seg6_segs":
                    cpp_type = "struct seg6_seg_stack*"
                if fprop.get("type") == "array" and fprop.get("minItems", 0) == 0:
                    fields.append({"name": fname, "cpp_type": cpp_type, "zeroarray": True})
                else:
                    fields.append({"name": fname, "cpp_type": cpp_type})
            structs[name] = {"name": name, "fields": fields}
    # return structs

    # Now we have already build the structs dict,
    # but more we need to do is to sort it.
    # When we are using this dict to construct to/from_json in nexthopgroupfull_json.h.j2,
    # an dependency issue among the structs will occur.
    # Therefore, we sort it from non-depending ones to the complicated.

    # Build the deps
    deps = {}
    for name, struct_info in structs.items():
        deps[name] = set()
        for field in struct_info.get("fields", []):
            field_type = field.get("cpp_type", "")
            # One's type is another struct indicating it's has dependency
            for other_name in structs.keys():
                if other_name != name and other_name in field_type:
                    deps[name].add(other_name)

    # Sort the dict by DFS topo
    ordered = {}
    visited = set()

    def visit(name):
        if name in visited:
            return
        visited.add(name)
        # We access the deps first
        for dep in deps[name]:
            visit(dep)
        # Then add self in
        ordered[name] = structs[name]

    for name in structs.keys():
        visit(name)

    return ordered


def main():
    if len(sys.argv) != 5:
        print("Usage: ./render_schema.py <schema.json> <template_dir> <output_file> <mode>")
        print("  mode: 'header', 'source', 'json_bindings', or 'c_header'")
        sys.exit(1)

    schema_path = sys.argv[1]
    template_dir = sys.argv[2]
    output_path = sys.argv[3]
    mode = sys.argv[4]

    if mode not in ("header", "source", "json_bindings", "c_header"):
        print("Error: mode must be 'header', 'source', 'json_bindings', or 'c_header'")
        sys.exit(1)

    # Load and parse schema
    with open(schema_path, 'r') as f:
        schema = json.load(f)

    defs = schema.get("$defs", {})
    enums = extract_enums(defs)
    c_enums = extract_c_enums(defs)

    # cpp header
    root_struct = build_root_struct(schema, defs)
    def_structs = build_def_structs(defs)

    all_structs = def_structs.copy()
    all_structs[root_struct["name"]] = root_struct

    root_struct_name = root_struct["name"]

    special_structs = {"nexthop_srv6", "seg6_seg_stack", root_struct_name}

    # c header
    c_root_struct = build_c_root_struct(schema, defs)
    c_def_structs = build_c_def_structs(defs)

    c_all_structs = c_def_structs.copy()
    c_all_structs[c_root_struct["name"]] = c_root_struct

    c_root_struct_name = c_root_struct["name"]

    c_special_structs = {"C_nexthop_srv6", "C_seg6_seg_stack", c_root_struct_name}

    # Jinja setup
    # Note: This script generates C/C++ code, not HTML.
    # XSS is not applicable in this context.
    # nosem: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
    env = Environment(loader=FileSystemLoader(template_dir))
    template_name = None

    if mode == "header":
        template_name = "nexthopgroupfull.h.j2"
        context = {
            "enums": enums,
            "structs": all_structs,
            "special_structs": special_structs,
            "root_struct_name": root_struct_name
        }
    elif mode == "source":
        template_name = "nexthopgroupfull.cpp.j2"
        context = {
            "root_struct_name": root_struct_name
        }
    elif mode == "json_bindings":
        template_name = "nexthopgroupfull_json.h.j2"
        context = {
            "enums": enums,  # dict: name -> list of strings (e.g., ["NEXTHOP_TYPE_INVALID", ...])
            "root_struct_name": root_struct_name,
            "root_struct": root_struct,
            "special_structs": special_structs,
            "all_structs": all_structs
        }
    elif mode == "c_header":
        template_name = "c_nexthopgroupfull.h.j2"
        context = {
            "c_enums": c_enums,
            "structs": c_all_structs,
            "special_structs": c_special_structs,
            "root_struct": c_root_struct,
            "root_struct_name": c_root_struct_name
        }

    # Render
    # nosem: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
    template = env.get_template(template_name)
    # nosem: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
    output = template.render(**context)

    # Write
    os.makedirs(os.path.dirname(output_path) if os.path.dirname(output_path) else ".", exist_ok=True)
    with open(output_path, 'w') as f:
        f.write(output)

    print(f"✅ Generated {output_path} (mode: {mode})")


if __name__ == '__main__':
    main()