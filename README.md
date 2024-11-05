# Proiect PSO - DNS Server

## Project details
The goal of this project is to implement a DNS server in order to develop a better understanding of:
1.  How the DNS protocol works
2.  How the DNS server works from a systems programming point of view

Implementation will require use of internet-related c libraries (like `arpa/inet.h`, `netinet/in.h`) and other related c language functionalities (sockets, timers, ...);


## TODO

1.  Develop a barebones server that can translate a static list of predefined IP addresses (for websites & e-mail)
    -   Expand documentation: diagrams, code snippets, program workflow etc.
2.  Implement forwarding + ~~Replace static list with a dynamic one (adding new entries when a forwarding request is called)~~ (<-- this is basically DNS caching)
3.  DNS caching
    -   Subdomains (Subdomain authority support)
    -   IP Filtering (see how it's done in BIND)
4.  DNS Resolver

## Notes

-   Either SQL or plaintext can be used for storing DNS tables
-   DNS has different classes of requests depending on what it's trying to discover: A for a browser website, MX for an e-mail domain [etc.][ch1lnk2]
-   Things to look into:
    -   [Reverse DNS Lookup][ntslnk1]
    -   OSP: Threading, Syncronizing
    -   [DNS over HTTPS][ntslnk2]

##  Documentation for DNS

-   ### DNS Basics & Explanation
    1.  [Wiki Page][ch1lnk1]
    2.  [List of record types][ch1lnk2]
    3.  opensource.com DNS articles: [1st][ch1lnk3] & [2nd][ch1lnk4]
    4.  [hello-dns][ch1lnk5]: A tutorial on how to (correctly) implement a DNS server

-   ### Implementation examples & useful libraries
    1.  [SQLite][ch2lnk1]: Option for DNS table
    2.  [tdns][ch2lnk2]: a C++ DNS library from hello-dns

-   ### Software to experiment with
    1.  [BIND][ch3lnk1]: local dns server
    2.  [Dnsmasq][ch3lnk2]: simple(r) local dns server

[ntslnk1]: https://en.wikipedia.org/wiki/Reverse_DNS_lookup
[ntslnk2]: https://en.wikipedia.org/wiki/DNS_over_HTTPS

[ch1lnk1]: https://en.wikipedia.org/wiki/Domain_Name_System#
[ch1lnk2]: https://en.wikipedia.org/wiki/List_of_DNS_record_types
[ch1lnk3]: https://opensource.com/article/17/4/introduction-domain-name-system-dns
[ch1lnk4]: https://opensource.com/article/17/4/build-your-own-name-server
[ch1lnk5]: https://powerdns.org/hello-dns/

[ch2lnk1]: https://www.sqlite.org/docs.html
[ch2lnk2]: https://powerdns.org/hello-dns/tdns/README.md.html

[ch3lnk1]: https://bind9.readthedocs.io/en/v9.18.14/chapter1.html
[ch3lnk2]: https://thekelleys.org.uk/dnsmasq/doc.html