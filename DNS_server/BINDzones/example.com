$TTL 3600                      ; Default TTL for all records
@       IN  SOA ns1.example.com. admin.example.com. (
            2024111601         ; Serial
            3600               ; Refresh
            1800               ; Retry
            1209600            ; Expire
            3600               ; Minimum TTL
        )
        IN  NS   ns1.example.com
        IN  NS   ns2.example.com
ns1     IN  A    192.168.1.1
ns2     IN  A    192.168.1.2
www     IN  A    192.168.1.3
mail    IN  A    192.168.1.4
