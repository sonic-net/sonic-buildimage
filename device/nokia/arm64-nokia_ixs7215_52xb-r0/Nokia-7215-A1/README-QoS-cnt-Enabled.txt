README-QoS-cnt-Enabled.txt
==========================

Overview
--------
This README clarifies the purpose and impact of the queue counter settings for Nokia-7215-A1 devices.

The directory device/nokia/arm64-nokia_ixs7215_52xb-r0-000/Nokia-7215-A1
contains two configuration files related to SAI queue counters:
  SAI-AC5X-xb.xml             – DEFAULT and ACTIVE configuration.
  SAI-AC5X-xb.xml-QoS-cnc-PTF – Extended configuration, inactive by default (reference file)

Default File: SAI-AC5X-xb.xml
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In the default configuration, the queue counters are disabled:

<SAIplt:queue-pass-drop-counter SAIplt:type="Feature-enable">Disabled</SAIplt:queue-pass-drop-counter>
<SAIplt:queue-watermark-counter SAIplt:type="Feature-enable">Disabled</SAIplt:queue-watermark-counter>

This configuration ensures maximum performance and preserves the number of available HW-CNC counters.

Extended File: SAI-AC5X-xb.xml-QoS-cnc-PTF (given for a REFERENCE)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In the extended configuration, the queue counters are enabled:

<SAIplt:queue-pass-drop-counter SAIplt:type="Feature-enable">Enabled</SAIplt:queue-pass-drop-counter>
<SAIplt:queue-watermark-counter SAIplt:type="Feature-enable">Enabled</SAIplt:queue-watermark-counter>

Enabling these counters allows for PTF-test and detailed queue monitoring
but may impact system performance and reduce the number of free/available HW-CNC counters.

Note:
 This is a customer decision about default configuration in the default file-name SAI-AC5X-xb.xml.
 Swap the names for .xml and for appropriated .md5 files.

Summary Table
-------------
| Configuration File          | queue-pass-drop-counter | queue-watermark-counter | Active by Default
|-----------------------------|-------------------------|-------------------------|-----------------|
| SAI-AC5X-xb.xml             | Disabled                | Disabled                | Yes             |
| SAI-AC5X-xb.xml-QoS-cnc-PTF | Enabled                 | Enabled                 | No              |
