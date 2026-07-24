# Guidelines for developing gNOI microservices

This document delivers some guidance for helping develop gNOI microservices.

## Why gNOI instead of gNMI?

gNMI defines actions to retrieve or manipulate the state of a device via
telemetry or configuration data, respectively. gNOI, on the other hand, is used
to execute commands on a device rather than directly change its configuration.

## Characteristics of a gNOI microservice

After deciding that a gNOI microservice needs to be developed, a few aspects
should be taken into consideration. There are three fundamental components that
should be understood.

**Interpretation** - The microservice should be easy to read and understand. The
service and RPC names should intuitively deduce its purpose.

**Implementation** - The microservice should be easy to code. There should be no
ambiguities. The combination of the service definition together with its
documentation should offer a complete understanding of all the code paths.

**Operation** - The microservice should be straightforward to use. Not in the
versatile sense of a single RPC fits all swiss army knife style, but that a
single RPC behaves equally across different implementations. The following
guidelines help with achieving this.

### Thin and focused

The microservice should be simple and focus on the process it aims to implement.
The microservice must not be overarching, it must be specialized. As such, avoid
defining a microservice that implements different unrelated actions. The
developed microservice should be categorized by the nature of the process it
implements, and not by the nature of the Target it is meant to be used on. This
also applies to the naming of the microservice.

*Instead of this:*

```
service Car {
  rpc Turn(...) returns (...);
  rpc SetSpeed(...) returns (...);
  rpc SetStation(...) returns (...);
  rpc SetVolume(...) returns (...);
}
```

*Prefer this:*

```
service Guidance {
  rpc Turn(...) returns (...);
  rpc SetSpeed(...) returns (...);
}

service Radio {
  rpc SetStation(...) returns (...);
  rpc SetVolume(...) returns (...);
}
```

### Shallow RPCs

Aim for more RPCs instead of single RPCs overloaded with actions. This provides
a better path for augmenting the microservice without breaking backward
compatibility.

*Instead of this:*

```
service Direction {
  rpc Adjust(AdjustRequest) returns (...);
}

message AdjustRequest {
  speed = 0;
  turn_angle = 1;
}
```

*Prefer this:**

```
service Guidance {
  rpc Turn(...) returns (...);
  rpc SetSpeed(...) returns (...);
}
```

### Precisely specified behavior

By virtue of its proto definition, the microservice should already specify a
good amount of detail on what is expected of a Target that implements it.
However some details usually fall out of the proto definition. These details
must be specified meticulously as a complement to the proto definition. Take
also in consideration that prescriptions should focus on the behaviour of the
implementation, as opposed to the method of the implementation.

*Instead of this:*

```
service Guidance {
  // Turn sets the turn angle. The
  // forward wheels must rotate to the
  // expected angle. The rear wheels must
  // help if applicable.
  rpc Turn(...) returns (...);
}
```

*Prefer this:*

```
service Guidance {
  // Turn sets the turn angle. The
  // system is expected to reach the
  // turn angle in less than one second.
  rpc Turn(...) returns (...);
}
```

## Error handling

Error handling is an important factor to take into consideration in a
microservice. There are generally two approaches that can be used for individual
scenarios.

### Vanilla gRPC Errors

RPC's return a gRPC error using the status definitions. See in
[golang](http://google.golang.org/genproto/googleapis/rpc/status) for examples.
These errors immediately terminate the RPC and should additionally provide
detailed messages about the nature of the error. They are useful for the
following scenarios:

*   RPC's that have a singular pass/fail criteria;
*   Status RPC's that request unexpected or non existing data;
*   Unexpected oneof message type used in a stream RPC workflow;
*   Authentication issues;
*   Network issues;

An example:

```
Service Bubble {
 ...
 // An gRPC Unimplemented error will be returned if the target cannot generate bubbles.
  rpc Generate(GenerateRequest) returns (GenerateResponse)

  rpc Count(CountRequest) returns (CountResponse)
}
```

### Structured Error messages

Vanilla gRPC errors are straightforward to use but may allow ambiguity to creep
if applied on more complex scenarios. Structured Error messages help with
further documenting and scoping the proto definition which limits ambiguity.
They are, in addition, well defined signals that can be trivially consumed by
upstream workflows operating with a specific gNOI microservice. An example of an
RPC with structured error messages is seen below.

```
message StepLeft {}

message StepRight {}

message StepError {
  enum Type {
    UNSPECIFIED = 0;
    NO_LEGS_ERROR = 1;
    TIRED_ERROR = 2;
  }
  Type type = 1;
  // The detail message must be used if type is UNSPECIFIED.
  string detail = 2;
}

message StepResponse {
  oneof request {
    StepLeft step_left = 1;
    StepRight step_right = 2;
    StepError step_error = 3;
  }
}

message StepRequest {}

// Abort with a gRPC error if Metadata authentication fails.
rpc Step(stream StepRequest) returns (stream StepResponse)
```

Non zero (UNSPECIFIED) enum error types should be descriptive and in general not
require filling up the detail message.

Note that a single RPC can still make use of both the vanilla gRPC Error and
Structured Error message approaches as long as they are applied to different
scenarios. Structured Error messages must never be accompanied by a Vanilla gRPC
Error. Structured Error messages don't necessarily require the RPC to return
immediately.

## Versioning

It is plausible that not all scenarios may have been taken into consideration
during the conception of a microservice. Equally, novel requirements can be
introduced. Microservices may need to expand in order to mature and accommodate
improvements. However there are two fundamental pathways to accomplish this.

### Enhancement

These add backward compatible incremental functionalities to the microservice.
Enhancements require little effort to implement. Enhancements must be reflected
in a version string that accompanies the specific microservice.

```
syntax = "proto3";

package gnoi.whack;

import "github.com/openconfig/gnoi/types/types.proto";

option (types.gnoi_version) = "0.1.0";

service Whack { ...
```

The version string details the major, minor & micro revisions to the gNOI
microservice.

### Evolution

These add or change functionality to the microservice that will break previous
versions. Evolution is a result of a fundamental alteration in the expected
behaviour of the microservice. When a microservice evolves, it must not
necessarily replace the previous one. Evolutions typically result in a new, more
precise and specific service name.
