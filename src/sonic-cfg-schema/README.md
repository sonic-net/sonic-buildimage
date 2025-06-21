# sonic-cfg-schema

SONiC Config Schema Package generates `cfg_schema.h` which includes macros for SONiC configuration table names.

## Installation

```bash
pip install sonic-cfg-schema
```

## Usage

```python
# Get the path to the cfg_schema.h file
from cfg_schema import get_schema_path
schema_path = get_schema_path()

# Generate a new schema file
from cfg_schema import generate_schema
generate_schema('/path/to/output/cfg_schema.h')
```

## Purpose

This package is used by the sonic-swss-common package to include the generated schema file in `src/sonic-swss-common/common/schema.h`.
