# **Using JSON Schema to Define NexthopGroup Information**

# Purpose of Adopting Data Schema
The primary objective of adopting a data schema is to unambiguously define the data format for the nexthop group used in both Zebra and fpmsyncd. By leveraging this schema, we aim to automatically generate the corresponding C++ class shared by both processes, along with serialization and deserialization logic, ensuring consistent and synchronized data exchange. This approach minimizes errors typically introduced through manual coding and enforces system-wide consistency.

# Why JSON Schema?
JSON Schema is chosen because it offers a standardized, human-readable, and language-agnostic format for describing data structure, types, and validation rules. This facilitates reliable data validation and seamless code generation across heterogeneous components—specifically Zebra ( C-based) and fpmsyncd (C++), while promoting interoperability. Furthermore, JSON Schema works well with modern development tooling, enabling automated code generation, runtime data validation, and clear, self-documenting interfaces between Zebra and fpmsyncd.

# Design Philosophy
The JSON schema defines the logical data model and serves as the single source of truth for the message format. However, C++-specific implementation details such as constructors, memory management, unions, and logging—are either explicitly handled in handwritten code or generated via customized render script logic.

# Code Layout
To support this approach, three new directories have been introduced:

* schema/ – Contains the JSON schemas that define the data exchanged between Zebra and fpmsyncd.
* templates/ – Holds code generation templates used to produce C++ serialization/deserialization logic from the schemas.
  * templates/references  - Contains generated codes for debugging and reference only. They would NOT be used in compile.
* scripts/ – Includes render_schema.py, a script that processes the JSON schemas using the templates to generate the final C++ source files.
* c-api/ - existing folder for holding c-api header file and its C wrapping function
  * nexthopgroup_capi.h
  * nexthopgroup_capi.cpp