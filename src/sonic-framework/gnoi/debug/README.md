# gNOI `Debug` Streaming RPC Design

**Contributors**: hines@google.com, robjs@google.com
**Last Updated**: 2023-11-04

## Background

* [gNOI Repository](https://github.com/openconfig/gnoi)
* [gNOI `Debug` service](https://github.com/openconfig/gnoi/tree/master/debug)

For all legacy devices that provided a CLI on box, providers  have leveraged this CLI through services to provide users with the ability to interact with device CLI's to both get information as well as set operational state on devices.  This interaction is very vendor specific and requires significant overhead to maintain the vendor specific bindings throughout the operational life cycle of a device.

With the introduction of g* services, the goal has been to remove vendor specific data formats from the view operators.  This lets operators have standard models for interacting with any number of vendor devices consistently. There however are gaps between those endpoints versioning and the ability to troubleshoot specific data on a device before API's can be updated.  This proposal is to enable a lightweight interface via gRPC to still access shell level interactions on the device in a secure, maintainable way.

## Architecture

The service will run on a specified port. This service upon recieving a command the server will validate the user has access to the service via Authz check. If user has access the server will then parse the request and check if the user has both the acccess to run the command and if provided act as the role user. If the user is allowed the device will then open "shell" in the mode requested and execute the command in that mode. 

## User Experience

### User needs to get custom state from device

User Request -> Stream of data returned

Example requests are CLI ‘show’ commands.  Ie:

`show proc cpu | json`

### User needs to be able to shell to subcomponent (linecard / backup supervisor) to get data

`shell <linecard>; show memory`

### User needs to tail a process to get output

`tail -f /var/log/foo`

### User needs to capture a trace of process

`strace <cmd>`

