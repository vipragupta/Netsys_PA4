# PA4 Proxy Server

###Files Description
1. **webproxy.c** -- This file has all the code.
2. **htimhtim** -- This is the hostname to ip cache file.
3. **forbiddenIp.txt** -- This file contains all the forbidden IPs.
4. **cacheFileData.txt** -- This file contains the URI md5sum/hash to the timestamp mapping. This is used for URI data caching.

## Functionalities

1. Blocks the forbidden IPs.
2. Maintains a hostname to IP cache.
3. Multiple Client's can talk to the proxy at same time.
4. Maintains a cache of URI data.


## How to run:

gcc webproxy.c -o proxy -lcrypto -lssl
./proxy <portnumber> <seconds the cache data will be alive>