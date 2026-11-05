# gNOI `Containerz`
**Contributors**: alshabib@google.com, robjs@google.com, 
morrowc@google.com
**Last Updated**: 2024-05-06

## Background Documentation

* [gNOI Repository](https://github.com/openconfig/gnoi)
* [gNOI `Containerz` service](https://github.com/openconfig/gnoi/tree/master/containerz)
* [gNOI `Containerz` Reference Implementation](https://github.com/openconfig/containerz)
* [gNOI `Containerz` Motivation and Design](https://github.com/openconfig/containerz/blob/master/doc/motivation.md)

## Use Case and Purpose

The purpose of `Containerz` is to allow container operations over gRPC.
`Containerz` is intended to abstract the underlying container runtime system   
(e.g. docker, kubernetes, etc.) thereby ensuring a common operational model 
irrespective of the underlying architecture. 

Recent advances (including the availability of g* APIs) in vendor network 
operating systems enable custom code to be executed on network devices as docker 
containers. Running software as containers on network devices requires new 
infrastructure to manage the container’s lifecycle. While this infrastructure 
largely exists in the compute world, it is currently not available for network 
devices. Furthermore, it is not immediately evident that the same approach as 
the compute world for managing containers is desirable given this new context 
where we do not want to enable arbitrary code execution but rather only 
sanctioned applications.

Currently, the common approach for transferring a container to a device is via 
proprietary or CLI methods. This process is not appropriate for production since 
under no circumstances should an operator log into a device to make changes, it 
is also inconvenient when testing in the lab as the entire manual process is 
complex and error prone.

Finally, utilising gRPC for container operations allows for reuse of the same 
security infrastructure as gNMI, gRIBI, and other gNOI services. Without 
`Containerz` each container runtime would need to either implement or be 
configured with the appropriate security method.

