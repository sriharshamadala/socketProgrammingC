"""
---------------------------------------------------------------------------
ECEN 602 : HW3 : HTTP ProxyServer and client implementation

Run the HTTPProxyServer code by passing arguments: <serverIP> <serverPort>

Clients can connect to the server on the specified IP and Ports and make requests.

To run the client code, Connect to the proxyserver by passing <serverip> <portno> <URI> 

---------------------------------------------------------------------------
What the code does:

* Clients make requests to the proxy server to access the webpages.
* proxyserver looks at the requested page, refers to its cache. 
		*If the page is stored in cache and its still not expired, the webpage is returned without contacting the actual HTTP server.
		*If the page is stored in cache but it is expired, the webpage is requested from the http server and relayed to the client.
		*If the webpage isn't stored in cache, the webpage is requested from the http server and relayed to the client.
*Used linked lists to maintain list of webpages and their expiry times. This list is updated depending upon the above 3 scenarios.
---------------------------------------------------------------------------
Project by:
Huang Bolun and Sriharsha Madala
---------------------------------------------------------------------------
"""

