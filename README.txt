This repository contains a simple TCP port scanner. This was written for fun many years ago, when I was trying to understand how network utilities worked.

A revelation that this port scanner prompted was that network utilities are typically very simple programs. For example, in order to write this port scanner, the only functions that the programmer needs to know are the functions that open TCP connections. For another example, writing a DNS lookup tool such as "dig" only requires an understanding of the structure of DNS records, which are flat byte arrays.

The other source code file in the repository is a DNS client written for fun recently. It allows the user to look up some common DNS record types for a domain, with an explicitly specified DNS server. Note that this program requires that domains have a trailing period (e.g. "example.com." rather than "example.com").
